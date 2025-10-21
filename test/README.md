# LAMMPS-GUI Unit Tests

This directory contains unit tests for the LAMMPS-GUI project using the GoogleTest framework.

## Overview

The test suite uses GoogleTest and is integrated with CMake's CTest framework. Tests are automatically discovered and can be run individually or as a complete suite.

## Building the Tests

Tests are built as part of the main project build when `ENABLE_TESTING` is enabled (default: ON).

### Quick Build (No Documentation)

```bash
cmake -S . -B build -D LAMMPS_GUI_USE_PLUGIN=yes -D BUILD_DOC=no -D ENABLE_TESTING=ON
cmake --build build --parallel 2
```

### Disable Tests

To build without tests:

```bash
cmake -S . -B build -D ENABLE_TESTING=OFF
```

## Running Tests

### Run All Tests

```bash
cd build
ctest
```

### Run Tests with Verbose Output

```bash
cd build
ctest --output-on-failure
```

### Run Specific Test

```bash
cd build
./test/test_helpers --gtest_filter="HelpersTest.MyStrdupStdString"
```

### List Available Tests

```bash
cd build
./test/test_helpers --gtest_list_tests
```

## Current Test Coverage

### test_helpers.cpp

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

## Adding New Tests

### Create a New Test File

1. Create a new test file in the `test/` directory (e.g., `test_newfile.cpp`)
2. Add the test executable to `test/CMakeLists.txt`:

```cmake
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
```

### Add Tests to Existing File

Add new test cases using GoogleTest macros:

```cpp
TEST_F(HelpersTest, NewTestName)
{
    // Arrange
    std::string input = "test data";
    
    // Act
    auto result = function_to_test(input);
    
    // Assert
    EXPECT_EQ(result, expected_value);
}
```

## Dependencies

- **GoogleTest**: Automatically fetched via CMake FetchContent (v1.15.2)
- **Qt6**: Required for Qt-dependent functions (Widgets component)
- **CTest**: Part of CMake, used for test execution

## Notes

- Tests that require a Qt application context use a `HelpersTest` fixture that creates a `QCoreApplication` instance.
- Platform-specific tests (e.g., `has_exe`) use conditional compilation to test appropriate commands on different operating systems.
- The test suite is designed to be easily extended with additional test files and test cases.
- GoogleTest is fetched automatically during CMake configuration, so no manual installation is required.

## CI Integration

The test suite integrates with existing CI workflows:
- Tests run as part of the standard build process when `ENABLE_TESTING=ON`
- CTest provides standard output for CI systems
- Tests can be disabled for documentation-only builds
