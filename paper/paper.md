---
title: 'LAMMPS-GUI: A Cross-Platform Graphical Tool to Learn and Explore Molecular Dynamics with LAMMPS'
tags:
  - C++
  - Qt
  - molecular dynamics
  - LAMMPS
  - simulation
  - scientific visualization
  - research software
  - education
authors:
  - name: Axel Kohlmeyer
    orcid: 0000-0001-6204-6475
    corresponding: true
    affiliation: 1
affiliations:
  - name: Institute for Computational Molecular Science, Temple University, Philadelphia, PA, USA
    index: 1
date: 22 June 2026
bibliography: paper.bib
---

# Summary

LAMMPS is a widely used, massively parallel classical molecular dynamics
(MD) engine for particle-based simulations at the atomic, mesoscopic,
and continuum scales [@thompson2022lammps]. It is implemented in C++ as
a no-frills, console-mode application, which makes it highly portable
and efficient, but also means that
learning to use it requires mastering separate -- and
often platform-specific -- tools for editing inputs, extracting and
plotting thermodynamic data, and visualizing atomic configurations
*before* one can begin to learn MD itself.

LAMMPS-GUI is a cross-platform graphical application, written in C++17
using version 6 of the Qt framework [@qt_home], that combines these
tasks into a single program: an input-script editor with syntax
highlighting, auto-completion, and context-sensitive documentation
lookup; live execution of LAMMPS with real-time monitoring of its screen
output; interactive line charts of thermodynamic data; an interactive
snapshot image creator; and a slide-show viewer for image sequences.
Rather than launching LAMMPS as an external process, LAMMPS-GUI embeds
it by loading a LAMMPS shared library and calling it through the LAMMPS
C-language library interface [@frantzdale2010library].  The simulation
runs in a concurrent worker thread, so that the interface stays
responsive and the running simulation can be monitored, plotted,
visualized, and cleanly stopped.  By preserving the
traditional "edit input, run LAMMPS, observe, and analyze" workflow,
LAMMPS-GUI lets users move freely between the graphical tool and the
command-line LAMMPS executable, which is essential once a workflow grows
beyond what a personal computer can run.  In addition to the
source code, pre-compiled packages are provided for Windows, macOS, and
Linux, so that most users can run LAMMPS-GUI and obtain an identical
experience instantly, without having to compile anything.

# Statement of need

The development of LAMMPS-GUI was driven by the experience of teaching
LAMMPS at tutorials and workshops, where a large fraction of the
available time was spent not on teaching molecular dynamics but on
installing and operating the surrounding tool chain -- a text editor, a
plotting program, and a molecular visualization package -- most of which
differ between Windows, macOS, and Linux.  Several attempted workarounds
either were too complex and unsustainable or broke down when Apple moved
to the ARM CPU architecture.  As a single, self-contained,
cross-platform tool that mirrors the console workflow, LAMMPS-GUI
removes this barrier and lets instructors teach one interface on all
platforms and focus on teaching LAMMPS and molecular dynamics.

![A LAMMPS-GUI session with a simulation in flight. The editor shows
syntax highlighting, line numbers, and a marker on the currently
processed input line; the status bar shows CPU utilization and run
progress; the Image Viewer shows the starting geometry; the Output window
the screen output; and the Charts window a live plot of a thermodynamic
data column.\label{fig:editor}](images/lammps-gui-deform.png){ width=70% }

Several other approaches to make running LAMMPS simulations more
accessible exist, but they target different needs.  Commercial
molecular-modeling packages typically provide structure editors that can
export LAMMPS inputs and re-import results; Atomify compiles a modified
version of LAMMPS to WebAssembly and runs simulations and visualization
inside a web browser [@atomify_home]; and dedicated visualization
programs such as OVITO [@ovito_home] and VMD [@vmd_home] render
trajectories with a breadth and quality that LAMMPS-GUI does not attempt
to match.  None of these, however, integrates input editing, live
simulation, output monitoring, plotting, and visualization into one
application that follows the same edit-run-observe loop as the
standalone executable. LAMMPS-GUI fills exactly this gap. It was
developed to support importing tutorial materials, for example from the
official set of LAMMPS tutorials [@gravelle2025lammps], and has been
used in that role at LAMMPS workshops.  While its primary audience is
beginners, several features -- rapid prototyping of input decks,
debugging of failing inputs by visualizing selected components, and the
interactive construction of reproducible `dump image` visualization
commands -- also make it useful to experienced researchers.

# Functionality

LAMMPS-GUI follows an object-oriented design in which a central window
coordinates largely self-contained components, all access to LAMMPS being
funneled through a single adapter class. Key capabilities include:

- **Editing.** A LAMMPS-aware editor with syntax highlighting,
  per-command-category auto-completion, in-place documentation lookup,
  block comment/uncomment, and command reformatting.
- **Running.** Execution of the editor buffer through the library
  interface with no intermediate file I/O, with a progress bar and the
  estimated completion of each run, support for the GPU, INTEL, KOKKOS, and
  OPENMP accelerator packages, and the ability to recover from simulation
  errors -- which the modernized library interface reports as catchable
  exceptions -- without crashing the application.
- **Output and charts.** An output window that highlights warnings and
  errors and makes embedded documentation URLs clickable, and a charts
  window that plots thermodynamic data read directly from the running
  simulation (rather than scraped from text) or imported from external data
  files, with Savitzky-Golay smoothing
  [@savitzkygolay1964] and post-processing such as autocorrelation,
  polynomial, Birch-Murnaghan equation-of-state, and nonlinear curve fits.
- **Visualization.** A snapshot image viewer that builds high-quality
  renderings using the LAMMPS `dump image` command, with interactive
  controls and the ability to copy the exact generated command back into
  the input script for reproducible figures, plus a slide-show viewer for
  sequences of images that can be exported as movies.
- **Tutorials and convenience.** A guided tutorial wizard that downloads
  and opens lesson inputs from several tutorial collections, a tabbed
  preferences dialog, hand-off to OVITO
  and VMD, and command-line flags that expose the charting and image
  viewers as standalone utilities.

A recurring theme is the co-evolution of LAMMPS-GUI and LAMMPS: needs
in the graphical front end motivated improvements to
the simulation engine and its library interface -- catchable exceptions,
a locked cache of live thermodynamic data, and greatly expanded snapshot
rendering that was collected into a dedicated GRAPHICS
package [@kohlmeyer2025lammps]. In its default *plugin mode*, LAMMPS-GUI
has no link-time dependency on LAMMPS and loads the shared library at
run time, so a single pre-compiled binary can be paired with -- or
download -- different LAMMPS builds. The project is documented online
[@lammpsgui_home] and is maintained with an automated test suite and
continuous-integration builds on all supported platforms.

# Acknowledgements

Financial support by Sandia National Laboratories under PO 2149742 and
PO 2407526 is gratefully acknowledged. The author thanks the LAMMPS
developers and the authors of the LAMMPS tutorials for their feedback and
for motivating many of the features described here.

# References
