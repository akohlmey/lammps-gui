LAMMPS-GUI TODO list:

# New feature ideas

## more Run menu features
  - add option to extend a run with N more steps
    (open dialog with fields to add run command settings and number of steps and then dispatch)
    (NB, we need to have a flag to determine if the system is in a runnable state, e.g. has been run before)
    possible use case: Click on STOP, write restart (see below), continue run
    or: run didn't produce a long enough animation and needs more steps
  - add option to write out a restart file with the current state of the system

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

## Snapshot viewer (SlideShow) enhancements
  Both follow-ups to the "view arbitrary image files in the snapshot viewer"
  feature are done: the conversion cache is `ImageCache`
  (`src/imagecache.{cpp,h}`), and movie frame extraction is `MovieImportDialog`
  plus the probe/extract free functions in `src/movieimport.{cpp,h}`, routed
  through `isMovieFile()` and `SlideShow::addMovie()`.
  Possible further work:
  - Reuse the extracted frames of a movie between sessions, e.g. by caching
    them next to the movie file instead of in a temporary folder.
  - Show a thumbnail of the sample frame in the movie import dialog, and
    refresh the size estimate when the selected range moves far from it.

