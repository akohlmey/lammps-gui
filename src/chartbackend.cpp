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

#include "chartbackend.h"

// Select the concrete chart backend here, in one place, so the rest of the code
// (notably ChartViewer) is free of backend #ifdefs.
#ifdef LAMMPS_GUI_USE_NATIVE_CHARTS
#include "nativechartbackend.h"
#elif defined(LAMMPS_GUI_USE_QTGRAPHS)
#include "qtgraphsbackend.h"
#else
#include "qtchartsbackend.h"
#endif

std::unique_ptr<ChartBackend> ChartBackend::create()
{
#ifdef LAMMPS_GUI_USE_NATIVE_CHARTS
    return std::make_unique<NativeChartBackend>();
#elif defined(LAMMPS_GUI_USE_QTGRAPHS)
    return std::make_unique<QtGraphsBackend>();
#else
    return std::make_unique<QtChartsBackend>();
#endif
}

// Local Variables:
// c-basic-offset: 4
// End:
