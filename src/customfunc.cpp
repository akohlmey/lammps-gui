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

#include <cmath>
#include <exception>
#include <map>
#include <string>

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

    try {
        // parse once, optimize, and compile to an interpreted program so the
        // per-point evaluation in the loop stays cheap
        LeptonMini::ParsedExpression parsed =
            LeptonMini::Parser::parse(expr.toStdString()).optimize();
        LeptonMini::ExpressionProgram program = parsed.createProgram();

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

// Local Variables:
// c-basic-offset: 4
// End:
