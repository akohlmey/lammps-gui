************
Architecture
************

Main Components
===============

The application architecture consists of several key components:

**LammpsGui (lammpsgui.h/.cpp)**
  The main window class that coordinates all other components. It manages
  the editor, handles file operations, controls LAMMPS execution, and
  manages the overall application state.

**CodeEditor (codeeditor.h/.cpp)**
  Custom text editor widget based on QPlainTextEdit, providing LAMMPS-specific
  features including syntax highlighting, auto-completion, line numbers,
  and context-sensitive help.

**LammpsWrapper (lammpswrapper.h/.cpp)**
  C++ wrapper around the LAMMPS C library interface. Provides a clean C++
  API and handles dynamic library loading in plugin mode.

**LammpsRunner (lammpsrunner.h)**
  Worker thread for executing LAMMPS simulations without blocking the GUI.
  Uses Qt's threading facilities to run simulations in the background.

**Highlighter (highlighter.h/.cpp)**
  Syntax highlighter for LAMMPS input scripts. Categorizes and colors
  different types of commands, keywords, variables, and comments.

Data Flow
=========

1. User edits LAMMPS input in CodeEditor
2. User triggers execution via menu or button
3. LammpsGui creates/configures LammpsWrapper
4. Commands sent to LammpsRunner thread
5. LammpsRunner executes via LammpsWrapper
6. Output captured via StdCapture
7. Results displayed in LogWindow, ImageViewer, or ChartWindow
