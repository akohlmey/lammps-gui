# Native chart backend -- design and staged plan

Status: planning / in development (started 2026-06-22).
Branch: `native-chart-backend` (off `develop`).

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

### Phase 1 -- optional native backend (this cycle, before v2.1.0)

Minimal blast radius. Default behavior unchanged.

1. Commit: `plotaxismath.{cpp,h}` (Qt-free) + GoogleTest unit tests for tick
   intervals and label formatting.
2. Commit: `plotseries.{cpp,h}` neutral model + `plotwidget.{cpp,h}` renderer,
   exercised by a standalone path so it can be screenshot-tested before wiring.
3. Commit: `nativechartbackend.{cpp,h}` adapter + `ChartBackend::create()`
   factory collapsing the `#ifdef`s; native restores dashed reference lines.
4. Commit: CMake `LAMMPS_GUI_USE_NATIVE_CHARTS` option (default OFF) + wiring;
   docs (CLAUDE.md "Chart backend abstraction" + CMake options table).

One signed commit per concern; each builds both plugin and linked configs.
Gate: native output is visually on par with QtCharts via the `-c` CLI
screenshot harness under `xvfb-run`.

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
