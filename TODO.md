LAMMPS-GUI TODO list:

**Current priorities (chosen 2026-06-20):** the Stage 8 plotting follow-ups and
the SlideShow enhancements, both marked "(priority)" below.  Everything else is
a longer-term idea or deliberately out of scope.

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

## Snapshot viewer (SlideShow) enhancements (priority)
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

The staged code-cleanup and C++17-modernization effort (stages 1-8 below) is
complete; the original "biggest opportunities" -- string-handling churn at the
char* / std::string / QString boundary, the LammpsWrapper plugin-vs-linked
dispatch duplication, and a handful of oversized methods -- have all been
addressed. The conventions that effort settled remain the standard for new and
refactored code. See CLAUDE.md ("String handling & modern C++ conventions").

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

## Completed stages 1-8 (done; see git history on the `refactor/cleanup` branch)

## Stage 8 plotting follow-ups (priority)

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
