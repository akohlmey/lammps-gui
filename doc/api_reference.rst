*************
API Reference
*************

The following sections provide detailed API documentation for the main
classes in LAMMPS-GUI.  Documentation is generated from `Doxygen comments
<https://doxygen.nl>`_ in the source code.


Main Window
===========

LammpsGui Class
---------------

.. doxygenclass:: LammpsGui
   :members:
   :protected-members:

-----

TutorialWizard Class
--------------------

.. doxygenclass:: TutorialWizard
   :members:
   :protected-members:

-----

Tutorial Collections
--------------------

The ``TutorialCollection`` struct (``src/tutorials.h``) is the single source of
truth for the tutorial collections offered in the *Tutorials* menu.  Each entry
describes one independently hosted collection (its files repository, web pages,
per-tutorial titles and blurbs, and how much of it is released).  The menu and
the ``TutorialWizard`` consume the table through the
``tutorialCollections()`` and ``tutorialCollection()`` accessors.  See
:ref:`add_tutorial` for how to add or update a tutorial or a whole collection.

.. doxygenstruct:: TutorialCollection
   :members:

-----

.. doxygenfunction:: tutorialCollections

.. doxygenfunction:: tutorialCollection

-----

Editor Components
=================

CodeEditor Class
----------------

.. doxygenclass:: CodeEditor
   :members:
   :protected-members:

-----

LineNumberArea Class
--------------------

.. doxygenclass:: LineNumberArea
   :members:
   :protected-members:

-----

Highlighter Class
-----------------

.. doxygenclass:: Highlighter
   :members:
   :protected-members:

-----

LAMMPS Interface
================

LammpsWrapper Class
-------------------

.. doxygenclass:: LammpsWrapper
   :members:
   :protected-members:

-----

LammpsRunner Class
------------------

.. doxygenclass:: LammpsRunner
   :members:
   :protected-members:

-----

Visualization Components
========================

ChartWindow Class
-----------------

.. doxygenclass:: ChartWindow
   :members:
   :protected-members:

-----

ChartViewer Class
-----------------

.. doxygenclass:: ChartViewer
   :members:
   :protected-members:

-----

ChartBackend Class
------------------

.. doxygenclass:: ChartBackend
   :members:
   :protected-members:

-----

QtGraphsBackend Class
---------------------

.. doxygenclass:: QtGraphsBackend
   :members:
   :protected-members:

-----

QtChartsBackend Class
---------------------

.. doxygenclass:: QtChartsBackend
   :members:
   :protected-members:

-----

ImageViewer Class
-----------------

.. doxygenclass:: ImageViewer
   :members:
   :protected-members:

-----

Dump Image Command Builder
--------------------------

The ``DumpImageParams`` struct and the ``buildDumpImageCommand()`` free
function (``src/dumpimage.h``) form a GUI-free, unit-testable core that
assembles the LAMMPS ``write_dump ... image ...`` command from a snapshot of
the viewer state.  ``ImageViewer`` populates the struct (resolving all LAMMPS
queries up front) and then calls the pure builder.

.. doxygenstruct:: DumpImageParams
   :members:

-----

.. doxygenfunction:: buildDumpImageCommand

-----

Color Maps
----------

The dump-image color maps are defined once, as a table of ``ColorMapDef``
entries in ``src/colormaps.cpp``.  Both the command builder
(``appendColorMapArgs()`` in ``src/dumpimage.cpp``) and the settings-dialog
preview swatches (``addColorMapItems()`` in ``src/imageviewersettings.cpp``)
consume this single source of truth, so they cannot drift apart.  See
:ref:`add_colormap` for how to add or modify a map.

.. doxygenfile:: colormaps.h

-----

SlideShow Class
---------------

.. doxygenclass:: SlideShow
   :members:
   :protected-members:

-----

Dialog Components
=================

FindAndReplace Class
--------------------

.. doxygenclass:: FindAndReplace
   :members:
   :protected-members:

-----

SetVariables Class
------------------

.. doxygenclass:: SetVariables
   :members:
   :protected-members:

-----

PlotDataDialog Class
--------------------

.. doxygenclass:: PlotDataDialog
   :members:
   :protected-members:

-----

Preferences Class
-----------------

.. doxygenclass:: Preferences
   :members:
   :protected-members:

-----

GeneralTab Class
----------------

.. doxygenclass:: GeneralTab
   :members:
   :protected-members:

-----

AcceleratorTab Class
--------------------

.. doxygenclass:: AcceleratorTab
   :members:
   :protected-members:

-----

SnapshotTab Class
-----------------

.. doxygenclass:: SnapshotTab
   :members:
   :protected-members:

-----

EditorTab Class
---------------

.. doxygenclass:: EditorTab
   :members:
   :protected-members:

-----

ChartsTab Class
---------------

.. doxygenclass:: ChartsTab
   :members:
   :protected-members:

-----

AboutDialog Class
-----------------

.. doxygenclass:: AboutDialog
   :members:
   :protected-members:

-----

Utility Components
==================

URLDownloader Class
-------------------

.. doxygenclass:: URLDownloader
   :members:
   :protected-members:

-----

FileViewer Class
----------------

.. doxygenclass:: FileViewer
   :members:
   :protected-members:

-----

LogWindow Class
---------------

.. doxygenclass:: LogWindow
   :members:
   :protected-members:

-----

FlagWarnings Class
------------------

.. doxygenclass:: FlagWarnings
   :members:
   :protected-members:

-----

ImageInfo Class
---------------

.. doxygenclass:: ImageInfo
   :members:
   :protected-members:

-----

RegionInfo Class
----------------

.. doxygenclass:: RegionInfo
   :members:
   :protected-members:

-----

StdCapture Class
----------------

.. doxygenclass:: StdCapture
   :members:
   :protected-members:

-----

Qt Helper Widgets
-----------------

.. doxygenclass:: QHline
   :members:
   :protected-members:

-----

.. doxygenclass:: QColorCompleter
   :members:
   :protected-members:

-----

.. doxygenclass:: QColorValidator
   :members:
   :protected-members:

-----

.. doxygenclass:: RangeSlider
   :members:
   :protected-members:

-----

.. doxygenclass:: VerticalLabel
   :members:
   :protected-members:

-----

StdoutSilencer Class
--------------------

.. doxygenclass:: StdoutSilencer
   :members:
   :protected-members:

-----

PlotData and File Parsers
-------------------------

Column-oriented numeric data model (``src/plotdata.h``) and the parsers for
external data files (whitespace/``.dat``, CSV, LAMMPS YAML, and JSON) used to
plot data from files.

.. doxygenfile:: plotdata.h

-----

Least-Squares Toolkit
---------------------

Self-contained (Qt-free) dense linear-algebra and least-squares routines
(``src/leastsquares.h``) used by the chart smoothing and reusable for
polynomial and equation-of-state fits.

.. doxygenfile:: leastsquares.h

-----

Post-Processing Analyses
------------------------

Self-contained (Qt-free) post-processing analyses (``src/analysis.h``) used
by the chart post-processing dialog.

.. doxygenfile:: analysis.h

-----

Curve Fitting
-------------

Linear-least-squares curve fits (``src/fitting.h``) -- polynomial and
4-parameter Birch-Murnaghan equation of state -- built on the leastsquares
toolkit and used by the chart post-processing dialog.

.. doxygenfile:: fitting.h

-----

Nonlinear Least Squares
-----------------------

Compact, self-contained (Qt-free) Levenberg-Marquardt solver
(``src/levmar.h``) for nonlinear least-squares fits. The model is supplied as a
residual/Jacobian callback, so the core is independent of how the model is
expressed; it is driven by the custom-fit code with LeptonMini expressions and
their symbolic derivatives, and solves the damped normal equations with the
leastsquares LU solver.

.. doxygenfile:: levmar.h

-----

Custom-Function Evaluation and Fitting
--------------------------------------

Evaluation and nonlinear fitting of user-supplied mathematical expressions
(``src/customfunc.h``) via the vendored LeptonMini parser, used for
custom-function plotting and custom curve fits in the chart post-processing
dialog. The fit builds its Jacobian from LeptonMini's analytic derivatives and
minimizes with the Levenberg-Marquardt solver.

.. doxygenfile:: customfunc.h

-----

.. _helper_functions:

Helper Functions
----------------

.. doxygenfile:: helpers.h
   :sections: func
