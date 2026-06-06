# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

LAMMPS-GUI (v2.0.6) is a Qt6-based graphical interface for the LAMMPS molecular dynamics simulation software. It provides a code editor with syntax highlighting and auto-completion, live LAMMPS simulation execution, log/chart/image visualization, and an integrated tutorial system. The project is GPLv2+ licensed (note: `src/rangeslider.{cpp,h}` is third-party under the CeCILL-A license).

- Online documentation: https://lammps-gui.lammps.org/
- C++17, CMake ≥ 3.20, Qt6 (minimum 6.2; 6.10+ enables QtGraphs backend)

## Build Commands

**Typical plugin-mode build** (LAMMPS library loaded dynamically at runtime — the default):
```bash
cmake -S . -B build -DLAMMPS_GUI_USE_PLUGIN=ON -DBUILD_DOC=OFF
cmake --build build -j$(nproc)
```

**Linked mode** (requires LAMMPS source tree and pre-built library):
```bash
cmake -S . -B build \
  -DLAMMPS_GUI_USE_PLUGIN=OFF \
  -DLAMMPS_SOURCE_DIR=/path/to/lammps/src \
  -DLAMMPS_LIBRARY=/path/to/liblammps.so
cmake --build build -j$(nproc)
```

Default install prefix is `$HOME/.local` (no root required).

**Useful CMake options:**

| Option | Default | Description |
|---|---|---|
| `LAMMPS_GUI_USE_PLUGIN` | `ON` | Load LAMMPS `.so` at runtime via dlopen |
| `LAMMPS_GUI_USE_QTCHARTS` | `OFF` | Force QtCharts even when Qt 6.10+ is available |
| `BUILD_DOC` | `ON` | Build Sphinx HTML docs along with the app (slow; disable for code-only work) |
| `BUILD_DOC_ONLY` | `OFF` | Build only Sphinx/Doxygen docs, skip the C++ binary entirely |
| `ENABLE_TESTING` | `OFF` | Enable unit + GUI tests (Linux only) |

**Documentation-only build** (no C++ compilation needed):
```bash
cmake -S . -B build-doc -DBUILD_DOC_ONLY=ON
cmake --build build-doc --target doc
# Output: build-doc/doc/html/index.html
```

**Documentation build targets** (when `BUILD_DOC=ON` or `BUILD_DOC_ONLY=ON`):

| Target | Description |
|---|---|
| `html` / `doc` | Full Sphinx HTML docs (runs Doxygen first) |
| `doxygen` | Doxygen XML only (intermediate step) |
| `pdf` | LaTeX → PDF (requires `pdflatex` + `latexmk`) |
| `spelling` | Sphinx spell checker |
| `linkcheck` | Sphinx broken-link checker |

## Testing

Tests are Linux-only and off by default. Enable them at configure time:
```bash
cmake -S . -B build -DLAMMPS_GUI_USE_PLUGIN=ON -DBUILD_DOC=OFF -DENABLE_TESTING=ON
cmake --build build -j$(nproc)
ctest --test-dir build --output-on-failure
```

Run a subset by name pattern:
```bash
ctest --test-dir build -R test_helpers --output-on-failure
ctest --test-dir build -R Framebuffer --output-on-failure   # GUI tests need Xvfb + screenshooter
```

**Test categories:**
- `test_helpers` / `test_stdcapture` / `test_flagwarnings` — C++ unit tests (GoogleTest v1.17.0, fetched automatically via FetchContent)
- `CommandLine.*` — command-line flag smoke tests
- `Framebuffer.*` — Python/PyAutoGUI GUI tests run inside Xvfb; require `xvfb-run` and one of: `magick`, `import`, `xfce4-screenshooter`, or `gnome-screenshooter`

## Code Style

All C++ source is formatted with **clang-format** using the config in `.clang-format` (LLVM base, 4-space indent, 100-column limit, custom brace wrapping). Before committing:
```bash
clang-format -i src/*.cpp src/*.h
```

File headers use `// -*- c++ -*-` Emacs mode line; maintain it on new files.

## Commit & Code Conventions

- **GPG-sign all commits.** Every commit must carry a verifiable GPG signature.
- **Doxygen comments on all new public APIs.** Use `/** @brief ... */` Javadoc style for classes and methods; `///< description` for member variables. See `src/lammpsgui.h` for a comprehensive example.
- **New public classes need a `.. doxygenclass::` entry in `doc/api_reference.rst`.** The `helpers.h` block uses `.. doxygenfile:: helpers.h :sections: func`, which renders only *free functions*, so a new helper class (e.g. an RAII guard) is otherwise missing from the generated API docs.
- **Documentation changes in American English with plain ASCII characters** (no typographic quotes, em-dashes as `--`, etc.).
- **New `.cpp`/`.h` files must be added to `PROJECT_SOURCES`** in `CMakeLists.txt` (around line 90). Qt's `AUTOMOC` handles `moc` generation automatically, but the file must be listed there.

## Architecture

### Core component relationships

```
main.cpp
  └─ LammpsGui (QMainWindow)           ← central coordinator
       ├─ CodeEditor (QPlainTextEdit)  ← input script editor
       │    ├─ Highlighter             ← LAMMPS syntax highlighting
       │    ├─ FindAndReplace          ← non-modal find/replace dialog
       │    └─ QCompleter × N         ← per-command-type auto-complete
       ├─ LammpsWrapper               ← thin C++ wrapper around LAMMPS C API
       ├─ LammpsRunner (QThread)       ← runs LAMMPS in background thread
       ├─ StdCapture                   ← redirects stdout→pipe to capture LAMMPS output
       ├─ LogWindow (QPlainTextEdit)   ← displays captured log; uses FlagWarnings highlighter
       ├─ ImageViewer (QDialog)        ← interactive dump-image viewer
       ├─ SlideShow (QDialog)          ← slideshow viewer for image sequences
       ├─ TutorialWizard (QWizard)     ← step-by-step tutorial setup wizard
       ├─ ChartWindow                  ← thermo chart container
       │    └─ ChartViewer × N        ← one chart per thermo column
       │         └─ ChartBackend*     ← abstract; QtGraphsBackend or QtChartsBackend
       └─ Preferences (QDialog)        ← settings; stored via QSettings
```

### Key design points

**Plugin vs. linked mode.** When built with `LAMMPS_GUI_USE_PLUGIN=ON` (default), the executable has no link-time dependency on LAMMPS. `plugin/liblammpsplugin.c` provides `dlopen`-based dispatch; `LammpsWrapper` calls through function pointers loaded at startup. This lets the GUI ship as a standalone binary that can download or swap LAMMPS shared libraries.

**Chart backend abstraction.** `ChartBackend` (`src/chartbackend.h`) is a pure virtual interface hiding the differences between QtGraphs (QML/Qt Quick, Qt ≥ 6.10) and QtCharts (widget-based, Qt ≥ 6.2). The concrete implementations are `QtGraphsBackend` and `QtChartsBackend`. The build system selects the right one at compile time via the `LAMMPS_GUI_USE_QTGRAPHS` preprocessor define.

**Threading model.** LAMMPS simulations run on a `LammpsRunner` (QThread). `StdCapture` intercepts the LAMMPS library's stdout by replacing the file descriptor before `LammpsRunner::run()` starts. A `QTimer` in `LammpsGui` polls `StdCapture::getChunk()` to feed `LogWindow` without blocking the UI thread.

**Auto-completion.** `CodeEditor` maintains a separate `QCompleter` instance for each LAMMPS command category (fix styles, compute styles, pair styles, etc.). Completions are populated from style lists queried from `LammpsWrapper` after LAMMPS is initialized, plus static tables embedded as Qt resources.

**Resources.** `resources/lammpsgui.qrc` embeds icons, `help_index.table` (maps LAMMPS commands to doc URLs), `image_style.table` (dump image options), and `lammps_internal_commands.txt`. The `.table` files are plain text and have companion shell scripts (`update-help-index.sh`, `update-image-styles.sh`) to regenerate them from a LAMMPS source tree.

**Constants.** Application-wide magic numbers and repeated string literals live in `src/constants.h` inside `namespace GuiConstants`. New hardcoded values should go there. QSettings key strings are currently bare literals (a known refactoring item — see `TODO.md`).

**Minimum LAMMPS version.** `GuiConstants::MIN_LAMMPS_VERSION` (currently `20260330`) is enforced at startup; the GUI warns and may refuse to run with older LAMMPS builds.

**Dialog widget wiring.** `ImageViewer` and the `Preferences` tabs connect widgets to slots via `setObjectName("...")` + later `findChild<T>("...")` rather than stored member pointers. Preserve object names exactly when refactoring these dialogs (a wrong/renamed name fails the lookup silently, with no compile error).

**Shared helpers (prefer over re-rolling).** Use the `StdoutSilencer` RAII guard (`helpers.h`) instead of manual `silenceStdout()`/`restoreStdout()` pairs; `LammpsWrapper::lastErrorMessage()` instead of a hand-managed `getLastErrorMessage()` buffer; `LammpsGui::addMenuAction()` to build menu actions.

### String handling & modern C++ conventions

These are the settled conventions for new and refactored code. A staged
plan for bringing existing code into line lives under "Refactoring status
and recommendations" in `TODO.md`.

**QString is the canonical internal string type.** It already dominates
(~350 declarations vs. ~25 `std::string`). Keep `char *` and `std::string`
out of internal interfaces; pass and return `QString`.

**Confine all string conversions to `LammpsWrapper`** (the LAMMPS C API is
the only place `char *` is unavoidable). Do not sprinkle `toStdString()` /
`.c_str()` / `char buf[N]` at call sites. Two patterns already in the
wrapper are the templates to copy:
- *Input:* provide three overloads -- `const char *`, `const QString &`,
  `const std::string &` -- as `command()`, `file()`, and
  `commandsString()` do, so callers pass whatever they hold.
- *Output:* return a `QString` and manage the buffer internally, as
  `lastErrorMessage()` does. Prefer this over `char *`-out-param APIs
  (`idName`, `styleName`, `variableInfo`, `getLastErrorMessage`), which are
  legacy and slated for QString-returning overloads.

Avoid `QString -> std::string -> QString` round-trips (the `splitLine`
call sites are the current offenders).

**Match the existing modern-C++ baseline.** This code already uses
`nullptr`, `auto`, range-based `for`, `override`, `constexpr`, `= default`,
and an explicit Rule-of-5 (`= delete` / `= default` for all five special
members) on essentially every class; mirror that on new classes. Prefer
`std::make_unique` and smart pointers for owned non-QObject resources
(QObject parent/child ownership via `new` with a parent is still the Qt
idiom and is fine). Use `static_cast` rather than C-style casts, the
function-pointer `connect()` form (never `SIGNAL()`/`SLOT()` strings), and
`enum class` for new internal enumerations that do not need implicit `int`
interop with the LAMMPS API.

### Source file map

| File(s) | Responsibility |
|---|---|
| `src/main.cpp` | App entry point, CLI parsing, font init |
| `src/lammpsgui.{cpp,h}` | Main window: menus, file ops, run control, tutorial wizard glue |
| `src/lammpswrapper.{cpp,h}` | All calls to the LAMMPS C library API |
| `src/lammpsrunner.{cpp,h}` | Background thread that calls `lammps->commandsString()` / `lammps->file()` |
| `src/stdcapture.{cpp,h}` | fd-level stdout capture using a pipe |
| `src/codeeditor.{cpp,h}` | Custom editor: line numbers, context menu help, drag-and-drop |
| `src/linenumberarea.h` | Header-only margin widget used internally by `CodeEditor` |
| `src/highlighter.{cpp,h}` | Syntax highlighting rules for LAMMPS input scripts |
| `src/findandreplace.{cpp,h}` | Non-modal find/replace dialog for the editor |
| `src/logwindow.{cpp,h}` | Log viewer; delegates warning highlighting to `FlagWarnings` |
| `src/flagwarnings.{cpp,h}` | QSyntaxHighlighter for warnings/errors/URLs in log text |
| `src/chartviewer.{cpp,h}` | `ChartWindow` (container) + `ChartViewer` (one chart per column) |
| `src/chartbackend.h` | Abstract chart backend interface |
| `src/qtchartsbackend.{cpp,h}` | QtCharts concrete backend |
| `src/qtgraphsbackend.{cpp,h}` | QtGraphs concrete backend |
| `src/imageviewer.{cpp,h}` | Dump-image viewer with interactive re-render controls |
| `src/slideshow.{cpp,h}` | Slideshow viewer for sequences of dump images with playback controls |
| `src/preferences.{cpp,h}` | Tabbed settings dialog (general, accelerators, editor, charts, images) |
| `src/setvariables.{cpp,h}` | Dialog for editing index-style LAMMPS variable name/value pairs |
| `src/tutorialwizard.{cpp,h}` | Step-by-step wizard for setting up and launching LAMMPS tutorials |
| `src/fileviewer.{cpp,h}` | Read-only text viewer for files referenced in input scripts |
| `src/aboutdialog.{cpp,h}` | Auto-scrolling About dialog showing LAMMPS version and style info |
| `src/urldownloader.{cpp,h}` | HTTPS file downloader (respects `https_proxy` setting) |
| `src/helpers.{cpp,h}` | Platform utilities, dialog helpers, stdout silence/restore |
| `src/qaddon.{cpp,h}` | Utility widgets: `QHline`, `QColorCompleter`, `QColorValidator`, `VerticalLabel` |
| `src/rangeslider.{cpp,h}` | Dual-handle range slider widget (third-party, **CeCILL-A license**) |
| `src/constants.h` | `GuiConstants` namespace: all magic numbers and string constants |
| `plugin/liblammpsplugin.{c,h}` | C shim for dynamic LAMMPS library loading |
| `resources/` | Qt resources: icons, help tables, commands list |
| `test/` | Unit tests (GoogleTest) and Python GUI tests (PyAutoGUI/Xvfb) |
| `doc/` | Sphinx documentation sources (`requirements.txt` for venv) |
| `packaging/` | Platform packaging scripts (flatpak, DMG, NSIS, tgz) |
