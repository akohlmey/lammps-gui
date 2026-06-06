LAMMPS-GUI TODO list:

# New feature ideas

## Implement data file manager GUI with the following features:
  - import coordinates and topology via VMD molfile plugins
  - import coordinates and topology from intermol
  - import coordinates and topology from OpenBabel
  - store data internally in a generalized YAML format
  - add/remove columns to per-atom data
  - change atom style for export to data file
  - merge one system to another
  - edit mapping between numeric and symbolic types. create labelmaps.
  - import/export LAMMPS data and molecule files
  - store coordinates internally as unwrapped coordinates
  - recenter coordinates
  - edit box boundaries
  - readjust box to extent of atoms (with or without estimated radius)
  - call to LAMMPS to create geometries from lattices (with/without molecule files) and STL files
  - call to LAMMPS to generate visualizations of geometries
  - edit force field parameters, e.g. apply charmm
  - edit / manage labelmap

## Write "wizard" dialogs for bootstrapping simulations
  this could be used for beginners to create an input file template for
  a few typical use scenarios (could perhaps use some LLM based KI to
  look up suggestions for answers?).

## Integrate / adapt Mark Tschopp's LAMMPS tutorials
  Available as python jupyter notebooks here (unchanged for 6 years):
  https://github.com/mrkllntschpp/lammps-tutorials
  these have not been updated in a long time.
  These could make a different category of tutorials
  relative to the existing: atomic / molecular simulations versus materials science

# Refactoring status and recommendations

This is a staged plan for code cleanups and C++17 modernization. The
codebase is already disciplined and modern (nullptr, auto, range-for,
override, constexpr, Rule-of-5 on every class, make_unique, no malloc /
sprintf / strcpy / typedef), so the work is targeted consolidation, not a
legacy rewrite. The biggest opportunities are: (a) string-handling churn
at the char* / std::string / QString boundary, (b) the LammpsWrapper
plugin-vs-linked dispatch duplication, and (c) a handful of oversized
methods. See CLAUDE.md ("String handling & modern C++ conventions") for
the settled design choices that govern this work.

**Settled north star:** QString is the canonical internal string type;
ALL conversions to/from char* and std::string are confined to
`LammpsWrapper` (the LAMMPS C API boundary). Callers pass and receive
QString. The existing `LammpsWrapper::command/file/commandsString`
overloads and `lastErrorMessage()` are the templates to follow.

**Ground rules for every stage below:**
- Work on a branch off `main` (not the `paper` branch).
- One concern per commit; GPG-sign every commit.
- Run `clang-format -i src/*.cpp src/*.h` before each commit.
- Cleanup commits must not change behavior; build with
  `-DENABLE_TESTING=ON` and keep `ctest` green after each stage.

## Stage 1 -- Centralize constants and settings keys (DONE)

- [x] **1a. Move remaining magic numbers to constants.h.** Done: chart /
  preferences UI ranges, sizes, and update-interval defaults moved into
  the `Cfg` namespace (deduplicating the smoothing window/order ranges and
  the chart update-interval default that were repeated in `preferences.cpp`
  and `chartviewer.cpp`). No behavior change.

- [x] **1b. Centralize QSettings key strings.** Done: 272 bare key/group
  literals across 12 files replaced with the `Keys` namespace
  (`constants.h`, 4 groups + 72 keys). Anchored strictly on the
  `settings` object plus the SnapshotTab factory lambdas, so non-settings
  `value()/contains()/remove()` calls (e.g. `"Program Files"`,
  `"system32"`) were left untouched. No behavior change.

  Follow-ups uncovered during Stage 1:
  - **Chart default size discrepancy:** `preferences.cpp` defaults the
    chart size to 500x320 while `chartviewer.cpp` reads the same
    `chartx`/`charty` keys with defaults 640x480. Left as literals (not
    centralized) to avoid changing behavior; needs a decision on the
    intended default, then a single `CHART_DEFAULT_WIDTH/HEIGHT` constant.
  - **Widget objectName / findChild wiring:** many settings keys double as
    `setObjectName()` / `findChild<>()` strings that were intentionally
    left as literals here. Consolidating them is folded into Stage 6e.

## Stage 2 -- String-handling consolidation at the wrapper boundary (DONE)

- [x] **2a. QString-returning overloads for the char-buffer wrapper APIs.**
  Done: `idName`, `styleName`, `variableInfo` now return `QString` (empty
  on failure, modeled on `lastErrorMessage()`); the char-buffer variants
  are private. Added QString overloads for `extractCompute`, `extractFix`,
  `extractVariableDatatype` so callers pass QString ids. All 11 call sites
  in codeeditor/lammpsgui/imageviewer lost their `char buf[N]` + `memset`
  + `strlen` boilerplate.

- [x] **2b. QStringList-native `splitLine` overload.** Done: added
  `splitLine(const QString&) -> QStringList` (wraps the tested quote-aware
  std::string implementation) and switched all 8 call sites, removing the
  QString -> std::string -> QString round-trips. `cmdToClipboard` now
  builds a QString so the clipboard write drops `.c_str()`.

- [x] **2c. Sweep remaining conversions.** Done: removed the four
  `.toStdString().c_str()` double-conversions (`loadLib` now uses
  `toLocal8Bit().constData()`; three `fprintf` sites use `qPrintable`).
  The remaining `toStdString`/`.c_str()` uses were reviewed and kept as
  deliberate boundaries: building `char**` argv for `execl`/LAMMPS,
  `LammpsRunner::setupRun` (std::string by value for clean cross-thread
  ownership -- see `lammpsrunner.h`), and the std::string-keyed image
  data model (`computes`/`fixes`/`regions` maps and their colors).
  Converting the data model is a larger change, not a string sweep.

## Stage 3 -- Reduce LammpsWrapper plugin/linked dispatch duplication (DONE)

- [x] **3. Collapsed the dispatch duplication via an `LMPFN` macro.** A
  single `LMPFN(fn)` macro resolves to the loaded plugin function table
  (`plugin_handle->fn`) or the `lammps_##fn` symbol at compile time; the 33
  straightforward dispatch methods plus `finalize()` now route through it.
  `#if defined(LAMMPS_GUI_USE_PLUGIN)` blocks dropped from 38 to 5
  (includes, the macro, the plugin-only `plugin_handle` init, `close()`
  which needs a different guard, and the `hasPlugin`/`loadLib` block);
  `lammpswrapper.cpp` shrank 628 -> 490 lines. The `loadLib` ABI check and
  `CHECKSYM` validation are untouched. Each expansion reproduces the exact
  prior call, so both build configurations are unchanged (the transform
  required the plugin and linked sides to match before rewriting).
  Confirmed by compiling both configurations: plugin mode (`build-gui`,
  58/58 tests) and linked mode (`build-lib`, `-DLAMMPS_GUI_USE_PLUGIN=off`).
  Note: a function-like dispatch macro is the right tool here (compile-time
  symbol selection, like `CHECKSYM`'s stringification) -- distinct from the
  logic-hiding macros that Stage 6 removes.

## Stage 4 -- Typed data-extraction helpers (DONE)

- [x] **4. Typed accessors for the `lastThermo` cast clusters.** Added
  `template <typename T> T lastThermoAs(keyword, idx)` (null-safe deref,
  returns `T{}` on null) and `QString lastThermoString(keyword, idx)` to
  `LammpsWrapper`. Both `lammpsgui.cpp` thermo-extraction clusters now use
  them, removing all `*(int *)ptr` / `*(double *)ptr` /
  `(double)*(int64_t *)ptr` / `(const char *)lastThermo(...)` casts and the
  raw `void *ptr` plumbing. Verified equivalent: the only divergence would
  be a null `type` with a non-null `data` (impossible for valid columns,
  and the existing second cluster already dereferenced `type` unguarded).
  Compiles in both plugin (`build-gui`, 58/58 tests) and linked
  (`build-lib`) configs.

  Scope note: `extractGlobal`/`extractAtom`/`extractCompute`/`extractFix`
  return pointers used as **arrays or strings** (`boxlo`, `mass`, `units`,
  ...), not scalars, so a scalar-deref template does not fit them; their
  remaining C-style pointer casts are left for the Stage 6c `static_cast`
  sweep.

## Stage 5 -- Decompose oversized methods and files

- [x] **5a. Break up large methods in `lammpsgui.cpp`.** Done, one method
  per commit (each built in plugin + linked and 58/58 tests):
  - `logUpdate()` -> `updateRunStatus()`, `updateChartData()`,
    `updateSlideShow()` (setup early-return kept in `logUpdate`).
  - `startLammps()` -> `appendAcceleratorArgs()`.
  - `setupTutorial()` -> `openTutorialWebpage()`, `downloadTutorialFiles()`
    (local `DownloadItem` promoted to a private nested struct).
  - `doRun()` -> `createLogWindow()`, `createChartWindow()`.
  - `runDone()` -> `warnHighBufferUsage()`, `finalizeChartData()`.

  Follow-ups noted: `setupPlugin()` (~165 lines) is also over the limit but
  was not in the original list; and several `QSettings().value("literal")`
  *temporaries* (e.g. `"viewslide"`, `"updfreq"`) were missed by Stage 1b's
  `settings`-object-anchored sweep -- a small Stage 1b completion.

- [ ] **5b. Break up the giant dialog builders in `imageviewer.cpp`.**
  `atomSettings()` (~497), `createImage()` (~476), `colorSettings()`
  (~293), `globalSettings()` (~288), `fixSettings()` (~231). Extract
  per-section builder helpers; consider moving the settings-dialog
  construction into its own translation unit to shrink the 3636-line file.

- [ ] **5b. Break up the giant dialog builders in `imageviewer.cpp`.**
  `atomSettings()` (~497), `createImage()` (~476), `colorSettings()`
  (~293), `globalSettings()` (~288), `fixSettings()` (~231). Extract
  per-section builder helpers; consider moving the settings-dialog
  construction into its own translation unit to shrink the 3636-line file.

## Stage 6 -- Interface simplification and modern-C++ polish (breadth pass)

- [ ] **6a. Replace old-style connects.** Two string-based
  `SIGNAL()/SLOT()` connects remain (`chartviewer.cpp:210,215`); convert
  to the function-pointer `connect()` form used everywhere else.

- [ ] **6b. `enum class` for internal enumerations.** Convert enums that
  do not need implicit int interop: `AccelType`/`AccelPrec`
  (`preferences.h`), `PIPES` (`stdcapture.h`). Leave the `LammpsWrapper`
  `StyleConst`/`ScopeConst`/`TypeConst` enums as plain enums where they
  map onto LAMMPS integer constants (or scope them and cast explicitly).

- [ ] **6c. `static_cast` over C-style casts.** Replace remaining
  `(int)`/`(double)` numeric casts in touched code.

- [ ] **6d. Add `[[nodiscard]]`** to pure query methods whose result must
  not be ignored (`LammpsWrapper::isOpen/hasError/version/configHas*`,
  `hasExe`, `isLightTheme`, etc.).

- [ ] **6e. Audit dialog `findChild`/`setObjectName` wiring** (137 / 113
  uses, concentrated in `imageviewer.cpp` and `preferences.cpp`). Where a
  tab is already being rewritten in Stage 5b, replace string-keyed lookups
  with typed member pointers (or a small struct) to remove the
  silent-failure-on-rename hazard; elsewhere leave as-is but keep object
  names exact.

- [ ] **6f. General API hygiene.** const-correctness on query methods,
  pass-by-const-ref for non-trivial parameters, `explicit` on
  single-argument constructors, in classes touched by earlier stages.
