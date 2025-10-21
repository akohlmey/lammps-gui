# Example: Adding New Tests

This file provides examples of how to add new tests to the LAMMPS-GUI test suite.

## Example 1: Adding a New Test to an Existing Test File

To add a new test to `test_helpers.cpp`:

```cpp
TEST_F(HelpersTest, MyNewTest)
{
    // Arrange: Set up test data
    std::string input = "test input";
    
    // Act: Call the function being tested
    auto result = function_to_test(input);
    
    // Assert: Verify the result
    EXPECT_EQ(result, "expected output");
}
```

## Example 2: Creating a New Test File for a Different Module

If you want to test functions from a different source file (e.g., `src/highlighter.cpp`):

### Step 1: Create `test/test_highlighter.cpp`

```cpp
/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "highlighter.h"

#include <gtest/gtest.h>
#include <QCoreApplication>

// Test fixture for highlighter tests
class HighlighterTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (!QCoreApplication::instance()) {
            static int argc    = 1;
            static char *argv[] = {(char *)"test_highlighter"};
            app                = new QCoreApplication(argc, argv);
        }
    }

    static QCoreApplication *app;
};

QCoreApplication *HighlighterTest::app = nullptr;

// Example test
TEST_F(HighlighterTest, SomeFeature)
{
    // Test implementation
    EXPECT_TRUE(true);
}
```

### Step 2: Update `test/CMakeLists.txt`

Add the following to `test/CMakeLists.txt`:

```cmake
# Test executable for highlighter
add_executable(test_highlighter
  test_highlighter.cpp
  ${CMAKE_SOURCE_DIR}/src/highlighter.cpp
)

target_include_directories(test_highlighter PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_highlighter
  GTest::gtest_main
  Qt${QT_VERSION_MAJOR}::Widgets
)

# Discover tests
gtest_discover_tests(test_highlighter)
```

### Step 3: Build and Run

```bash
cd build
cmake --build . --target test_highlighter
ctest -R Highlighter
```

## Common Test Patterns

### Testing String Functions

```cpp
TEST_F(MyTest, StringFunction)
{
    std::string input = "input string";
    std::string result = my_string_function(input);
    
    EXPECT_EQ(result, "expected string");
    EXPECT_NE(result, "wrong string");
    EXPECT_TRUE(result.find("substring") != std::string::npos);
}
```

### Testing Numeric Functions

```cpp
TEST_F(MyTest, NumericFunction)
{
    int result = my_numeric_function(5, 10);
    
    EXPECT_EQ(result, 15);
    EXPECT_GT(result, 10);
    EXPECT_LT(result, 20);
    EXPECT_GE(result, 15);
    EXPECT_LE(result, 15);
}
```

### Testing Boolean Functions

```cpp
TEST_F(MyTest, BooleanFunction)
{
    EXPECT_TRUE(my_boolean_function(true));
    EXPECT_FALSE(my_boolean_function(false));
}
```

### Testing Exception Handling

```cpp
TEST_F(MyTest, ThrowsException)
{
    EXPECT_THROW(my_function_that_throws(), std::runtime_error);
    EXPECT_NO_THROW(my_safe_function());
}
```

### Testing Qt Types

```cpp
TEST_F(MyTest, QStringFunction)
{
    QString input = "Qt String";
    QString result = my_qt_function(input);
    
    EXPECT_EQ(result, QString("Expected Qt String"));
    EXPECT_TRUE(result.contains("String"));
}
```

### Parameterized Tests

For testing the same function with multiple inputs:

```cpp
class MyParameterizedTest : public ::testing::TestWithParam<std::pair<std::string, std::string>> {
};

TEST_P(MyParameterizedTest, TestWithMultipleInputs)
{
    auto [input, expected] = GetParam();
    auto result = my_function(input);
    EXPECT_EQ(result, expected);
}

INSTANTIATE_TEST_SUITE_P(
    MyTestSuite,
    MyParameterizedTest,
    ::testing::Values(
        std::make_pair("input1", "output1"),
        std::make_pair("input2", "output2"),
        std::make_pair("input3", "output3")
    )
);
```

## Best Practices

1. **One concept per test**: Each test should verify one specific behavior
2. **Descriptive names**: Test names should clearly describe what they're testing
3. **Arrange-Act-Assert**: Structure tests with clear setup, execution, and verification
4. **Test edge cases**: Include tests for empty inputs, null values, boundary conditions
5. **Clean up resources**: Use fixtures to manage resource cleanup
6. **Avoid test dependencies**: Tests should be independent and runnable in any order
7. **Use appropriate assertions**: Choose the most specific assertion for better error messages

## Running Specific Tests

```bash
# Run all tests in a file
./test/test_helpers

# Run tests matching a pattern
./test/test_helpers --gtest_filter="HelpersTest.MyStrdup*"

# List all tests
./test/test_helpers --gtest_list_tests

# Run tests with verbose output
./test/test_helpers --gtest_verbose

# Run tests and repeat 10 times
./test/test_helpers --gtest_repeat=10

# Run tests in random order
./test/test_helpers --gtest_shuffle
```
