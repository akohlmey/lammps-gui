########################
LAMMPS-GUI Documentation
########################

.. toctree::
   :caption: About LAMMPS-GUI

.. image:: _images/lammps-gui-banner.png
   :align: center
   :scale: 75%

****************
About LAMMPS-GUI
****************

LAMMPS-GUI is a graphical text editor with syntax highlighting,
auto-completion, inline help, and indentation support for `LAMMPS
<https://www.lammps.org/>`_ input files.  It is programmed using the `Qt
Framework <https://www.qt.io/>`_ and customized for running, monitoring,
and visualizing LAMMPS simulations. It calls LAMMPS directly using the
`LAMMPS library interface
<https://docs.lammps.org/Library.html#lammps-c-library-api>`_ and does
not have to run an external LAMMPS executable. Therefore it can retrieve
and display information from LAMMPS *while it is running*, display
visualizations created with the dump image command.

The primary goal is to facilitate teaching LAMMPS to beginners using
just LAMMPS-GUI and have a consistent behavior across major platforms
like Linux, macOS, and Windows.  This way one can focus on teaching
LAMMPS.  As a demonstration LAMMPS-GUI is fully integrated with a
`collection of LAMMPS tutorials <https://lammpstutorials.github.io>`_.
LAMMPS-GUI also offers useful features beyond tutorials and for
intermediate and advanced LAMMPS users.

------------------

This document describes LAMMPS-GUI version |version|.

------------------

.. toctree::
   :caption: Table of Contents
   :maxdepth: 2
   :numbered: 3
   :name: userdoc
   :includehidden:

   installation
   overview
   basic_usage

----------

.. only:: html

  .. _webbrowser:
  .. admonition:: Web Browser Compatibility
     :class: note

     This website makes use of advanced features present in "modern" web
     browsers.  This leads to incompatibilities with older web browsers
     and specific vendor browsers (e.g. Internet Explorer on Windows)
     where parts of the pages are not rendered as expected (e.g. the
     layout is broken or mathematical expressions not typeset).

     The following web browser versions have been verified to work as
     expected on Linux, macOS, and Windows where available:

     - Safari version 11.1 and later
     - Firefox version 54 and later
     - Chrome version 54 and later
     - Opera version 41 and later
     - Edge version 80 and later

     Also Android version 7.1 and later and iOS version 11 and later have
     been verified to render this website as expected.
