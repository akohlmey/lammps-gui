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

#include "qtgraphsbackend.h"
#include "qaddon.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMetaObject>
#include <QPalette>
#include <QQuickItem>
#include <QQuickWidget>
#include <QSettings>
#include <QVBoxLayout>
#include <QVariant>
#include <QtGraphs/QGraphsTheme>
#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

#include <cmath>

QtGraphsBackend::QtGraphsBackend() :
    container(nullptr), quickWidget(nullptr), graphsView(nullptr), ylabelWidget(nullptr),
    xlabelWidget(nullptr), titleWidget(nullptr), xaxis(nullptr), yaxis(nullptr)
{
}

QtGraphsBackend::~QtGraphsBackend()
{
    // container, quickWidget, labels, and axes are Qt children of the parent widget
}

void QtGraphsBackend::init(QWidget *parent, const QString &title, QLineSeries *series)
{
    xaxis = new QValueAxis(parent);
    yaxis = new QValueAxis(parent);

    QSettings settings;
    settings.beginGroup("charts");

    xaxis->setTitleText("Time step");
    xaxis->setLabelFormat("%.0f");
    xaxis->setSubTickCount(4);
    yaxis->setSubTickCount(4);
    yaxis->setTitleText(title);
    series->setName(title);

    // hide built-in axis titles; external labels provide proper spacing
    xaxis->setTitleVisible(false);
    yaxis->setTitleVisible(false);

    // configure theme: white background for axis/tick area and plot area, grid appearance
    auto *theme = new QGraphsTheme;
    theme->setBackgroundVisible(true);
    theme->setBackgroundColor(Qt::white);
    theme->setPlotAreaBackgroundVisible(true);
    theme->setPlotAreaBackgroundColor(Qt::white);
    QGraphsLine gridLine;
    if (settings.value("grid", true).toBool()) {
        gridLine.setMainColor(QColor(160, 160, 160));
        gridLine.setMainWidth(2.0);
    } else {
        gridLine.setMainColor(Qt::white);
        gridLine.setMainWidth(0.0);
    }
    if (settings.value("minorgrid", true).toBool()) {
        gridLine.setSubColor(QColor(192, 192, 192));
        gridLine.setSubWidth(1.0);
    } else {
        gridLine.setSubColor(Qt::white);
        gridLine.setSubWidth(0.0);
    }
    theme->setGrid(gridLine);

    // embed QtGraphs QML GraphsView via QQuickWidget
    quickWidget = new QQuickWidget(parent);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    theme->setParent(quickWidget);
    quickWidget->setInitialProperties({
        {"theme", QVariant::fromValue(theme)},
        {"axisX", QVariant::fromValue(static_cast<QObject *>(xaxis))},
        {"axisY", QVariant::fromValue(static_cast<QObject *>(yaxis))},
    });
    quickWidget->loadFromModule("QtGraphs", "GraphsView");
    quickWidget->setContentsMargins(0, 0, 0, 0);
    graphsView = quickWidget->rootObject();
    if (!graphsView) {
        settings.endGroup();
        return;
    }

    // external axis and plot title labels: bold black, positioned outside the chart
    QFont titleFont;
    titleFont.setBold(true);

    ylabelWidget = new VerticalLabel(title, parent);
    ylabelWidget->setFont(titleFont);
    ylabelWidget->setAutoFillBackground(true);
    ylabelWidget->setContentsMargins(10, 0, 0, 0);
    QPalette pal = ylabelWidget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::WindowText, Qt::black);
    ylabelWidget->setPalette(pal);

    xlabelWidget = new QLabel("Time step", parent);
    xlabelWidget->setAlignment(Qt::AlignCenter);
    xlabelWidget->setFont(titleFont);
    xlabelWidget->setAutoFillBackground(true);
    xlabelWidget->setContentsMargins(0, 0, 0, 10);
    pal = xlabelWidget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::WindowText, Qt::black);
    xlabelWidget->setPalette(pal);

    titleWidget = new QLabel("", parent);
    titleWidget->setAlignment(Qt::AlignCenter);
    titleWidget->setFont(titleFont);
    titleWidget->setAutoFillBackground(true);
    titleWidget->setContentsMargins(0, 10, 0, 0);
    pal = titleWidget->palette();
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::WindowText, Qt::black);
    titleWidget->setPalette(pal);

    container     = new QWidget(parent);
    auto *hlayout = new QHBoxLayout;
    hlayout->setContentsMargins(0, 0, 0, 0);
    hlayout->setSpacing(0);
    hlayout->addWidget(ylabelWidget);
    hlayout->addWidget(quickWidget, 1);

    auto *vlayout = new QVBoxLayout(container);
    vlayout->setContentsMargins(0, 0, 0, 0);
    vlayout->setSpacing(0);
    vlayout->addWidget(titleWidget);
    vlayout->addLayout(hlayout, 1);
    vlayout->addWidget(xlabelWidget);
    settings.endGroup();
}

QWidget *QtGraphsBackend::widget() const
{
    return container;
}

QList<QAbstractAxis *> QtGraphsBackend::getAxes() const
{
    return {xaxis, yaxis};
}

void QtGraphsBackend::resetZoom(double xmin, double xmax, double ymin, double ymax)
{
    xaxis->setRange(xmin, xmax);
    yaxis->setRange(ymin, ymax);

    // compute "nice" tick intervals targeting about 5 major ticks per axis
    auto niceInterval = [](double range) -> double {
        if (range <= 0) return 1.0;
        double rough = range / 4.0;
        double p     = pow(10.0, floor(log10(rough)));
        double frac  = rough / p;
        double nice;
        if (frac < 1.5)
            nice = 1.0;
        else if (frac < 3.0)
            nice = 2.0;
        else if (frac < 7.0)
            nice = 5.0;
        else
            nice = 10.0;
        return nice * p;
    };
    double xspan = xmax - xmin;
    double yspan = ymax - ymin;
    xaxis->setTickAnchor(0.0);
    yaxis->setTickAnchor(0.0);
    xaxis->setTickInterval(niceInterval(xspan));
    yaxis->setTickInterval(niceInterval(yspan));
}

void QtGraphsBackend::addSeries(QLineSeries *s, const QColor &color, qreal width)
{
    s->setColor(color);
    s->setWidth(width);
    s->setCapStyle(Qt::RoundCap);
    if (graphsView) QMetaObject::invokeMethod(graphsView, "addSeries", Q_ARG(QObject *, s));
}

void QtGraphsBackend::removeSeries(QLineSeries *s)
{
    if (graphsView) QMetaObject::invokeMethod(graphsView, "removeSeries", Q_ARG(QObject *, s));
}

bool QtGraphsBackend::hasSeries(QLineSeries *s) const
{
    bool result = false;
    if (graphsView)
        QMetaObject::invokeMethod(graphsView, "hasSeries", Q_RETURN_ARG(bool, result),
                                  Q_ARG(QObject *, s));
    return result;
}

void QtGraphsBackend::setTLabel(const QString &tlabel)
{
    if (titleWidget) titleWidget->setText(tlabel);
}

QString QtGraphsBackend::getTLabel() const
{
    return titleWidget ? titleWidget->text() : QString();
}

void QtGraphsBackend::setYLabel(const QString &ylabel)
{
    yaxis->setTitleText(ylabel);
    if (ylabelWidget) ylabelWidget->setText(ylabel);
}

// Local Variables:
// c-basic-offset: 4
// End:
