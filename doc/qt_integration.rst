**************
Qt Integration
**************

LAMMPS-GUI makes extensive use of Qt features:

**Signals and Slots**
  Used for inter-component communication, especially between GUI
  components and background threads.

**Qt Designer Forms**
  Main window and dialogs use ``.ui`` files edited in Qt Designer.

**Qt Resource System**
  Icons and resources embedded via ``resources/lammpsgui.qrc``.

**Qt Models**
  Used for data display in various viewers and inspectors.

For more details on Qt usage, see the `Qt Documentation <https://doc.qt.io/>`_.
