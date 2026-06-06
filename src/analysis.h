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

#ifndef ANALYSIS_H
#define ANALYSIS_H

// Small, self-contained (Qt-free) post-processing analyses on a data series.
// Pure functions on std::vector<double> so they can be unit-tested without a
// GUI and reused by the chart post-processing dialog.

#include <vector>

/**
 * @brief Normalized autocorrelation function (ACF) of a data series
 * @param y      Input samples (assumed equally spaced)
 * @param maxlag Largest lag to compute; values <= 0 or >= y.size() are
 *               clamped to y.size()-1
 * @return ACF values for lags 0..maxlag (length maxlag+1), normalized so that
 *         the lag-0 value is 1; an empty vector if the input has fewer than
 *         two samples or zero variance (a constant series)
 *
 * Uses the standard biased estimator
 * @f$ \mathrm{ACF}(k) = \frac{\sum_{i=0}^{N-1-k}(y_i-\bar y)(y_{i+k}-\bar y)}
 * {\sum_{i=0}^{N-1}(y_i-\bar y)^2} @f$.
 */
std::vector<double> autocorrelation(const std::vector<double> &y, int maxlag);

#endif

// Local Variables:
// c-basic-offset: 4
// End:
