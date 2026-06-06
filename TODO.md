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

  Follow-ups: `setupPlugin()` (~165 lines) is over the limit but is left
  undecomposed by design -- it is a one-off, linear, plugin-only startup
  routine whose length is dialog boilerplate plus an `exit`/`execl`/`while`
  flow that resists clean extraction; decomposing it would add indirection
  for no reuse. Instead the genuine smell there -- the `execl` re-exec-self
  sequence duplicated 4x across `lammpsgui.cpp`/`preferences.cpp` plus the
  duplicated Windows `_execl` shim -- was factored into a shared
  `relaunchApplication()` helper (DONE). The six `QSettings().value(...)`
  *temporaries* Stage 1b missed (`plugin_path`, `https_proxy`,
  `viewslide` x2, `updfreq`, `imagereplace`) are now routed through
  `Keys::` -- DONE; no literal settings keys remain anywhere.

- [x] **5b. Break up the giant dialog builders in `imageviewer.cpp`.**
  Chosen approach: HYBRID -- TU-split the `*Settings` dialog builders,
  decompose `createImage` in-place, and only lightly decompose the
  `findChild`-free dialogs. Rationale: `atomSettings`/`globalSettings`
  read settings back via `findChild` after `exec()`, so in-place widget
  extraction risks a silent (compile-clean, runtime-lost) setting if an
  object name is dropped -- only a real run, not `build`/tests, catches it.

  - [x] `createImage()` (476 -> 293) decomposed in-place into
    `appendRegionArgs`, `appendFixComputeStyles`, `appendColorMapArgs`,
    `appendFixComputeColors`. Command-assembly order preserved; both
    configs build, 58/58 tests.
  - [x] **TU split of the five `*Settings` builders.** Done in two steps:
    (1) created `imageviewer_internal.h` holding the shared constants
    (`inline constexpr`), enums, `deftypecolors`, `ImageInfo`/`RegionInfo`,
    and declarations of the free helpers `color_icon`/`gradient_icon`/
    `sequence_icon`/`loadJsonColors`/`saveJsonColors`/`selectComboItem`
    (definitions stayed in `imageviewer.cpp`, moved out of the anonymous
    namespace; the pte tables / `get_pte_from_mass` / `defaultcolors` stay
    file-local); (2) moved the five builders + their exclusive helpers
    (a contiguous 1422-line block) into a new `imageviewersettings.cpp`.
    `imageviewer.cpp` shrank 3604 -> 2099 lines. Both configs build, 58/58.
  - [x] Light in-place decomposition of `fixSettings`/`regionSettings`/
    `colorSettings` (0 `findChild` each). `fixSettings` 231 -> 89 via shared
    `buildFixComputeRows`/`readFixComputeRows` (the compute and fix halves
    were near-identical); `regionSettings` 137 -> 113 and `colorSettings`
    293 -> 265 via `readRegionRows`/`readColorRows` readback helpers. Both
    configs build, 58/58 tests.

## Stage 6 -- Interface simplification and modern-C++ polish (breadth pass)

- [x] **6a. Replace old-style connects.** Done: the two `SIGNAL()/SLOT()`
  connects in `chartviewer.cpp` are now function-pointer connects.

- [~] **6b. `enum class` for internal enumerations.** Assessed, not done:
  `AccelType`/`AccelPrec` are persisted in QSettings (read via `.toInt()`,
  compared in `switch`/`==` against `int`, passed as `QVariant` defaults)
  and `PIPES` is used as array indices -- all genuinely need int interop,
  so `enum class` would only add `static_cast` noise and degrade them. The
  `LammpsWrapper` `StyleConst`/`ScopeConst`/`TypeConst` likewise map to
  LAMMPS ints. No suitable candidate; intentionally left as plain enums.

- [x] **6c. `static_cast` over C-style casts.** Done: ~35 numeric and
  `void*`->`T*` C-style casts across the touched files are now
  `static_cast`. Left the `execl` `(char *)nullptr` varargs sentinels and
  the `QByteArray` `readLine()` conversions as-is.

- [x] **6d. Add `[[nodiscard]]`.** Done on `LammpsWrapper` state/config
  queries (`version`, `isOpen`, `isRunning`, `hasError`, `hasPlugin`,
  `hasGpuDevice`, `configHasPackage`, `configAccelerator`,
  `configHas{Curl,Omp,Png,Jpeg}Support`) and `helpers` `hasExe`/
  `isLightTheme`.

- [~] **6e. Audit dialog `findChild`/`setObjectName` wiring** (137 / 113
  uses). Left as-is by design: the Stage 5b TU split *moved* the dialog
  builders but did not rewrite their wiring, and replacing the
  `findChild`-based readback with typed member pointers is exactly the
  silent-failure-on-rename operation that is unsafe to do here (only a real
  GUI run, not `build`/tests, would catch a mistake). Object names were
  kept exact. Revisit alongside Stage 7 (when the renderer becomes
  testable).

