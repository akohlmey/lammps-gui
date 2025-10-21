# CMakeLists.txt for LAMMPS-GUI unit tests
cmake_minimum_required(VERSION 3.20)

# Fetch GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Enable testing
enable_testing()
include(GoogleTest)

# Find Qt (use same logic as parent)
if(NOT LAMMPS_GUI_USE_QT5)
  find_package(Qt6 6.2 QUIET COMPONENTS Widgets)
endif()
if(NOT Qt6_FOUND)
  find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets)
  set(QT_VERSION_MAJOR 5)
else()
  set(QT_VERSION_MAJOR 6)
endif()

# Test executable for helpers
add_executable(test_helpers
  test_helpers.cpp
  ${CMAKE_SOURCE_DIR}/src/helpers.cpp
)

target_include_directories(test_helpers PRIVATE
  ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_helpers
  GTest::gtest_main
  Qt${QT_VERSION_MAJOR}::Widgets
)

# Discover tests
gtest_discover_tests(test_helpers)
