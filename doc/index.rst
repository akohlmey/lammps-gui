########################
LAMMPS-GUI Documentation
########################

.. toctree::
   :caption: LAMMPS-GUI Documentation

.. only:: html

   .. image:: _images/lammps-gui-banner.png
      :align: center
      :scale: 75%

.. raw:: latex

   \clearpage

****************
About LAMMPS-GUI
****************

LAMMPS-GUI is a graphical text editor with syntax highlighting,
auto-completion, inline help, and indentation support for `LAMMPS
<https://www.lammps.org/>`_ input files.  It is programmed using the `Qt
Framework <https://www.qt.io/>`_ and customized for running, monitoring,
and visualizing LAMMPS simulations.  It calls LAMMPS directly using the
`LAMMPS library interface
<https://docs.lammps.org/Library.html#lammps-c-library-api>`_ instead of
launching an external LAMMPS executable.  Therefore it can retrieve and
display information from LAMMPS *while it is running* and *immediately*
display visualizations created by a dump image command in the input.

The primary motivation for implementing LAMMPS-GUI is to facilitate
teaching LAMMPS to beginners using only LAMMPS-GUI and to have a
consistent behavior across major platforms like Linux, macOS, and
Windows.  This way one can focus on teaching LAMMPS and avoid having to
spent time explaining different tools (for editing inputs, plotting
graphs, visualizing systems) on the different platforms.  Also,
LAMMPS-GUI is fully integrated with a `collection of LAMMPS tutorials
<https://lammpstutorials.github.io>`_.

Many of the features in LAMMPS-GUI are useful beyond working on
tutorials.  For instance, it can streamline the process of prototyping
new simulation projects or debugging misbehaving simulations.

LAMMPS-GUI is Copyright (c) |copyright|, and distributed under the
terms of the GNU public license version 2.0 or later (GPL-2.0-or-later).

--------

*******************
About this document
*******************

This document contains the documentation of LAMMPS-GUI and how to
compile, install, use, configure, and modify it.  Suggestions for new
features and reports of bugs are always welcome.  You can use the `the
same channels as for LAMMPS itself
<https://docs.lammps.org/Errors_bugs.html>`_ for that purpose or submit
bug reports or pull requests in the `LAMMPS-GUI GitHub repository
<https://github.com/akohlmey/lammps-gui>`_

------------------

.. raw:: html

   <h2>

This document describes LAMMPS-GUI version |version|.

.. raw:: html

   </h2>
   <hr>
   <h3>Test Status of the development branch:</h3>
   <p dir="auto"><a href="https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt5.yml"><img src="https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt5.yml/badge.svg" alt="Compile with Qt 5.15LTS" style="max-width: 100%;"></a>
   <a href="https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt6.yml"><img src="https://github.com/akohlmey/lammps-gui/actions/workflows/compile-linux-qt6.yml/badge.svg" alt="Compile with Qt 6.x" style="max-width: 100%;"></a>
   <a href="https://github.com/akohlmey/lammps-gui/actions/workflows/codeql-analysis.yml"><img src="https://github.com/akohlmey/lammps-gui/actions/workflows/codeql-analysis.yml/badge.svg" alt="CodeQL Code Analysis" style="max-width: 100%;"></a>
   <a href="https://github.com/akohlmey/lammps-gui/actions/workflows/build-html-docs.yml"><img src="https://github.com/akohlmey/lammps-gui/actions/workflows/build-html-docs.yml/badge.svg" alt="Build Documentation in HTML" style="max-width: 100%;"></a></p>

------------------

*****************
Citing LAMMPS-GUI
*****************

There is currently no citation specifically describing LAMMPS-GUI, but
an introduction to LAMMPS-GUI is included in the following publication
in LiveCoMS for the LAMMPS tutorials that are linked from LAMMPS-GUI, so
the suggestion is to cite that publication for now:

   Gravelle, S., Alvares, C. M. S., Gissinger, J. R., &
   Kohlmeyer, A. (2025). A Set of Tutorials for the LAMMPS Simulation
   Package [Article v1.0]. Living Journal of Computational Molecular
   Science, 6(1), 3027. https://doi.org/10.33011/livecoms.6.1.3037

or in BibTeX format:

.. code-block:: bibtex

   @article{lammps_tutorials_2025,
     author={Gravelle, Simon and Alvares, Cecilia M. S. and Gissinger, Jacob R. and Kohlmeyer, Axel},
     title={A Set of Tutorials for the {LAMMPS} Simulation Package [Article v1.0]},
     journal={Living Journal of Computational Molecular Science},
     pages={3027},
     volume={6},
     number={1},
     year={2025},
     month={Sep.},
     url={https://livecomsjournal.org/index.php/livecoms/article/view/v6i1e3037},
     DOI={10.33011/livecoms.6.1.3037}
   }

------------------

.. raw:: latex

   \clearpage

************
User's Guide
************

.. toctree::
   :caption: Table of Contents
   :maxdepth: 2
   :numbered: 3
   :name: userdoc
   :includehidden:

   installation
   overview
   basic_usage
   output
   visualization
   editor
   menus
   dialogs
   shortcuts

------------------

.. raw:: latex

   \clearpage

******************
Programmer's Guide
******************

This guide provides documentation for developers who want to understand
the internals of LAMMPS-GUI or contribute to its development.

.. admonition:: AI Generated Content
   :class: note

   The initial version of the Programmer's Guide section was created by
   a `GitHub Copilot Coding Agent <https://docs.github.com/en/copilot>`_
   and not everything has been carefully checked yet.  It is therefore
   possible that it contains errors where the LLM has misinterpreted the
   LAMMPS-GUI source code.  If you spot any such errors or inconsistencies,
   please submit a bug report to point them out or a pull request with
   corrections.

.. toctree::
   :caption: Table of Contents
   :maxdepth: 2
   :numbered: 3
   :name: progdoc
   :includehidden:

   introduction
   architecture
   api_reference
   guidelines
   lammps_interface
   qt_integration
   testing

----------

.. only:: html

   ****************
   Index and Search
   ****************

          * :ref:`genindex`
          * :ref:`search`

   ----------

   .. _webbrowser:
   .. admonition:: Web Browser Compatibility

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
