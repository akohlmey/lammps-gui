##################
Programmer's Guide
##################

This guide provides API documentation for developers who want to understand
the internals of LAMMPS-GUI or contribute to its development.

************
Introduction
************

LAMMPS-GUI is built using C++17 and the Qt framework (Qt 5.15+ or Qt 6.2+).
The application follows object-oriented design principles with clear separation
of concerns between different components:

- **Editor Components**: Handle text editing, syntax highlighting, and auto-completion
- **LAMMPS Interface**: Wraps the LAMMPS C library API
- **Visualization**: Displays images, charts, and simulation output
- **GUI Framework**: Main window, dialogs, and preferences

The codebase consists of approximately 37 C++ source files in the ``src/``
directory, totaling around 11,000 lines of code.

****************
Architecture
****************

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

********************
Core API Reference
********************

The following sections provide detailed API documentation for the main
classes in LAMMPS-GUI. Documentation is generated from Doxygen comments
in the source code.

Editor Components
=================

CodeEditor Class
----------------

.. doxygenclass:: CodeEditor
   :members:
   :protected-members:

LineNumberArea Class
--------------------

.. doxygenclass:: LineNumberArea
   :members:

Highlighter Class
-----------------

.. doxygenclass:: Highlighter
   :members:
   :protected-members:

LAMMPS Interface
================

LammpsWrapper Class
-------------------

.. doxygenclass:: LammpsWrapper
   :members:

LammpsRunner Class
------------------

.. doxygenclass:: LammpsRunner
   :members:

Utility Classes
===============

StdCapture Class
----------------

.. doxygenclass:: StdCapture
   :members:

Helper Functions
----------------

.. doxygenfile:: helpers.h
   :sections: func

************************
Development Guidelines
************************

Code Style
==========

The project follows these coding conventions:

- **Indentation**: 4 spaces (no tabs)
- **Line length**: Maximum 100 characters
- **Formatting**: Enforced by ``.clang-format`` configuration (LLVM-based)
- **Comments**: Use Doxygen-style documentation comments
- **Naming**: 
  - Classes: CamelCase (e.g., ``CodeEditor``)
  - Functions: camelCase (e.g., ``reformatLine``)
  - Members: snake_case (e.g., ``reformat_on_return``)

Documentation
=============

All public classes and functions should have Doxygen documentation:

.. code-block:: cpp

   /**
    * @brief Brief description
    * 
    * Detailed description if needed
    * 
    * @param param1 Description of parameter
    * @return Description of return value
    */
   int myFunction(int param1);

Building
========

See :doc:`installation` for detailed build instructions. For development:

.. code-block:: bash

   # Debug build with Qt6
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug \\
         -DLAMMPS_GUI_USE_PLUGIN=yes -DBUILD_DOC=no
   cmake --build build --parallel 2

Testing
=======

Currently, LAMMPS-GUI does not have automated unit tests. Testing is done
manually by:

1. Building the application
2. Running with ``--platform offscreen -v`` to verify startup
3. Testing functionality with sample LAMMPS input files
4. Checking that documentation builds without errors

Contributing
============

To contribute to LAMMPS-GUI:

1. Fork the repository on GitHub
2. Create a feature branch
3. Make your changes with proper documentation
4. Ensure code compiles with both Qt5 and Qt6 (if possible)
5. Test your changes thoroughly
6. Submit a pull request

All contributions must:

- Follow the existing code style
- Include Doxygen documentation for new public APIs
- Not break existing functionality
- Have GPG-signed commits

******************
Plugin System
******************

LAMMPS-GUI can operate in two modes:

**Plugin Mode** (default)
  LAMMPS library is loaded dynamically at runtime. This allows using
  different LAMMPS builds without recompiling the GUI. The library
  loading is handled in ``lammpswrapper.cpp`` using platform-specific
  dynamic loading functions (dlopen on Unix, LoadLibrary on Windows).

**Linked Mode**
  LAMMPS library is linked at compile time. Used when building as part
  of the LAMMPS build system with ``-D BUILD_LAMMPS_GUI=on``.

The mode is controlled by the ``LAMMPS_GUI_USE_PLUGIN`` CMake option.

******************
Qt Integration
******************

LAMMPS-GUI makes extensive use of Qt features:

**Signals and Slots**
  Used for inter-component communication, especially between GUI
  components and background threads.

**Qt Designer Forms**
  Main window and dialogs use ``.ui`` files edited in Qt Designer.

**Qt Resource System**
  Icons and resources embedded via ``resources/lammpsgui.qrc``.

**Qt Models**
  Used for data display in various viewers and inspectors.

For more details on Qt usage, see the `Qt Documentation <https://doc.qt.io/>`_.
