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

#include "nativechartbackend.h"

#include "constants.h"
#include "plotwidget.h"

#include <QLineSeries>
#include <QSettings>
#include <QValueAxis>
#include <QXYSeries>

NativeChartBackend::NativeChartBackend() : m_plot(nullptr), xaxis(nullptr), yaxis(nullptr) {}

NativeChartBackend::~NativeChartBackend()
{
    // deleting the axes also drops their rangeChanged connections
    delete xaxis;
    delete yaxis;
    // m_plot is owned by its parent widget; the Entry PlotSeries are freed with
    // m_entries (PlotWidget only holds non-owning pointers)
}

void NativeChartBackend::init(QWidget *parent, const QString &title, QLineSeries *series)
{
    m_plot = new PlotWidget(parent);
    xaxis  = new QValueAxis;
    yaxis  = new QValueAxis;

    xaxis->setTitleText("Time step");
    xaxis->setLabelFormat("%d");
    yaxis->setTitleText(title);

    m_plot->setXTitle("Time step");
    m_plot->setYTitle(title);
    m_plot->setXLabelFormat("%d");

    QSettings settings;
    settings.beginGroup(Keys::GROUP_CHARTS);
    const bool grid      = settings.value(Keys::GRID, true).toBool();
    const bool minorgrid = settings.value(Keys::MINORGRID, true).toBool();
    settings.endGroup();
    m_plot->setGrid(grid, minorgrid);

    if (series) series->setName(title);

    // The range sliders (ChartWindow::updateXRange/updateYRange) call setRange()
    // directly on these axes; forward that to the renderer. Using m_plot as the
    // connection context drops the connection automatically if it is destroyed.
    QObject::connect(xaxis, &QValueAxis::rangeChanged, m_plot, [this](qreal lo, qreal hi) {
        m_plot->setXRange(lo, hi);
    });
    QObject::connect(yaxis, &QValueAxis::rangeChanged, m_plot, [this](qreal lo, qreal hi) {
        m_plot->setYRange(lo, hi);
    });
}

QWidget *NativeChartBackend::widget() const
{
    return m_plot;
}

QList<QAbstractAxis *> NativeChartBackend::getAxes() const
{
    return {xaxis, yaxis};
}

void NativeChartBackend::resetZoom(double xmin, double xmax, double ymin, double ymax)
{
    syncSeries();
    // keep the QValueAxis state consistent (read by getAxes() and the sliders)
    xaxis->setRange(xmin, xmax);
    yaxis->setRange(ymin, ymax);
    // apply directly too, so a data-only change (unchanged range, no rangeChanged
    // signal) still repaints
    m_plot->setXRange(xmin, xmax);
    m_plot->setYRange(ymin, ymax);
    m_plot->update();
}

NativeChartBackend::Entry *NativeChartBackend::findEntry(QXYSeries *s)
{
    for (auto &e : m_entries)
        if (e.qt == s) return &e;
    return nullptr;
}

void NativeChartBackend::syncSeries()
{
    for (auto &e : m_entries) {
        e.plot->points  = e.qt->points();
        e.plot->visible = e.qt->isVisible();
        e.plot->name    = e.qt->name();
    }
}

void NativeChartBackend::addSeries(QXYSeries *s, const QColor &color, qreal width)
{
    if (!s) return;
    if (findEntry(s)) {
        styleSeries(s, color, width);
        return;
    }

    Entry e;
    e.qt          = s;
    e.plot        = std::make_unique<PlotSeries>();
    e.plot->type  = qobject_cast<QLineSeries *>(s) ? PlotSeriesType::Line : PlotSeriesType::Scatter;
    e.plot->color = color;
    if (e.plot->type == PlotSeriesType::Line) e.plot->width = width;

    PlotSeries *ptr = e.plot.get();
    m_entries.push_back(std::move(e));
    m_plot->addSeries(ptr);
    syncSeries();
    m_plot->update();
}

void NativeChartBackend::styleSeries(QXYSeries *s, const QColor &color, qreal width)
{
    if (Entry *e = findEntry(s)) {
        e->plot->color = color;
        if (e->plot->type == PlotSeriesType::Line) e->plot->width = width;
        m_plot->update();
    }
}

void NativeChartBackend::setSeriesLineStyle(QXYSeries *s, Qt::PenStyle style)
{
    if (Entry *e = findEntry(s)) {
        e->plot->style = style;
        m_plot->update();
    }
}

void NativeChartBackend::removeSeries(QXYSeries *s)
{
    for (auto it = m_entries.begin(); it != m_entries.end(); ++it) {
        if (it->qt == s) {
            m_plot->removeSeries(it->plot.get());
            m_entries.erase(it);
            m_plot->update();
            return;
        }
    }
}

bool NativeChartBackend::hasSeries(QXYSeries *s) const
{
    for (const auto &e : m_entries)
        if (e.qt == s) return true;
    return false;
}

void NativeChartBackend::setTLabel(const QString &tlabel)
{
    if (m_plot) m_plot->setTitle(tlabel);
}

QString NativeChartBackend::getTLabel() const
{
    return m_plot ? m_plot->title() : QString();
}

void NativeChartBackend::setXLabel(const QString &xlabel)
{
    xaxis->setTitleText(xlabel);
    m_plot->setXTitle(xlabel);
}

void NativeChartBackend::setXLabelFormat(const QString &fmt)
{
    xaxis->setLabelFormat(fmt);
    m_plot->setXLabelFormat(fmt);
}

void NativeChartBackend::setYLabel(const QString &ylabel)
{
    yaxis->setTitleText(ylabel);
    m_plot->setYTitle(ylabel);
}

// Local Variables:
// c-basic-offset: 4
// End:
