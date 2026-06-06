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

#include "analysis.h"

std::vector<double> autocorrelation(const std::vector<double> &y, int maxlag)
{
    const int n = static_cast<int>(y.size());
    if (n < 2) return {};

    if ((maxlag <= 0) || (maxlag >= n)) maxlag = n - 1;

    // mean
    double mean = 0.0;
    for (double v : y)
        mean += v;
    mean /= static_cast<double>(n);

    // total variance (denominator); zero for a constant series
    double denom = 0.0;
    for (double v : y) {
        const double d = v - mean;
        denom += d * d;
    }
    if (denom <= 0.0) return {};

    std::vector<double> acf(maxlag + 1, 0.0);
    for (int k = 0; k <= maxlag; ++k) {
        double num = 0.0;
        for (int i = 0; i + k < n; ++i)
            num += (y[i] - mean) * (y[i + k] - mean);
        acf[k] = num / denom;
    }
    return acf;
}

// Local Variables:
// c-basic-offset: 4
// End:
