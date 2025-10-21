*******
Testing
*******

The ``test`` directory contains some tests for the LAMMPS-GUI project
using the `GoogleTest <https://github.com/google/googletest/>`_ framework.

Overview
^^^^^^^^

The test suite uses CMake's CTest framework. Tests implemented with GoogleTest
are automatically discovered and can be run individually or as a complete suite
for each test program.

Building the Tests
^^^^^^^^^^^^^^^^^^

Tests are built as part of the main project build when using
``-D ENABLE_TESTING=ON`` during CMake configuration (default setting is `OFF`).

Quick Build
===========

For running the tests, it is not necessary to build the documentation, so it
can be skipped during configuration.

.. code-block:: bash

   cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no -D ENABLE_TESTING=ON
   cmake --build build --parallel 2

Disable Tests
=============

Tests are disabled by default.  If they have been enabled during CMake configuration
they can be disabled at a later point with:To build without tests:

.. code-block:: bash

   cmake -S . -B build -D ENABLE_TESTING=OFF

Running Tests
^^^^^^^^^^^^^

Below are some frequently used command line examples for running tests.
These examples assume that LAMMPS-GUI was compiled in the folder ``build``

Run All Tests
=============

.. code-block:: bash

   ctest --test-dir build/test

Run Tests with Verbose Output
=============================

.. code-block:: bash

   ctest --test-dir build/test -V

Run Specific Test
=================

Individual tests can be selected in different ways.  Most common is the
use of regular expressions. Example:

.. code-block:: bash

   ctest --test-dir build/test -R MyStrdup

List Available Tests
====================

The list of the names of all available tests can be obtained with:

.. code-block:: bash

   ctest --test-dir build/test -N


Current Test Coverage
^^^^^^^^^^^^^^^^^^^^^

The test infrastructure is very much a work in progress and thus only
a very limited number of tests are available.

test_helpers.cpp
================

Tests for functions in `src/helpers.h` and `src/helpers.cpp`:

- **mystrdup**: String duplication functions (3 overloads)
  - `mystrdup(const std::string&)`
  - `mystrdup(const char*)`
  - `mystrdup(const QString&)`
  
- **date_compare**: Date string comparison
  - Same dates
  - Different years, months, days
  - Full month names
  - Invalid formats
  
- **split_line**: String splitting with quote handling
  - Simple whitespace splitting
  - Single and double quotes
  - Escaped quotes
  - Mixed quotes
  - Triple quotes
  - Multiple whitespace characters
  
- **has_exe**: Executable detection in PATH
  - System commands
  - Non-existent commands
  
- **is_light_theme**: Theme detection
  - Boolean return value validation

Command Line Tests
==================
    
There are two tests that launch LAMMPS-GUI but print some text output
and then terminate before the GUI is initialized.

CommandLine.GetVersion
**********************

This runs the ``lammps-gui`` executable with the "-v" flag to have it
print the LAMMPS-GUI version number which then compared to the
PROJECT_VERSION CMake variable.

CommandLine.HasPlugin
*********************

This runs the ``lammps-gui`` executable with the "-h" flag to have it
print the LAMMPS-GUI command line usage help text.  Depending on whether
CMake configuration was done with ``-D LAMMPS_GUI_USE_PLUGIN=ON`` (the
default) or ``-D LAMMPS_GUI_USE_PLUGIN=OFF`` the help message will either
contain some text about using the ``-p`` or not.  This test checks whether
the help message is consistent with the configuration settings.

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
