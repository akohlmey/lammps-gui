************
Architecture
************

Main Components
===============

The application architecture consists of several key components organized into
functional groups:

Main Window and Application Control
------------------------------------

**LammpsGui (lammpsgui.h/.cpp)**
  The main window class that coordinates all other components. It manages
  the editor, handles file operations, controls LAMMPS execution, and
  manages the overall application state. This is the central hub of the
  application that integrates all other components.

**TutorialWizard (lammpsgui.h/.cpp)**
  Wizard dialog for interactive LAMMPS tutorials. Guides users through
  setting up tutorial directories and files, providing a structured
  learning experience.

Editor Components
-----------------

**CodeEditor (codeeditor.h/.cpp)**
  Custom text editor widget based on QPlainTextEdit, providing LAMMPS-specific
  features including syntax highlighting, auto-completion, line numbers,
  and context-sensitive help. The main editing surface for LAMMPS input scripts.

**LineNumberArea (linenumberarea.h/.cpp)**
  Widget that displays line numbers in the left margin of the CodeEditor.
  Updates dynamically as text is added or removed.

**Highlighter (highlighter.h/.cpp)**
  Syntax highlighter for LAMMPS input scripts. Categorizes and colors
  different types of commands, keywords, variables, and comments using
  Qt's QSyntaxHighlighter framework.

**FindAndReplace (findandreplace.h/.cpp)**
  Dialog for searching and replacing text in the editor. Supports
  regular expressions, case sensitivity, and whole word matching.

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
  Dialog for viewing single images (PNG, JPEG, etc.). Supports zooming,
  panning, and saving images. Used for viewing simulation snapshots and
  rendered atomic configurations.

**ChartWindow (chartviewer.h/.cpp)**
  Window for displaying thermodynamic data as charts using Qt Charts.
  Supports line plots, scatter plots, and multiple data series.

**ChartViewer (chartviewer.h/.cpp)**
  Custom chart view widget based on QChartView that provides interactive
  features like zooming and panning for data visualization.

**SlideShow (slideshow.h/.cpp)**
  Dialog for viewing multiple images as a slideshow with navigation
  controls and a range slider for selecting image sequences.

**RangeSlider (rangeslider.h/.cpp)**
  Custom slider widget with two handles for selecting a range of values.
  Used in slideshow for selecting image ranges.

Dialog and Utility Components
------------------------------

**Preferences (preferences.h/.cpp)**
  Dialog for configuring application settings including accelerator packages,
  editor appearance, snapshot settings, and chart preferences. Settings are
  persisted using QSettings.

**SetVariables (setvariables.h/.cpp)**
  Dialog for editing LAMMPS index-style variable definitions. Allows users
  to define name-value pairs that are substituted in input scripts using
  ${varname} syntax.

**FileViewer (fileviewer.h/.cpp)**
  Read-only text viewer dialog for displaying file contents. Used for
  viewing auxiliary files without allowing modifications.

**LogWindow (logwindow.h/.cpp)**
  Window displaying captured output from LAMMPS simulations. Updates
  in real-time as the simulation progresses and provides search functionality.

Support Components
------------------

**StdCapture (stdcapture.h/.cpp)**
  Utility class that captures stdout and stderr output from LAMMPS.
  Redirects C-level file descriptors to allow capturing output from
  the LAMMPS library.

**FlagWarnings (flagwarnings.h/.cpp)**
  Syntax highlighter that validates LAMMPS command flags and highlights
  potential errors or deprecated usage patterns.

**QHline (qaddon.h/.cpp)**
  Simple horizontal line widget for visual separation in dialogs and forms.

**QColorCompleter (qaddon.h/.cpp)**
  Auto-completer for color name inputs, suggesting valid Qt color names
  as the user types.

**QColorValidator (qaddon.h/.cpp)**
  Validator for color input fields, ensuring they contain valid color
  names or hex color codes.

Helper Functions (helpers.h/.cpp)
----------------------------------

The helpers module provides utility functions used throughout the application:

- String manipulation (mystrdup variants for different string types)
- Date comparison (date_compare for version comparisons)
- Command-line parsing (split_line with quote handling)
- System utilities (has_exe for executable detection)
- UI utilities (is_light_theme for theme detection)

Data Flow
=========

1. **User Input**: User edits LAMMPS input in CodeEditor with syntax highlighting
2. **Execution Request**: User triggers execution via menu or button
3. **Preparation**: LammpsGui creates/configures LammpsWrapper and prepares variables
4. **Threading**: Commands sent to LammpsRunner thread to avoid UI blocking
5. **Execution**: LammpsRunner executes commands via LammpsWrapper
6. **Output Capture**: Output captured via StdCapture for display
7. **Visualization**: Results displayed in LogWindow, ImageViewer, or ChartWindow
8. **Completion**: UI updated when execution completes, progress indicators cleared

Settings and State Management
==============================

The application uses Qt's QSettings mechanism to persist:

- Recent files list
- Window geometry and state
- Editor preferences (font, colors)
- Accelerator settings
- LAMMPS plugin path
- Tutorial preferences

Settings are stored in platform-specific locations:

- Linux: ``~/.config/LAMMPS-GUI/LAMMPS-GUI.conf``
- macOS: ``~/Library/Preferences/org.lammps.LAMMPS-GUI.plist``
- Windows: Registry under ``HKEY_CURRENT_USER\Software\LAMMPS-GUI\LAMMPS-GUI``

Plugin vs Linked Modes
=======================

LAMMPS-GUI can operate in two modes:

**Plugin Mode** (Default)
  LAMMPS is loaded dynamically at runtime from a shared library (.so/.dylib/.dll).
  The path is auto-detected or configured via preferences. This allows using
  different LAMMPS versions without recompiling the GUI.

**Linked Mode**
  LAMMPS is linked directly at compile time. Used when building LAMMPS-GUI
  as part of the LAMMPS build system with ``-D BUILD_LAMMPS_GUI=on``.

Threading Model
===============

The application uses Qt's event-driven architecture with careful threading:

- **Main Thread**: Handles all UI operations and user interactions
- **LAMMPS Thread**: LammpsRunner executes LAMMPS in a separate QThread
- **Communication**: Signals/slots for thread-safe communication
- **Synchronization**: Mutex protection for shared state access

This design keeps the UI responsive even during long-running simulations.
