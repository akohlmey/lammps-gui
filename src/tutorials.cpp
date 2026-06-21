// -*- c++ -*- /////////////////////////////////////////////////////////////////////////
// LAMMPS-GUI - A Graphical Tool to Learn and Explore the LAMMPS MD Simulation Software
//
// Copyright (c) 2023, 2024, 2025, 2026  Axel Kohlmeyer
//
// Documentation: https://lammps-gui.lammps.org/
// Contact: akohlmey@gmail.com
//
// This software is distributed under the GNU General Public License version 2 or later.
////////////////////////////////////////////////////////////////////////////////////////

#include "tutorials.h"

// Single source of truth for the tutorial collections offered in the Tutorials
// menu.  Each collection is downloaded from its own (public) files repository
// and laid out as files/tutorial<N>/ with a .manifest and optional solution/
// subfolder.  Add a collection by appending an entry here.

namespace {

// --- Collection 1: the molecular soft-matter tutorials (published) -----------
TutorialCollection molecular()
{
    TutorialCollection c;
    c.key          = QStringLiteral("softmatter");
    c.name         = QStringLiteral("Soft Matter");
    c.dirPrefix    = QStringLiteral("tutorial");
    c.author       = QStringLiteral("Simon Gravelle, Cecilia Alvares, Jake Gissinger, "
                                          "and Axel Kohlmeyer");
    c.filesUrl     = QStringLiteral("https://raw.githubusercontent.com/lammpstutorials/"
                                        "lammpstutorials-article/main/files/tutorial%1/%2");
    c.filesRepoUrl = QStringLiteral("https://github.com/lammpstutorials/lammpstutorials-article");
    c.webUrl       = QStringLiteral("https://lammpstutorials.github.io/sphinx/build/html/"
                                          "tutorial%1/%2.html");
    c.siteUrl      = QStringLiteral("https://lammpstutorials.github.io/");
    c.logo         = QStringLiteral(":/icons/tutorial%1-logo.png");
    c.published    = true;

    c.titles = {"Lennard-Jones fluid",      "Breaking a carbon nanotube",
                "Polymer in water",         "Nanosheared electrolyte",
                "Reactive silicon dioxide", "Water adsorption in silica",
                "Free energy calculation",  "Reactive molecular dynamics"};

    c.slugs = {"lennard-jones-fluid",      "breaking-a-carbon-nanotube",
               "polymer-in-water",         "nanosheared-electrolyte",
               "reactive-silicon-dioxide", "water-adsorption-in-silica",
               "free-energy-calculation",  "reactive-molecular-dynamics"};

    c.blurbs = {
        "<p>In tutorial 1 you will learn about LAMMPS input files, their syntax and "
        "structure, how to create and set up models and their interactions, how to run a "
        "minimization and a molecular dynamics trajectory, how to plot thermodynamic data "
        "and how to create visualizations of your system</p>",
        "<p>In tutorial 2 you will learn about setting up a simulation for a molecular "
        "system with bonds.  The target is to simulate a carbon nanotube with a "
        "conventional molecular force field under growing strain and observe the response "
        "to it.  Since bonds are represented by a harmonic potential, they cannot break.  "
        "This is then compared to simulating the same system with a reactive force field "
        "(AIREBO) where bonds may be broken and formed.</p>",
        "<p>In tutorial 3 you will learn setting up a multi-component, a polymer molecule embedded "
        "in liquid water.  The model employs a long-range Coulomb solver and a stretching force is "
        "applied to the polymer. This is used to demonstrate how to use the type label facility in "
        "LAMMPS to make components more generic.</p>",
        "<p>In tutorial 4 an electrolyte is simulated while confined between two walls and "
        "thus illustrating the specifics of simulating systems with fluid-solid "
        "interfaces.  The water model is more complex than in tutorial 3 and also a "
        "non-equilibrium MD simulation is performed by imposing shearing forces on the "
        "electrolyte through moving the walls.</p>",
        "<p>Tutorial 5 demonstrates the use of the ReaxFF reactive force field which "
        "includes a dynamic bond topology based on determining the bond order.  ReaxFF "
        "includes charge equilibration (QEq) and thus the atoms can change their partial "
        "charges according to the local environment.</p>",
        "<p>In tutorial 6 an MD simulation is combined with Monte Carlo (MC) steps to implement "
        "a Grand Canonical ensemble.  This represents an open system where atoms or "
        "molecules may be exchanged with a reservoir.</p>",
        "<p>In tutorial 7 you will determine the height of a free energy barrier through "
        "using umbrella sampling.  This is one of many advanced methods using specific "
        "reaction coordinates or so-called collective variables to map out relevant parts "
        "of free energy landscapes, where unbiased MD or MC simulation may take too "
        "long.</p>",
        "<p>In tutorial 8 a CNT embedded in a Nylon-6,6 polymer melt is simulated.  The "
        "REACTER protocol is used to model the polymerization of Nylon without having to "
        "employ far more computationally demanding models like ReaxFF.  Also, the "
        "formation of water molecules is tracked over time.</p>"};

    c.available = c.blurbs.size(); // fully published: all tutorials are launchable
    return c;
}

// --- Collection 2: the materials-science tutorials (rolling out) --------------
//
// Input/solution files for all 14 tutorials are hosted publicly in the
// matsci-tutorials-inputs repository, so the collection downloads like any other.
// TODO: add per-tutorial logos, webUrl/slugs and set published = true once all 14 are released.

TutorialCollection matsci()
{
    TutorialCollection c;
    c.key          = QStringLiteral("matsci");
    c.name         = QStringLiteral("Materials Science");
    c.dirPrefix    = QStringLiteral("matsci-tutorial");
    c.author       = QStringLiteral("the LAMMPS Materials Science tutorials authors");
    c.filesUrl     = QStringLiteral("https://raw.githubusercontent.com/lammpstutorials/"
                                        "matsci-tutorials-inputs/main/tutorial%1/%2");
    c.filesRepoUrl = QStringLiteral("https://github.com/lammpstutorials/matsci-tutorials-inputs");
    c.webUrl       = QString(); // TODO: per-tutorial web pages once published
    c.siteUrl      = QStringLiteral("https://lammpstutorials.github.io/");
    c.logo         = QStringLiteral(":/icons/tutorial-logo.png");
    c.published    = false;
    c.status       = QStringLiteral("in development");

    c.titles = {"Crystalline metals and the EAM potential",
                "Variables, automation, and the energy-volume curve",
                "Uniaxial deformation of a single crystal",
                "Grain boundaries: construction and fracture",
                "Nanoindentation and nano-stamping",
                "Elastic constants of crystalline silicon",
                "Generalized stacking fault energy",
                "Thermal conductivity by reverse NEMD",
                "Phonon dispersion from MD",
                "Zone-center phonons from the dynamical matrix",
                "Radiation damage cascade",
                "Melting point determination",
                "Diffusion in a liquid metal",
                "Chemical short-range order in a high-entropy alloy"};

    c.blurbs = {
        "<p>In tutorial 1 a face-centered cubic metal is constructed, an EAM potential is applied, "
        "and the equilibrium lattice constant and cohesive energy of the crystal are obtained by "
        "energy minimization.</p>",
        "<p>In tutorial 2 the use of variables, loops, and command-line arguments in LAMMPS is "
        "demonstrated to scan the lattice parameter of a metal crystal, plot its energy-volume "
        "curve, and fit it to an equation of state to determine the equilibrium lattice constant "
        "and bulk modulus.</p>",
        "In tutorial 3 a single crystal is equilibrated and then subjected to tensile and "
        "compressive strain, the stress-strain response is extracted, and dislocation nucleation "
        "is visualized.</p>",
        "Tutorial 4 introduces grain boundaries: first a &Sigma;5(310) symmetric tilt boundary in "
        "fcc aluminum is constructed and its energy computed, then a &Sigma;3(111) boundary in bcc "
        "iron is strained to fracture and the per-atom stress is analyzed.</p>",
        "In tutorial 5 an aluminum surface is indented with rigid objects and a shaped tool is "
        "stamped into the surface using a hybrid potential.</p>",
        "In tutorial 6 the full elastic stiffness tensor of crystalline silicon is computed from "
        "small-strain deformations using the Stillinger-Weber potential.</p>",
        "In tutorial 7 the generalized stacking fault energy (&gamma;-surface) of fcc aluminum is "
        "traced along the &lt;112&gt; direction in the (111) plane, "
        "explaining dislocation glide and partial splitting.</p>",
        "In tutorial 8 the thermal conductivity of silicon is computed using reverse "
        "non-equilibrium molecular dynamics (NEMD), and its finite-size dependence is "
        "examined.</p>",
        "In tutorial 9 the full phonon dispersion of fcc copper is obtained by accumulating "
        "displacement correlations during a finite-temperature MD run using <b><code>fix "
        "phonon</code></b>.</p>",
        "In tutorial 10 the zone-center phonon frequencies of silicon are computed at zero "
        "temperature from the dynamical matrix, and their connection to elastic constants is "
        "discussed.</p>",
        "In tutorial 11 a primary knock-on atom (PKA) displacement cascade is simulated in "
        "bcc iron, and the resulting Frenkel-pair defects are counted on the fly with the "
        "<b><code>compute frenkel</code></b> command.</p>",
        "In tutorial 12 the melting temperature of fcc aluminum is determined; first by "
        "direct NPT heating and then by the more rigorous two-phase coexistence method.</p>",
        "In tutorial 13 the self-diffusion coefficient of liquid aluminum is measured from the "
        "mean-squared displacement, and its temperature dependence is fitted to an Arrhenius law "
        "to obtain an activation energy.</p>",
        "In tutorial 14 the chemical short-range order of the high-entropy alloy CoCrFeNi is "
        "equilibrated with a hybrid molecular dynamics / Monte Carlo simulation and quantified "
        "with the Warren-Cowley parameters for three different published potentials.</p>"};

    c.available = c.blurbs.size(); // fully published: all tutorials are launchable
    return c;
}

// --- Collection 3: granular / discrete-element-method tutorials (PLANNED) -----
//
// A placeholder for a future tutorial collection.  No tutorials are defined yet,
// so it shows in the menu as a disabled "Granular / DEM (planned)" entry.  Add
// titles/blurbs (and eventually hosting + published = true) as it is developed.
TutorialCollection granular()
{
    TutorialCollection c;
    c.key       = QStringLiteral("granular");
    c.name      = QStringLiteral("Granular / DEM");
    c.dirPrefix = QStringLiteral("granular-tutorial");
    c.author    = QStringLiteral("the LAMMPS Granular / DEM tutorials authors");
    c.siteUrl   = QStringLiteral("https://lammpstutorials.github.io/");
    c.logo      = QStringLiteral(":/icons/tutorial-logo.png");
    c.published = false;
    c.status    = QStringLiteral("planned");
    // no tutorials defined yet
    return c;
}

const QList<TutorialCollection> &collections()
{
    static const QList<TutorialCollection> all = {molecular(), matsci(), granular()};
    return all;
}

} // namespace

const QList<TutorialCollection> &tutorialCollections()
{
    return collections();
}

const TutorialCollection &tutorialCollection(int index)
{
    const auto &all = collections();
    if (index < 0 || index >= all.size()) index = 0;
    return all[index];
}

// Local Variables:
// c-basic-offset: 4
// End:
