cmake_minimum_required(VERSION 3.20)

project(lammps-gui
  VERSION 1.9.0.4
  LANGUAGES CXX
  DESCRIPTION "The LAMMPS GUI"
  HOMEPAGE_URL "https://lammps-gui.lammps.org/")
include(GNUInstallDirs)

##################################################################################
# configuration settings

# prefer compiling plugin version only if standalone compile
option(LAMMPS_GUI_USE_PLUGIN "Load LAMMPS library dynamically at runtime" ON)

# prefer Qt6 by default now that Qt5 is no longer officially supported
option(LAMMPS_GUI_USE_QT5 "Prefer using Qt5 over Qt6" OFF)

# build html documentation by default
option(BUILD_DOC "Build LAMMPS-GUI HTML documentation" ON)
##################################################################################

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (NOT BUILD_DOC_ONLY)
  # checks
  if (NOT LAMMPS_SOURCE_DIR AND NOT LAMMPS_GUI_USE_PLUGIN)
    message(FATAL_ERROR "Must provide -D LAMMPS_SOURCE_DIR=<path to LAMMPS sources> when not compiling with -D LAMMPS_GUI_USE_PLUGIN=on")
  endif()
  set(LAMMPS_HEADER_DIR ${LAMMPS_SOURCE_DIR})
  if (NOT LAMMPS_GUI_USE_PLUGIN AND NOT LAMMPS_LIBRARY)
    message(FATAL_ERROR "Must provide -D LAMMPS_LIBRARY=<path to LAMMPS library file> when not compiling with -D LAMMPS_GUI_USE_PLUGIN=on")
  endif()

  # by default, install into $HOME/.local (not /usr/local),
  # so that no root access (and sudo) is needed
  if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Default install path" FORCE)
  endif()

  # ugly hacks for MSVC which by default always reports an old C++ standard in
  # the __cplusplus macro and prints lots of pointless warnings about "unsafe" functions
  if(MSVC)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      add_compile_options(/Zc:__cplusplus)
      add_compile_options(/wd4244)
      add_compile_options(/wd4267)
      add_compile_options(/wd4250)
      add_compile_options(/EHsc)
      add_compile_options(/utf-8)
    endif()
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
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
    find_package(Qt6 6.2 QUIET COMPONENTS Gui Widgets Charts)
  endif()
  if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 REQUIRED COMPONENTS Gui Widgets Charts)
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
    ${CMAKE_SOURCE_DIR}/src/fileviewer.cpp
    ${CMAKE_SOURCE_DIR}/src/fileviewer.h
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
    ${CMAKE_SOURCE_DIR}/src/preferences.cpp
    ${CMAKE_SOURCE_DIR}/src/preferences.h
    ${CMAKE_SOURCE_DIR}/src/qaddon.cpp
    ${CMAKE_SOURCE_DIR}/src/qaddon.h
    ${CMAKE_SOURCE_DIR}/src/rangeslider.cpp
    ${CMAKE_SOURCE_DIR}/src/rangeslider.h
    ${CMAKE_SOURCE_DIR}/src/setvariables.cpp
    ${CMAKE_SOURCE_DIR}/src/setvariables.h
    ${CMAKE_SOURCE_DIR}/src/slideshow.cpp
    ${CMAKE_SOURCE_DIR}/src/slideshow.h
    ${CMAKE_SOURCE_DIR}/src/stdcapture.cpp
    ${PLUGIN_LOADER_SRC}
    ${ICON_RC_FILE}
  )
  if(QT_VERSION_MAJOR EQUAL 6)
    qt6_add_resources(PROJECT_SOURCES resources/lammpsgui.qrc)
  else()
    qt5_add_resources(PROJECT_SOURCES resources/lammpsgui.qrc)
  endif()

  if(APPLE)
    if (LAMMPS_SOURCE_DIR)
      set(MACOSX_ICON_FILE ${LAMMPS_SOURCE_DIR}/../cmake/packaging/lammps-gui.icns)
      set(MACOSX_README_FILE ${LAMMPS_SOURCE_DIR}/../cmake/packaging/README.macos)
      set(MACOSX_BACKGROUND_FILE ${LAMMPS_SOURCE_DIR}/../cmake/packaging/LAMMPS_DMG_Background.png)
      set(MACOSX_PLIST_FILE ${LAMMPS_SOURCE_DIR}/../cmake/packaging/MacOSXBundleInfo.plist.in)
    else()
      set(MACOSX_ICON_FILE ${CMAKE_SOURCE_DIR}/packaging/lammps-gui.icns)
      set(MACOSX_README_FILE ${CMAKE_SOURCE_DIR}/packaging/README.macos)
      set(MACOSX_BACKGROUND_FILE ${CMAKE_SOURCE_DIR}/packaging/LAMMPS_DMG_Background.png)
      set(MACOSX_PLIST_FILE ${CMAKE_SOURCE_DIR}/packaging/MacOSXBundleInfo.plist.in)
    endif()
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

  # when compiling for macOS request compilation of an app-bundle
  if(APPLE)
    string(TIMESTAMP CURRENT_YEAR "%Y")
    set_target_properties(lammps-gui PROPERTIES
      MACOSX_BUNDLE_INFO_PLIST ${MACOSX_PLIST_FILE}
      MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
      MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
      MACOSX_BUNDLE_ICON_FILE lammps-gui.icns
      MACOSX_BUNDLE_COPYRIGHT "(c) 2003 - ${CURRENT_YEAR}, Axel Kohlmeyer"
      MACOSX_BUNDLE TRUE
    )
    # purge app bundle directory during configure
    file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/lammps-gui.app)
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
  target_link_libraries(lammps-gui PRIVATE Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)

  ###################
  # unit tests, only enabled on Linux
  option(ENABLE_TESTING "Build unit tests" OFF)
  if(ENABLE_TESTING AND (NOT CMAKE_CROSSCOMPILING) AND (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    message(STATUS "Testing is enabled")
    include(CTest)
    add_subdirectory(test)
  else()
    message(STATUS "Testing is disabled")
  endif()
endif()

###################
# documentation

if(BUILD_DOC OR BUILD_DOC_ONLY)
  # Find Doxygen for API documentation
  find_package(Doxygen)
  if(DOXYGEN_FOUND)
    set(DOXYGEN_CONFIG_FILE_IN ${CMAKE_SOURCE_DIR}/doc/Doxyfile.in)
    set(DOXYGEN_CONFIG_FILE ${CMAKE_BINARY_DIR}/doc/Doxyfile)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_BINARY_DIR}/doc/doxygen)
    
    # Configure Doxyfile
    configure_file(${DOXYGEN_CONFIG_FILE_IN} ${DOXYGEN_CONFIG_FILE} @ONLY)
    
    # Create custom target for Doxygen
    add_custom_target(doxygen
      COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_CONFIG_FILE}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Generating API documentation with Doxygen"
      VERBATIM
    )
  else()
    message(WARNING "Doxygen not found. API documentation will not be generated.")
  endif()

  # Current Sphinx versions require at least Python 3.8
  # use default (or custom) Python executable, if version is sufficient
  if(Python_VERSION VERSION_GREATER_EQUAL 3.8)
    set(Python3_EXECUTABLE ${Python_EXECUTABLE})
  endif()
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
  if(Python3_VERSION VERSION_LESS 3.8)
    message(FATAL_ERROR "Python 3.8 and up is required to build the LAMMPS-GUI HTML documentation")
  endif()
  if (CMAKE_CROSSCOMPILING)
    # must force host python when cross-compiling
    set(VIRTUALENV python3 -m venv)
  else()
    set(VIRTUALENV ${Python3_EXECUTABLE} -m venv)
  endif()

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
    OUTPUT docenv/bin
    COMMAND ${VIRTUALENV} docenv
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install --upgrade pip
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install -r ${DOCENV_REQUIREMENTS_FILE} --upgrade
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  # custom target to update the python virtual environment
  add_custom_target(
    upgrade
    DEPENDS docenv/bin
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install --upgrade pip
    COMMAND ${DOCENV_BINARY_DIR}/pip $ENV{PIP_OPTIONS} install -r ${DOCENV_REQUIREMENTS_FILE} --upgrade
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  if(NOT TARGET Sphinx::sphinx-build)
    add_executable(Sphinx::sphinx-build IMPORTED GLOBAL)
    set_target_properties(Sphinx::sphinx-build PROPERTIES IMPORTED_LOCATION "${DOCENV_BINARY_DIR}/sphinx-build")
  endif()

  add_custom_target(
    html
    DEPENDS docenv/bin ${DOC_SOURCES} ${DOCENV_DEPS} ${SPHINX_CONFIG_FILE_TEMPLATE} ${SPHINX_DIR}/_static/style.css
    COMMAND Sphinx::sphinx-build ${SPHINX_EXTRA_OPTS} -b html -c ${DOC_BUILD_DIR} -d ${DOC_BUILD_DIR}/doctrees ${CMAKE_SOURCE_DIR}/doc ${DOC_BUILD_DIR}/html ${DOC_SOURCES}
  )
  # remove virtual environment on "make clean"
  set_target_properties(html
    PROPERTIES ADDITIONAL_CLEAN_FILES "${CMAKE_BINARY_DIR}/docenv")

  # Make html target depend on doxygen if available
  if(DOXYGEN_FOUND)
    add_dependencies(html doxygen)
  endif()

  add_custom_target(
    spelling
    DEPENDS docenv/bin ${DOC_SOURCES} ${DOCENV_DEPS} ${SPHINX_CONFIG_FILE_TEMPLATE} ${SPHINX_DIR}/_static/style.css
    COMMAND Sphinx::sphinx-build ${SPHINX_EXTRA_OPTS} -b spelling -c ${DOC_BUILD_DIR} -d ${DOC_BUILD_DIR}/doctrees ${CMAKE_SOURCE_DIR}/doc ${DOC_BUILD_DIR}/spelling ${DOC_SOURCES}
  )

  add_custom_target(
    latex
    DEPENDS docenv/bin ${DOC_SOURCES} ${DOCENV_DEPS} ${SPHINX_CONFIG_FILE_TEMPLATE} ${SPHINX_DIR}/_static/style.css
    COMMAND Sphinx::sphinx-build ${SPHINX_EXTRA_OPTS} -b latex -c ${DOC_BUILD_DIR} -d ${DOC_BUILD_DIR}/doctrees ${CMAKE_SOURCE_DIR}/doc ${DOC_BUILD_DIR}/latex ${DOC_SOURCES}
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/doc/idxlayout.sty ${DOC_BUILD_DIR}/latex/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/doc/ellipse.sty ${DOC_BUILD_DIR}/latex/
  )

  # need latexmk and pdflatex for tranlating LaTeX to PDF
  find_program(PDFLATEX_COMMAND pdflatex DOC "Path to pdflatex command")
  find_program(LATEXMK_COMMAND latexmk DOC "Path to latex command")
  if (PDFLATEX_COMMAND AND LATEXMK_COMMAND)
    add_custom_target(
      pdf
      DEPENDS latex
      COMMAND ${LATEXMK_COMMAND} -pdf -dvi- -ps- lammps-gui.tex
      COMMAND ${CMAKE_COMMAND} -E rename lammps-gui.pdf ../../lammps-gui-v${PROJECT_VERSION}.pdf
      WORKING_DIRECTORY ${DOC_BUILD_DIR}/latex
    )
  else()
    add_custom_target(
      pdf
      COMMAND ${CMAKE_COMMAND} -E echo "Need to have pdflatex and latexmk installed to create PDF of docs"
    )
  endif()

  add_custom_target(
    doc ALL
    DEPENDS docenv/bin html
    SOURCES ${CMAKE_SOURCE_DIR}/doc/requirements.txt ${DOC_SOURCES}
  )

  install(DIRECTORY ${DOC_BUILD_DIR}/html DESTINATION ${CMAKE_INSTALL_DOCDIR})
endif()

###################
# packaging support
# make this only available when compiling in plugin mode
# when linking to LAMMPS we are compiled as external project
# and then LAMMPS provides the packaging targets.

if (LAMMPS_GUI_USE_PLUGIN AND NOT BUILD_DOC_ONLY)
  # build LAMMPS-GUI as flatpak
  if (WE_DONT_SUPPORT_FLATPAK_YET)
    find_program(FLATPAK_COMMAND flatpak DOC "Path to flatpak command")
    find_program(FLATPAK_BUILDER flatpak-builder DOC "Path to flatpak-builder command")
    if(FLATPAK_COMMAND AND FLATPAK_BUILDER)
      set(FLATPAK_BUNDLE "LAMMPS-GUI-Linux-x86_64-${PROJECT_VERSION}.flatpak")
      add_custom_target(flatpak
        COMMAND ${FLATPAK_COMMAND} --user remote-add --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
        COMMAND ${FLATPAK_BUILDER} --force-clean --verbose --repo=${CMAKE_CURRENT_BINARY_DIR}/flatpak-repo
                                   --install-deps-from=flathub --state-dir=${CMAKE_CURRENT_BINARY_DIR}
                                   --user --ccache --default-branch=${PROJECT_VERSION}
                                   flatpak-build ${CMAKE_SOURCE_DIR}/packaging/org.lammps.lammps-gui.yml
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
  else()
    add_custom_target(flatpak
      COMMAND ${CMAKE_COMMAND} -E echo "Building LAMMPS-GUI in plugin mode for flatpak is currently not supported")
  endif()

  # when compiling on macOS, create an "app bundle"
  if(APPLE)
    # additional targets to populate the bundle tree and create the .dmg image file
    set(APP_CONTENTS ${CMAKE_BINARY_DIR}/lammps-gui.app/Contents)
    add_custom_target(complete-bundle
      ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/bin
      COMMAND ${CMAKE_COMMAND} -E create_symlink ../MacOS/lammps-gui ${APP_CONTENTS}/bin/lammps-gui
      COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_CONTENTS}/Resources
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_README_FILE} ${APP_CONTENTS}/Resources/README.txt
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_ICON_FILE} ${APP_CONTENTS}/Resources
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MACOSX_BACKGROUND_FILE} ${APP_CONTENTS}/Resources
      DEPENDS lammps-gui
      COMMENT "Copying additional files into macOS app bundle tree"
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
    add_custom_target(dmg
      COMMAND ${CMAKE_SOURCE_DIR}/packaging/build_macos_dmg.sh ${PROJECT_VERSION}
      DEPENDS complete-bundle
      COMMENT "Create Drag-n-Drop installer disk image from app bundle"
      BYPRODUCT LAMMPS-GUI-macOS-multiarch-${PROJECT_VERSION}.dmg
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  # settings for building on Windows with Visual Studio
  elseif(MSVC)
    install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES $<TARGET_RUNTIME_DLLS:lammps-gui> TYPE BIN)
    # find path to VC++ init batch file
    get_filename_component(VC_COMPILER_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(VC_BASE_DIR "${VC_COMPILER_DIR}/../../../../../.." ABSOLUTE)
    set(VC_INIT "${VC_BASE_DIR}/Auxiliary/Build/vcvarsall.bat")
    get_filename_component(QT5_BIN_DIR "${Qt5Core_DIR}/../../../bin" ABSOLUTE)
    get_filename_component(INSTNAME ${CMAKE_INSTALL_PREFIX} NAME)
    install(CODE "execute_process(COMMAND \"${CMAKE_COMMAND}\" -D INSTNAME=${INSTNAME} -D VC_INIT=\"${VC_INIT}\" -D QT5_BIN_DIR=\"${QT5_BIN_DIR}\" -P \"${CMAKE_SOURCE_DIR}/packaging/build_windows_vs.cmake\" WORKING_DIRECTORY \"${CMAKE_INSTALL_PREFIX}/..\" COMMAND_ECHO STDOUT)")
  # settings for packaging Windows NSIS installer on Linux with MinGW cross-compiler
  elseif((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND CMAKE_CROSSCOMPILING)
    install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    add_custom_target(nsis
      COMMAND sh -vx ${CMAKE_SOURCE_DIR}/packaging/build_windows_cross_nsis.sh ${CMAKE_INSTALL_PREFIX} ${PROJECT_VERSION} ${CMAKE_SOURCE_DIR}
      DEPENDS lammps-gui pdf
      COMMENT "Create nsis installer with windows binaries"
      BYPRODUCT LAMMPS-GUI-Win10-x86_64-${PROJECT_VERSION}.exe
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
  elseif((CMAKE_SYSTEM_NAME STREQUAL "Linux") AND LAMMPS_GUI_USE_PLUGIN)
    install(TARGETS lammps-gui DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/lammps-gui.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications/)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/lammps-gui.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/appdata/)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages/)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/packaging/lammps-input.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/text/x-application-lammps.xml)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/)
    install(CODE [[
      file(GET_RUNTIME_DEPENDENCIES
        EXECUTABLES $<TARGET_FILE:lammps-gui>
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
      DEPENDS lammps-gui
      COMMENT "Create compressed tar file of LAMMPS-GUI with dependent libraries and wrapper"
      BYPRODUCT LAMMPS-GUI-Linux-x86_64-${PROJECT_VERSION}.tar.gz
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
  endif()
endif()

find_package(Git)
if (GIT_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/.git)
  add_custom_target(tarball
    COMMAND ${GIT_EXECUTABLE} archive --prefix=lammps-gui-v${PROJECT_VERSION}/
            --output=${CMAKE_BINARY_DIR}/lammps-gui-src-v${PROJECT_VERSION}.tar.gz  HEAD
    COMMENT "Create compressed tar file of the LAMMPS-GUI sources"
    BYPRODUCT lammps-gui-src-v${PROJECT_VERSION}.tar.gz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()
