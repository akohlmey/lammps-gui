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

# check that the Xvfb runner and a supported screenshooter are present
find_program(XVFB_RUNNER
  NAMES xvfb-run
  REQUIRED
  DOC "Path to Xvfb runner script")

find_program(SHOOTER_BINARY
  NAMES magick import xfce4-screenshooter gnome-screenshooter
  REQUIRED
  DOC "Path to a supported screenshooter program")
cmake_path(GET SHOOTER_BINARY FILENAME SHOOTER)
message(STATUS "Using ${SHOOTER} as screen shooter")

# configure virtual environment for tests using PyAutoGUI and Xvfb
find_package(Python3 3.8 REQUIRED COMPONENTS Interpreter)
set(TESTENV_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/testenv/bin/")
add_custom_command(
  OUTPUT ${TESTENV_BINARY_DIR}
  COMMAND ${Python3_EXECUTABLE} -m venv "${CMAKE_CURRENT_BINARY_DIR}/testenv"
  COMMAND ${TESTENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install --upgrade pip
  COMMAND ${TESTENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt --upgrade
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/shooter.${SHOOTER} ${TESTENV_BINARY_DIR}/shooter
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
)
add_custom_target(setup_tests ALL
  ${CMAKE_COMMAND} -E echo "Setting up virtual environment for testing"
  DEPENDS ${TESTENV_BINARY_DIR}
  SOURCES requirements.txt shooter.magick shooter.import shooter.xfce4-screenshooter shooter.gnome-screenshooter
)
set_target_properties(setup_tests PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/testenv)

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
    PASS_REGULAR_EXPRESSION "Options:.?  -p, --pluginpath <path> +Set path to LAMMPS")
else()
  set_tests_properties(CommandLine.HasPlugin PROPERTIES
    ENVIRONMENT "OMP_NUM_THREADS=1"
    PASS_REGULAR_EXPRESSION "Options:.?  -h, --help   +Displays help on commandline options.")
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

########################################################################
# Tests that check the GUI environment and the GUI
# IMPORTANT NOTE: we must use a different display number for the -n flag
# for each XFVB_RUNNER invocation, to allow parallel execution of tests.

# set up environment for python testing
set(PYTHON_TEST_ENVIRONMENT PYTHONUNBUFFERED=1)
list(APPEND PYTHON_TEST_ENVIRONMENT "PYTHONDONTWRITEBYTECODE=1")
list(APPEND PYTHON_TEST_ENVIRONMENT "OMP_NUM_THREADS=1")
list(APPEND PYTHON_TEST_ENVIRONMENT "LAMMPS_GUI=$<TARGET_FILE:lammps-gui>")
list(APPEND PYTHON_TEST_ENVIRONMENT "PATH=${TESTENV_BINARY_DIR}:$ENV{PATH}")

# test screen shooter wrapper script
add_test(NAME Framebuffer.CreateScreenshot
  COMMAND ${XVFB_RUNNER} -n 11 -s "-screen 0 1024x768x24" -w 1
  ${TESTENV_BINARY_DIR}/python ${CMAKE_CURRENT_SOURCE_DIR}/test_shooter.py
)

# test if PyAutoGUI is usable
add_test(NAME Framebuffer.CheckSize
  COMMAND ${XVFB_RUNNER} -n 12 -s "-screen 0 1024x768x24" -w 1
  ${TESTENV_BINARY_DIR}/python ${CMAKE_CURRENT_SOURCE_DIR}/test_xvfbsize.py
)
set_tests_properties(Framebuffer.CreateScreenshot  Framebuffer.CheckSize
                     PROPERTIES ENVIRONMENT "${PYTHON_TEST_ENVIRONMENT}")

# test basic LAMMPS-GUI editor tasks
add_test(NAME Framebuffer.GUIEditorChecks
  COMMAND ${XVFB_RUNNER} -n 13 -s "-screen 0 1024x768x24" -w 1
  ${TESTENV_BINARY_DIR}/python ${CMAKE_CURRENT_SOURCE_DIR}/test_gui_edit.py
)
set_tests_properties(Framebuffer.GUIEditorChecks
                     PROPERTIES ENVIRONMENT "${PYTHON_TEST_ENVIRONMENT}")
