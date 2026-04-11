LAMMPS-GUI TODO list:

# Short term goals (v1.x)

- add a "Colors" menu to the image viewer to adjust color settings for the
  current image (unlike the defaults in the perferences) including assigning
  colors to individual atom types.

- implement data file manager GUI with the following features:
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

# Long term ideas (v2.x)
- add option to attach a debugger to the running program (highly non-portable, need customization support in preferences)
- write a "wizard" dialog that can be used for beginners to create an input file template for a few typical use scenarios
  (could perhaps use some LLM based KI to look up suggestions for answers?).

# Refactoring status and recommendations

The following code-quality refactoring items have been identified.
The items below are listed roughly in order of impact vs. effort.

- [ ] **1. Move remaining magic numbers to constants.h** — Several
  hardcoded defaults remain in `preferences.cpp` (e.g. `resize(700, 500)`,
  `setRange(1, 1000)`, `setRange(1, 5000)`, `setRange(5, 999)`,
  `(400, 40000)`, `(300, 30000)`) and `chartviewer.cpp`
  (`settings.value("updchart", "500")`).  Centralizing these in
  `GuiConstants` improves discoverability and reduces duplication.
  **Strategy**: audit every `resize()`, `setRange()`, and
  `settings.value(…, <default>)` call; introduce named constants for each
  default.

- [ ] **2. Centralize QSettings key strings** — Settings keys like
  `"updchart"`, `"xsize"`, `"ysize"`, `"zoom"` are bare string literals
  repeated in multiple files (`imageviewer.cpp` lines 342-344 *and*
  749-751, `chartviewer.cpp`, `preferences.cpp`).  Creating an
  `inline constexpr` string table (or enum + lookup) would catch typos at
  compile time and make rename-safe refactoring trivial.
  **Strategy**: define `namespace SettingsKeys { ... }` in `constants.h`
  with one `inline const QString` per key; replace all bare literals in a
  single sweep.

- [ ] **3. Break up large methods in lammpsgui.cpp** — Five methods
  exceed 100 lines: `logUpdate()` (154), `setupTutorial()` (135),
  `startLammps()` (132), `doRun()` (108), `runDone()` (105).  Each can
  be decomposed without changing behavior:
  - `logUpdate()` → extract chart-update and image-update loops
  - `setupTutorial()` → extract download-and-extract step
  - `startLammps()` → extract argument-building step
  **Strategy**: one method at a time, each in its own commit, running
  tests after each extraction.
