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

#ifndef QTCHARTSBACKEND_H
#define QTCHARTSBACKEND_H

#include "chartbackend.h"

#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

class QChart;

/**
 * @brief QtCharts-based chart rendering backend
 *
 * Implements the ChartBackend interface using the QtCharts module (Qt 6.2+).
 * Uses QChart and QChartView for rendering with a widget-based approach.
 */
class QtChartsBackend : public ChartBackend {
public:
    QtChartsBackend();
    ~QtChartsBackend() override;

    void init(QWidget *parent, const QString &title, QLineSeries *series) override;
    QWidget *widget() const override;
    QList<QAbstractAxis *> getAxes() const override;
    QValueAxis *xAxis() const override { return xaxis; }
    QValueAxis *yAxis() const override { return yaxis; }
    void resetZoom(double xmin, double xmax, double ymin, double ymax) override;
    void addSeries(QLineSeries *s, const QColor &color, qreal width) override;
    void removeSeries(QLineSeries *s) override;
    bool hasSeries(QLineSeries *s) const override;
    void setTLabel(const QString &tlabel) override;
    QString getTLabel() const override;
    void setYLabel(const QString &ylabel) override;

private:
    QChart *chart;         ///< The chart object
    QChartView *chartView; ///< The chart view widget
    QValueAxis *xaxis;     ///< X-axis (time/step)
    QValueAxis *yaxis;     ///< Y-axis (property value)
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
