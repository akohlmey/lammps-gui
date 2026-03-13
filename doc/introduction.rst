********
Overview
********

LAMMPS-GUI is built using C++17 and the Qt framework (Qt 5.15+ or Qt 6.2+).
The application follows object-oriented design principles with clear separation
of concerns between different components:

- **Editor Components**: Handle text editing, syntax highlighting, and auto-completion
- **LAMMPS Interface**: Wraps the LAMMPS C library API
- **Visualization**: Displays images, charts, and simulation output
- **GUI Framework**: Main window, dialogs, and preferences

==================
 LAMMPS Interface
==================

LAMMPS-GUI can operate in two modes: **Plugin Mode** and **Linked
Mode**.  The mode is controlled by the ``-D
LAMMPS_GUI_USE_PLUGIN=(ON|OFF)`` CMake configuration option.

**Plugin Mode** (default)
  LAMMPS is loaded dynamically at runtime from a shared library file
  (.so, .dll, .dylib).  This allows using different LAMMPS builds with
  different compilation setting and different LAMMPS versions without
  recompiling the GUI. The library loading is handled in
  :cpp:class:`LammpsWrapper` using platform-specific dynamic loading
  functions (``dlopen()`` on Unix/Linux/macOS, ``LoadLibrary()`` on
  Windows).  The path to the shared library file is auto-detected or
  configured via command line or preferences.

**Linked Mode** LAMMPS library is linked at compile time.  Used by
  default when building LAMMPS-GUI as part of a LAMMPS CMake build with
  ``-D BUILD_LAMMPS_GUI=on``.  For standalone builds also the ``-D
  LAMMPS_SOURCE_DIR=<path to LAMMPS' src folder>`` and ``-D
  LAMMPS_LIBRARY=<path to LAMMPS shared or static library file>``
  settings are required when configuring with CMake.  It may be needed
  to adjust the environment variable to find shared libraries (``LD_LIBRARY_PATH``
  on Linux, ``DYLD_LIBRARY_PATH`` on macOS, or ``PATH`` on Windows)
  when linked to a shared library.

================
 Qt Integration
================

LAMMPS-GUI makes extensive use of Qt features:

**Signals and Slots**
  Used for inter-component communication, especially between GUI
  components and background threads.

**Qt Designer Forms**
  The main window layout uses a ``.ui`` file edited in Qt Designer.
  Dialogs are created programmatically in C++.

**Qt Resource System**
  Icons and resources embedded via ``resources/lammpsgui.qrc``.

**Qt Models**
  Used for data display in various viewers and inspectors.

For more details on Qt usage, see the `Qt Documentation <https://doc.qt.io/>`_.

------------------

************
Architecture
************

=================
 Main Components
=================

The application architecture consists of several key components organized into
functional groups:

Main Window
-----------

**LammpsGui (lammpsgui.h/.cpp)**
  The main window class that coordinates all other components. It manages
  the editor, handles file operations, controls LAMMPS execution, and
  manages the overall application state. This is the central hub of the
  application that integrates all other components.

Editor Components
-----------------

**CodeEditor (codeeditor.h/.cpp)**
  Custom text editor widget based on QPlainTextEdit, providing LAMMPS-specific
  features including syntax highlighting, auto-completion, line numbers,
  and context-sensitive help. The main editing surface for LAMMPS input scripts.

**LineNumberArea (linenumberarea.h)**
  Widget that displays line numbers in the left margin of the CodeEditor.
  Updates dynamically as text is added or removed.

**Highlighter (highlighter.h/.cpp)**
  Syntax highlighter for LAMMPS input scripts. Categorizes and colors
  different types of commands, keywords, variables, and comments using
  Qt's QSyntaxHighlighter framework.

**FindAndReplace (findandreplace.h/.cpp)**
  Dialog for searching and replacing text in the editor. Supports
  case sensitivity and whole word matching options.

LAMMPS Interface
----------------

**LammpsWrapper (lammpswrapper.h/.cpp)**
  C++ wrapper around the LAMMPS C library interface. Provides a clean C++
  API and handles dynamic library loading in plugin mode. Manages LAMMPS
  initialization, command execution, and error handling.

**LammpsRunner (lammpsrunner.h)**
  Worker thread for executing LAMMPS simulations without blocking the GUI.
  Uses Qt's threading facilities to run simulations in the background,
  allowing the UI to remain responsive during long calculations.

Visualization Components
------------------------

**ImageViewer (imageviewer.h/.cpp)**
  Dialog for viewing and manipulating LAMMPS snapshot images created by
  the ``dump image`` command.  Supports interactive control of
  visualization parameters such as zoom, rotation, atom size, coloring,
  and rendering options.  Changes can be applied to regenerate the
  image using the LAMMPS library interface.  Uses two internal helper
  classes:

  - **ImageInfo** - Stores settings for displaying graphics from a LAMMPS
    compute or fix in snapshot images.
  - **RegionInfo** - Stores settings for displaying a region in snapshot images.

**ChartWindow (chartviewer.h/.cpp)**
  Window for displaying thermodynamic data as charts using Qt Charts.
  Supports line plots and multiple data series.

