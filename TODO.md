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

## Offer a few more perceptually-uniform color maps (optional)
  Done: the preview swatches and the emitted `amap`/`bmap` stops now share a
  single source of truth (`colormaps.{h,cpp}`, consumed by both
  `appendColorMapArgs()` in `dumpimage.cpp` and `addColorMapItems()` in
  `imageviewersettings.cpp`), the perceptually-uniform maps were corrected
  against canonical matplotlib data (Viridis/Plasma/Inferno; the old "Inferno"
  was really magma), and Magma was added. Optionally add Cividis and Turbo
  (canonical stops are easy to resample from matplotlib) to the table and the
  `colorMapNames()` list.

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

## Completed stages 1-8 (done; see git history for details)

All eight stages are complete and committed on the `refactor/cleanup`
branch: constants/keys centralization (1), wrapper string consolidation
(2), `LammpsWrapper` plugin/linked dispatch de-duplication (3), typed
data-extraction helpers (4), oversized-method/file decomposition (5), the
modern-C++ breadth pass (6), the `createImage` struct-driven renderer
extraction (7), and the reusable-ChartViewer external-data plotting +
post-processing feature -- including the vendored `LeptonMini` expression
parser and the Levenberg-Marquardt custom fit (8).

## Remaining refactor items (deferred from Stage 6; low priority)

Both were assessed and intentionally left as-is -- high churn for low
value. Revisit only when already editing the surrounding code:

- **`enum class` for internal enumerations.** `AccelType`/`AccelPrec` are
  persisted in QSettings (read via `.toInt()`, compared against `int`,
  passed as `QVariant` defaults) and `PIPES` is used as array indices, so
  they genuinely need int interop; the `LammpsWrapper`
  `StyleConst`/`ScopeConst`/`TypeConst` likewise map to LAMMPS ints.
  `enum class` would only add `static_cast` noise. No suitable candidate.
- **Audit dialog `findChild`/`setObjectName` wiring** (137 / 113 uses).
  Replacing the `findChild`-based readback with typed member pointers is
  the silent-failure-on-rename operation that is unsafe to do without a
  real GUI run to catch mistakes (build/tests would not). Object names
  were kept exact.

## Stage 8 plotting -- optional follow-ups

- *Custom-fit parameter bounds.* `levmar` is currently unconstrained; the
  Grace template offers per-parameter bounds. Add box constraints
  (projected steps or a transform) and a bounds column to the "Custom fit"
  parameters input.
- *Visible legend for overlays.* The fit/overlay curve is now nameable
  (`ChartViewer::setFitCurve(points, name)`), but both backends hide the
  legend (`chart->legend()->hide()` in QtCharts). Optionally expose a
  legend so the custom-fit "Label", smoothed/raw series, and fit curve are
  identified.
- *Reusing fitted models.* Offer "plot this fit's expression with the
  fitted parameters" or send fitted params back into a "Custom function"
  plot.

**Deliberately NOT in scope:** multiple Y axes; log/log or date axes;
spreadsheet/data editing; annotations or legend-as-objects; many-dataset
overlay management beyond raw+derived; 3D/surface/bar/pie; session files.
(These are exactly where Veusz/LabPlot earn their complexity.)

**Survey references (minimalist scientific plotting + fitting + EOS):**
Grace/Xmgrace (non-linear fit popup with named params a0..a9 + bounds +
tolerance is the UI template; per-set line/symbol appearance);
SciDAVis/LabPlot/Veusz (ASCII import -> columns -> fit flow to borrow,
depth to avoid); BM4 EOS linear form (DFTTK, murnaghan2017); Lepton
expression syntax + analytic differentiation (LAMMPS/OpenMM docs).
