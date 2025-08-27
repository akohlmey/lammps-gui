cmake_minimum_required(VERSION 3.20)

project(lammps-gui
  VERSION 1.8.0
  LANGUAGES CXX
  DESCRIPTION "The LAMMPS GUI"
  HOMEPAGE_URL "https://lammps-gui.lammps.org/")
include(GNUInstallDirs)

##################################################################################
# configuration settings

# prefer compiling plugin version only if standalone compile and not on macOS
if(LAMMPS_DIR OR APPLE)
  option(LAMMPS_GUI_USE_PLUGIN "Load LAMMPS library dynamically at runtime" OFF)
else()
  option(LAMMPS_GUI_USE_PLUGIN "Load LAMMPS library dynamically at runtime" ON)
endif()

# prefer Qt6 by default
option(LAMMPS_GUI_USE_QT5 "Prefer using Qt5 over Qt6" OFF)

# build html documentation by default
option(BUILD_DOC "Build LAMMPS-GUI HTML documentation" ON)
##################################################################################

include(CheckIncludeFileCXX)
# helper function to check for usable omp.h header
function(check_omp_h_include)
  find_package(OpenMP COMPONENTS CXX QUIET)
  if(OpenMP_CXX_FOUND)
    set(CMAKE_REQUIRED_FLAGS ${OpenMP_CXX_FLAGS})
    set(CMAKE_REQUIRED_INCLUDES ${OpenMP_CXX_INCLUDE_DIRS})
    set(CMAKE_REQUIRED_LINK_OPTIONS ${OpenMP_CXX_FLAGS})
    set(CMAKE_REQUIRED_LIBRARIES ${OpenMP_CXX_LIBRARIES})
    # there are all kinds of problems with finding omp.h
    # for Clang and derived compilers so we pretend it is there.
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
      set(_have_omp_h TRUE)
    else()
      check_include_file_cxx(omp.h _have_omp_h)
    endif()
  else()
    set(_have_omp_h FALSE)
  endif()
  set(HAVE_OMP_H_INCLUDE ${_have_omp_h} PARENT_SCOPE)
