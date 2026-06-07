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

## Snapshot viewer (SlideShow) enhancements
  Follow-ups to the "view arbitrary image files in the snapshot viewer"
  feature (helpers `isImageFile()`, `SlideShow` ImageMagick conversion +
  standalone mode, `viewFile()` routing, and "Open Image File(s)...").
  - Cache converted images. For formats Qt cannot decode, `SlideShow` shells
    out to ImageMagick (`magick`/`convert`) in `readImageFile()` and currently
    re-converts the file every time it is displayed (e.g. on each navigation
    back to it). Cache the temporary PNGs -- keyed by source path + mtime so
    stale conversions are refreshed -- so each exotic-format file is converted
    at most once per session; clean up the temp files on close.
  - Extract movie frames with FFmpeg. Treat movie files (mp4/avi/mkv/webm/
    animated gif/...) as image sources: if `ffmpeg` is available, extract the
    frames to temporary PNGs (e.g. `ffmpeg -i movie.mp4 frame_%05d.png`) and
    load them into the `SlideShow` so animations can be reviewed frame by
    frame with the existing navigation/zoom/playback controls. This is the
    inverse of the existing FFmpeg movie *export*. Detect movie files by
    extension (extend an `isMovieFile()` helper alongside `isImageFile()`),
    route them in `viewFile()` / "Open Image File(s)..." (or a sibling "Open
    Movie..."), and reuse the conversion-cache cleanup from the item above.

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

## Stage 8 -- Reusable ChartViewer for external-data plotting + post-processing (feature, high-level)

**CURRENT STATUS (paused here):** Layers 0, 1, 2, 3, and 4a are DONE and
committed on branch `refactor/cleanup` (108/108 unit tests; builds in
plugin/QtGraphs, plugin/QtCharts, and linked configs; zero Doxygen warnings;
nothing pushed). **Layer 4b (Lepton + Levenberg-Marquardt) is intentionally
DEFERRED** -- resume after testing the current feature set and gathering
feedback on what nonlinear/custom-function fitting is actually wanted. See the
Layer 4b note below for the Lepton symbol-clash constraint that must be solved
when it is picked back up.

Make the chart code a reusable component (mirroring how the log/file
viewer was generalized to display arbitrary text files and restart-explore
output) so it can plot data from external structured files (CSV, whitespace
`.dat`, YAML, JSON) -- including files LAMMPS itself wrote via LAMMPS-GUI --
and run a small set of post-processing/fitting analyses on them. New entry
point under the Run menu (e.g. "Plot data file..."), opening a standalone
`ChartWindow` the same way `FileViewer` is opened for arbitrary files
(`lammpsgui.cpp:1107,1216`).

This MUST stay deliberately minimalist: it is not a plotting program. The
existing capabilities (select data columns, apply a post-process, show,
rename axis labels / plot title, export) are nearly the whole feature set.
The only genuinely new user-facing pieces are file import, line/points/both
display with a small style dialog, and a post-processing/fitting dialog.

**Current coupling to remove (refactor prerequisite).**
- `ChartWindow(filename, LammpsGui*)` is *push-fed* live thermo data during
  a run (`addChart`/`addData` at `lammpsgui.cpp:1478-1496,1559-1583`); the
  window never reads data itself.
- X is hardwired to the integer step: `ChartViewer::addData(int step,
  double)`. External data needs an arbitrary `double` X column.
- Hard `LammpsGui` deps: `stopRun()`, `setUnits()`, `setNorm()`,
  `getStep()`.
- `ChartBackend` only handles `QLineSeries` (`addSeries(QLineSeries*,
  color, width)`) -- lines only, no markers.
- Reusable assets already in tree: the self-contained LU least-squares
  solver behind Savitzky-Golay (`float_mat`/`lu_factorize`, approx.
  `chartviewer.cpp:833-1174`), the export formatters (PNG/CSV/YAML/DAT),
  and the existing "one single-series chart per column, switch via combo"
  model -- which maps cleanly onto "pick one X column, each selected Y
  column becomes a chart."

**Layered strategy (each layer independently shippable + testable):**

- **Layer 0 -- decouple the data source (pure refactor, no UI change). DONE
  (incremental).** Introduce a column-oriented model: `PlotData` (named
  `double` columns + units) and `PlotSeries` (x-col index, y-col index,
  style). The live run becomes one adapter that appends rows; a file loader
  is another adapter that fills columns. `ChartWindow` renders from
  `PlotData`; the `LammpsGui*`/`stopRun` wiring is confined to the live
  adapter (nullable callback, not a hard dependency). Generalize
  `addData(int step,...)` -> `addPoint(double x, double y)`. Also extract the
  LU least-squares code into `leastsquares.{cpp,h}` -- reusable for
  polynomial/EOS fits and unit-testable (same testability win as Stage 7's
  `dumpimage`).
  - Alternatives considered: a "file mode" flag with branches (A1, rots);
    a sibling class for files (A3, duplication). Column model (A2) chosen.
  - Done so far: (8.0a) extracted `leastsquares.{cpp,h}` (Qt-free LU solver +
    Savitzky-Golay) with 8 GoogleTest cases and an api_reference entry;
    (8.0b, incremental per decision) generalized `addData` -> `addPoint`
    (double abscissa, monotonic-x guard preserved) and made `ChartWindow`
    usable with a null `LammpsGui*` (Stop action disabled when absent). The
    `PlotData`/`PlotSeries` model itself was deferred to Layer 1, where the
    file loader first needs it -- chosen over reworking the build-only-
    verifiable live path up front, to maximize testability and minimize
    churn to untested code.

- **Layer 1 -- file import. DONE.** Reuse the inverse of the existing
  CSV/YAML/DAT formatters. Minimal import dialog: detect delimiter + optional
  header row, preview columns, pick X column and one-or-more Y columns. JSON
  limited to the simple array-of-rows / object-of-arrays shapes.
  - Done: (8.1a) `plotdata.{cpp,h}` -- the deferred `PlotData` column model
    (named `std::vector<double>` columns, leastsquares-ready) plus parsers for
    CSV, whitespace/`.dat`, LAMMPS YAML (`keywords:`/`data:`), and JSON
    (array-of-rows / object-of-arrays), with `loadPlotData()` dispatching by
    extension; round-trips the existing exporters; 12 unit tests. (8.1b)
    `ChartWindow::loadData()` + `ChartViewer::setPoints()`/`setXLabel()` --
    bulk file load bypassing the live monotonic-x guard. (8.1c) `PlotDataDialog`
    (x combo + y checkboxes + row preview) and a Run-menu "Plot Data File..."
    entry opening a standalone, self-deleting file-mode `ChartWindow`. Format
    auto-detection by extension/content replaced the planned explicit
    delimiter/header controls (simpler, no UI needed). 94/94 ctest, zero
    Doxygen warnings, both configs build.

- **Layer 2 -- series styling (lines / points / lines+points). DONE
  (minimalist).** Extend `ChartBackend` with `QScatterSeries` support (both
  QtCharts and QtGraphs provide it) and a `SeriesStyle` {mode, color, marker
  shape, marker size, line width} + a compact style dialog. This is the only
  change touching *both* backends, so it is the riskiest piece.
  - Done: `ChartBackend` `addSeries`/`styleSeries`/`removeSeries`/`hasSeries`
    generalized to the common `QXYSeries` base (line or scatter), with
    `styleSeries()` (re)applying color and, for lines, width; both backends
    updated. `ChartViewer` gained a `ChartDisplayMode` enum and
    `setDisplayStyle(mode, color, width)` -- an on-demand `QScatterSeries`
    mirror of the raw data is kept in sync and shown/hidden per mode, with
    defaults reproducing the prior line behavior. A compact "Chart Style..."
    dialog (mode / color / line width) was added to the chart window's File
    menu.
  - **Marker shape and size were dropped** (decision): QtGraphs (the default
    Qt 6.10+ backend) has no C++ marker API -- shape/size are reachable only
    via a QML `pointDelegate`. Per the minimalist goal, points use each
    backend's default marker; the `SeriesStyle` is {mode, color, line width}.
    Cross-session style persistence also deferred. **Build note:** the
    QtCharts backend is normally not compiled (QtGraphs is default on 6.10+),
    so changes here must also build with `-DLAMMPS_GUI_USE_QTCHARTS=ON` (the
    `build-charts` dir) -- a real bug-surface that `build-gui`/`build-lib`
    miss.

- **Layer 3 -- post-processing / analysis. DONE (autocorrelation; fits are
  Layer 4a).** Generalize today's "smooth" into a tiny `Transform` interface
  (source series -> derived series + optional parameter report). Concrete
  transforms: Savitzky-Golay (exists), autocorrelation (direct or FFT; lag vs
  ACF), EOS fit, custom-function fit. Single "Postprocess..." dialog whose
  transform selector swaps in the relevant parameter widgets (the Grace
  pattern). Fits overlay a fit curve and show a small results readout (params
  + derived quantities + RMS residual).
  - Done: (8.3a) `analysis.{cpp,h}` -- a Qt-free post-processing core with a
    normalized `autocorrelation()` (6 unit tests, api_reference entry). (8.3b)
    a "Postprocess..." entry on the chart window's File menu with an analysis
    selector (currently Autocorrelation) + max-lag parameter; it computes the
    ACF of the current chart and opens the (lag, ACF) result in a new
    standalone file-mode `ChartWindow` (reusing the Layer 1 plumbing, since
    the abscissa changes to lag).
  - Deferred by design: the formal `Transform` interface was not introduced
    (premature for one analysis; the dialog dispatches directly). EOS /
    polynomial / custom-function **fits land in Layer 4a/4b** and will extend
    the same Postprocess dialog (selector + parameter area + results readout +
    fit-curve overlay). All three backend configs build; 100/100 ctest; zero
    Doxygen warnings.

- **Layer 4 -- fitting.**
  - *4a linear-in-params (first): DONE.* polynomial + 4-parameter
    Birch-Murnaghan EOS, which is linear in its coefficients
    (`E(V)=a+b*V^(-2/3)+c*V^(-4/3)+d*V^(-2)`) and so fits with the extracted
    LU solver -- no nonlinear code needed for the headline energy-vs-volume
    case. V0/E0/B0/B0' are closed-form in a,b,c,d.
    - Done: `fitting.{cpp,h}` (`polynomialFit`, `birchMurnaghanFit`,
      `evalPolynomial`/`evalBirchMurnaghan`) on top of the leastsquares LU
      solver; V0 is the physical root of `3d u^2 + 2c u + b = 0` (u=V^-2/3) and
      B0=V0 E''(V0), B0'=-1-V0 E'''/E'' from the analytic derivatives. 7 unit
      tests incl. an exact-recovery BM model. The Postprocess dialog gained
      "Polynomial fit" (degree) and "Birch-Murnaghan EOS fit" options that
      overlay the fit curve on the current chart (new `ChartViewer::setFitCurve`)
      and show a results readout (coeffs / V0,E0,B0,B0' / RMS).
    - **Latent bug fixed along the way:** the extracted `lin_solve` mishandled
      non-square right-hand sides (it iterated the RHS columns over the
      coefficient matrix's column count); only `invert()`'s square RHS had ever
      exercised it. Fixed to use `a.nr_cols()`, with a multi-column-RHS
      regression test.
  - *4b nonlinear (extension): DEFERRED -- on hold pending real-world use.*
    Stop here for now: ship Layers 0-4a, gather experience and user feedback
    on what custom-function / nonlinear fitting is actually wanted before
    building this. When resumed: vendor a compact Levenberg-Marquardt routine.
    Combined with a vendored Lepton subset this realizes the "custom EOS as a
    predefined expression" idea: the EOS becomes an expression string + named
    params (with initial guesses/bounds); Lepton parses it and supplies
    analytic derivatives for the Jacobian; LM iterates (the Grace non-linear
    fit popup is the UI template). Custom function *plotting* (evaluate an
    expression over the X range) is a trivial subset worth shipping first.

**Lepton vendoring note -- IMPORTANT symbol-clash constraint.** Lepton (LAMMPS
`lib/lepton`, OpenMM-origin, permissive/MIT-style license -- verify the header
notice; GPLv2+-compatible) cannot simply be bundled wholesale: LAMMPS already
contains Lepton, so in BOTH build modes (linked, and plugin via `dlopen`) the
GUI would end up with **two copies of the same Lepton symbols** -- an ODR
violation / symbol clash that is very bad (undefined behavior, wrong vtables,
crashes). Mitigation: import only a **lightweight subset** of Lepton (drop the
JIT/`ExpressionProgram` compilation path -- we only need parse + evaluate +
differentiate) and place it in a **dedicated namespace** (e.g.
`lammpsgui::lepton`) so its symbols cannot collide with the LAMMPS-provided
ones. Add it to `PROJECT_SOURCES` / a `thirdparty` group with its license
recorded. This namespacing + subsetting is itself part of the Layer 4b work
and a reason it is non-trivial.

**Minimalist guardrails -- deliberately NOT in scope:** multiple Y axes;
log/log or date axes; spreadsheet/data editing; annotations or
legend-as-objects; many-dataset overlay management beyond raw+derived;
3D/surface/bar/pie; session files. (These are exactly where Veusz/LabPlot
earn their complexity.)

**Suggested phasing:** Layer 0 refactor -> Layer 1 import (first visible
payoff) -> Layer 2 styling -> Layer 3 + 4a (autocorrelation + linear/BM4
fit, no new third-party code) -> Layer 4b (Lepton + LM). Phases 1-4a need
no new third-party dependency.

**Survey references (minimalist scientific plotting + fitting + EOS):**
Grace/Xmgrace (non-linear fit popup with named params a0..a9 + bounds +
tolerance is the UI template; per-set line/symbol appearance);
SciDAVis/LabPlot/Veusz (ASCII import -> columns -> fit flow to borrow, depth
to avoid); BM4 EOS linear form (DFTTK, murnaghan2017); Lepton expression
syntax + analytic differentiation (LAMMPS/OpenMM docs).
