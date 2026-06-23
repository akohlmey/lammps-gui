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

#include "customfunc.h"

#include "lepton_mini.h"
#include "levmar.h"

#include <cmath>
#include <exception>
#include <map>
#include <string>
#include <vector>

CompiledExpression::CompiledExpression(const QString &expression)
{
    try {
        // parse once, optimize, and compile to an interpreted program so the
        // per-point evaluation in the caller's loop stays cheap
        LeptonMini::ParsedExpression parsed =
            LeptonMini::Parser::parse(expression.toStdString()).optimize();
        program = std::make_unique<LeptonMini::ExpressionProgram>(parsed.createProgram());
        valid   = true;
    } catch (const std::exception &e) {
        errorMsg = QString::fromStdString(e.what());
        valid    = false;
    }
}

CompiledExpression::~CompiledExpression() = default;

double CompiledExpression::evaluate(const std::map<std::string, double> &variables) const
{
    return program->evaluate(variables);
}

CustomCurve evalCustomCurve(const QString &expression, double xmin, double xmax, int nsamples,
                            const QString &variable)
{
    CustomCurve result;

    const QString expr = expression.trimmed();
    if (expr.isEmpty()) {
        result.error = QStringLiteral("The expression is empty.");
        return result;
    }
    if (nsamples < 1) nsamples = 1;

    const std::string var = variable.toStdString();

    CompiledExpression program(expr);
    if (!program.isValid()) {
        result.error = program.error();
        return result;
    }

    try {
        std::map<std::string, double> vars;
        for (int k = 0; k <= nsamples; ++k) {
            const double x = xmin + (xmax - xmin) * static_cast<double>(k) / nsamples;
            vars[var]      = x;
            const double y = program.evaluate(vars);
            if (std::isfinite(y)) result.points.append(QPointF(x, y));
        }
    } catch (const std::exception &e) {
        result.error = QString::fromStdString(e.what());
        result.points.clear();
        return result;
    }

    result.ok = true;
    return result;
}

CustomFit fitCustomCurve(const QString &expression, const QList<FitParam> &initialParams,
                         const std::vector<double> &xdata, const std::vector<double> &ydata,
                         double xmin, double xmax, int nsamples, const QString &variable)
{
    CustomFit result;

    const QString expr = expression.trimmed();
    if (expr.isEmpty()) {
        result.error = QStringLiteral("The expression is empty.");
        return result;
    }
    if (initialParams.isEmpty()) {
        result.error = QStringLiteral("No fit parameters were given.");
        return result;
    }
    if (xdata.size() != ydata.size()) {
        result.error = QStringLiteral("The x and y data have different lengths.");
        return result;
    }

    const int m = static_cast<int>(xdata.size());
    const int n = initialParams.size();
    if (m < n) {
        result.error = QStringLiteral("Too few data points (%1) for %2 parameters.").arg(m).arg(n);
        return result;
    }
    if (nsamples < 1) nsamples = 1;

    const std::string var = variable.toStdString();

    // collect parameter names; reject empties, duplicates, and clashes with the
    // independent variable
    std::vector<std::string> pnames;
    pnames.reserve(n);
    for (const FitParam &p : initialParams) {
        const QString nm = p.name.trimmed();
        if (nm.isEmpty()) {
            result.error = QStringLiteral("A fit parameter has an empty name.");
            return result;
        }
        if (nm == variable) {
            result.error =
                QStringLiteral("Parameter name '%1' clashes with the variable name.").arg(nm);
            return result;
        }
        const std::string s = nm.toStdString();
        for (const auto &e : pnames) {
            if (e == s) {
                result.error = QStringLiteral("Duplicate parameter name '%1'.").arg(nm);
                return result;
            }
        }
        pnames.push_back(s);
    }

    try {
        // parse once; build the optimized model and the analytic derivative with
        // respect to each parameter (the Jacobian columns)
        const LeptonMini::ParsedExpression base  = LeptonMini::Parser::parse(expr.toStdString());
        const LeptonMini::ParsedExpression model = base.optimize();
        std::vector<LeptonMini::ParsedExpression> derivs;
        derivs.reserve(n);
        for (const auto &s : pnames)
            derivs.push_back(base.differentiate(s).optimize());

        // pre-flight evaluation at the initial guess so an expression referencing
        // an undeclared symbol fails with a descriptive LeptonMini message
        std::map<std::string, double> probe;
        probe[var] = xdata.empty() ? 0.0 : xdata.front();
        for (int j = 0; j < n; ++j)
            probe[pnames[j]] = initialParams[j].value;
        (void)model.evaluate(probe);

        // residual/Jacobian callback for the Levenberg-Marquardt solver
        const LevmarModel fn = [&](const std::vector<double> &p, std::vector<double> &res,
                                   std::vector<std::vector<double>> &jac) -> bool {
            std::map<std::string, double> vars;
            for (int j = 0; j < n; ++j)
                vars[pnames[j]] = p[j];
            try {
                for (int i = 0; i < m; ++i) {
                    vars[var]            = xdata[i];
                    const double modeled = model.evaluate(vars);
                    if (!std::isfinite(modeled)) return false;
                    res[i] = modeled - ydata[i];
                    for (int j = 0; j < n; ++j) {
                        const double d = derivs[j].evaluate(vars);
                        if (!std::isfinite(d)) return false;
                        jac[i][j] = d;
                    }
                }
            } catch (const std::exception &) {
                return false;
            }
            return true;
        };

        std::vector<double> initial(n, 0.0);
        for (int j = 0; j < n; ++j)
            initial[j] = initialParams[j].value;

        const LevmarResult lm = levmarFit(m, n, initial, fn);
        if (!lm.ok) {
            result.error = QString::fromStdString(lm.message);
            return result;
        }

        // report fitted parameters in the input order
        for (int j = 0; j < n; ++j)
            result.params.append(FitParam{initialParams[j].name.trimmed(), lm.params[j]});

        // sample the fitted model over the requested range
        std::map<std::string, double> fitted;
        for (int j = 0; j < n; ++j)
            fitted[pnames[j]] = lm.params[j];
        for (int k = 0; k <= nsamples; ++k) {
            const double x = xmin + (xmax - xmin) * static_cast<double>(k) / nsamples;
            fitted[var]    = x;
            const double y = model.evaluate(fitted);
            if (std::isfinite(y)) result.curve.append(QPointF(x, y));
        }

        result.rms        = lm.rms;
        result.iterations = lm.iterations;
        result.ok         = true;
    } catch (const std::exception &e) {
        result.error = QString::fromStdString(e.what());
        result.params.clear();
        result.curve.clear();
        result.ok = false;
        return result;
    }

    return result;
}

// Local Variables:
// c-basic-offset: 4
// End:
