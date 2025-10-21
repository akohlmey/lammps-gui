# LAMMPS-GUI Development Guide for Copilot

## Project Overview

**LAMMPS-GUI** is a graphical user interface for LAMMPS (Large-scale Atomic/Molecular Massively Parallel Simulator). It's a standalone Qt-based C++ application that provides:
- A syntax-highlighting code editor for LAMMPS input files
- Direct execution of LAMMPS simulations
- Real-time visualization and monitoring of simulation output
- Chart plotting capabilities
- Image/slideshow viewing of simulation snapshots

**Repository**: https://github.com/akohlmey/lammps-gui
**Documentation**: https://lammps-gui.lammps.org/
**Version**: 1.8.3.2 (see CMakeLists.txt line 4)
**License**: GNU GPL v2

### Key Statistics
- **Language**: C++ (C++17 standard)
- **Framework**: Qt (supports both Qt 5.15+ and Qt 6.2+)
- **Build System**: CMake 3.20+
- **Codebase Size**: ~11k lines across 38 source files in `src/`
- **Total Files**: ~11,290 files (includes docs, resources)
- **Disk Size**: ~263 MB

## Architecture & File Layout

### Root Directory Structure
```
├── CMakeLists.txt          # Main build configuration (415 lines)
├── README.md               # Brief overview with CI badges
├── TODO.md                 # Feature roadmap
├── LICENSE                 # GPL v2 license
├── .clang-format           # C++ code formatting rules (LLVM-based, 100 char limit)
├── .gitignore              # Build artifacts, temp files
├── src/                    # C++ source files (38 files)
├── doc/                    # Sphinx documentation (RST format)
├── resources/              # Icons, QRC files, help text
├── plugin/                 # Plugin loader for dynamic LAMMPS library loading
├── packaging/              # Platform-specific packaging scripts
└── .github/                # CI workflows, issue templates
```

### Source Code Organization (`src/`)
**Main application files**:
- `main.cpp` - Application entry point, command-line parsing
- `lammpsgui.{cpp,h,ui}` - Main window and central GUI logic
- `lammpswrapper.{cpp,h}` - C++ interface to LAMMPS C library

**Editor components**:
- `codeeditor.{cpp,h}` - Custom editor widget with line numbers
- `highlighter.{cpp,h}` - LAMMPS syntax highlighting
- `linenumberarea.h` - Line number display
- `findandreplace.{cpp,h}` - Search/replace dialog

**Visualization components**:
- `imageviewer.{cpp,h}` - Image display with zoom/pan
- `chartviewer.{cpp,h}` - Qt Charts integration for plotting
- `slideshow.{cpp,h}` - Multi-image slideshow
- `rangeslider.{cpp,h}` - Custom slider widget for image sequences

**Dialogs & helpers**:
- `preferences.{cpp,h}` - Settings dialog
- `setvariables.{cpp,h}` - Variable substitution dialog
- `fileviewer.{cpp,h}` - File content viewer
- `logwindow.{cpp,h}` - Log output viewer
- `helpers.cpp` - Utility functions
- `stdcapture.cpp` - stdout/stderr capture
- `flagwarnings.{cpp,h}` - LAMMPS flag validation

**Runner**:
- `lammpsrunner.h` - Thread-based LAMMPS execution

### Documentation (`doc/`)
- **Format**: reStructuredText (Sphinx)
- **Build Target**: `html` (creates `build-doc/doc/html/`)
- **Requirements**: `doc/requirements.txt` (Sphinx 6-8.2, extensions)
- **Key Files**: index.rst, installation.rst, basic_usage.rst, etc.

### Resources (`resources/`)
- `lammpsgui.qrc` - Qt resource collection file
- `icons/` - 80+ PNG icons, ICO/ICNS files
- `lammps_internal_commands.txt` - Command reference for auto-completion
- `help_index.table` - Help system index

### Packaging Scripts (`packaging/`)
- `build_linux_tgz.sh` - Linux tarball creation
- `build_macos_dmg.sh` - macOS DMG installer
- `lammps-gui.desktop` - Linux desktop entry
- `lammps-gui.appdata.xml` - Linux appdata metadata
- `org.lammps.lammps-gui.yml` - Flatpak manifest

## Build System & Configuration

### CMake Build Modes

**1. Plugin Mode (Default, Standalone)**
```bash
cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no
cmake --build build --parallel 2
```
- Loads LAMMPS library dynamically at runtime
- Does NOT require LAMMPS source or library at build time
- Default mode when building standalone

**2. Linked Mode (When Built with LAMMPS)**
```bash
cmake -S . -B build \
  -D LAMMPS_GUI_USE_PLUGIN=off \
  -D LAMMPS_SOURCE_DIR=/path/to/lammps/src \
  -D LAMMPS_LIBRARY=/path/to/liblammps.so \
  -D BUILD_DOC=no
cmake --build build
```
- Links directly to LAMMPS library
- Required when building as part of LAMMPS with `-D BUILD_LAMMPS_GUI=on`

**3. Documentation-Only Build**
```bash
cmake -S . -B build-doc -D BUILD_DOC_ONLY=yes
cmake --build build-doc --target doc
```
- Does NOT build the C++ application
- Only builds Sphinx HTML documentation
- Output: `build-doc/doc/html/`

### Important CMake Options
- `LAMMPS_GUI_USE_PLUGIN` - ON (default) for plugin mode, OFF for linked mode
- `LAMMPS_GUI_USE_QT5` - ON to prefer Qt5, OFF (default) prefers Qt6
- `BUILD_DOC` - ON (default) builds docs with app, OFF skips docs
- `BUILD_DOC_ONLY` - ON builds only docs (no app), OFF (default) builds app
- `CMAKE_CXX_STANDARD` - 17 (required minimum), 23 supported
- `CMAKE_BUILD_TYPE` - Release or Debug

### Platform-Specific Notes

#### Linux (Ubuntu 22.04+)
**Required packages**:
```bash
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    libfontconfig1 \
    libglu1-mesa-dev \
    libomp-dev \
    mesa-common-dev \
    qtbase5-dev libqt5charts5-dev    # For Qt5
    # OR
    qt6-base-dev qt6-charts-dev      # For Qt6
```

#### macOS (11+)
- Supports both arm64 and x86_64 architectures
- Multi-arch build: `-D CMAKE_OSX_ARCHITECTURES=arm64;x86_64`
- Compatibility: `-D CMAKE_OSX_DEPLOYMENT_TARGET=11.0`
- Creates app bundle automatically
- DMG target: `cmake --build build --target dmg`

#### Windows
- **Not supported in plugin mode** (LAMMPS_GUI_USE_PLUGIN=ON fails on Windows)
- Visual Studio 2022 + Visual C++ 14.36+ required
- MinGW cross-compilation from Linux is supported
- Qt installation typically in `C:\Qt\`

### Build Targets
- `lammps-gui` (default) - Main executable
- `doc` - Build HTML documentation (alias for `html`)
- `html` - Build Sphinx HTML docs
- `pdf` - Build PDF documentation (requires pdflatex, latexmk)
- `spelling` - Run spell check on docs
- `tarball` - Create source tarball (requires Git)
- `tgz` - Linux binary tarball (plugin mode only)
- `dmg` - macOS DMG installer (macOS only, plugin mode only)
- `flatpak` - Flatpak bundle (currently not supported in plugin mode)

## Continuous Integration & Quality Checks

### GitHub Actions Workflows (`.github/workflows/`)

**1. `compile-linux-qt6.yml`** - Qt 6.x Build
- **Runs on**: Push/PR to `develop` branch
- **Platform**: ubuntu-latest (Qt 6.x)
- **Config**: Debug build, C++23, `-Wall -Wextra`
- **Validation**: Runs `lammps-gui --platform offscreen -v`
- **Duration**: ~2-5 minutes

**2. `compile-linux-qt5.yml`** - Qt 5.15LTS Build
- **Runs on**: Push/PR to `develop` branch
- **Platform**: ubuntu-22.04 (Qt 5.15)
- **Config**: Release build, C++17, Ninja generator
- **Validation**: Runs `lammps-gui --platform offscreen -v`
- **Duration**: ~2-5 minutes

**3. `build-html-docs.yml`** - Documentation Build
- **Runs on**: Push/PR to `develop` branch
- **Platform**: ubuntu-latest
- **Build**: `cmake -S . -B build-doc -D BUILD_DOC_ONLY=yes`
- **Target**: `cmake --build build-doc --target doc`
- **Duration**: ~3-5 minutes (includes pip install)

**4. `codeql-analysis.yml`** - Static Analysis
- **Runs on**: Push to `develop` (NOT on PRs)
- **Platform**: ubuntu-latest (Qt 6.x)
- **Tool**: GitHub CodeQL for C++
- **Config**: `.github/codeql/cpp.yml` (scans only `src/`)
- **Duration**: ~5-10 minutes

### Pre-Commit Validation Checklist
Before submitting a PR, ensure:
1. **Code compiles** with both Qt5 and Qt6 (if possible, or at least one)
2. **Documentation builds** without errors (`cmake --build build-doc --target doc`)
3. **Application runs** (`./build/lammps-gui --platform offscreen -v` shows version)
4. **Code follows style**: Use `.clang-format` (LLVM-based, 100 char limit)
5. **No new compiler warnings** with `-Wall -Wextra`
6. **CodeQL scans pass** (checked automatically on push to develop)

## Common Build Patterns & Issues

### Pattern 1: Clean Build (Documentation Only)
```bash
rm -rf build-doc
cmake -S . -B build-doc -D BUILD_DOC_ONLY=yes
cmake --build build-doc --target doc
# Output: build-doc/doc/html/index.html
```
**Time**: 3-5 minutes (includes virtualenv setup and pip install)  
**Pitfall**: None, very reliable

### Pattern 2: Quick Application Build (No Docs, Plugin Mode)
```bash
mkdir -p build
cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no
cmake --build build --parallel 2
./build/lammps-gui --platform offscreen -v
```
**Time**: 1-3 minutes (after Qt installed)  
**Pitfall**: Requires Qt5 or Qt6 development packages installed

### Pattern 3: Full Build with Documentation
```bash
mkdir -p build
cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=yes
cmake --build build --parallel 2
```
**Time**: 5-8 minutes  
**Pitfall**: Longer due to virtualenv + Sphinx installation

### Common Build Errors & Solutions

**Error**: `Could NOT find Qt6` and `Could NOT find Qt5`
- **Cause**: Qt development packages not installed
- **Solution**: Install Qt dev packages (see platform-specific notes above)
- **Alternative**: For docs only, use `-D BUILD_DOC_ONLY=yes`

**Error**: `Building LAMMPS-GUI with -D LAMMPS_GUI_USE_PLUGIN=on is currently not supported on Windows`
- **Cause**: Plugin mode disabled on Windows in CMakeLists.txt line 46-48
- **Solution**: Use `-D LAMMPS_GUI_USE_PLUGIN=off` and provide LAMMPS library

**Error**: Sphinx import errors during doc build
- **Cause**: virtualenv creation or pip install failed
- **Solution**: Check Python 3.8+ is available; delete `build-doc/docenv` and retry
- **Workaround**: Run manually: `python3 -m venv docenv && docenv/bin/pip install -r doc/requirements.txt`

**Error**: `lammps-gui` command not found after build
- **Location**: Built executable is at `build/lammps-gui` (not installed by default)
- **To install**: `cmake --install build --prefix ~/.local`

## Development Workflow Best Practices

### Making Code Changes
1. **Always test with offscreen platform** first: `./build/lammps-gui --platform offscreen -v`
2. **Check for compiler warnings**: Build with `-D CMAKE_BUILD_TYPE=Debug` and `-Wall -Wextra`
3. **Format before commit**: Use `clang-format` with project's `.clang-format` config
4. **GPG sign commits**: All git commits must be GPG signed with a verifiable signature
5. **Test both Qt versions** if possible (Qt5 and Qt6 have subtle differences)
6. **Update docs** if changing user-facing features (files in `doc/` directory)

### Adding New Source Files
1. Add to `PROJECT_SOURCES` list in `CMakeLists.txt` (lines 88-127)
2. If using Qt signals/slots, ensure `CMAKE_AUTOMOC=ON` is set (line 24)
3. If using `.ui` files, ensure `CMAKE_AUTOUIC=ON` is set (line 23)
4. Include file in `#include` directives in relevant source files

### Modifying Resources
- Icons: Add to `resources/icons/` and reference in `resources/lammpsgui.qrc`
- Help text: Update `resources/lammps_internal_commands.txt`
- After changes: Qt resource system auto-compiles via `qt6_add_resources()` (line 129)

### Documentation Updates
- **Format**: reStructuredText (`.rst` files in `doc/`)
- **Spell check**: Run `cmake --build build-doc --target spelling`
- **Preview locally**: Open `build-doc/doc/html/index.html` in browser
- **CI validates**: Every PR builds docs automatically

## Dependency Management

### Python Dependencies (Documentation)
- **File**: `doc/requirements.txt`
- **Version constraints**: Sphinx 6-8.2.3, docutils 0.18-0.23
- **Update frequency**: Weekly (Dependabot monitors)
- **Installation**: Automatic via CMake in virtualenv `build-doc/docenv/`

### GitHub Actions Dependencies
- **File**: Workflow YAML files use `actions/checkout@v5`, `codeql-action/init@v4`
- **Update frequency**: Weekly (Dependabot monitors)

### No Package Manager for C++
- Project has no `package.json`, `requirements.txt` for C++, `Cargo.toml`, etc.
- All C++ dependencies managed via system package manager (apt, brew, etc.)

## Testing & Validation

### No Automated Unit Tests
- **Important**: This project has NO automated test suite
- **No test framework**: No gtest, catch2, Qt Test, etc.
- **Validation method**: Manual testing and CI compilation checks
- **CI only verifies**: Code compiles and app starts (`--platform offscreen -v`)

### Manual Testing Checklist
When making changes:
1. Build succeeds on Linux with Qt5 and Qt6
2. Application launches: `./build/lammps-gui`
3. Can open/edit LAMMPS input files
4. Syntax highlighting works
5. Can run simulations (requires LAMMPS library in plugin mode)
6. Documentation builds without errors
7. No new compiler warnings

## Important Notes & Constraints

### ALWAYS Follow These Rules
1. **Never use `git push` or `gh` commands directly** - Use `report_progress` tool to commit/push
2. **Always use absolute paths** starting with `/home/runner/work/lammps-gui/lammps-gui/`
3. **Document build must complete in <300s** - It's a real constraint (virtualenv setup takes time)
4. **Plugin mode doesn't work on Windows** - This is a known limitation (see CMakeLists.txt:46-48)
5. **Qt version auto-detection** - Qt6 preferred over Qt5 unless `LAMMPS_GUI_USE_QT5=ON`
6. **C++17 minimum** - Can use C++23 for Qt6 builds, but C++17 for Qt5 compatibility

### Known Issues & Workarounds
- **No TBD/TODO in code**: Only one found at `src/lammpsgui.cpp` (update tutorial URL)
- **No HACK/FIXME comments**: Code is generally clean
- **Install prefix**: Defaults to `~/.local` (not `/usr/local`) for user installs without sudo

### Version Compatibility
- **CMake**: 3.20+ required (check: `cmake --version`)
- **Qt**: 5.15+ or 6.2+ required
- **Python**: 3.8+ required (for documentation build)
- **LAMMPS library**: Minimum version "22 July 2025 update 2" (see `doc/installation.rst:82`)

## Quick Reference Commands

### Build Application Only (No Docs)
```bash
cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no
cmake --build build --parallel 2
./build/lammps-gui --platform offscreen -v
```

### Build Documentation Only
```bash
cmake -S . -B build-doc -D BUILD_DOC_ONLY=yes
cmake --build build-doc --target doc
# Open build-doc/doc/html/index.html
```

### Clean Build
```bash
rm -rf build build-doc
# Then rebuild as needed
```

### Check for Warnings
```bash
cmake -S . -B build -D CMAKE_BUILD_TYPE=Debug \
  -D CMAKE_CXX_FLAGS_DEBUG="-Og -g -Wall -Wextra" \
  -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no
cmake --build build 2>&1 | grep -i "warning:"
```
## Code Review

When performing a code review, check any changes to the documentation
(in the `doc/` folder) to be written in American English and with plain
ASCII characters.

## Trust These Instructions

These instructions have been thoroughly researched by examining:
- All workflow files, CMakeLists.txt, build scripts
- Documentation (README, installation.rst, TODO.md)
- Source code structure and build patterns
- Actual successful build of documentation

**Only search or explore further if**:
- These instructions are incomplete for your specific task
- You encounter errors not covered here
- The information appears outdated or incorrect
- You need details about specific source files not covered

For implementation details of specific features, refer to the source files in `src/`. For user-facing behavior, check the documentation in `doc/`. For build system internals, study `CMakeLists.txt` (well-commented, 415 lines).
