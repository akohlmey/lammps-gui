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

## Stage 1 -- Centralize constants and settings keys (low risk, mechanical)

- [ ] **1a. Move remaining magic numbers to constants.h.** Hardcoded
  defaults remain in `preferences.cpp` (e.g. `resize(700, 500)`,
  `setRange(1, 1000)`, `setRange(1, 5000)`, `setRange(5, 999)`,
  `(400, 40000)`, `(300, 30000)`) and `chartviewer.cpp`
  (`settings.value("updchart", "500")`). Centralizing these in
  `GuiConstants` improves discoverability and reduces duplication.
  **Strategy**: audit every `resize()`, `setRange()`, and
  `settings.value(..., <default>)` call; introduce a named constant for
  each default.

- [ ] **1b. Centralize QSettings key strings.** ~215 `value()/setValue()`
  calls use bare string literals, many repeated across files (e.g.
  `"updchart"`, `"xsize"`, `"ysize"`, `"zoom"`, `"accelerator"`,
  `"monosize"` in `imageviewer.cpp`, `chartviewer.cpp`,
  `preferences.cpp`). A typo silently reads/writes the wrong key with no
  compile error. **Strategy**: define `namespace SettingsKeys { ... }` in
  `constants.h` with one `inline const QString` per key; replace all bare
  literals in a single sweep, one file per commit.

## Stage 2 -- String-handling consolidation at the wrapper boundary (core)

- [ ] **2a. Add QString-returning overloads for the char-buffer wrapper
  APIs.** `idName`, `styleName`, and `variableInfo` still force every
  caller to declare `char buf[N]` (+ `memset` + reconstruction) at ~11
  sites (`codeeditor.cpp:324`, `lammpsgui.cpp:688/735/970/1439`,
  `imageviewer.cpp:619/626/637/3460/3465/3473/3499/3611`). Add QString
  overloads modeled on the existing `lastErrorMessage()`; keep the raw
  char* signatures private or thin. Then replace the call sites.

- [ ] **2b. Add a QString/QStringList-native `splitLine` overload.**
  `helpers::splitLine(std::string)` currently forces
  QString -> std::string -> QString round-trips
  (`codeeditor.cpp:303,341`). Provide a QString overload returning
  QStringList and drop the `.toStdString()` / `.c_str()` churn.

- [ ] **2c. Sweep remaining conversions.** Audit the ~46 `toStdString` and
  ~66 `.c_str()` uses; eliminate those that exist only to cross an
  internal interface, pushing any genuinely required conversion down into
  `LammpsWrapper`.

## Stage 3 -- Reduce LammpsWrapper plugin/linked dispatch duplication (structural)

- [ ] **3. Collapse the 38 `#ifdef LAMMPS_GUI_USE_PLUGIN` blocks /
  37 `((liblammpsplugin_t *)plugin_handle)->fn(...)` repetitions** in
  `lammpswrapper.cpp` into one mechanism. Options: store a typed
  `liblammpsplugin_t *` member instead of a `void *`, and introduce a
  single dispatch macro/inline helper so each method body appears once.
  Preserve the ABI check and the `CHECKSYM` required-symbol validation in
  `loadLib()`. Highest-duplication target; review carefully because it is
  the central abstraction.

## Stage 4 -- Typed data-extraction helpers (cast consolidation)

- [ ] **4. Wrap the `void*`-returning extract APIs in typed accessors.**
  `extractGlobal`, `extractAtom`, `lastThermo`, etc. produce the
  `*(int *)ptr` / `*(double *)ptr` / `(double)*(int64_t *)ptr` cast
  clusters in `lammpsgui.cpp` (~lines 1429-1648). Add small typed
  template helpers (e.g. `template <typename T> T lastThermoAs(...)`) on
  `LammpsWrapper` so the reinterpretation lives in one audited place and
  call sites lose their C-style casts.

## Stage 5 -- Decompose oversized methods and files

- [ ] **5a. Break up large methods in `lammpsgui.cpp`.** `logUpdate()`
  (154), `setupTutorial()` (135), `startLammps()` (132), `doRun()` (108),
  `runDone()` (105). Suggested extractions: `logUpdate()` -> chart-update
  and image-update loops; `setupTutorial()` -> download-and-extract step;
  `startLammps()` -> argument-building step. One method per commit,
  tests after each.

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
