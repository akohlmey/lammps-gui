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

#include <memory>

class QAbstractAxis;
class QLineSeries;
class QXYSeries;
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
     * @brief Create the chart backend selected at build time
     * @return A new backend instance (native QPainter, QtGraphs, or QtCharts)
     *
     * The concrete type is chosen by the build via the LAMMPS_GUI_USE_NATIVE_CHARTS
     * and LAMMPS_GUI_USE_QTGRAPHS preprocessor defines, keeping that choice in a
     * single translation unit instead of scattered #ifdefs at the call sites.
     */
    static std::unique_ptr<ChartBackend> create();

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
     * @brief Add a series (line or scatter) to the chart display
     * @param s Series to add (a QLineSeries or QScatterSeries)
     * @param color Color for the series
     * @param width Line width (applied only to line series)
     */
    virtual void addSeries(QXYSeries *s, const QColor &color, qreal width) = 0;

    /**
     * @brief Update the appearance of an already-added series
     * @param s Series to restyle (a QLineSeries or QScatterSeries)
     * @param color New color
     * @param width New line width (applied only to line series)
     */
    virtual void styleSeries(QXYSeries *s, const QColor &color, qreal width) = 0;

    /**
     * @brief Set the line style of a series (e.g. dashed reference lines)
     * @param s     Series to restyle (only line series are affected)
     * @param style Qt pen style to apply
     *
     * Backends that cannot render dashed lines (QtGraphs) treat this as a no-op.
     */
    virtual void setSeriesLineStyle(QXYSeries *s, Qt::PenStyle style) = 0;

    /**
     * @brief Set the marker diameter of a scatter series
     * @param s    The series (only scatter series are affected)
     * @param size Marker diameter in pixels
     *
     * Optional: backends that do not size markers leave this as a no-op.
     */
    virtual void setMarkerSize(QXYSeries *s, qreal size)
    {
        Q_UNUSED(s);
        Q_UNUSED(size);
    }

    /**
     * @brief Mark a series as a labeled reference line
     * @param s     The reference-line series
     * @param label Text to draw next to the line
     *
     * Optional capability: the native backend draws the label next to the line
     * (inferring horizontal/vertical from the series geometry). Backends that do
     * not render annotations leave this as a no-op and only draw the line.
     */
    virtual void setReferenceLabel(QXYSeries *s, const QString &label)
    {
        Q_UNUSED(s);
        Q_UNUSED(label);
    }

    /**
     * @brief Remove a series from the chart display
     * @param s Series to remove
     */
    virtual void removeSeries(QXYSeries *s) = 0;

    /**
     * @brief Check if a series is currently displayed
     * @param s Series to check
     * @return true if series is in the chart
     */
    virtual bool hasSeries(QXYSeries *s) const = 0;

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
     * @brief Set the X-axis label
     * @param xlabel New X-axis label text
     */
    virtual void setXLabel(const QString &xlabel) = 0;

    /**
     * @brief Set the X-axis tick label format string
     * @param fmt printf-style format string (e.g. "%d" or "%.6g")
     *
     * For the QtGraphs backend the format is stored and re-applied after
     * every resetZoom() call so that QML tick regeneration does not revert it.
     */
    virtual void setXLabelFormat(const QString &fmt) = 0;

    /**
     * @brief Set the Y-axis label
     * @param ylabel New Y-axis label text
     */
    virtual void setYLabel(const QString &ylabel) = 0;
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