endfunction()

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT BUILD_DOC_ONLY)
  # detect if we may enable OpenMP support by default
  set(BUILD_OMP_DEFAULT OFF)
  find_package(OpenMP COMPONENTS CXX QUIET)
  if(OpenMP_CXX_FOUND)
    check_omp_h_include()
    if(HAVE_OMP_H_INCLUDE)
      set(BUILD_OMP_DEFAULT ON)
    endif()
  endif()

  option(BUILD_OMP "Build with OpenMP support" ${BUILD_OMP_DEFAULT})
  if(BUILD_OMP)
    find_package(OpenMP COMPONENTS CXX REQUIRED)
    check_omp_h_include()
    if(NOT HAVE_OMP_H_INCLUDE)
      message(FATAL_ERROR "Cannot find the 'omp.h' header file required for full OpenMP support")
    endif()
  endif()

  # checks
  # when this file is included as subdirectory in the LAMMPS build, many settings are directly imported
  if(LAMMPS_DIR)
    set(LAMMPS_HEADER_DIR ${LAMMPS_SOURCE_DIR})
    set(LAMMPS_LIBRARY lammps)
  else()
    if (NOT LAMMPS_SOURCE_DIR AND NOT LAMMPS_GUI_USE_PLUGIN)
      message(FATAL_ERROR "Must provide -D LAMMPS_SOURCE_DIR=<path to LAMMPS sources> when not compiling with -D LAMMPS_GUI_USE_PLUGIN=on")
    endif()
    # by default, install into $HOME/.local (not /usr/local),
    # so that no root access (and sudo) is needed
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
      set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Default install path" FORCE)
    endif()

    # ugly hacks for MSVC which by default always reports an old C++ standard in the __cplusplus macro
    # and prints lots of pointless warnings about "unsafe" functions
    if(MSVC)
      if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/Zc:__cplusplus)
        add_compile_options(/wd4244)
        add_compile_options(/wd4267)
        add_compile_options(/wd4250)
        add_compile_options(/EHsc)
      endif()
      add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    endif()

    if (NOT LAMMPS_GUI_USE_PLUGIN)
      get_filename_component(LAMMPS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src ABSOLUTE)
      set(LAMMPS_HEADER_DIR ${LAMMPS_SOURCE_DIR} CACHE PATH "Location of LAMMPS headers")
      if(NOT LAMMPS_HEADER_DIR)
        message(FATAL_ERROR "Must set LAMMPS_HEADER_DIR")
      endif()
      get_filename_component(LAMMPS_DIR ${LAMMPS_SOURCE_DIR}/.. ABSOLUTE)
      # find LAMMPS library
      find_library(LAMMPS_LIBRARY
        NAMES lammps lammps_serial
        HINTS ${LAMMPS_LIB_DIR} ${LAMMPS_HEADER_DIR}/../build ${LAMMPS_HEADER_DIR}/../build-clang ${LAMMPS_HEADER_DIR}/../build-test ${LAMMPS_HEADER_DIR}
        REQUIRED
      )
    endif()
  endif()

  set(LAMMPS_PLUGINLIB_DIR ${CMAKE_SOURCE_DIR}/plugin)
  if(LAMMPS_GUI_USE_PLUGIN)
    enable_language(C)
    set(PLUGIN_LOADER_SRC ${LAMMPS_PLUGINLIB_DIR}/liblammpsplugin.c)
  endif()

  # include resource compiler to embed icons into the executable on Windows
  if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    enable_language(RC)
    set(ICON_RC_FILE ${CMAKE_SOURCE_DIR}/resources/lmpicons.rc)
  endif()

  # we require at least Qt version 5.15, but prefer Qt 6.2 or later.
  if(NOT LAMMPS_GUI_USE_QT5)
    find_package(Qt6 6.2 QUIET COMPONENTS Widgets Charts)
  endif()
  if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Widgets Charts)
    set(QT_VERSION_MAJOR 5)
  else()
    set(QT_VERSION_MAJOR 6)
  endif()
  message(STATUS "Using Qt version ${Qt${QT_VERSION_MAJOR}_VERSION} for LAMMPS-GUI")

  set(PROJECT_SOURCES
    ${CMAKE_SOURCE_DIR}/src/main.cpp
    ${CMAKE_SOURCE_DIR}/src/lammpsgui.cpp
    ${CMAKE_SOURCE_DIR}/src/lammpsgui.h
    ${CMAKE_SOURCE_DIR}/src/lammpsgui.ui
    ${CMAKE_SOURCE_DIR}/src/chartviewer.cpp
    ${CMAKE_SOURCE_DIR}/src/chartviewer.h
    ${CMAKE_SOURCE_DIR}/src/codeeditor.cpp
    ${CMAKE_SOURCE_DIR}/src/codeeditor.h
    ${CMAKE_SOURCE_DIR}/src/findandreplace.cpp
    ${CMAKE_SOURCE_DIR}/src/findandreplace.h
    ${CMAKE_SOURCE_DIR}/src/flagwarnings.cpp
    ${CMAKE_SOURCE_DIR}/src/flagwarnings.h
    ${CMAKE_SOURCE_DIR}/src/helpers.cpp
    ${CMAKE_SOURCE_DIR}/src/highlighter.cpp
    ${CMAKE_SOURCE_DIR}/src/highlighter.h
    ${CMAKE_SOURCE_DIR}/src/imageviewer.cpp
    ${CMAKE_SOURCE_DIR}/src/imageviewer.h
    ${CMAKE_SOURCE_DIR}/src/lammpsrunner.h
    ${CMAKE_SOURCE_DIR}/src/lammpswrapper.cpp
    ${CMAKE_SOURCE_DIR}/src/lammpswrapper.h
    ${CMAKE_SOURCE_DIR}/src/linenumberarea.h
    ${CMAKE_SOURCE_DIR}/src/logwindow.cpp
    ${CMAKE_SOURCE_DIR}/src/logwindow.h
    ${CMAKE_SOURCE_DIR}/src/fileviewer.cpp
    ${CMAKE_SOURCE_DIR}/src/fileviewer.h
    ${CMAKE_SOURCE_DIR}/src/preferences.cpp
    ${CMAKE_SOURCE_DIR}/src/preferences.h
    ${CMAKE_SOURCE_DIR}/src/qaddon.cpp
    ${CMAKE_SOURCE_DIR}/src/qaddon.h
    ${CMAKE_SOURCE_DIR}/src/setvariables.cpp
    ${CMAKE_SOURCE_DIR}/src/setvariables.h
    ${CMAKE_SOURCE_DIR}/src/slideshow.h
    ${CMAKE_SOURCE_DIR}/src/slideshow.cpp
    ${CMAKE_SOURCE_DIR}/src/stdcapture.cpp
    ${CMAKE_SOURCE_DIR}/src/rangeslider.h
    ${CMAKE_SOURCE_DIR}/src/rangeslider.cpp
    ${PLUGIN_LOADER_SRC}
    ${ICON_RC_FILE}
  )
  if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_resources(PROJECT_SOURCES resources/lammpsgui.qrc)
  else()
    qt5_add_resources(PROJECT_SOURCES resources/lammpsgui.qrc)
  endif()

  if(APPLE)
    set(MACOSX_ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/lammps.icns)
    set(MACOSX_README_FILE ${CMAKE_SOURCE_DIR}/packaging/README.macos)
    set(MACOSX_BACKGROUND_FILE ${CMAKE_SOURCE_DIR}/packaging/LAMMPS_DMG_Background.png)
  endif()

  if(QT_VERSION_MAJOR EQUAL 6)
    qt_add_executable(lammps-gui
      MANUAL_FINALIZATION
      ${MACOSX_ICON_FILE}
      ${PROJECT_SOURCES}
    )
  else()
    add_executable(lammps-gui
      ${MACOSX_ICON_FILE}
      ${PROJECT_SOURCES}
    )
  endif()

  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})

  if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(lammps-gui)
  endif()

  # compilation settings
  if(LAMMPS_GUI_USE_PLUGIN)
    target_compile_definitions(lammps-gui PRIVATE LAMMPS_GUI_USE_PLUGIN)
    target_include_directories(lammps-gui PRIVATE ${LAMMPS_PLUGINLIB_DIR})
  else()
    target_include_directories(lammps-gui PRIVATE ${LAMMPS_HEADER_DIR})
    target_link_libraries(lammps-gui PRIVATE ${LAMMPS_LIBRARY})
  endif()
  target_include_directories(lammps-gui PRIVATE ${CMAKE_SOURCE_DIR}/src)
  target_compile_definitions(lammps-gui PRIVATE LAMMPS_GUI_VERSION="${PROJECT_VERSION}")
  target_link_libraries(lammps-gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)

  if(BUILD_OMP)
    find_package(OpenMP COMPONENTS CXX REQUIRED)
    target_link_libraries(lammps-gui PRIVATE OpenMP::OpenMP_CXX)
  endif()
endif()

###################
# documentation

if(BUILD_DOC OR BUILD_DOC_ONLY)
  # Current Sphinx versions require at least Python 3.8
  # use default (or custom) Python executable, if version is sufficient
  if(Python_VERSION VERSION_GREATER_EQUAL 3.8)
    set(Python3_EXECUTABLE ${Python_EXECUTABLE})
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  if(Python3_VERSION VERSION_LESS 3.8)
    message(FATAL_ERROR "Python 3.8 and up is required to build the LAMMPS-GUI HTML documentation")
  endif()
  set(VIRTUALENV ${Python3_EXECUTABLE} -m venv)

  set(SPHINX_DIR ${CMAKE_SOURCE_DIR}/doc)
  set(SPHINX_CONFIG_FILE_TEMPLATE ${SPHINX_DIR}/conf.py.in)
  file(GLOB DOC_SOURCES CONFIGURE_DEPENDS ${SPHINX_DIR}/[a-z]*.rst)

  # configuration and static files are copied to binary dir to avoid collisions with parallel builds
  set(DOC_BUILD_DIR ${CMAKE_BINARY_DIR}/doc)
  set(DOC_BUILD_STATIC_DIR ${DOC_BUILD_DIR}/_static)
  set(DOC_BUILD_CONFIG_FILE ${DOC_BUILD_DIR}/conf.py)

  file(MAKE_DIRECTORY ${DOC_BUILD_DIR})
  configure_file(${SPHINX_CONFIG_FILE_TEMPLATE} ${DOC_BUILD_CONFIG_FILE})
  file(COPY ${SPHINX_DIR}/_static DESTINATION ${DOC_BUILD_DIR}/)
  file(COPY ${SPHINX_DIR}/_templates DESTINATION ${DOC_BUILD_DIR}/)
  file(COPY ${SPHINX_DIR}/LAMMPSLexer.py DESTINATION ${DOC_BUILD_DIR}/)
  file(COPY ${CMAKE_SOURCE_DIR}/resources/icons/lammps-gui-banner.png DESTINATION ${DOC_BUILD_DIR})

  # configure virtual environment for sphinx-build and related
  set(DOCENV_BINARY_DIR ${CMAKE_BINARY_DIR}/docenv/bin)
  set(DOCENV_REQUIREMENTS_FILE ${SPHINX_DIR}/requirements.txt)
  set(DOCENV_DEPS docenv ${DOCENV_REQUIREMENTS_FILE} ${SPHINX_DIR}/_templates/page.html)
  add_custom_command(
    OUTPUT docenv
    COMMAND ${VIRTUALENV} docenv
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install --upgrade pip
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install -r ${DOCENV_REQUIREMENTS_FILE} --upgrade
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  if(NOT TARGET Sphinx::sphinx-build)
    add_executable(Sphinx::sphinx-build IMPORTED GLOBAL)
    set_target_properties(Sphinx::sphinx-build PROPERTIES IMPORTED_LOCATION "${DOCENV_BINARY_DIR}/sphinx-build")
  endif()

  set(MATHJAX_URL "https://github.com/mathjax/MathJax/archive/3.2.2.tar.gz" CACHE STRING "URL for MathJax tarball")
  set(MATHJAX_MD5 "08dd6ef33ca08870220d9aade2a62845" CACHE STRING "MD5 checksum of MathJax tarball")
  mark_as_advanced(MATHJAX_URL)

  # download mathjax distribution and unpack to folder "mathjax"
  if(NOT EXISTS ${DOC_BUILD_STATIC_DIR}/mathjax/es5)
    if(EXISTS ${CMAKE_BINARY_DIR}/mathjax.tar.gz)
      file(MD5 ${CMAKE_BINARY_DIR}/mathjax.tar.gz DL_MD5)
    endif()
    if(NOT "${DL_MD5}" STREQUAL "${MATHJAX_MD5}")
      file(DOWNLOAD ${MATHJAX_URL} "${CMAKE_BINARY_DIR}/mathjax.tar.gz" STATUS DL_STATUS SHOW_PROGRESS)
      file(MD5 ${CMAKE_BINARY_DIR}/mathjax.tar.gz DL_MD5)
      if((NOT DL_STATUS EQUAL 0) OR (NOT "${DL_MD5}" STREQUAL "${MATHJAX_MD5}"))
        message(WARNING "Download from primary URL ${MATHJAX_URL} failed\nTrying fallback URL ${MATHJAX_FALLBACK}")
        file(DOWNLOAD ${MATHJAX_FALLBACK} ${CMAKE_BINARY_DIR}/libpace.tar.gz EXPECTED_HASH MD5=${MATHJAX_MD5} SHOW_PROGRESS)
      endif()
    else()
      message(STATUS "Using already downloaded archive ${CMAKE_BINARY_DIR}/libpace.tar.gz")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf mathjax.tar.gz WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    file(GLOB MATHJAX_VERSION_DIR CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/MathJax-*)
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${DOC_BUILD_STATIC_DIR}/mathjax)
    execute_process(COMMAND ${CMAKE_COMMAND} -E rename ${MATHJAX_VERSION_DIR} ${DOC_BUILD_STATIC_DIR}/mathjax)
  endif()

  add_custom_command(
    OUTPUT ${DOC_BUILD_DIR}/html/index.html
    DEPENDS docenv ${DOC_SOURCES} ${DOCENV_DEPS} ${SPHINX_CONFIG_FILE_TEMPLATE}
    COMMAND Sphinx::sphinx-build ${SPHINX_EXTRA_OPTS} -b html -c ${DOC_BUILD_DIR} -d ${DOC_BUILD_DIR}/doctrees ${CMAKE_SOURCE_DIR}/doc ${DOC_BUILD_DIR}/html ${DOC_SOURCES}
  )

  add_custom_target(
    doc ALL
    DEPENDS docenv ${DOC_BUILD_DIR}/html/index.html ${DOC_BUILD_STATIC_DIR}/mathjax/es5
    SOURCES ${CMAKE_SOURCE_DIR}/doc/requirements.txt ${DOC_SOURCES}
  )

  install(DIRECTORY ${DOC_BUILD_DIR}/html DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

###################
# packaging support

# build LAMMPS-GUI and LAMMPS as flatpak, if tools are installed
find_program(FLATPAK_COMMAND flatpak DOC "Path to flatpak command")
find_program(FLATPAK_BUILDER flatpak-builder DOC "Path to flatpak-builder command")
if(FLATPAK_COMMAND AND FLATPAK_BUILDER)
  set(FLATPAK_BUNDLE "LAMMPS-GUI-Linux-x86_64-${PROJECT_VERSION}.flatpak")
  add_custom_target(flatpak
    COMMAND ${FLATPAK_COMMAND} --user remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
    COMMAND ${FLATPAK_BUILDER} --force-clean --verbose --repo=${CMAKE_CURRENT_BINARY_DIR}/flatpak-repo
                               --install-deps-from=flathub --state-dir=${CMAKE_CURRENT_BINARY_DIR}
                               --user --ccache --default-branch=${PROJECT_VERSION}
                               flatpak-build ${CMAKE_SOURCE_DIR}/resources/org.lammps.lammps-gui.yml
    COMMAND ${FLATPAK_COMMAND} build-bundle --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo --verbose
                               ${CMAKE_CURRENT_BINARY_DIR}/flatpak-repo
                               ${FLATPAK_BUNDLE} org.lammps.lammps-gui ${PROJECT_VERSION}
    COMMENT "Create Flatpak bundle file of LAMMPS-GUI"
    BYPRODUCT ${FLATPAK_BUNDLE}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
else()
  add_custom_target(flatpak
    COMMAND ${CMAKE_COMMAND} -E echo "The flatpak and flatpak-builder commands required to build a LAMMPS-GUI flatpak bundle were not found. Skipping.")
endif()

# when compiling on macOS, create an "app bundle"
if(APPLE)
  set_target_properties(lammps-gui PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/packaging/MacOSXBundleInfo.plist.in
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE_ICON_FILE lammps.icns
    MACOSX_BUNDLE_COPYRIGHT "(c) 2003 - 2025, The LAMMPS Developers"
    MACOSX_BUNDLE TRUE
  )
  # purge app bundle directory during configure
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/lammps-gui.app)

  # additional targets to populate the bundle tree and create the .dmg image file
  set(APP_CONTENTS ${CMAKE_BINARY_DIR}/lammps-gui.app/Contents)
  add_custom_target(complete-bundle
    ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/bin
    COMMAND ${CMAKE_COMMAND} -E create_symlink ../MacOS/lammps-gui ${APP_CONTENTS}/bin/lammps-gui
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/Resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/packaging/README.macos ${APP_CONTENTS}/Resources/README.txt
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/packaging/lammps.icns ${APP_CONTENTS}/Resources
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/packaging/LAMMPS_DMG_Background.png ${APP_CONTENTS}/Resources
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/share/lammps
    COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/share/lammps/man/man1
    DEPENDS lammps-gui
    COMMENT "Copying additional files into macOS app bundle tree"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_custom_target(dmg
    COMMAND ${CMAKE_SOURCE_DIR}/packaging/build_macos_dmg.sh ${PROJECT_VERSION}
    DEPENDS complete-bundle
    COMMENT "Create Drag-n-Drop installer disk image from app bundle"
    BYPRODUCT LAMMPS-macOS-multiarch-${PROJECT_VERSION}.dmg
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  # settings or building on Windows with Visual Studio
elseif(MSVC)
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES $<TARGET_RUNTIME_DLLS:lammps-gui> TYPE BIN)
  if(BUILD_SHARED_LIBS)
    install(FILES $<TARGET_RUNTIME_DLLS:lammps> TYPE BIN)
  endif()
  install(FILES $<TARGET_RUNTIME_DLLS:lmp> TYPE BIN)
  # find path to VC++ init batch file
  get_filename_component(VC_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
  get_filename_component(VC_BASE_DIR "${VC_COMPILER_DIR}/../../../../../.." ABSOLUTE)
  set(VC_INIT "${VC_BASE_DIR}/Auxiliary/Build/vcvarsall.bat")
  get_filename_component(QT5_BIN_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
  get_filename_component(INSTNAME ${CMAKE_INSTALL_PREFIX} NAME)
  install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -D INSTNAME=${INSTNAME} -D VC_INIT=\"${VC_INIT}\" -D QT5_BIN_DIR=\"${QT5_BIN_DIR}\" -P \"${CMAKE_SOURCE_DIR}/packaging/build_windows_vs.cmake\" WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/..\" COMMAND_ECHO STDOUT)")
elseif((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND CMAKE_CROSSCOMPILING)
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES $<TARGET_RUNTIME_DLLS:lammps-gui> TYPE BIN)
  if(BUILD_SHARED_LIBS)
    install(FILES $<TARGET_RUNTIME_DLLS:lammps> TYPE BIN)
  endif()
  install(FILES $<TARGET_RUNTIME_DLLS:lmp> TYPE BIN)
  add_custom_target(zip
    COMMAND sh -vx ${CMAKE_SOURCE_DIR}/packaging/build_windows_cross_zip.sh ${CMAKE_INSTALL_PREFIX} ${PROJECT_VERSION}
    DEPENDS lammps-gui
    COMMENT "Create zip file with windows binaries"
    BYPRODUCT LAMMPS-GUI-Win10-amd64-${PROJECT_VERSION}.zip
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND NOT LAMMPS_GUI_USE_PLUGIN)
  file(STRINGS ${LAMMPS_DIR}/src/version.h line REGEX LAMMPS_VERSION)
  string(REGEX REPLACE "#define LAMMPS_VERSION \"([0-9]+) ([A-Za-z][A-Za-z][A-Za-z])[A-Za-z]* ([0-9]+)\""
                        "\\1\\2\\3" LAMMPS_RELEASE "${line}")
  install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-gui.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-gui.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages/)
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/text/x-application-lammps.xml)
  install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/)
  install(CODE [[
    file(GET_RUNTIME_DEPENDENCIES
      LIBRARIES $<TARGET_FILE:lammps>
      EXECUTABLES $<TARGET_FILE:lammps-gui> $<TARGET_FILE:lmp>
      RESOLVED_DEPENDENCIES_VAR _r_deps
      UNRESOLVED_DEPENDENCIES_VAR _u_deps
    )
    foreach(_file ${_r_deps})
      file(INSTALL
        DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
        TYPE SHARED_LIBRARY
        FOLLOW_SYMLINK_CHAIN
        FILES "${_file}"
      )
    endforeach()
    list(LENGTH _u_deps _u_length)
    if("${_u_length}" GREATER 0)
      message(WARNING "Unresolved dependencies detected: ${_u_deps}")
    endif() ]]
  )

  add_custom_target(tgz
    COMMAND ${CMAKE_SOURCE_DIR}/packaging/build_linux_tgz.sh ${PROJECT_VERSION}
    DEPENDS lmp lammps-gui ${WHAM_EXE}
    COMMENT "Create compressed tar file of LAMMPS-GUI with dependent libraries and wrapper"
    BYPRODUCT LAMMPS-Linux-amd64-${PROJECT_VERSION}.tar.gz
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
endif()
