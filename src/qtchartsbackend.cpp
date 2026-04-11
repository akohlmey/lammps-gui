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

#include "qtchartsbackend.h"

#include <QChart>
#include <QPen>
#include <QSettings>

QtChartsBackend::QtChartsBackend() : chart(nullptr), chartView(nullptr), xaxis(nullptr),
                                     yaxis(nullptr)
{
}

QtChartsBackend::~QtChartsBackend()
{
    delete xaxis;
    delete yaxis;
    // chart is owned by chartView; chartView is a child of the parent widget
}

void QtChartsBackend::init(QWidget *parent, const QString &title, QLineSeries *series)
{
    chart = new QChart;
    xaxis = new QValueAxis;
    yaxis = new QValueAxis;

    QSettings settings;
    settings.beginGroup("charts");

    chart->legend()->hide();
    chart->addAxis(xaxis, Qt::AlignBottom);
    chart->addAxis(yaxis, Qt::AlignLeft);
    chart->setTitle("");
    xaxis->setTitleText("Time step");
    xaxis->setTickCount(5);
    xaxis->setLabelFormat("%d");
    yaxis->setTickCount(5);
    xaxis->setGridLineVisible(settings.value("grid", true).toBool());
    xaxis->setMinorGridLineVisible(settings.value("minorgrid", true).toBool());
    xaxis->setMinorGridLineColor(QColor(192, 192, 192));
    xaxis->setGridLineColor(QColor(160, 160, 160));
    xaxis->setMinorTickCount(4);
    yaxis->setMinorTickCount(4);
    yaxis->setTitleText(title);
    yaxis->setGridLineVisible(settings.value("grid", true).toBool());
    yaxis->setMinorGridLineVisible(settings.value("minorgrid", true).toBool());
    yaxis->setMinorGridLineColor(QColor(192, 192, 192));
    yaxis->setGridLineColor(QColor(160, 160, 160));

    series->setName(title);
    settings.endGroup();

    chartView = new QChartView(chart, parent);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::NoRubberBand);
}

QWidget *QtChartsBackend::widget() const
{
    return chartView;
}

QList<QAbstractAxis *> QtChartsBackend::getAxes() const
{
    return chart->axes();
}

void QtChartsBackend::resetZoom(double xmin, double xmax, double ymin, double ymax)
{
    xaxis->setRange(xmin, xmax);
    yaxis->setRange(ymin, ymax);
}

void QtChartsBackend::addSeries(QLineSeries *s, const QColor &color, qreal width)
{
    s->setPen(QPen(QBrush(color), width, Qt::SolidLine, Qt::RoundCap));
    chart->addSeries(s);
    s->attachAxis(xaxis);
    s->attachAxis(yaxis);
}

void QtChartsBackend::removeSeries(QLineSeries *s)
{
    chart->removeSeries(s);
}

bool QtChartsBackend::hasSeries(QLineSeries *s) const
{
    return chart->series().contains(s);
}

void QtChartsBackend::setTLabel(const QString &tlabel)
{
    if (chart) chart->setTitle(tlabel);
}

QString QtChartsBackend::getTLabel() const
{
    return chart ? chart->title() : QString();
}

void QtChartsBackend::setYLabel(const QString &ylabel)
{
    yaxis->setTitleText(ylabel);
}

// Local Variables:
// c-basic-offset: 4
// End:
