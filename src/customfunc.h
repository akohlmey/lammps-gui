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

#include <vector>

/**
 * @brief Result of sampling a custom expression over an x range
 */
struct CustomCurve {
    bool ok = false;       ///< true if the expression parsed and evaluated
    QString error;         ///< human-readable error message when @ref ok is false
    QList<QPointF> points; ///< sampled (x, y) points; non-finite y values are skipped
};

/**
 * @brief A named nonlinear-fit parameter
 *
 * Carries the initial guess on input to @ref fitCustomCurve and the fitted
 * value on output.
 */
struct FitParam {
    QString name;       ///< parameter name as it appears in the expression
    double value = 0.0; ///< initial guess (input) / fitted value (output)
};

/**
 * @brief Result of a nonlinear least-squares fit of a custom expression
 */
struct CustomFit {
    bool ok = false;        ///< true if the fit produced a usable solution
    QString error;          ///< human-readable error message when @ref ok is false
    QList<FitParam> params; ///< fitted parameters, in the input order
    QList<QPointF> curve;   ///< fitted model sampled over the x range
    double rms     = 0.0;   ///< root-mean-square residual at the solution
    int iterations = 0;     ///< Levenberg-Marquardt iterations performed
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

/**
 * @brief Nonlinear least-squares fit of a custom expression to (x, y) data
 *
 * Fits @p expression -- a function of the independent variable @p variable and
 * the named parameters in @p initialParams -- to the data (@p xdata, @p ydata)
 * by Levenberg-Marquardt. The Jacobian is built from analytic derivatives of
 * the expression with respect to each parameter (via LeptonMini's symbolic
 * differentiation). On success the fitted model is sampled at @p nsamples + 1
 * points over [@p xmin, @p xmax] for overlaying on the chart.
 *
 * @param expression    Math expression in @p variable and the parameter names
 * @param initialParams Parameters with their initial guesses (at least one)
 * @param xdata         Independent-variable data
 * @param ydata         Dependent-variable data (same length as @p xdata)
 * @param xmin          Lower bound for sampling the fitted curve
 * @param xmax          Upper bound for sampling the fitted curve
 * @param nsamples      Number of sub-intervals (clamped to >= 1); nsamples+1 points
 * @param variable      Name of the independent variable (default "x")
 * @return Fit result; on a parse/dimension/evaluation error @ref CustomFit::ok
 *         is false and @ref CustomFit::error describes the problem
 */
CustomFit fitCustomCurve(const QString &expression, const QList<FitParam> &initialParams,
                         const std::vector<double> &xdata, const std::vector<double> &ydata,
                         double xmin, double xmax, int nsamples,
                         const QString &variable = QStringLiteral("x"));

#endif

// Local Variables:
// c-basic-offset: 4
// End:
