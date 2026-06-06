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

.. _helper_functions:

Helper Functions
----------------

.. doxygenfile:: helpers.h
   :sections: func
