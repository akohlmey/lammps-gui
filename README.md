# LAMMPS-GUI - The graphical interface for learning and running LAMMPS

Please see the online documentation at https://lammps-gui.lammps.org/

## Test Status of the development branch

[![Compile with Qt 5.15LTS](https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt5.yml/badge.svg)](https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt5.yml)
[![Compile with Qt 6.x](https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt6.yml/badge.svg)](https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt6.yml)
[![CodeQL Code Analysis](https://github.com/akohlmey/lammps-gui/actions/workflows/codeql-analysis.yml/badge.svg)](https://github.com/akohlmey/lammps-gui/actions/workflows/codeql-analysis.yml)

## LAMMPS-GUI vs. LAMMPS

LAMMPS-GUI used to "live" in the "tools/lammps-gui" folder of the LAMMPS source distribution
and the LAMMPS git repository.  This made it easy to build LAMMPS-GUI together with LAMMPS.
However, LAMMPS-GUI has matured to the point, that it is easier to maintain it as a separate
package and make its release schedule independent from LAMMPS releases.  It is still possible
to build LAMMPS-GUI as before, but that will just automatically first download the LAMMPS-GUI
sources from this repository.


