************
Introduction
************

LAMMPS-GUI is built using C++17 and the Qt framework (Qt 5.15+ or Qt 6.2+).
The application follows object-oriented design principles with clear separation
of concerns between different components:

- **Editor Components**: Handle text editing, syntax highlighting, and auto-completion
- **LAMMPS Interface**: Wraps the LAMMPS C library API
- **Visualization**: Displays images, charts, and simulation output
- **GUI Framework**: Main window, dialogs, and preferences
