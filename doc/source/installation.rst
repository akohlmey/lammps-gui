************
Installation
************

Pre-compiled executables
^^^^^^^^^^^^^^^^^^^^^^^^

Pre-compiled LAMMPS executable packages that include the GUI are
currently available from https://download.lammps.org/static/ or
https://github.com/lammps/lammps/releases.  For Windows, you need to
download and then run the application installer.  For macOS you download
and mount the disk image and then drag the application bundle to the
Applications folder.  For Linux (x86_64) you currently have two
options: 1) you can download the tar.gz archive, unpack it and run the
GUI directly in place.  The ``LAMMPS_GUI`` folder may also be moved
around and added to the ``PATH`` environment variable so the executables
will be found automatically.  2) you can download the `Flatpak file
<https://www.flatpak.org/>`_ and then install it locally with the
*flatpak* command: ``flatpak install --user
LAMMPS-Linux-x86_64-GUI-<version>.flatpak`` and run it with ``flatpak
run org.lammps.lammps-gui``.  The flatpak bundle also includes the
command-line version of LAMMPS and some LAMMPS tools like msi2lmp.  The
can be launched by using the ``--command`` flag. For example to run
LAMMPS directly on the ``in.lj`` benchmark input you would type in the
``bench`` folder: ``flatpak run --command=lmp -in in.lj`` The flatpak
version should also appear in the applications menu of standard desktop
environments.  The LAMMPS-GUI executable is called ``lammps-gui`` and
either takes no arguments or attempts to load the first argument as
LAMMPS input file.

.. _lammps_gui_compilation:

Compilation
^^^^^^^^^^^

The source for the LAMMPS-GUI is included with the LAMMPS source code
distribution in the folder ``tools/lammps-gui`` and thus it can be can
be built as part of a regular LAMMPS compilation.  :doc:`Using CMake
<Howto_cmake>` is required.  To enable its compilation, the CMake
variable ``-D BUILD_LAMMPS_GUI=on`` must be set when creating the CMake
configuration.  All other settings (compiler, flags, compile type) for
LAMMPS-GUI are then inherited from the regular LAMMPS build.  If the Qt
library is packaged for Linux distributions, then its location is
typically auto-detected since the required CMake configuration files are
stored in a location where CMake can find them without additional help.
Otherwise, the location of the Qt library installation must be indicated
by setting ``-D Qt5_DIR=/path/to/qt5/lib/cmake/Qt5``, which is a path to
a folder inside the Qt installation that contains the file
``Qt5Config.cmake``. Similarly, for Qt6 the location of the Qt library
installation can be indicated by setting ``-D
Qt6_DIR=/path/to/qt6/lib/cmake/Qt6``, if necessary.  When both, Qt5 and
Qt6 are available, Qt6 will be preferred unless ``-D
LAMMPS_GUI_USE_QT5=yes`` is set.

It is possible to build the LAMMPS-GUI as a standalone compilation
(e.g. when LAMMPS has been compiled with traditional make).  Then the
CMake configuration needs to be told where to find the LAMMPS headers
and the LAMMPS library, via ``-D LAMMPS_SOURCE_DIR=/path/to/lammps/src``.
CMake will try to guess a build folder with the LAMMPS library from that
path, but it can also be set with ``-D LAMMPS_LIB_DIR=/path/to/lammps/lib``.

Plugin version
""""""""""""""

Rather than linking to the LAMMPS library during compilation, it is also
possible to compile the GUI with a plugin loader that will load the
LAMMPS library dynamically at runtime during the start of the GUI from a
shared library; e.g. ``liblammps.so`` or ``liblammps.dylib`` or
``liblammps.dll`` (depending on the operating system).  This has the
advantage that the LAMMPS library can be built from updated or modified
LAMMPS source without having to recompile the GUI.  The ABI of the
LAMMPS C-library interface is very stable and generally backward
compatible.  This feature is enabled by setting ``-D
LAMMPS_GUI_USE_PLUGIN=on`` and then ``-D
LAMMPS_PLUGINLIB_DIR=/path/to/lammps/plugin/loader``. Typically, this
would be the ``examples/COUPLE/plugin`` folder of the LAMMPS
distribution.

When compiling LAMMPS-GUI with plugin support, there is an additional
command-line flag (``-p <path>`` or ``--pluginpath <path>``) which
allows to override the path to LAMMPS shared library used by the GUI.
This is usually auto-detected on the first run and can be changed in the
LAMMPS-GUI *Preferences* dialog.  The command-line flag allows to reset
this path to a valid value in case the original setting has become
invalid.  An empty path ("") as argument restores the default setting.

.. versionchanged:: TBD

When loading a LAMMPS library, it must be at least version 27 August
2025 for LAMMPS-GUI version 1.7.1, since it uses features only available
since that LAMMPS version.  Older LAMMPS versions will be rejected.

Platform notes
^^^^^^^^^^^^^^

macOS
"""""

When building on macOS, the build procedure will try to manufacture a
drag-n-drop installer, ``LAMMPS-macOS-multiarch.dmg``, when using the
'dmg' target (i.e. ``cmake --build <build dir> --target dmg`` or ``make dmg``.

To build multi-arch executables that will run on both, arm64 and x86_64
architectures natively, it is necessary to set the CMake variable ``-D
CMAKE_OSX_ARCHITECTURES=arm64;x86_64``.  To achieve wide compatibility
with different macOS versions, you can also set ``-D
CMAKE_OSX_DEPLOYMENT_TARGET=11.0`` which will set compatibility to macOS
11 (Big Sur) and later, even if you are compiling on a more recent macOS
version.

Windows
"""""""

On Windows either native compilation from within Visual Studio 2022 with
Visual C++ is supported and tested, or compilation with the MinGW / GCC
cross-compiler environment on Fedora Linux.

**Visual Studio**

Using CMake and Ninja as build system are required.  Qt needs to be
installed, tested was a binary package downloaded from
https://www.qt.io, which installs into the ``C:\\Qt`` folder by default.
There is a custom `x64-GUI-MSVC` build configuration provided in the
``CMakeSettings.json`` file that Visual Studio uses to store different
compilation settings for project.  Choosing this configuration will
activate building the `lammps-gui.exe` executable in addition to LAMMPS
through importing package selection from the ``windows.cmake`` preset
file and enabling building the LAMMPS-GUI and disabling building with MPI.
When requesting an installation from the `Build` menu in Visual Studio,
it will create a compressed ``LAMMPS-Win10-amd64.zip`` zip file with the
executables and required dependent .dll files.  This zip file can be
uncompressed and ``lammps-gui.exe`` run directly from there.  The
uncompressed folder can be added to the ``PATH`` environment and LAMMPS
and LAMMPS-GUI can be launched from anywhere from the command-line.

**MinGW64 Cross-compiler**

The standard CMake build procedure can be applied and the
``mingw-cross.cmake`` preset used. By using ``mingw64-cmake`` the CMake
command will automatically include a suitable CMake toolchain file (the
regular cmake command can be used after that to modify the configuration
settings, if needed).  After building the libraries and executables,
you can build the target 'zip' (i.e. ``cmake --build <build dir> --target zip``
or ``make zip`` to stage all installed files into a LAMMPS_GUI folder
and then run a script to copy all required dependencies, some other files,
and create a zip file from it.

Linux
"""""

Version 5.12 or later of the Qt library is required. Those are provided
by, e.g., Ubuntu 20.04LTS.  Thus older Linux distributions are not
likely to be supported, while more recent ones will work, even for
pre-compiled executables (see above).  After compiling with
``cmake --build <build folder>``, use ``cmake --build <build
folder> --target tgz`` or ``make tgz`` to build a
``LAMMPS-Linux-amd64.tar.gz`` file with the executables and their
support libraries.

It is also possible to build a `flatpak bundle
<https://docs.flatpak.org/en/latest/single-file-bundles.html>`_ which is
a way to distribute applications in a way that is compatible with most
Linux distributions.  Use the "flatpak" target to trigger a compile
(``cmake --build <build folder> --target flatpak`` or ``make flatpak``).
Please note that this will not build from the local sources but from the
repository and branch listed in the ``org.lammps.lammps-gui.yml``
LAMMPS-GUI source folder.

-----

Installing Pre-compiled LAMMPS-GUI Packages
-------------------------------------------

LAMMPS-GUI is available for download as pre-compiled binary packages for
Linux x86\_64 (Ubuntu 22.04LTS or later and compatible), macOS (version
11 aka Big Sur or later), and Windows (version 10 or later) from the
`LAMMPS release pages on GitHub <https://github.com/lammps/lammps/releases/>`_.
A backup download location is at https://download.lammps.org/static/
Alternately, LAMMPS-GUI can be compiled from source when building LAMMPS.

.. admonition:: GPU support and MPI parallelization
   :class: note

   The pre-compiled packages include support for GPUs through the GPU
   package with OpenCL (in mixed precision).  However, this requires
   that you have a compatible driver and the OpenCL runtime installed.
   This is not always available and when using the flatpak package, the
   flatpak sandbox prevents accessing the GPU.  GPU support through
   KOKKOS is currently not available for technical reasons, but serial
   and OpenMP multi-threading is available.

   The design decisions for LAMMPS-GUI and how it launches LAMMPS
   conflict with parallel runs using MPI.  You have to :doc:`use a
   regular LAMMPS executable <Run_basics>` compiled with MPI support for
   that.  For the use cases that LAMMPS-GUI has been conceived for this
   is not a significant limitation.

Windows 10 and later
^^^^^^^^^^^^^^^^^^^^

After downloading the ``LAMMPS-Win10-64bit-GUI-<version>.exe`` installer
package, you need to execute it, and start the installation process.
Since those packages are currently unsigned, you have to enable "Developer Mode"
in the Windows System Settings to run the installer.

MacOS 11 and later
^^^^^^^^^^^^^^^^^^

After downloading the ``LAMMPS-macOS-multiarch-GUI-<version>.dmg``
application bundle disk image, you need to double-click it and then, in
the window that opens, drag the app bundle as indicated into the
"Applications" folder.  Afterwards, the disk image can be unmounted.
Then follow the instructions in the "README.txt" file to get access to
the other included command-line executables.

Linux on x86\_64
^^^^^^^^^^^^^^^^

For Linux with x86\_64 CPU there are currently two variants. The first
is compiled on Ubuntu 20.04LTS, is using some wrapper scripts, and
should be compatible with more recent Linux distributions.  After
downloading and unpacking the
``LAMMPS-Linux-x86_64-GUI-<version>.tar.gz`` package.  You can switch
into the "LAMMPS_GUI" folder and execute "./lammps-gui" directly.

The second variant uses `flatpak <https://www.flatpak.org>`_ and
requires the flatpak management and runtime software to be installed.
After downloading the ``LAMMPS-GUI-Linux-x86_64-GUI-<version>.flatpak``
flatpak bundle, you can install it with ``flatpak install --user
LAMMPS-GUI-Linux-x86_64-GUI-<version>.flatpak``.  After installation,
LAMMPS-GUI should be integrated into your desktop environment under
"Applications > Science" but also can be launched from the console with
``flatpak run org.lammps.lammps-gui``.  The flatpak bundle also includes
the console LAMMPS executable ``lmp`` which can be launched to run
simulations with, for example with:

.. code-block:: sh

   flatpak run --command=lmp org.lammps.lammps-gui -in in.melt

Other bundled command-line executables are run the same way and can be
listed with:

.. code-block:: sh

   ls $(flatpak info --show-location org.lammps.lammps-gui )/files/bin

Compiling from Source
^^^^^^^^^^^^^^^^^^^^^

There also are instructions for :ref:`compiling LAMMPS-GUI from source
code <lammps_gui_compilation>` available elsewhere in the manual.
Compilation from source *requires* using CMake.


Installing Pre-compiled LAMMPS-GUI Packages
-------------------------------------------

LAMMPS-GUI is available for download as pre-compiled binary packages for
Linux x86\_64 (Ubuntu 22.04LTS or later and compatible), macOS (version
11 aka Big Sur or later), and Windows (version 10 or later) from the
`LAMMPS release pages on GitHub <https://github.com/lammps/lammps/releases/>`_.
A backup download location is at https://download.lammps.org/static/
Alternately, LAMMPS-GUI can be compiled from source when building LAMMPS.

.. admonition:: GPU support and MPI parallelization
   :class: note

   The pre-compiled packages include support for GPUs through the GPU
   package with OpenCL (in mixed precision).  However, this requires
   that you have a compatible driver and the OpenCL runtime installed.
   This is not always available and when using the flatpak package, the
   flatpak sandbox prevents accessing the GPU.  GPU support through
   KOKKOS is currently not available for technical reasons, but serial
   and OpenMP multi-threading is available.

   The design decisions for LAMMPS-GUI and how it launches LAMMPS
   conflict with parallel runs using MPI.  You have to :doc:`use a
   regular LAMMPS executable <Run_basics>` compiled with MPI support for
   that.  For the use cases that LAMMPS-GUI has been conceived for this
   is not a significant limitation.

Windows 10 and later
^^^^^^^^^^^^^^^^^^^^

After downloading the ``LAMMPS-Win10-64bit-GUI-<version>.exe`` installer
package, you need to execute it, and start the installation process.
Since those packages are currently unsigned, you have to enable "Developer Mode"
in the Windows System Settings to run the installer.

MacOS 11 and later
^^^^^^^^^^^^^^^^^^

After downloading the ``LAMMPS-macOS-multiarch-GUI-<version>.dmg``
application bundle disk image, you need to double-click it and then, in
the window that opens, drag the app bundle as indicated into the
"Applications" folder.  Afterwards, the disk image can be unmounted.
Then follow the instructions in the "README.txt" file to get access to
the other included command-line executables.

Linux on x86\_64
^^^^^^^^^^^^^^^^

For Linux with x86\_64 CPU there are currently two variants. The first
is compiled on Ubuntu 20.04LTS, is using some wrapper scripts, and
should be compatible with more recent Linux distributions.  After
downloading and unpacking the
``LAMMPS-Linux-x86_64-GUI-<version>.tar.gz`` package.  You can switch
into the "LAMMPS_GUI" folder and execute "./lammps-gui" directly.

The second variant uses `flatpak <https://www.flatpak.org>`_ and
requires the flatpak management and runtime software to be installed.
After downloading the ``LAMMPS-GUI-Linux-x86_64-GUI-<version>.flatpak``
flatpak bundle, you can install it with ``flatpak install --user
LAMMPS-GUI-Linux-x86_64-GUI-<version>.flatpak``.  After installation,
LAMMPS-GUI should be integrated into your desktop environment under
"Applications > Science" but also can be launched from the console with
``flatpak run org.lammps.lammps-gui``.  The flatpak bundle also includes
the console LAMMPS executable ``lmp`` which can be launched to run
simulations with, for example with:

.. code-block:: sh

   flatpak run --command=lmp org.lammps.lammps-gui -in in.melt

Other bundled command-line executables are run the same way and can be
listed with:

.. code-block:: sh

   ls $(flatpak info --show-location org.lammps.lammps-gui )/files/bin

Compiling from Source
^^^^^^^^^^^^^^^^^^^^^

There also are instructions for :ref:`compiling LAMMPS-GUI from source
code <lammps_gui_compilation>` available elsewhere in the manual.
Compilation from source *requires* using CMake.
