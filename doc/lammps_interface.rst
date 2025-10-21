****************
LAMMPS Interface
****************

LAMMPS-GUI can operate in two modes:

**Plugin Mode** (default)
  LAMMPS library is loaded dynamically at runtime. This allows using
  different LAMMPS builds without recompiling the GUI. The library
  loading is handled in ``lammpswrapper.cpp`` using platform-specific
  dynamic loading functions (dlopen on Unix, LoadLibrary on Windows).

**Linked Mode**
  LAMMPS library is linked at compile time. Used when building as part
  of the LAMMPS build system with ``-D BUILD_LAMMPS_GUI=on``.

The mode is controlled by the ``LAMMPS_GUI_USE_PLUGIN`` CMake option.
