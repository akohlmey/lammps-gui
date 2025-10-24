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

# check that Xvfb and a supported screenshooter are present
find_program(XVFB_BINARY
  NAMES Xvfb
  REQUIRED
  DOC "Path to Xvfb executable")

find_program(SHOOTER_BINARY
  NAMES magick import xfce4-screenshooter gnome-screenshooter
  REQUIRED
  DOC "Path to a supported screenshooter program")
cmake_path(GET SHOOTER_BINARY FILENAME SHOOTER)
message(STATUS "Using ${SHOOTER} as screen shooter")

# configure virtual environment for tests using PyAutoGUI and Xvfb
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
set(TESTENV_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/testenv/bin)
set(TESTENV_REQUIREMENTS_FILE ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt)
add_custom_command(
  OUTPUT ${TESTENV_BINARY_DIR}/activate
  COMMAND ${Python3_EXECUTABLE} -m venv testenv
  COMMAND ${TESTENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install --upgrade pip
  COMMAND ${TESTENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install -r ${TESTENV_REQUIREMENTS_FILE} --upgrade
  DEPENDS ${TESTENV_REQUIREMENTS_FILE}
)

# create wrapper script for screen shooter
if(SHOOTER STREQUAL "magick")
  file(WRITE ${TESTENV_BINARY_DIR}/shooter "#!/bin/sh\n\nmagick import -window root \"$@\"\n")
elseif(SHOOTER STREQUAL "import")
  file(WRITE ${TESTENV_BINARY_DIR}/shooter "#!/bin/sh\n\nimport -window root \"$@\"\n")
elseif(SHOOTER STREQUAL "xfce4-screenshooter")
  file(WRITE ${TESTENV_BINARY_DIR}/shooter "#!/bin/sh\n\nxfce4-screenshooter -f -s \"$@\"\n")
elseif(SHOOTER STREQUAL "gnome-screenshooter")
  file(WRITE ${TESTENV_BINARY_DIR}/shooter "#!/bin/sh\n\ngnome-screenshooter -f \"$@\"\n")
endif()
file(CHMOD ${TESTENV_BINARY_DIR}/shooter PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE)

# General command line tests on the LAMMPS-GUI executable
add_test(NAME CommandLine.GetVersion
  COMMAND $<TARGET_FILE:lammps-gui> --platform offscreen -v)
set_tests_properties(CommandLine.GetVersion PROPERTIES
  ENVIRONMENT "OMP_NUM_THREADS=1"
  PASS_REGULAR_EXPRESSION "LAMMPS-GUI \\(QT${QT_VERSION_MAJOR}\\) ${PROJECT_VERSION}")

add_test(NAME CommandLine.HasPlugin
  COMMAND $<TARGET_FILE:lammps-gui> --platform offscreen -h)
if (LAMMPS_GUI_USE_PLUGIN)
  set_tests_properties(CommandLine.HasPlugin PROPERTIES
    ENVIRONMENT "OMP_NUM_THREADS=1"
    PASS_REGULAR_EXPRESSION "Options:.?  -p, --pluginpath <path>  Set path to LAMMPS")
else()
  set_tests_properties(CommandLine.HasPlugin PROPERTIES
    ENVIRONMENT "OMP_NUM_THREADS=1"
    PASS_REGULAR_EXPRESSION "Options:.?  -h, --help     Displays help on commandline options.")
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

# Discover tests for test executables
gtest_discover_tests(test_helpers)

# test screen shooter wrapper script
add_test(NAME Screenshot.Create
  COMMAND ${TESTENV_BINARY_DIR}/shooter shot.png
  )
