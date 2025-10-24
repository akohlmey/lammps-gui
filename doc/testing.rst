*******
Testing
*******

The ``test`` directory contains some tests for the LAMMPS-GUI project
using either the `GoogleTest framework
<https://github.com/google/googletest/>`_ or the `Python unittest
framework <https://docs.python.org/dev/library/unittest.html>`_

Overview
^^^^^^^^

The test suite uses CMake's CTest front end to select and run the
tests. Tests implemented with GoogleTest are automatically discovered
and can be run individually or as a complete suite for each test
program.  Test running LAMMPS-GUI itself use the "virtual frame buffer"
X server ``Xvfb`` and are written in Python using the ``unittest``
Python module and the `PyAutoGUI module
<https://pyautogui.readthedocs.io/>`_

Building the Tests
^^^^^^^^^^^^^^^^^^

Tests are built as part of the main project build when using ``-D
ENABLE_TESTING=ON`` during CMake configuration (default setting is
`OFF`).  Due to technical requirements, testing is currently only
enabled for native Linux builds of LAMMPS-GUI.  In any other build
environments the ``-D ENABLE_TESTING=ON`` setting is ignored.

Quick Build
===========

For running the tests, it is not necessary to build the documentation,
so its build can be skipped during configuration.

.. code-block:: text

   cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no -D ENABLE_TESTING=ON
   cmake --build build --parallel 2

Disable Tests
=============

Tests are disabled by default.  If they have been enabled during CMake configuration
they can be disabled at a later point with:To build without tests:

.. code-block:: text

   cmake -S . -B build -D ENABLE_TESTING=OFF

Running Tests
^^^^^^^^^^^^^

Below are some frequently used command line examples for running tests.
These examples assume that LAMMPS-GUI was compiled in the folder ``build``

Run All Tests
=============

.. code-block:: text

   ctest --test-dir build/test

Run Tests with Verbose Output
=============================

.. code-block:: text

   ctest --test-dir build/test -V

List Available Tests
====================

The list of the names of all available tests can be obtained with:

.. code-block:: text

   ctest --test-dir build/test -N


Run Specific Tests
==================

Individual tests can be selected in different ways.  Most common is the
use of regular expressions to select (``-R``) or exclude (``-E``) tests.
It is also possible to select tests by a range of Test numbers (``-I``)
from the -N test list output. Examples:

.. code-block:: text

   ctest --test-dir build/test -R MyStrdup
   ctest --test-dir build/test -E Frame
   ctest --test-dir build/test -I 20,25

Current Test Coverage
^^^^^^^^^^^^^^^^^^^^^

The test infrastructure is under active development. Currently, the test suite
focuses on utility functions and command-line interface validation. Future
expansion will include GUI component testing and integration tests.

Test Organization
=================

Tests are organized into three main categories:

1. **Unit Tests**: Using GoogleTest framework to test individual functions
2. **Command-Line Tests**: Using command-line to validate basic executable behavior
3. **GUI Tests**: Tests using the Python unittest framework and
   PyAutoGUI to run LAMMPS-GUI inside a virtual frame buffer in a
   "remote controlled fashion".

Unit Tests
==========

test_helpers.cpp
----------------

Comprehensive tests for functions in ``src/helpers.h`` and ``src/helpers.cpp``.
This module contains 28 test cases covering utility functions used throughout
the application.

**String Duplication (mystrdup)**
  Tests for the three overloaded mystrdup functions that create heap-allocated
  C-style strings from different input types:
  
  - ``mystrdup(const std::string&)`` - From std::string
  - ``mystrdup(const char*)`` - From C string (handles nullptr)
  - ``mystrdup(const QString&)`` - From Qt QString
  
  Coverage includes:
  
  - Normal strings with content
  - Empty strings
  - Null pointers (C string variant)
  - UTF-8 and special characters
  - Long strings

**Date Comparison (date_compare)**
  Tests for the date_compare function that compares version date strings
  in LAMMPS date format (e.g., "22 Jul 2025"):
  
  - Same dates (returns 0)
  - Different years (returns positive/negative)
  - Different months (returns positive/negative)
  - Different days (returns positive/negative)
  - Full month names vs. abbreviations
  - Invalid date formats
  - Edge cases (year boundaries, month boundaries)

**Line Splitting (split_line)**
  Tests for the split_line function that parses command-line style input
  with proper quote handling:
  
  - Simple whitespace-separated tokens
  - Single-quoted strings
  - Double-quoted strings
  - Escaped quotes within strings
  - Mixed quoting styles
  - Triple-nested quotes
  - Multiple consecutive whitespace characters
  - Empty input
  - Quotes at string boundaries

**Executable Detection (has_exe)**
  Tests for the has_exe function that checks if an executable exists in PATH:
  
  - Common system commands (sh, ls on Unix; cmd on Windows)
  - Non-existent commands
  - Commands with spaces in paths
  - Platform-specific behavior (conditional compilation)

**Theme Detection (is_light_theme)**
  Tests for the is_light_theme function that determines if the current
  Qt theme is light or dark:
  
  - Boolean return value validation
  - Consistency across calls
  - No crashes on theme query

Command-Line Tests
==================

These tests validate the ``lammps-gui`` executable behavior without starting
the full GUI. They run quickly and are useful for CI/CD pipelines.

CommandLine.GetVersion
-----------------------

**Purpose**: Verify version reporting consistency

This test runs::

  lammps-gui --platform offscreen -v

and validates that:

- The executable launches successfully
- Version output includes "LAMMPS-GUI (QT5)" or "LAMMPS-GUI (QT6)"
- Version number matches the ``PROJECT_VERSION`` CMake variable
- Process exits cleanly with status 0

**Environment**: ``OMP_NUM_THREADS=1`` to ensure consistent behavior

CommandLine.HasPlugin
----------------------

**Purpose**: Verify build configuration is reflected in help text

This test runs::

  lammps-gui --platform offscreen -h

and validates that help text is consistent with CMake configuration:

- **Plugin Mode** (``LAMMPS_GUI_USE_PLUGIN=ON``): Help text includes
  "-p, --pluginpath <path>" option
- **Linked Mode** (``LAMMPS_GUI_USE_PLUGIN=OFF``): Help text omits
  plugin path option

**Environment**: ``OMP_NUM_THREADS=1`` to ensure consistent behavior

Test Fixtures and Utilities
============================

**HelpersTest Fixture**
  Base test fixture that creates a ``QCoreApplication`` instance for tests
  that require Qt functionality. The application is created once per test
  suite and reused across tests for efficiency.

**Platform-Specific Testing**
  Tests use conditional compilation (``#ifdef _WIN32``) to adapt to
  platform differences in:
  
  - Path separators
  - Line endings
  - Available system executables
  - Default shell commands

Future Test Expansion
=====================

Planned additions to the test suite include:

**GUI Component Tests**
  - CodeEditor text manipulation
  - Syntax highlighter accuracy
  - Find/replace functionality
  - Auto-completion behavior

**LAMMPS Integration Tests**
  - LammpsWrapper command execution
  - Variable substitution
  - Error handling
  - Output capture

**File I/O Tests**
  - File opening/saving
  - Recent files management
  - Auto-save functionality
  - Data file inspection

**Preferences Tests**
  - Settings persistence
  - Default value initialization
  - Migration between versions

**Tutorial Tests**
  - Tutorial file generation
  - Directory setup
  - Resource extraction

Adding Tests
^^^^^^^^^^^^

Create a New Test File
======================

1. Create a new test file in the `test/` directory (e.g., `test_newfile.cpp`)
2. Add the test executable to `test/CMakeLists.txt`:

.. code-block:: cmake

   add_executable(test_newfile
     test_newfile.cpp
     ${CMAKE_SOURCE_DIR}/src/newfile.cpp
   )

   target_include_directories(test_newfile PRIVATE
     ${CMAKE_SOURCE_DIR}/src
   )

   target_link_libraries(test_newfile
     GTest::gtest_main
     Qt${QT_VERSION_MAJOR}::Widgets
   )

   gtest_discover_tests(test_newfile)


Add Tests to Existing File
==========================

Add new test cases using GoogleTest macros:

.. code-block:: cpp

   TEST_F(HelpersTest, NewTestName)
   {
       // Arrange
       std::string input = "test data";

       // Act
       auto result = function_to_test(input);

       // Assert
       EXPECT_EQ(result, expected_value);
   }

Dependencies
^^^^^^^^^^^^

- **GoogleTest**: Automatically fetched via CMake FetchContent (v1.15.2)
- **Qt6**: Required for Qt-dependent functions (Widgets component)
- **CTest**: Part of CMake, used for test execution

Notes
^^^^^

- Tests that require a Qt application context use a `HelpersTest` fixture that creates a `QCoreApplication` instance.
- Platform-specific tests (e.g., `has_exe`) use conditional compilation to test appropriate commands on different operating systems.
- The test suite is designed to be easily extended with additional test files and test cases.
- GoogleTest is fetched automatically during CMake configuration, so no manual installation is required.

CI Integration
^^^^^^^^^^^^^^

The test suite integrates with existing CI workflows:
- Tests run as part of the standard build process when `ENABLE_TESTING=ON`
- CTest provides standard output for CI systems
- Tests can be disabled for documentation-only builds
