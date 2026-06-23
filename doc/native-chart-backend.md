# Native chart backend -- design and staged plan

Status: NATIVE-ONLY (2026-06-22). The native `QPainter` renderer is now the sole
chart backend. The `ChartBackend` abstraction and the QtCharts and QtGraphs
backends have been deleted; `ChartViewer` owns `PlotSeries` objects and renders
them with `PlotWidget` directly. The build links only Qt Gui/Widgets/Network --
no Qt Charts, Graphs, or Quick/QML. On a feature branch we collapsed Phases 2-4
into one jump (decision: native is clearly better; reset/redo is cheap while
`develop` is not merged to `main`). All 174 non-GUI tests pass; charts verified
via the `-c` CLI. Branch: `native-chart-backend` (off `develop`).

Historical staged plan (Phases 1-4) is kept below for context; it was executed
as Phase 1 (opt-in) then a direct collapse to native-only.

This is the durable design memory for replacing the two Qt-module chart
backends (QtCharts and QtGraphs) with a single, self-contained renderer built
only on Qt Widgets and `QPainter`. Treat the decisions here as binding unless
we explicitly revise them.

## Motivation

LAMMPS-GUI currently renders thermodynamic charts through one of two backends
selected at build time behind the `ChartBackend` interface (`src/chartbackend.h`):

- **QtCharts** (`src/qtchartsbackend.cpp`) -- widget-based, Qt 6.2+. Mature, no
  QML. But Qt Charts is a **GPLv3 / commercial add-on module**, not part of the
  LGPLv3 Qt Essentials, and Qt has positioned it as legacy.
- **QtGraphs** (`src/qtgraphsbackend.cpp`) -- the nominal successor, Qt 6.10+.
  It is QML / Qt Quick based and pulls in `Quick` + `QuickWidgets` + `Graphs`.

The QtGraphs path has accumulated workarounds that we consider technical debt:

- Axis titles are faked with external `VerticalLabel` / `QLabel` widgets in a
  hand-built layout because the QML `GraphsView`'s own titles do not position
  acceptably (`setTitleVisible(false)` plus an overlay layout).
- Series add / remove / query go through stringly-typed QML reflection --
  `QMetaObject::invokeMethod(graphsView, "addSeries", ...)` -- with no
  compile-time checking.
- The tick-label format must be re-applied after every `resetZoom()` because
  QML tick regeneration reverts it.
- Dashed reference lines are not possible in QtGraphs; only the QtCharts path
  draws dashed vertical markers (`#ifndef LAMMPS_GUI_USE_QTGRAPHS` in
  `src/chartviewer.cpp`).

Both modules are therefore liabilities: QtCharts is an aging GPLv3 add-on,
QtGraphs is heavier and brought the QML quirks above. A native `QPainter`
renderer drops **both** module dependencies and the QML runtime, leaving only
LGPLv3 Qt Widgets, and gives us full control over rendering behavior.

**Decision (2026-06-22): we are going all the way to native-only.** QtCharts is
going away upstream regardless, so once the native backend is complete we switch
to it exclusively and stop carrying compatibility code for the Qt modules.

## Current coupling (what has to change)

The consumer (`src/chartviewer.cpp` / `.h`) is coupled to Qt chart types in a
small, well-bounded way:

- All rendering goes through the 16-method `ChartBackend` interface.
- A tiny direct series API is used on the series objects:
  `points()`, `replace(QList<QPointF>)`, `append()`, `count()`,
  `setVisible()` / `isVisible()`, `setName()` / `name()`,
  `setPen()` / `pen()`, and one axis getter `titleText()`.
- The data itself lives inside Qt's `QLineSeries` / `QScatterSeries` objects,
  which `chartviewer` owns as members (`series`, `smooth`, `scatter`,
  `smoothScatter`, `fit`, `overlaySeries`, `vlines`).

The two backends compile against the same source because Qt deliberately made
the QtGraphs 2D series / axis classes (`QLineSeries`, `QScatterSeries`,
`QValueAxis`) source-compatible with QtCharts; `chartviewer.h` only swaps the
include path.

Backend selection lives at three `#ifdef LAMMPS_GUI_USE_QTGRAPHS` sites in
`chartviewer.cpp` (include switch, instantiation, dashed-line block), one in
`chartviewer.h` (include switch), and one in `lammpsgui.cpp`.

The "nice number" tick-interval math is currently inlined inside
`qtgraphsbackend.cpp::resetZoom()` and is a good candidate to extract into a
Qt-free, unit-testable helper.

## Target architecture (native-only end state)

A single backend, no Qt chart module, no QML:

```
ChartViewer
  -> ChartBackend (interface, retyped onto neutral model types)
       -> NativeChartBackend
            -> PlotWidget (QWidget + QPainter renderer)
                 consumes neutral model: PlotLineSeries / PlotScatterSeries / PlotAxis
            -> plotaxismath (Qt-free: ticks, ranges, label formatting)
```

Key design choice: **the renderer speaks a neutral data model from day one.**
`PlotWidget` consumes plain `PlotLineSeries` / `PlotScatterSeries` / `PlotAxis`
value types (point list + color / width / pen style; axis min / max / ticks /
format). During the optional phase, `NativeChartBackend` adapts the Qt series it
receives through the `ChartBackend` interface into those types at the boundary,
so the renderer never touches a Qt chart type. The final retype of the interface
and `chartviewer` then changes only the boundary, never the renderer.

### New files

| File | Role | Qt-free |
|---|---|---|
| `src/plotaxismath.{cpp,h}` | nice-number tick interval, tick positions, range rounding, printf-style label formatting (lifted from `qtgraphsbackend.cpp`) | yes -- GoogleTest |
| `src/plotwidget.{cpp,h}` | `QWidget` + `QPainter` 2D line / scatter renderer: axes, gridlines, ticks, labels, title, series, light / dark theme, `grab()` and arbitrary-size export | no (Widgets only) |
| `src/nativechartbackend.{cpp,h}` | implements `ChartBackend`; feeds `PlotWidget` from the model | no |
| `src/plotseries.{cpp,h}` | neutral model value types (`PlotLineSeries`, `PlotScatterSeries`, `PlotAxis`) | yes |

Each is a `.h/.cpp` pair listed in `PROJECT_SOURCES` per the project's widget
file convention.

### One small refactor up front

Replace the inline `#ifdef` instantiation in `chartviewer.cpp` with a single
`ChartBackend::create()` factory in one translation unit, so backend selection
lives in one place instead of sprawling 3-way `#ifdef`s.

### Feature contract (parity with current usage only)

The renderer must match what `chartviewer` uses today -- no more:

- multiple line series and scatter series, each with color and width;
- linear X and Y value axes with nice-number major ticks, minor subticks, and
  major / minor gridlines whose visibility comes from the `Charts` QSettings
  group (`Keys::GRID`, `Keys::MINORGRID`);
- printf-style tick label format per axis (e.g. `%d`, `%.6g`);
- X-axis title, Y-axis title, and chart title;
- programmatic zoom only -- `resetZoom(xmin, xmax, ymin, ymax)`; there is no
  in-chart mouse pan / rubber-band today (zoom is driven by the range sliders);
- dashed vertical reference lines (a QtGraphs gap the native renderer fixes for
  all builds);
- antialiased, high-DPI-correct output;
- `grab()` to an image for "Save Graph As...", plus arbitrary-size export.

Explicitly out of scope (matches the existing "deliberately NOT in scope" list
in `TODO.md`): multiple Y axes, log / date axes, in-chart legend objects,
annotations, 3D / bar / pie, spreadsheet editing.

## Staged plan

The trajectory: land native optional and default-off this cycle (so the pending
v2.1.0 release ships with unchanged behavior), make it the default next cycle,
delete the QML / QtGraphs backend, then delete QtCharts and go native-only.

### Phase 1 -- optional native backend (this cycle, before v2.1.0) -- DONE

Minimal blast radius. Default behavior unchanged. Landed as four signed commits:

1. `plotaxismath.{cpp,h}` (Qt-free) + GoogleTest unit tests for tick intervals
   and label formatting (19 cases).
2. `plotseries.h` neutral model + `plotwidget.{cpp,h}` renderer, screenshot-
   tested standalone before wiring.
3. `ChartBackend::create()` factory (`chartbackend.cpp`) + a new
   `setSeriesLineStyle()` interface method, removing the backend instantiation
   and the dashed-reference-line `#ifdef`s from `ChartViewer`. Behavior-
   preserving for QtCharts/QtGraphs; native restores dashed reference lines.
4. `nativechartbackend.{cpp,h}` adapter + the `LAMMPS_GUI_USE_NATIVE_CHARTS`
   CMake option (default OFF) + docs.

Verified: native output renders correctly through the `-c` chart CLI under
`xvfb-run`; the QtGraphs (plugin) and QtCharts (linked) builds still compile and
all non-GUI unit tests stay green.

Feature-parity sweep on the native backend (2026-06-22): verified via the `-c`
chart CLI under Xvfb -- Points / Lines+Points (scatter rendering), multiple Y
columns + Data-dropdown switching, X/Y range sliders, Postprocess
(Autocorrelation new-chart and Polynomial-fit overlay), overlay series from "Add
Data from File", Save Graph As (widget grab), Chart Style point size, and
title/axis-label editing. All rendered correctly. Two UX fixes came out of it:
a point-diameter control in Chart Style (default raised 6 -> 8 px), and the
processed-series plot choice now labels "(empty)" until it holds data.

Outstanding for Phase 2 hardening (noted here so they are not forgotten):
a committed render smoke test for `PlotWidget`; dark-mode and high-DPI parity;
save-as-image at an explicit export resolution (it currently grabs at widget
size); the Custom function / Custom fit and EOS-fit postprocess paths (only
Autocorrelation + Polynomial were exercised); and live-run (streaming thermo)
rendering.

### Phase 2 -- native becomes default (next cycle)

Flip the default to native where built; QtGraphs / QtCharts become opt-in.
Harden: tick aesthetics, dark-mode parity, high-DPI / `devicePixelRatio`,
save-as-image at export resolution, live-update repaint throttling. Expand tests.

### Phase 3 -- delete the QML / QtGraphs backend (next cycle)

Remove `qtgraphsbackend.{cpp,h}`, drop `Graphs` / `Quick` / `QuickWidgets` from
`find_package`, and delete the QML workarounds. Native + QtCharts remain. This
is where the QML overhead and its workarounds leave the codebase.

### Phase 4 -- native-only (after native has a release of real-world use)

Retype the `ChartBackend` interface and `chartviewer.{cpp,h}` off Qt chart types
onto the neutral model, delete `qtchartsbackend.{cpp,h}`, and drop the Qt Charts
module from the build. Result: one backend, LGPLv3 Qt Widgets only. This is the
only large refactor and is deliberately last, behind a proven renderer.

## Verification

- Unit tests (ctest): `plotaxismath` tick / format correctness -- deterministic,
  Qt-free, alongside `test_helpers` etc.
- Visual parity: build each backend, run `lammps-gui -c <sample.dat>` under
  `xvfb-run`, capture with `import`, and diff native vs QtCharts for identical
  data (the established GUI-visual-verification workflow).
- Build both link modes (plugin and linked) each phase; keep `ctest` green.

## Risks and mitigations

- Visual-quality gap (tick aesthetics, fonts, AA, high-DPI): screenshot-gated
  parity; reuse Qt's nice-number algorithm; `QPainter` antialiasing + DPR.
- Save-image fidelity: `PlotWidget` renders to an arbitrary-size `QPixmap`, not
  just `grab()`.
- Live-update cost: repaint on the existing timer cadence; a few thousand points
  is trivial for `QPainter`.
- Scope creep: Phase 1 matches current usage only; no new chart features.

## Ground rules

- One concern per commit; GPG-sign every commit.
- Run `clang-format -i src/*.cpp src/*.h` before each commit.
- New `.cpp` / `.h` files go into `PROJECT_SOURCES` in `CMakeLists.txt`.
- Documentation in American English, plain ASCII (`--` not em-dash).
- Build with `-DENABLE_TESTING=ON` and keep `ctest` green after each stage.

## Phase 5 -- collapse the multi-view layout to a single PlotWidget

Status: PLANNED (2026-06-23). Branch: `chart-single-view` (off `develop`).

### Why

`ChartWindow` builds **one full `ChartViewer` widget per thermo column**
(`addChart`), stacks them all in one `QVBoxLayout`, and `hide()`s every one but
the column selected by the `columns` combo (`changeChart`). Each `ChartViewer`
is a `QWidget` wrapping its own `PlotWidget`. That "N heavyweight views, N-1
hidden" shape was dictated by the old Qt chart modules: a `QChartView` /
`GraphsView` was bound to its own chart model and could not be cheaply
repointed at another column's data, so the only way to switch columns was to
build a separate view per column and toggle visibility.

The native `PlotWidget` just paints whatever `PlotSeries` it is handed, so a
single instance can render any column on demand. Collapsing to one renderer
gives: one `PlotWidget` instead of N (less memory, fewer Qt child widgets); a
live-run path that only ever repaints a single widget (compounds with the
per-point/​per-tick perf work already on `develop`); and a clean
data-model-vs-one-view split matching the renderer's design intent.

This is the only remaining QtCharts-era structural artifact in the chart code;
the axis-title overlay workaround (`VerticalLabel`/`QLabel`) was already removed
during the native conversion.

### What makes it tractable / safe

- The data/view split already exists: a `ChartViewer` owns neutral `PlotSeries`
  objects (`series`/`smooth`/`scatter`/`fit`/`overlaySeries`/`vlines`) and a
  child `PlotWidget`; the renderer never holds Qt chart types.
- `changeChart` already **resets the range sliders to full on every switch**, so
  per-column zoom is not persisted today -- the collapse need not preserve it.
- Per-column raw bounds are already tracked incrementally (`rawXmin..rawYmax`).

### Staged commits (verify each with the `-c` CLI screenshot workflow and one
short live run; keep `ctest` green)

1. **Extract per-column state into a non-widget `ChartColumn`.** Move the
   `PlotSeries` set, cached bounds, smoothing params, display style, EOS/fit
   flags, and per-column title/ylabel out of the `ChartViewer` QWidget into a
   plain `ChartColumn` data+logic struct. `ChartViewer` keeps only the QWidget
   shell + its `PlotWidget` for now (no behavior change yet).

   Blast-radius note (measured 2026-06-23): this is *not* a trivial mechanical
   move. There are ~150 bare member references across the `ChartViewer` method
   region, and -- critically -- five member names collide with `ChartWindow`'s
   own members: `smooth`/`window`/`order`/`doRaw`/`doSmooth` exist in **both**
   classes (a `PlotSeries`/`int`/`bool` in `ChartViewer`, a `QComboBox`/
   `QSpinBox`/`bool` in `ChartWindow`). The safe way to do the rename is to scope
   edits strictly to the `ChartViewer` method bodies, which start at the first
   `ChartViewer::` definition in `chartviewer.cpp` (no bare `ChartViewer` member
   is touched before then; `ChartWindow` only reaches a viewer through its public
   `c->method()` API). The `PlotSeries` members are `unique_ptr` (move-only), so
   `ChartColumn` is a move-only type -- fine for the eventual `vector<ChartColumn>`
   as long as it is never copied. Do this as one focused, screenshot-verified
   commit, not a global find/replace (a blind replace would corrupt the
   identically-named `ChartWindow` members).
2. **Make `PlotWidget` render a `ChartColumn` on demand.** Add a
   `ChartWindow`-owned single `PlotWidget` and a `renderColumn(const
   ChartColumn&)` path that feeds it the column's series + labels + zoom. Still
   parallel to the existing per-column viewers (dormant), so it can be
   screenshot-diffed against the live ones.
3. **Switch `ChartWindow` to own `vector<ChartColumn>` + one `PlotWidget`.**
   `addChart` appends a `ChartColumn`; `changeChart` sets an active index and
   calls `renderColumn`; `addData` appends to the target column and repaints
   only when it is active. Delete the per-column `ChartViewer` widgets and the
   show/hide scheme. Rewire the control slots (`updateSmooth`, `updateTLabel`,
   `updateXRange/Y`, `selectSmooth`) to act on the active column (smoothing
   params still applied to all columns' data).
4. **Move the per-column-attached features onto `ChartColumn`.** Postprocess
   fits (autocorrelation new-chart, polynomial/EOS/custom), "Add Data from File"
   overlays, and reference lines currently attach to a specific `ChartViewer`;
   re-point them at the active `ChartColumn`. Repoint the export paths
   (`saveAs`/`copy`/`exportCsv`/`exportDat`/`exportYaml`) and `getStep` to read
   from the column structs.
5. **Remove the now-empty `ChartViewer` widget type** (or rename `ChartColumn`
   to it), update `doc/api_reference.rst`, and refresh this file's status.

### Verification matrix (the established GUI-visual-verification workflow)

Live thermo run (streaming, multi-column switching) + `-c` standalone for:
Points / Lines+Points; multiple Y columns + Data-dropdown switching; X/Y range
sliders; Raw/Smoothed/Both; Postprocess (autocorrelation new-chart, polynomial
overlay, EOS fit, custom function/fit); overlay from "Add Data from File";
reference lines; Save Graph As / Copy; CSV/DAT/YAML export. Screenshot-diff each
against `develop`.
