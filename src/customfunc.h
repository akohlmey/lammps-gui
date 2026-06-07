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

#ifndef CUSTOMFUNC_H
#define CUSTOMFUNC_H

// Evaluate user-supplied mathematical expressions (via the vendored LeptonMini
// parser) for custom-function plotting in the chart post-processing dialog.
// The interface is QString in / QString out so call sites stay free of
// std::string conversions; the LeptonMini (std::string) boundary is confined
// to the implementation, mirroring how LammpsWrapper confines the LAMMPS C API.

#include <QList>
#include <QPointF>
#include <QString>

/**
 * @brief Result of sampling a custom expression over an x range
 */
struct CustomCurve {
    bool ok = false;       ///< true if the expression parsed and evaluated
    QString error;         ///< human-readable error message when @ref ok is false
    QList<QPointF> points; ///< sampled (x, y) points; non-finite y values are skipped
};

/**
 * @brief Parse and evaluate a single-variable expression over an x range
 *
 * Parses @p expression with the vendored LeptonMini parser, optimizes it, and
 * evaluates it at @p nsamples + 1 equally spaced points spanning
 * [@p xmin, @p xmax]. Points whose value is not finite (NaN/inf) are omitted.
 *
 * @param expression Math expression in the variable @p variable (LeptonMini syntax)
 * @param xmin       Lower bound of the sampling range
 * @param xmax       Upper bound of the sampling range
 * @param nsamples   Number of sub-intervals (clamped to >= 1); nsamples+1 points
 * @param variable   Name of the independent variable in @p expression (default "x")
 * @return Curve result; on a parse/evaluation error @ref CustomCurve::ok is
 *         false and @ref CustomCurve::error describes the problem
 */
CustomCurve evalCustomCurve(const QString &expression, double xmin, double xmax, int nsamples,
                            const QString &variable = QStringLiteral("x"));

#endif

// Local Variables:
// c-basic-offset: 4
// End:
