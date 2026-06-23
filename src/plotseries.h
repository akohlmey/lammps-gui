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

#ifndef PLOTSERIES_H
#define PLOTSERIES_H

// Neutral value types describing what to draw on a chart.
// They depend only on Qt value types (QtCore / QtGui).
// PlotWidget consumes these directly.

#include <QColor>
#include <QList>
#include <QPointF>
#include <QString>

/** @brief Whether a PlotSeries is drawn as a connected line or as markers */
enum class PlotSeriesType { Line, Scatter };

/**
 * @brief One data series in the neutral chart model
 *
 * Carries the points plus the minimal styling the native renderer needs.
 */
struct PlotSeries {
    PlotSeriesType type = PlotSeriesType::Line; ///< line vs. scatter rendering
    QList<QPointF> points;                      ///< data points in axis coordinates
    QColor color       = Qt::black;             ///< line / marker color
    qreal width        = 1.0;                   ///< line width (Line series)
    Qt::PenStyle style = Qt::SolidLine;         ///< line style, e.g. dashed reference lines
    qreal markerSize   = 8.0;                   ///< marker diameter (Scatter series)
    QString name;                               ///< series label (for a future legend)
    bool visible     = true;                    ///< whether the series is drawn
    bool isReference = false;                   ///< draw as a labeled reference line
    QString refLabel;                           ///< text drawn next to a reference line

    // convenience accessors mirroring the subset of the Qt series API used by
    // ChartViewer, so callers need not poke the points list directly
    /** @brief Append one (x, y) point */
    void append(double x, double y) { points.append(QPointF(x, y)); }
    /** @brief Replace all points */
    void replace(const QList<QPointF> &p) { points = p; }
    /** @brief Number of points */
    int count() const { return static_cast<int>(points.size()); }
    /** @brief Point at index i */
    QPointF at(int i) const { return points.at(i); }
    /** @brief Set visibility */
    void setVisible(bool v) { visible = v; }
    /** @brief Whether the series is visible */
    bool isVisible() const { return visible; }
    /** @brief Remove all points */
    void clear() { points.clear(); }
};

/**
 * @brief One linear value axis in the neutral chart model
 */
struct PlotAxis {
    double min = 0.0;                             ///< lower end of the displayed range
    double max = 1.0;                             ///< upper end of the displayed range
    QString title;                                ///< axis title
    QString labelFormat   = QStringLiteral("%g"); ///< printf-style tick label format
    int subTicks          = 4;                    ///< minor subdivisions between major ticks
    bool gridVisible      = true;                 ///< draw major gridlines
    bool minorGridVisible = true;                 ///< draw minor gridlines
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
