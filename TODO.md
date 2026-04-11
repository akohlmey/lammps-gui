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

The following code-quality refactoring items have been identified.  Items
already implemented are marked with [x]; remaining items are marked with [ ].

## Completed refactoring

- [x] **1. Extract constants** — Created `constants.h` with `GuiConstants`
  namespace.  Moved magic numbers (buffer sizes, window dimensions, file
  limits, resource paths, status messages) out of scattered source files.

- [x] **2. Unsaved-changes dialog** — Extracted
  `showUnsavedChangesDialog()` helper into `helpers.h/cpp` to replace
  five near-identical `QMessageBox` blocks in `lammpsgui.cpp`.

- [x] **3. Memory management fixes**
  - [x] 3a. `GUI_MONOFONT`/`GUI_ALLFONT` → `std::unique_ptr<QFont>`
  - [x] 3b. `LammpsRunner::setupRun` → takes `std::string` (not raw `char*`)
  - [x] 3c. `StdCapture` internal buffer → `std::vector<char>`
  - [x] 3d. `FileViewer` compression lookup → static table instead of
    repeated `if-else` chain

- [x] **4d. Replace mystrdup** — `lammpsArgs` changed from
  `vector<char*>` (manual `mystrdup`/`delete[]`) to `vector<std::string>`.
  A temporary `char**` adapter is built at the `lammps.open()` call site.
  `mystrdup` is no longer called in production code.

- [x] **4e. CodeEditor destructor cleanup** — Removed 21 redundant
  `delete` calls (Qt parent-child ownership suffices).  Replaced the
  `COMPLETER_SETUP` preprocessor macro with a lambda.

- [x] **5. Decompose setupUi** — Split the monolithic `setupUi()` into
  `createFileMenu()`, `createEditMenu()`, `createRunMenu()`,
  `createViewMenu()`, `createTutorialMenu()`, `createAboutMenu()`,
  `createStatusBar()`, and `connectSignalsAndSlots()`.  Sub-window
  initialization consolidated into `configureSubWindow()`.

- [x] **10. ChartViewer backend abstraction** — Extracted a
  `ChartBackend` abstract interface (`chartbackend.h`) with
  `QtGraphsBackend` and `QtChartsBackend` implementations.  `ChartViewer`
  always inherits `QWidget` and delegates via `unique_ptr<ChartBackend>`.
  Eliminated all `#ifdef LAMMPS_GUI_USE_QTGRAPHS` from `chartviewer.cpp`
  except backend instantiation.

- [x] **12. Naming convention cleanup** — Renamed `StdCapture` methods
  to camelCase (`beginCapture`, `endCapture`, `getCapture`, `getChunk`,
  `getBufferUse`).  Renamed `LammpsWrapper::is_open()` → `isOpen()`.
  Updated all callers and test files.

- [x] **14. Reduce include dependencies in lammpsgui.h** — Removed
  `QGridLayout`, `QSpacerItem`, `QEvent` includes; replaced with forward
  declarations where needed.

## Pending refactoring (recommendations)

The items below are listed roughly in order of impact vs. effort.

- [ ] **6. Move remaining magic numbers to constants.h** — Several
  hardcoded defaults remain in `preferences.cpp` (e.g. `resize(700, 500)`,
  `setRange(1, 1000)`, `setRange(1, 5000)`, `setRange(5, 999)`,
  `(400, 40000)`, `(300, 30000)`) and `chartviewer.cpp`
  (`settings.value("updchart", "500")`).  Centralizing these in
  `GuiConstants` improves discoverability and reduces duplication.
  **Strategy**: audit every `resize()`, `setRange()`, and
  `settings.value(…, <default>)` call; introduce named constants for each
  default.

- [ ] **7. Centralize QSettings key strings** — Settings keys like
  `"updchart"`, `"xsize"`, `"ysize"`, `"zoom"` are bare string literals
  repeated in multiple files (`imageviewer.cpp` lines 342-344 *and*
  749-751, `chartviewer.cpp`, `preferences.cpp`).  Creating an
  `inline constexpr` string table (or enum + lookup) would catch typos at
  compile time and make rename-safe refactoring trivial.
  **Strategy**: define `namespace SettingsKeys { ... }` in `constants.h`
  with one `inline const QString` per key; replace all bare literals in a
  single sweep.

- [x] **8. Remove duplicate image-settings fetch in ImageViewer** —
  Extracted ``ImageViewer::readImageSettings()`` private method that
  reads all snapshot QSettings and resets member variables.  Both the
  constructor and ``resetView()`` now call this method instead of
  duplicating the settings-fetch code.

- [ ] **9. Break up large methods in lammpsgui.cpp** — Five methods
  exceed 100 lines: `logUpdate()` (154), `setupTutorial()` (135),
  `startLammps()` (132), `doRun()` (108), `runDone()` (105).  Each can
  be decomposed without changing behavior:
  - `logUpdate()` → extract chart-update and image-update loops
  - `setupTutorial()` → extract download-and-extract step
  - `startLammps()` → extract argument-building step
  **Strategy**: one method at a time, each in its own commit, running
  tests after each extraction.

- [x] **11. Remove dead `mystrdup` code** — Removed the three
  ``mystrdup`` overloads from ``helpers.h/cpp`` (no longer called by
  any production code).  The functions and their seven test cases are
  preserved inside ``test/test_helpers.cpp`` in an anonymous namespace
  so regression coverage is maintained without polluting the public API.

- [x] **13. Further include-dependency reduction** — Audited all 18
  non-``lammpsgui.h`` headers.  Replaced ``#include "codeeditor.h"``
  with a forward declaration in ``findandreplace.h`` (CodeEditor is only
  used as a pointer).  Removed the redundant ``class QComboBox;``
  forward declaration in ``imageviewer.h`` (already fully included).
  All remaining headers were already optimal.