**ChartViewer (chartviewer.h/.cpp)**
  Custom chart view widget based on QChartView that provides interactive
  features like zooming, smoothing, and panning for data visualization.

**SlideShow (slideshow.h/.cpp)**
  Dialog for viewing multiple images as a slideshow or animation
  with navigation controls.  Supports converting an animation to
  a movie file when FFmpeg or ImageMagick is available.

**RangeSlider (rangeslider.h/.cpp)**
  Custom slider widget with two handles for selecting a range of values.
  Used in ChartWindow for selecting x- and y-direction plot ranges.

Dialog and Utility Components
-----------------------------

**LogWindow (logwindow.h/.cpp)**
  Window displaying captured output from LAMMPS simulations.  Updates
  in real-time as the simulation progresses and highlights warning
  and error messages.  Provides navigation to jump between warnings.

**Preferences (preferences.h/.cpp)**
  Dialog for configuring application settings including accelerator packages,
  editor appearance, snapshot settings, and chart preferences. Settings are
  made persistent across LAMMPS-GUI sessions using the QSettings class.
  The dialog is organized into five tabs, each implemented as a separate
  widget class:

  - **GeneralTab** - General settings (LAMMPS library path, fonts, etc.)
  - **AcceleratorTab** - LAMMPS accelerator package configuration
  - **SnapshotTab** - Snapshot image viewer defaults
  - **EditorTab** - Editor appearance and behavior settings
  - **ChartsTab** - Chart viewer display settings

**SetVariables (setvariables.h/.cpp)**
  Dialog for editing LAMMPS index-style variable definitions. Allows users
  to define name-value pairs that are substituted in input scripts using
  ${varname} syntax.

**FileViewer (fileviewer.h/.cpp)**
  Read-only text viewer dialog for displaying file contents. Used for
  viewing auxiliary files without allowing modifications.

**TutorialWizard (lammpsgui.h/.cpp)**
  Wizard dialog for interactive LAMMPS tutorials. Guides users through
  setting up tutorial directories and files, providing a structured
  learning experience.

**AboutDialog (aboutdialog.h/.cpp)**
  Custom About dialog that displays version information, LAMMPS
  configuration details, and available styles in scrollable text areas.
  The dialog automatically scrolls down when the content exceeds the
  visible area, pauses at the bottom, and then returns back to the top.

Support Components
------------------

**StdCapture (stdcapture.h/.cpp)**
  Utility class that captures stdout and stderr output from LAMMPS.
  Redirects C-level file descriptors to allow capturing output from
  the LAMMPS library.

**FlagWarnings (flagwarnings.h/.cpp)**
  Syntax highlighter for LAMMPS warning and error messages in log
  output.  Detects and highlights WARNING/ERROR lines and URLs for
  documentation links.  Maintains a count of warnings and updates a
  summary label.

**QHline (qaddon.h/.cpp)**
  Simple horizontal line widget for visual separation in dialogs and forms.

**QColorCompleter (qaddon.h/.cpp)**
  Auto-completer for color name inputs, suggesting valid Qt color names
  as the user types.

**QColorValidator (qaddon.h/.cpp)**
  Validator for color input fields, ensuring they contain valid color
  names or hex color codes.

Helper Functions
----------------

The helpers module provides utility functions used throughout the application:

- String manipulation (mystrdup variants for different string types)
- Date comparison (date_compare for version comparisons)
- Command-line parsing (split_line with quote handling)
- System utilities (has_exe for executable detection)
- UI utilities (is_light_theme for theme detection)

===========
 Data Flow
===========

1. **User Input**: User edits LAMMPS input in CodeEditor with syntax highlighting
2. **Execution Request**: User triggers execution via menu or button
3. **Preparation**: LammpsGui creates/configures LammpsWrapper and prepares variables
4. **Threading**: Commands sent to LammpsRunner thread to avoid UI blocking
5. **Execution**: LammpsRunner executes commands via LammpsWrapper
6. **Output Capture**: Output captured via StdCapture for display
7. **Visualization**: Results displayed in LogWindow, ImageViewer, or ChartWindow
8. **Completion**: UI updated when execution completes, progress indicators cleared

===============================
 Settings and State Management
===============================

The application uses Qt's QSettings mechanism to persist:

- Recent files list
- Window geometry and state
- Editor preferences (font, colors)
- Accelerator settings
- LAMMPS plugin path
- Tutorial preferences

Settings are stored in platform-specific locations (the application name
includes the Qt major version, e.g. ``LAMMPS-GUI (QT6)``):

- Linux: ``~/.config/The LAMMPS Developers/LAMMPS-GUI (QT6).conf``
- macOS: ``~/Library/Preferences/org.lammps.LAMMPS-GUI (QT6).plist``
- Windows: Registry under ``HKEY_CURRENT_USER\Software\The LAMMPS Developers\LAMMPS-GUI (QT6)``

=================
 Threading Model
=================

The application uses Qt's event-driven architecture with careful threading:

- **Main Thread**: Handles all UI operations and user interactions
- **LAMMPS Thread**: LammpsRunner executes LAMMPS in a separate QThread
- **Communication**: Signals/slots for thread-safe communication between threads

This design keeps the UI responsive even during long-running simulations.