- [x] **6f. General API hygiene.** Audited: all single-argument
  constructors in the project are already `explicit`, query methods are
  already `const`, and non-trivial parameters already pass by const-ref --
  the codebase was disciplined here from the start, so no changes were
  needed. (Marked done to reflect the audit; revisit opportunistically.)

## Stage 7 -- Extract createImage into a standalone, struct-driven renderer (DONE)

Done: the command-assembly core was extracted into a new, GUI-free
translation unit `src/dumpimage.{cpp,h}`. `DumpImageParams` (in
`dumpimage.h`) is a plain struct holding every command-relevant value with
all LAMMPS-derived data (`ntypes`, `*_flag`, `dimension`, `version`,
element/sigma detection results, etc.) pre-resolved; `QString
buildDumpImageCommand(const DumpImageParams&)` is a pure function with no
GUI or LAMMPS access. The four `append*` helpers moved there as file-static
free functions. `ImageViewer::createImage()` now: (1) gathers the struct via
the new `gatherDumpImageParams()` (which still refreshes the
`useelements`/`usediameter`/`usesigma`/`atomcolor` members the settings
dialogs read), (2) syncs the atom-size widgets via the new GUI-only
`syncAtomSizeWidgets()` -- separating concern 2 (widget side effects) from
concern 1/3 (the adiams string + command), (3) calls the pure builder, then
(4) does the temp-atom/write_dump/render orchestration. `imageviewer.cpp`
shed ~400 net lines. A new GoogleTest suite `test/test_dumpimage.cpp` (16
cases) exercises the builder directly -- the first test coverage of the
image code, which was previously only build-verifiable. Builds clean in both
plugin and linked configs; 74/74 ctest pass.

Design choices settled during the work:
- The builder takes the fully pre-captured struct (no `LammpsWrapper*`), so
  it is genuinely pure/testable; all LAMMPS queries stay in the gather step.
- `DumpImageParams` holds the `regions`/`computes`/`fixes` maps as non-owning
  pointer copies and `color_list` by value; the struct is short-lived.
- Concern 4 (orchestration) stays in `ImageViewer` -- moving it out would
  only relocate GUI/LAMMPS coupling without improving testability.

Original plan (for reference):

Turn `ImageViewer::createImage()` into a free function in its own
`.cpp`/`.h` (e.g. `dumpimage.{cpp,h}`), fed by a plain parameter struct
(e.g. `DumpImageParams`) that `ImageViewer` populates before the call,
rather than the function reaching into ~50 `ImageViewer` members directly.

**Primary payoff:** the dump-image command generation becomes a *pure*
`QString buildDumpImageCommand(const DumpImageParams&)` that can be
**unit-tested without a GUI or a live LAMMPS** -- closing the single
biggest verification gap in this codebase (the image/dialog code is
otherwise only `build`-verifiable, never exercised by the test suite).
Secondary: shrinks `imageviewer.cpp` by ~290 lines and makes the
ImageViewer/renderer boundary explicit.

**This step MUST be preceded by an assessment of the intra-function
refactoring it forces**, because `createImage` currently interleaves four
concerns that have to be separated first:
  1. *Parameter gathering* -- ~50 members plus LAMMPS queries
     (`ntypes`, `nbondtypes`, `mass`, `pair_style`, `units`, `dimension`,
     the various `*_flag` settings, `version`) and derived data (element
     detection from masses, sigma fallback). These become the struct's
     fields, computed by `ImageViewer` (or a gather step) before the call.
  2. *UI widget side effects* -- it shows/hides/enables the atom-size
     `QLineEdit`/`QLabel`/`vdw` button via `findChild` based on
     `useelements`/`usediameter`/`atomcustom`. This is GUI-only and must be
     pulled back into `ImageViewer` (e.g. a `syncAtomSizeWidgets()`),
     otherwise the function cannot be GUI-free/testable.
  3. *Pure command assembly* -- the `dumpcmd` building (already partly
     factored into `appendRegionArgs/appendFixComputeStyles/`
     `appendColorMapArgs/appendFixComputeColors`, which would be
     re-parameterized to take the struct instead of `this`). This is the
     testable core.
  4. *Rendering/orchestration* -- temp-atom creation for molecule view,
     `write_dump`, error handling, reading the PPM, updating `imageLabel`.
     Thin glue; can stay in `ImageViewer` or move with the function (it
     still needs `LammpsWrapper`).

**Open design questions for the assessment:** whether the function takes
`LammpsWrapper*` (simpler) or the struct fully pre-captures all LAMMPS
data (purer/more testable); whether `DumpImageParams` owns copies of the
`regions`/`computes`/`fixes`/`color_list` data or references them; and how
much of (4) moves vs. stays. The Stage 5b in-place `createImage`
decomposition is a stepping stone but its helpers would be reshaped here.
