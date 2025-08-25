########################
LAMMPS-GUI Documentation
########################

.. toctree::
   :caption: About LAMMPS-GUI

****************
About LAMMPS-GUI
****************

.. image:: JPG/lammps-gui-banner.png
   :align: center
   :scale: 75%

LAMMPS-GUI is a graphical text editor programmed using the `Qt Framework
<https://www.qt.io/>`_ and customized for editing and running LAMMPS
input files.  It is linked to the :ref:`LAMMPS library <lammps_c_api>`
and thus can run LAMMPS directly using the contents of the editor's text
buffer as input and without having to launch the LAMMPS executable.

It *differs* from other known interfaces to LAMMPS in that it can
retrieve and display information from LAMMPS *while it is running*,
display visualizations created with the :doc:`dump image command
<dump_image>`, can launch the online LAMMPS documentation for known
LAMMPS commands and styles, and directly integrates with a collection
of LAMMPS tutorials (:ref:`Gravelle1 <Gravelle1>`).

This document describes LAMMPS-GUI version |version|.

.. toctree::
   :caption: LAMMPS-GUI Documentation
   :maxdepth: 2
   :numbered: 3
   :name: userdoc
   :includehidden:

   overview
   installation
             
----------

.. only:: html

  .. _webbrowser:
  .. admonition:: Web Browser Compatibility
     :class: note

     The HTML version of the manual makes use of advanced features present
     in "modern" web browsers.  This leads to incompatibilities with older
     web browsers and specific vendor browsers (e.g. Internet Explorer on Windows)
     where parts of the pages are not rendered as expected (e.g. the layout is
     broken or mathematical expressions not typeset).

     The following web browser versions have been verified to work as
     expected on Linux, macOS, and Windows where available:

     - Safari version 11.1 and later
     - Firefox version 54 and later
     - Chrome version 54 and later
     - Opera version 41 and later
     - Edge version 80 and later

     Also Android version 7.1 and later and iOS version 11 and later have
     been verified to render this website as expected.
