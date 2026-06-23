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

#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

// Lightweight 2D line / scatter chart drawn directly with QPainter, with
// no dependency on any Qt module.  It consumes the neutral PlotSeries / PlotAxis
// model and reproduces the subset of chart features that LAMMPS-GUI actually uses:
// linear axes with nice-number major ticks, minor subticks, major / minor gridlines,
// printf-style tick labels, axis and chart titles, and multiple line / scatter
// series including dashed reference lines. Zoom is programmatic (setXRange / setYRange);
// there is no in-widget mouse interaction (the chart window drives ranges externally).

#include "plotseries.h"

#include <QList>
#include <QSize>
#include <QString>
#include <QWidget>

class QImage;
class QPainter;
class QPaintEvent;
class QRectF;

/**
 * @brief QPainter-based renderer for the neutral chart model
 *
 * Series objects are referenced, not owned: the caller owns each PlotSeries and
 * registers / unregisters it here, then calls update() after changing its data or style.
 */
class PlotWidget : public QWidget {
public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit PlotWidget(QWidget *parent = nullptr);

    /** @brief Set the chart title (drawn centered at the top) */
    void setTitle(const QString &title);
    /** @brief Get the chart title */
    QString title() const;

    /** @brief Set the X-axis title */
    void setXTitle(const QString &title);
    /** @brief Get the X-axis title */
    QString xTitle() const;
    /** @brief Set the Y-axis title */
    void setYTitle(const QString &title);
    /** @brief Get the Y-axis title */
    QString yTitle() const;

    /** @brief Set the displayed X-axis range */
    void setXRange(double min, double max);
    /** @brief Set the displayed Y-axis range */
    void setYRange(double min, double max);

    /** @brief Set the printf-style tick label format of the X-axis */
    void setXLabelFormat(const QString &fmt);
    /** @brief Set the printf-style tick label format of the Y-axis */
    void setYLabelFormat(const QString &fmt);

    /** @brief Toggle major and minor gridline visibility on both axes */
    void setGrid(bool major, bool minor);

    /**
     * @brief Register a series for drawing (non-owning; ignored if already present)
     * @param series Series owned by the caller
     */
    void addSeries(const PlotSeries *series);
    /** @brief Remove a previously registered series (does not delete it) */
    void removeSeries(const PlotSeries *series);
    /** @brief Whether a series is currently registered */
    bool hasSeries(const PlotSeries *series) const;

    /**
     * @brief Render the chart into a freshly allocated image
     * @param size Pixel size of the returned image
     * @return The rendered image (white background)
     */
    QImage renderToImage(const QSize &size) const;

protected:
    /** @brief Paint the chart onto the widget */
    void paintEvent(QPaintEvent *event) override;
    /** @brief Recommended size */
    QSize sizeHint() const override;

private:
    /** @brief Shared painting routine used by paintEvent() and renderToImage() */
    void doRender(QPainter &p, const QRectF &target) const;

    PlotAxis m_xaxis;                   ///< X-axis configuration
    PlotAxis m_yaxis;                   ///< Y-axis configuration
    QString m_title;                    ///< chart title
    QList<const PlotSeries *> m_series; ///< registered series (not owned)
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
