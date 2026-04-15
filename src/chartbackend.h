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

#ifndef CHARTBACKEND_H
#define CHARTBACKEND_H

#include <QColor>
#include <QList>
#include <QString>
#include <QtGlobal>

class QAbstractAxis;
class QLineSeries;
class QValueAxis;
class QWidget;

/**
 * @brief Abstract interface for chart rendering backends
 *
 * Provides a common interface for the two supported Qt chart backends:
 * QtGraphs (QML-based, Qt 6.10+) and QtCharts (widget-based, Qt 6.2+).
 * Each backend manages its own rendering widget, axes, and series display.
 * ChartViewer delegates all backend-specific operations through this interface.
 */
class ChartBackend {
public:
    virtual ~ChartBackend() = default;

    /**
     * @brief Initialize the backend chart widget and axes
     * @param parent Parent widget for the chart
     * @param title Initial chart/Y-axis title
     * @param series Raw data series to display
     */
    virtual void init(QWidget *parent, const QString &title, QLineSeries *series) = 0;

    /**
     * @brief Get the main chart widget for embedding in layouts
     * @return The chart widget (QQuickWidget or QChartView)
     */
    virtual QWidget *widget() const = 0;

    /**
     * @brief Get list of chart axes
     * @return List of axes (X and Y)
     */
    virtual QList<QAbstractAxis *> getAxes() const = 0;

    /**
     * @brief Get X-axis object
     * @return X-axis
     */
    virtual QValueAxis *xAxis() const = 0;

    /**
     * @brief Get Y-axis object
     * @return Y-axis
     */
    virtual QValueAxis *yAxis() const = 0;

    /**
     * @brief Reset zoom to show all data with proper tick intervals
     * @param xmin Minimum X value
     * @param xmax Maximum X value
     * @param ymin Minimum Y value
     * @param ymax Maximum Y value
     */
    virtual void resetZoom(double xmin, double xmax, double ymin, double ymax) = 0;

    /**
     * @brief Add a series to the chart display
     * @param s Series to add
     * @param color Color for the series
     * @param width Line width
     */
    virtual void addSeries(QLineSeries *s, const QColor &color, qreal width) = 0;

    /**
     * @brief Remove a series from the chart display
     * @param s Series to remove
     */
    virtual void removeSeries(QLineSeries *s) = 0;

    /**
     * @brief Check if a series is currently displayed
     * @param s Series to check
     * @return true if series is in the chart
     */
    virtual bool hasSeries(QLineSeries *s) const = 0;

    /**
     * @brief Set the chart title
     * @param tlabel New title text
     */
    virtual void setTLabel(const QString &tlabel) = 0;

    /**
     * @brief Get the chart title
     * @return Current title text
     */
    virtual QString getTLabel() const = 0;

    /**
     * @brief Set the Y-axis label
     * @param ylabel New Y-axis label text
     */
    virtual void setYLabel(const QString &ylabel) = 0;

    /**
     * @brief Refresh the series display by removing and re-adding all series
     *
     * This is a no-op for QtCharts, which handles axis range changes
     * automatically. The QtGraphs backend overrides this to clear the
     * stale full-range rendering and create a fresh one for the current
     * axis range after a range slider update.
     */
    virtual void refreshSeries() {}
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
