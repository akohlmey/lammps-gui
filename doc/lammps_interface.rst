****************
LAMMPS Interface
****************

LAMMPS-GUI can operate in two modes: **Plugin Mode** and **Linked
Mode**.  The mode is controlled by the ``-D
LAMMPS_GUI_USE_PLUGIN=(ON|OFF)`` CMake configuration option.

**Plugin Mode** (default)
  LAMMPS is loaded dynamically at runtime from a shared library file
  (.so, .dll, .dylib).  This allows using different LAMMPS builds with
  different compilation setting and different LAMMPS versions without
  recompiling the GUI. The library loading is handled in
  :cpp:class:`LammpsWrapper` using platform-specific dynamic loading
  functions (``dlopen()`` on Unix/Linux/macOS, ``LoadLibrary()`` on
  Windows).  The path to the shared library file is auto-detected or
  configured via command line or preferences.

**Linked Mode** LAMMPS library is linked at compile time.  Used by
  default when building LAMMPS-GUI as part of a LAMMPS CMake build with
  ``-D BUILD_LAMMPS_GUI=on``.  For standalone builds also the ``-D
  LAMMPS_SOURCE_DIR=<path to LAMMPS' src folder>`` and ``-D
  LAMMPS_LIBRARY=<path to LAMMPS shared or static library file>``
  settings are required when configuring with CMake.  It may be needed
  to adjust the environment variable to find shared libraries (``LD_LIBRARY_PATH``
  on Linux, ``DYLD_LIBRARY_PATH`` on macOS, or ``PATH`` on Windows)
  when linked to a shared library.
