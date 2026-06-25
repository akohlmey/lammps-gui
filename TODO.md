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

