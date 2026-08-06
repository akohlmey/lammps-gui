************************
Monitoring LAMMPS output
************************

.. admonition:: LAMMPS-GUI Viewing Modes
   :class: note

   Since version 3.1 LAMMPS-GUI supports two viewing
   modes: 1. individual windows mode, where each output is displayed in
   a separate window and 2. combined window mode, where there is only
   one main window that may be split once vertically and the upper part
   once more horizontally and then the upper left section is the editor
   window and the upper right and bottom sections contain one or more tabs
   with the same content as the individual windows.  This viewing mode
   can be changes in the :doc:`Preferences dialog <dialogs>`.

   Any references to a "window" throughout the remainder of the
   documentation thus refers to either the corresponding individual
   window or the corresponding tab in the upper right or bottom part of
   the combined window.

   The behavior for both modes is largely the same with two exceptions:

   1. There can be only one menu bar, so the displayed menu bar is that
      of the section / tab that has currently the focus. If access to
      the menu bar of a specific window is needed, e.g. to open a new
      LAMMPS input file, it may be needed to first click into the
      corresponding area to switch focus.
   2. *Image Viewer* and *Slide Show Viewer* lose the auto-resize option
      to show the image without scroll bars if the screen size supports it.

.. _logfile:

Output Window
^^^^^^^^^^^^^

.. index:: output window
.. index:: log window
.. index:: screen output

.. image:: JPG/lammps-gui-log.png
   :align: right
   :scale: 50%

By default, when starting a run, an *Output* window opens that displays
the screen output of the running LAMMPS calculation, as shown below.
This text would normally be seen in the command-line window.

LAMMPS-GUI captures the screen output from LAMMPS as it is generated and
updates the *Output* window regularly during a run.  If there are any
warnings or errors in the LAMMPS output, they are highlighted by using
bold text colored in red.  There is a small panel at the bottom center
of the *Output* window showing how many warnings and errors were
detected and how many lines the entire output has.  By clicking on the
button on the right with the warning symbol or by using the keyboard
shortcut `Ctrl-N` (`Command-N` on macOS), you can jump to the next
line with a warning or error.  If there is a URL pointing to additional
explanations in the online manual, that URL will be highlighted and
double-clicking on it shall open the corresponding manual page in
the web browser.  The option is also available from the context menu.

The *Output* window is reused for every run: starting a run replaces
its content.  The runs are counted and the run number for the current
run is displayed in the window title (with the *Combined Main Window*
layout, in the title of the main window).  Whether the *Output* window
opens by default can be changed in the preferences dialog, and it can be
shown or hidden at any time from the *View* menu.

The text in the *Output* window is read-only and cannot be modified, but
keyboard shortcuts to select and copy all or parts of the text can be
used to transfer text to another program. Also, the keyboard shortcut
`Ctrl-S` (`Command-S` on macOS) is available to save the *Output* buffer to a
file.  The window has a *File* menu carrying these functions, followed
by the application-wide *Run*, *View*, *Tutorials*, and *About* menus
that every output window shows, so a run can be started or stopped from
here as well.  The "Select All" and "Copy" functions, as well as a "Save
Log to File" option are also available from a context menu by clicking
with the right mouse button into the *Output* window text area.

.. image:: JPG/lammps-gui-yaml.png
   :align: center
   :scale: 50%

Should the *Output* window contain embedded YAML format text (see above
for a demonstration), for example from using `thermo_style yaml
<https://docs.lammps.org/thermo_style.html>`_ or `thermo_modify line
yaml <https://docs.lammps.org/thermo_modify.html>`_, the keyboard
shortcut `Ctrl-Y` (`Command-Y` on macOS) is available to save only the
YAML parts to a file.  This option is also available from a context menu
by clicking with the right mouse button into the *Output* window text
area.

.. _charts:

Charts Window
^^^^^^^^^^^^^

.. index:: charts window
.. index:: plotting
.. index:: thermodynamic output
.. index:: data visualization

.. image:: JPG/lammps-gui-chart.png
   :align: right
   :scale: 33%

By default, when starting a run, a *Charts* window opens that displays a
plot of thermodynamic output of the LAMMPS calculation as shown below.

.. index:: smoothing
.. index:: Savitzky-Golay filter

The "Data:" drop-down menu on the top right allows selection of
different properties that are computed and written as thermodynamic
output to the output window.  Only one property can be shown at a time.
The plots are updated regularly with new data as the run progresses, so
they can be used to visually monitor the evolution of available
properties.  The update interval can be set in the *Preferences* dialog.
By default, the raw data for the selected property is plotted as a blue
graph.  From the "Plot:" drop-down menu on the second row (immediately to the right of
the *Chart Style...* and *Postprocess...* quick-access buttons),
you can select whether to plot only raw data graph, only a smoothed data
graph, or both graphs on top of each other.  The smoothing process uses
a `Savitzky-Golay convolution filter
<https://en.wikipedia.org/wiki/Savitzky%E2%80%93Golay_filter>`_.  The
convolution window width (left) and order (right) parameters can be set
in the boxes next to the drop-down menu.  Default settings are 10 and 4
which means that the smoothing window includes 10 points each to the
left and the right of the current data point for a total of 21 points
and a fourth order polynomial is fitted to the data in the window.

The "Title:" and "Y:" input boxes let you edit the text shown as the
plot title and the y-axis label, respectively.  The text entered in the
"Title:" box is applied to *all* charts, while the "Y:" text changes
only the y-axis label of the currently *selected* plot.  In standalone
plot mode (when plotting an external data file), an additional "X-Axis:"
input box is shown to the right of "Y:" and sets the x-axis label for all
charts.

The window title shows the current run number that this chart window
corresponds to.  Same as for the *Output* window, the chart window is
reused and its charts are replaced on each new run.

.. index:: CSV export
.. index:: YAML export
.. index:: data export

From the *File* menu on the top left, it is possible to save an image
of the currently displayed plot or export the data in either plain text
columns (for use by plotting tools like `gnuplot
<http://www.gnuplot.info/>`_ or `grace
<https://plasma-gate.weizmann.ac.il/Grace/>`_), as CSV data which can be
imported for further processing with Microsoft Excel, `LibreOffice Calc
<https://www.libreoffice.org/>`_, or with Python via `pandas
<https://pandas.pydata.org/>`_, or as YAML which can be imported into
Python with `PyYAML <https://pyyaml.org/>`_ or pandas.

Thermo output data from successive run commands in the input script is
combined into a single data set unless the format, number, or names of
output columns are changed with a `thermo_style
<https://docs.lammps.org/thermo_style.html>`_ or a `thermo_modify
<https://docs.lammps.org/thermo_modify.html>`_ command, or the current
time step is reset with `reset_timestep
<https://docs.lammps.org/reset_timestep.html>`_, or if a `clear
<https://docs.lammps.org/clear.html>`_ command is issued.  This is where
the YAML export from the *Charts* window differs from that of the
*Output* window: here you get the compounded data set starting with the
last change of output fields or timestep setting, while the export from
the log will contain *all* YAML output but *segmented* into individual
runs.

.. index:: chart style
.. index:: legend

Adjust chart style
------------------

.. image:: JPG/lammps-gui-eos-plot.png
   :align: right
   :scale: 33%

The *Chart Style...* entry in the chart window's *File* menu, or the
chart-style quick-access button at the far left of the second toolbar
row, opens a dialog to change how the data is drawn.  The *Raw data* and
*Processed data* series each have independent settings for the display
style (*Lines*, *Points*, or *Lines and Points*), the color, the line
width, and the point size.  This makes it possible, for example, to show
the raw data as faint points and the smoothed curve as a bold line.  The
*Legend* placement selector in the same dialog adds an in-plot legend
that lists the visible named series; it can be turned *Off* or anchored
to any of the four plot corners (*Top left*, *Top right*, *Bottom
right*, or *Bottom left*).  The selected placement is remembered across
sessions.

.. index:: reference lines

Reference lines
---------------

.. image:: JPG/lammps-gui-plot-lines.png
   :align: right
   :scale: 33%

The *Reference Lines...* entry in the chart window's *File* menu opens a
dialog for adding straight annotation lines that are drawn on *every*
chart in the window.  Each line is either *Vertical* (at a chosen x
value) or *Horizontal* (at a chosen y value) and can carry a text label
and an individually chosen color.  The label position along the line
(*Top*, *Center*, or *Bottom* for vertical lines; *Left*, *Center*, or
*Right* for horizontal lines) is selected per line, while the label font
size, the gap between the label and the line, and whether the labels are
drawn in an opaque box apply to the whole window.  Reference lines are
useful, for example, to mark a target temperature, a transition point,
or a fitted value.

.. index:: post-processing
.. index:: curve fitting
.. index:: autocorrelation
.. index:: polynomial fit
.. index:: Birch-Murnaghan EOS
.. index:: equation of state
.. index:: custom function
.. index:: custom fit

Post-process data
-----------------

The *Postprocess...* entry in the chart window's *File* menu, or the
quick-access button immediately to the right of the *Chart Style...*
button, runs an analysis on the data of the currently selected property.
The following analyses are available:

- *Autocorrelation* computes the normalized autocorrelation function of
  the selected data up to a chosen maximum lag and shows it in a new
  chart window (the abscissa becomes the lag).  This is useful, for
  example, for estimating correlation times of fluctuating quantities.
- *Polynomial fit* performs a least-squares fit of a polynomial of a
  chosen degree, overlays the fitted curve on the chart, and reports the
  coefficients and the root-mean-square residual.
- *Birch-Murnaghan EOS fit* fits a 4-parameter `Birch-Murnaghan equation
  of state
  <https://en.wikipedia.org/wiki/Birch%E2%80%93Murnaghan_equation_of_state>`_
  to energy-versus-volume data (x = volume per unit cell, y = energy).  A
  confirmation dialog lets you verify the x and y column assignments and
  enter the number of atoms in the conventional unit cell N (default 1;
  e.g. 4 for FCC, 2 for BCC or HCP; use N = 1 when the x data is already
  the conventional cell volume rather than volume per atom).  The fit reports the equilibrium
  volume V\ :sub:`0`, the derived lattice constant
  a\ :sub:`0` = (N V\ :sub:`0`)\ :sup:`1/3`, the equilibrium energy
  E\ :sub:`0`, the bulk modulus B\ :sub:`0`, and its pressure
  derivative B\ :sub:`0`'.
- *Custom function* evaluates a user-supplied mathematical expression
  ``f(x)`` over the x range of the data and overlays it as a curve.  The
  expression uses the variable ``x`` for the abscissa and supports the
  usual arithmetic operators and functions (for example
  ``2*x^2 + 3*sin(x)``).
- *Custom fit* performs a nonlinear least-squares fit of a user-supplied
  expression to the data.  In addition to the expression, you provide the
  fit parameters and their initial guesses as ``name=value`` pairs (for
  example ``a=1, b=0.5``) and, optionally, a label for the fitted curve.
  The fit uses a Levenberg-Marquardt algorithm with analytic derivatives
  of the expression; on success the fitted curve is overlaid and the
  fitted parameters, the root-mean-square residual, and the number of
  iterations are reported.

The expressions for *Custom function* and *Custom fit* are parsed and
evaluated with a bundled subset of the Lepton expression parser, the same
library used by the LAMMPS `Lepton-based styles
<https://docs.lammps.org/pair_lepton.html>`_, so the supported syntax
matches.

When any fit or custom-function overlay is active, the "Plot:" drop-down
treats the overlay as the "smoothed" series: selecting "Smoothed" or
"Both" shows the overlay curve, while selecting "Raw" hides it.  This
applies uniformly to all analysis types (polynomial, EOS, custom function,
and custom fit).

.. figure:: JPG/lammps-gui-post-function.png
   :align: center
   :width: 50%

   The *Postprocess* dialog with a *Custom function* expression entered.

.. figure:: JPG/lammps-gui-eos-fit.png
   :align: center
   :width: 55%

   An example post-processing result: a Birch-Murnaghan equation-of-state
   fit overlaid on energy-versus-volume data.

.. index:: plotting external data
.. index:: plot data file

Plot imported data
------------------

The same *Charts* window is also used to plot data from an external file
opened with *File* -> *Plot Data File...* (`Ctrl-Shift-P`, see
:ref:`the File menu <files>`); in that standalone mode there is no
associated simulation, so the *Units* and *Norm* controls are hidden.
The column-picker dialog shown before the chart opens lets you select
which column provides the x axis and which columns to plot, and also
allows renaming columns.  An "X-Axis:" label field in the first toolbar
row (to the right of "Title:" and "Y:") lets you edit the x-axis label
after the chart opens.  All the styling, export, and post-processing
features described above work the same way.  The same column-picker and
standalone chart window are also launched when LAMMPS-GUI is invoked from
the command line with the ``-c``/``--chart`` flag (see
:ref:`command-line options <command-line-options>`).

.. figure:: JPG/lammps-gui-import-data.png
   :align: center
   :width: 45%

   The column-picker dialog shown when opening an external data file with
   *Plot Data File...*.

The *Preferences* dialog has a *Charts* tab, where you can configure
multiple chart-related settings, like the default title, colors for the
graphs, default choice of the raw / smooth graph selection, whether the
grid for the major and minor ticks is drawn, and the default chart graph
size.

.. admonition:: Slowdown of Simulations from Charts Data Processing
   :class: warning

   Using frequent thermo output during long simulations can result in a
   significant slowdown of that simulation since it is accumulating many
   data points for each of the thermo properties in the chart window to
   be redrawn with every update.  The updates are consuming additional
   CPU time when smoothing enabled.  This slowdown can be confirmed when
   an increasing percentage of the total run time is spent in the
   "Output" or "Other" sections of the `MPI task timing breakdown
   <https://docs.lammps.org/Run_output.html>`_.  It is thus recommended
   to use a large enough value as argument `N` for the `thermo command
   <https://docs.lammps.org/thermo.html>`_ and to select plotting only
   the "Raw" data in the *Charts* window during such simulations.  It is
   always possible to switch between the different display styles for
   charts during the simulation and after it has finished.

Variable Info
^^^^^^^^^^^^^

.. index:: variable info window
.. index:: variables
.. index:: input script variables

.. image:: JPG/lammps-gui-variable-info.png
   :align: right
   :scale: 50%

During a run, it may be of interest to monitor the value of input script
variables, for example to monitor the progress of loops.  This can be
done by enabling the "Variables Window" in the *View* menu or by using
the `Ctrl-Shift-W` keyboard shortcut.  This shows info similar to the
`info variables <https://docs.lammps.org/info.html>`_ command in a
separate window as shown below.

Like for the *Output* and *Charts* windows, its content is continuously
updated during a run.  It will show "(none)" if there are no variables
defined.  Note that it is also possible to *set* `index style variables
<https://docs.lammps.org/variable.html>`_, that would normally be set
via command-line flags, via the "Set Variables..." dialog from the *Run*
menu.  LAMMPS-GUI automatically defines the variable "gui_run" to the
current value of the run counter.  That way it is possible to
automatically record a separate log for each run attempt by using the
command

.. code-block:: LAMMPS

   log logfile-${gui_run}.txt

at the beginning of an input file. That would record logs to files
``logfile-1.txt``, ``logfile-2.txt``, and so on for successive runs.

.. _commandwindow:

Command window
--------------

.. index:: command window
.. index:: shell

The *Command window* is opened from the *Run* menu with *Open Command
Window* or the `Ctrl-Shift-X` keyboard shortcut.  It shows a shell
prompt with a scrollback, for the ordinary work that surrounds a
simulation: post-processing a dump file with a Python script, looking at
what a run just wrote, calling a plotting tool, all without leaving
LAMMPS-GUI.

Lines typed at the prompt are handed to a single shell process that is
kept running between commands, so ``cd``, ``pushd``/``popd``,
environment variables, and the rest of the shell state behave as they
would in a terminal.  The shell is started as an interactive one, so it
reads the usual start-up file and the aliases and shell functions
defined there are available -- with one exception, noted below.  The
directory shown in front of the prompt follows
the shell, however it was changed.  A directory inside the home
directory is shown with a leading ``~``, the way a shell prompt writes
it, and hovering over it shows the path in full.  The window starts in the directory
of the current input file, which is where a run leaves its output, and
stays independent afterwards: opening a different input file does not
move a shell that may be busy.  *File* > *Change to Input Directory*
moves it to the current input file's directory on request.  The
up and down arrow keys walk through previously entered commands, which
are remembered between sessions.

The *Tab* key completes the word it is in.  On a word that starts a
command -- the first of the line, and equally the first after a ``;``,
``|``, ``&&`` or ``||`` -- it offers whole lines entered before, sorted
and without repeats, and then command names from the search path, so a
few characters of a long command line bring the whole of it back, which
is what a reverse history search is for.  On any other word it offers
the names in the directory the shell is in, with a ``/`` after the ones
that are directories.  Only
that directory is offered, and only plain names: there is no completion
across a path, and none of what the shell itself would complete, since
the shell never sees the line until it is entered.  The list is taken
again whenever the shell changes directory, and each time completion
starts on a new argument, so a file a command has just written is
offered without reopening the panel.

When a single name matches, *Tab* completes it outright and no list
appears.  When several do, it offers them in a list; pressing *Tab*
again walks through them and *Shift-Tab* walks back, with each entry
appearing in the input line as it is reached.  *Enter* takes the entry
that is highlighted and goes no further, so the completed line is run by
the next *Enter*; with nothing highlighted there is nothing to take and
*Enter* runs the line as it stands.

The window adds three commands of its own, which hand files to
LAMMPS-GUI rather than printing them, so that a whole project can be
worked on without leaving the prompt:

.. list-table::
   :header-rows: 1
   :widths: 12 88

   * - Command
     - What it does with the files named after it
   * - ``open``
     - Image and movie files go to a :ref:`slide show <slideshow>`
       viewer, all of the ones named in the same command together in
       one; any other file goes to a read-only text viewer.
   * - ``edit``
     - Loads the file into the editor, as *File* > *Open Input File*
       does -- including the offer to save the current buffer first.
       The editor holds one file, so naming several says so and opens
       the first.
   * - ``plot``
     - Reads the file as a data file and asks which columns to draw,
       then opens the plot.  With several files it asks for each in
       turn, and canceling stops the rest.

These are ordinary shell commands, so the shell expands their arguments
before they run and wildcards, quoting, ``~`` and variables work there as
they do anywhere else:

.. code-block:: bash

   open melt-*.png                 # the whole sequence, in one slide show
   edit in.melt                    # into the editor
   plot log.lammps                 # pick columns, then plot
   open $(ls -t *.png | head -1)   # the most recent image

Each is defined only when nothing else on the system claims that name,
which is checked in the shell itself, so an alias or function from the
start-up file counts as well as a program on the search path.  On macOS
``open`` already exists and does much the same job, and GNU plotutils
installs a ``plot``; where that is the case the command that was there
keeps working and the panel adds nothing.  None of them is available with
``cmd.exe``, which has no way to define them.

The shell is the one selected in the *Preferences* dialog (*Command
window shell*, offering the shells installed on the machine).  Until one
is selected there, it is the one named by the ``SHELL`` environment
variable on Unix-like systems (falling back to ``/bin/bash`` and then
``/bin/sh``) and by ``COMSPEC`` on Windows.  Commands run with ``TERM`` set to
``dumb`` and with ``PYTHONUNBUFFERED`` set, so that the output of a
Python script appears as it is produced rather than all at once when it
exits.  On macOS, where an application launched from the Finder inherits
a minimal ``PATH``, the common package manager locations (Homebrew,
MacPorts) are appended to it, for the shell and for command completion
alike.  ``COLUMNS`` and ``LINES`` are set to the size of the panel and
follow it as it is resized, so a program that formats its output to a
width uses the width that is actually there rather than the 80 columns
it would otherwise assume.

*File* > *Command Aliases...* lists aliases that are defined in every
shell this window starts.  Two things make them worth having.  A section
of the start-up file guarded by a test for a terminal never runs here,
as described below, and on several distributions that is where ``ls``
and ``ll`` are defined.  Programs also drop formatting they keep only
for a terminal: ``ls`` lists one entry per line rather than in columns,
because that is what it is required to do when its output is not a
terminal.  The list therefore starts out with

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Alias
     - Expands to
   * - ``ls``
     - ``ls -aCF``
   * - ``ll``
     - ``ls -laCF``

which restores the multi-column, classified listing a terminal would
have produced.  Rows can be added, changed or removed, *Restore
Defaults* puts the two back, and a change applies to the shell that is
already running as well as to later ones.  Aliases are not available
with ``cmd.exe``, which has no equivalent.

A command runs in the foreground and holds the shell until it finishes,
exactly as it would in a terminal.  Append ``&`` to start a program --
a graphical one in particular -- without waiting for it.

While a command is running the prompt says ``running >`` and refuses
input.  A line entered then could not be a command waiting its turn: it
would go down the same pipe and be read by the running program, if it
reads at all, and by the shell only once that program had finished.

There is no ``Ctrl-Z`` followed by ``bg`` to fall back on.  Job control
needs a controlling terminal, and there is none here, so the shell has
no list of jobs for ``bg`` to act on and reports as much when it starts.
To recover from a program started without ``&`` there are two choices.
*File* > *Restart Shell* ends the shell and starts a fresh one in the
same directory while whatever the shell had started keeps running, so a
graphical application stays open and the prompt comes back -- the
outcome ``Ctrl-Z`` and ``bg`` would have produced.  The kill button
below ends the program itself.

The button with the skull at the right of the prompt, and *File* > *Kill
Command*, end the running command outright.  They ask it to quit and
insist a moment later if it has not, so the shell becomes free again and
the prompt returns; the button is only active while something is
running.  A command that started programs of its own leaves those
behind, since without job control there is no group of processes to end
in one go.

*File* > *Interrupt Command* is the gentler option and sends an
interrupt instead.  It is a best effort: without job control the shell
starts its children with the interrupt signal ignored, so a program that
does not install a handler of its own will sit through it.

It is also the way out of an unfinished multi-line construct.  A line
such as ``if true; then`` with nothing after it leaves the shell waiting
for the rest of it, and what the shell reads next as part of that
construct is the marker this window ends every command with -- so the
command never appears to finish, and since the prompt refuses input
while a command runs, the closing ``fi`` cannot be typed either.
Interrupting puts the shell back at a prompt and asks it for a fresh
marker, which brings the prompt back with the session intact.  Several
commands on one line separated by ``;`` are otherwise nothing special:
they run as they would in a terminal, and the exit status reported is
the one of the last of them.

On Windows, neither killing nor interrupting a command is supported;
*File* > *Restart Shell* is the way to get the prompt back there.

.. admonition:: Output may only appear when a program exits

   A program that writes to a terminal usually flushes each line, but
   when its output is a pipe -- as it is here -- the C runtime collects
   it into blocks instead and writes them out when the buffer fills or
   the program exits.  Nothing is lost, but a long-running program can
   appear silent until it is done.  Python is handled already, through
   ``PYTHONUNBUFFERED``; for other programs, ``stdbuf -oL`` in front of
   the command asks for line buffering where that tool is available.

.. admonition:: Start-up file sections that require a terminal are skipped

   The shell reads the start-up file, but a section of it guarded by a
   test for a terminal, such as ``[ ! -t 0 ] && return``, stops there,
   because the shell is reading a pipe and not a terminal.  Most aliases
   and functions are unaffected; the ones defined in such a section are
   missing, which is confusing precisely because everything around them
   is there.  On Fedora and related distributions this is where ``ls``,
   ``ll`` and ``l.`` are defined, so those three are absent while the
   rest of the aliases are present.  *File* > *Command Aliases...* is
   where to put them back, and it starts out holding exactly those.

.. admonition:: This is not a terminal emulator

   There is no pseudo terminal behind the prompt, only a pipe.  Programs
   that need a real terminal -- editors, pagers, anything using curses,
   anything asking for a password -- will either report that the terminal
   is insufficient or misbehave.  A program cannot be fed from the prompt
   either: input is refused while a command is running, precisely so that
   a line meant for the shell is not swallowed by whatever it started.
