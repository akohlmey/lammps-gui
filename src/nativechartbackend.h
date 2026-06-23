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

#ifndef NATIVECHARTBACKEND_H
#define NATIVECHARTBACKEND_H

#include "chartbackend.h"
#include "plotseries.h"

#include <memory>
#include <vector>

class PlotWidget;
class QValueAxis;

/**
 * @brief Native (QPainter) chart rendering backend
 *
 * Implements the ChartBackend interface with the self-contained PlotWidget
 * renderer, with no dependency on the QtCharts or QtGraphs rendering paths or
 * QML. ChartViewer still owns the Qt series objects (QLineSeries /
 * QScatterSeries) and mutates them directly; this backend mirrors each one into
 * a neutral PlotSeries that PlotWidget draws, re-syncing on every interface call
 * that can change what is shown. Two QValueAxis objects are kept to satisfy the
 * axis accessors and to receive setRange() from the range sliders, which is
 * forwarded to the renderer.
 */
class NativeChartBackend : public ChartBackend {
public:
    NativeChartBackend();
    ~NativeChartBackend() override;

    void init(QWidget *parent, const QString &title, QLineSeries *series) override;
    QWidget *widget() const override;
    QList<QAbstractAxis *> getAxes() const override;
    QValueAxis *xAxis() const override { return xaxis; }
    QValueAxis *yAxis() const override { return yaxis; }
    void resetZoom(double xmin, double xmax, double ymin, double ymax) override;
    void addSeries(QXYSeries *s, const QColor &color, qreal width) override;
    void styleSeries(QXYSeries *s, const QColor &color, qreal width) override;
    void setSeriesLineStyle(QXYSeries *s, Qt::PenStyle style) override;
    void setMarkerSize(QXYSeries *s, qreal size) override;
    void setReferenceLabel(QXYSeries *s, const QString &label) override;
    void removeSeries(QXYSeries *s) override;
    bool hasSeries(QXYSeries *s) const override;
    void setTLabel(const QString &tlabel) override;
    QString getTLabel() const override;
    void setXLabel(const QString &xlabel) override;
    void setXLabelFormat(const QString &fmt) override;
    void setYLabel(const QString &ylabel) override;

private:
    /** @brief One mirrored series: the Qt source (not owned) and its render model (owned) */
    struct Entry {
        QXYSeries *qt;                    ///< source series, owned by ChartViewer
        std::unique_ptr<PlotSeries> plot; ///< render model, referenced by PlotWidget
    };

    Entry *findEntry(QXYSeries *s);
    /** @brief Copy points, visibility, and name from each Qt series into its PlotSeries */
    void syncSeries();

    PlotWidget *m_plot;           ///< renderer; owned by the parent widget (Qt)
    QValueAxis *xaxis;            ///< X-axis; owned here (accessor + slider zoom target)
    QValueAxis *yaxis;            ///< Y-axis; owned here
    std::vector<Entry> m_entries; ///< registered series mirrors
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
