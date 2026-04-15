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

#ifndef QTGRAPHSBACKEND_H
#define QTGRAPHSBACKEND_H

#include "chartbackend.h"

#include <QtGraphs/QAbstractAxis>
#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

class QLabel;
class QQuickWidget;
class QQuickItem;
class VerticalLabel;

/**
 * @brief QtGraphs-based chart rendering backend
 *
 * Implements the ChartBackend interface using the QtGraphs module (Qt 6.10+).
 * Uses QML-based QGraphsView via QQuickWidget for rendering, with external
 * labels for axis titles to avoid overlap with tick labels.
 */
class QtGraphsBackend : public ChartBackend {
public:
    QtGraphsBackend();
    ~QtGraphsBackend() override;

    void init(QWidget *parent, const QString &title, QLineSeries *series) override;
    QWidget *widget() const override;
    QList<QAbstractAxis *> getAxes() const override;
    QValueAxis *xAxis() const override { return xaxis; }
    QValueAxis *yAxis() const override { return yaxis; }
    void resetZoom(double xmin, double xmax, double ymin, double ymax) override;
    void addSeries(QLineSeries *s, const QColor &color, qreal width) override;
    void removeSeries(QLineSeries *s) override;
    bool hasSeries(QLineSeries *s) const override;
    void refreshSeries() override;
    void setTLabel(const QString &tlabel) override;
    QString getTLabel() const override;
    void setYLabel(const QString &ylabel) override;

private:
    /**
     * @brief Stores a series together with its display style for refresh
     */
    struct SeriesRecord {
        QLineSeries *series = nullptr; ///< The series pointer
        QColor color;                  ///< Line color
        qreal width = 0.0;             ///< Line width
    };

    QWidget *container;          ///< Container widget holding the layout
    QQuickWidget *quickWidget;   ///< Widget hosting the QGraphsView QML item
    QQuickItem *graphsView;      ///< Root QGraphsView QML item
    VerticalLabel *ylabelWidget; ///< External y-axis title label (avoids overlap)
    QLabel *xlabelWidget;        ///< External x-axis title label (with spacing)
    QLabel *titleWidget;         ///< Chart title label (with spacing)
    QValueAxis *xaxis;           ///< X-axis (time/step)
    QValueAxis *yaxis;           ///< Y-axis (property value)
    QList<SeriesRecord> trackedSeries; ///< Tracks all added series for refresh
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
