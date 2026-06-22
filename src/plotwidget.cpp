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

#include "plotwidget.h"

#include "plotaxismath.h"

#include <QFont>
#include <QFontMetricsF>
#include <QImage>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QRectF>

#include <algorithm>

namespace {

// layout constants (logical pixels)
constexpr double OUTER     = 10.0; ///< outer padding around the whole chart
constexpr double TITLE_GAP = 8.0;  ///< gap between a title and the adjacent labels/plot
constexpr double LABEL_GAP = 5.0;  ///< gap between tick labels and the axis
constexpr double TICK_LEN  = 5.0;  ///< length of axis tick marks

// gridline / frame colors, matched to the previous QtGraphs/QtCharts appearance
const QColor MAJOR_GRID(160, 160, 160);
const QColor MINOR_GRID(208, 208, 208);
const QColor FRAME_COLOR(80, 80, 80);

// Ensure a strictly positive span so coordinate mapping never divides by zero.
void ensureSpan(double &min, double &max)
{
    if (!(max > min)) {
        const double mid = 0.5 * (min + max);
        min              = mid - 0.5;
        max              = mid + 0.5;
    }
}

QString labelText(double value, const QString &format)
{
    return QString::fromStdString(PlotAxisMath::formatAxisLabel(value, format.toStdString()));
}

} // namespace

PlotWidget::PlotWidget(QWidget *parent) : QWidget(parent)
{
    // we paint our own (white) background
    setAutoFillBackground(false);
}

void PlotWidget::setTitle(const QString &title)
{
    m_title = title;
    update();
}

QString PlotWidget::title() const
{
    return m_title;
}

void PlotWidget::setXTitle(const QString &title)
{
    m_xaxis.title = title;
    update();
}

void PlotWidget::setYTitle(const QString &title)
{
    m_yaxis.title = title;
    update();
}

void PlotWidget::setXRange(double min, double max)
{
    m_xaxis.min = min;
    m_xaxis.max = max;
    update();
}

void PlotWidget::setYRange(double min, double max)
{
    m_yaxis.min = min;
    m_yaxis.max = max;
    update();
}

void PlotWidget::setXLabelFormat(const QString &fmt)
{
    m_xaxis.labelFormat = fmt;
    update();
}

void PlotWidget::setYLabelFormat(const QString &fmt)
{
    m_yaxis.labelFormat = fmt;
    update();
}

void PlotWidget::setGrid(bool major, bool minor)
{
    m_xaxis.gridVisible = m_yaxis.gridVisible = major;
    m_xaxis.minorGridVisible = m_yaxis.minorGridVisible = minor;
    update();
}

void PlotWidget::addSeries(const PlotSeries *series)
{
    if (series && !m_series.contains(series)) {
        m_series.append(series);
        update();
    }
}

void PlotWidget::removeSeries(const PlotSeries *series)
{
    if (m_series.removeAll(series) > 0) update();
}

bool PlotWidget::hasSeries(const PlotSeries *series) const
{
    return m_series.contains(series);
}

QImage PlotWidget::renderToImage(const QSize &size) const
{
    QImage image(size, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter p(&image);
    doRender(p, QRectF(QPointF(0.0, 0.0), QSizeF(size)));
    return image;
}

void PlotWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    doRender(p, QRectF(rect()));
}

QSize PlotWidget::sizeHint() const
{
    return {400, 300};
}

void PlotWidget::doRender(QPainter &p, const QRectF &target) const
{
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::TextAntialiasing, true);
    p.fillRect(target, Qt::white);

    const QFontMetricsF fm(p.font());
    const double labelH = fm.height();

    // bold font for the chart and axis titles
    QFont titleFont = p.font();
    titleFont.setBold(true);
    const QFontMetricsF fmT(titleFont);
    const double titleH = fmT.height();

    // axis ranges (guarded against a zero span)
    double xmin = m_xaxis.min, xmax = m_xaxis.max;
    double ymin = m_yaxis.min, ymax = m_yaxis.max;
    ensureSpan(xmin, xmax);
    ensureSpan(ymin, ymax);

    // major tick values for both axes
    const double xMajor              = PlotAxisMath::niceTickInterval(xmax - xmin);
    const double yMajor              = PlotAxisMath::niceTickInterval(ymax - ymin);
    const std::vector<double> xticks = PlotAxisMath::tickValues(xmin, xmax, xMajor);
    const std::vector<double> yticks = PlotAxisMath::tickValues(ymin, ymax, yMajor);

    // widest Y tick label drives the left margin
    double maxYLabelW = 0.0;
    for (double v : yticks)
        maxYLabelW = std::max(maxYLabelW, fm.horizontalAdvance(labelText(v, m_yaxis.labelFormat)));

    // margins
    const bool hasTitle  = !m_title.isEmpty();
    const bool hasXTitle = !m_xaxis.title.isEmpty();
    const bool hasYTitle = !m_yaxis.title.isEmpty();

    const double leftMargin =
        OUTER + (hasYTitle ? titleH + TITLE_GAP : 0.0) + maxYLabelW + LABEL_GAP + TICK_LEN;
    const double bottomMargin =
        OUTER + (hasXTitle ? titleH + TITLE_GAP : 0.0) + labelH + LABEL_GAP + TICK_LEN;
    const double topMargin = OUTER + (hasTitle ? titleH + TITLE_GAP : 0.5 * labelH);
    // leave room so the last X tick label is not clipped at the right edge
    double lastXLabelW = 0.0;
    if (!xticks.empty())
        lastXLabelW = fm.horizontalAdvance(labelText(xticks.back(), m_xaxis.labelFormat));
    const double rightMargin = OUTER + 0.5 * lastXLabelW;

    QRectF plot(target.left() + leftMargin, target.top() + topMargin,
                target.width() - leftMargin - rightMargin,
                target.height() - topMargin - bottomMargin);
    if (plot.width() <= 1.0 || plot.height() <= 1.0) return; // too small to draw

    // coordinate mapping
    auto mapX = [&](double vx) {
        return plot.left() + (vx - xmin) / (xmax - xmin) * plot.width();
    };
    auto mapY = [&](double vy) {
        return plot.bottom() - (vy - ymin) / (ymax - ymin) * plot.height();
    };

    // gridlines: dashed minor (lighter) first, then solid major (darker)
    auto drawVLines = [&](const std::vector<double> &vals, const QPen &pen) {
        p.setPen(pen);
        for (double v : vals) {
            const double x = mapX(v);
            if (x < plot.left() - 0.5 || x > plot.right() + 0.5) continue;
            p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        }
    };
    auto drawHLines = [&](const std::vector<double> &vals, const QPen &pen) {
        p.setPen(pen);
        for (double v : vals) {
            const double y = mapY(v);
            if (y < plot.top() - 0.5 || y > plot.bottom() + 0.5) continue;
            p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        }
    };

    QPen majorPen(MAJOR_GRID);
    majorPen.setWidth(0); // cosmetic 1px, solid
    QPen minorPen(MINOR_GRID);
    minorPen.setWidth(0);
    minorPen.setStyle(Qt::DashLine); // dashed, matching the QtCharts look

    const int xsub = std::max(0, m_xaxis.subTicks);
    const int ysub = std::max(0, m_yaxis.subTicks);
    if (m_xaxis.minorGridVisible && xsub > 0)
        drawVLines(PlotAxisMath::tickValues(xmin, xmax, xMajor / (xsub + 1)), minorPen);
    if (m_yaxis.minorGridVisible && ysub > 0)
        drawHLines(PlotAxisMath::tickValues(ymin, ymax, yMajor / (ysub + 1)), minorPen);
    if (m_xaxis.gridVisible) drawVLines(xticks, majorPen);
    if (m_yaxis.gridVisible) drawHLines(yticks, majorPen);

    // plot frame
    {
        QPen pen(FRAME_COLOR);
        pen.setWidth(0);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawRect(plot);
    }

    // ticks and tick labels
    p.setPen(Qt::black);
    for (double v : xticks) {
        const double x = mapX(v);
        if (x < plot.left() - 0.5 || x > plot.right() + 0.5) continue;
        p.drawLine(QPointF(x, plot.bottom()), QPointF(x, plot.bottom() + TICK_LEN));
        const QString lbl = labelText(v, m_xaxis.labelFormat);
        const double w    = fm.horizontalAdvance(lbl);
        p.drawText(QPointF(x - 0.5 * w, plot.bottom() + TICK_LEN + LABEL_GAP + fm.ascent()), lbl);
    }
    for (double v : yticks) {
        const double y = mapY(v);
        if (y < plot.top() - 0.5 || y > plot.bottom() + 0.5) continue;
        p.drawLine(QPointF(plot.left() - TICK_LEN, y), QPointF(plot.left(), y));
        const QString lbl = labelText(v, m_yaxis.labelFormat);
        const double w    = fm.horizontalAdvance(lbl);
        p.drawText(QPointF(plot.left() - TICK_LEN - LABEL_GAP - w,
                           y + 0.5 * fm.ascent() - 0.5 * fm.descent()),
                   lbl);
    }

    // titles are drawn bold
    p.setFont(titleFont);
    p.setPen(Qt::black);
    if (hasXTitle) {
        const double w = fmT.horizontalAdvance(m_xaxis.title);
        p.drawText(QPointF(plot.center().x() - 0.5 * w, target.bottom() - OUTER - fmT.descent()),
                   m_xaxis.title);
    }
    if (hasYTitle) {
        p.save();
        const double w = fmT.horizontalAdvance(m_yaxis.title);
        p.translate(target.left() + OUTER + fmT.ascent(), plot.center().y() + 0.5 * w);
        p.rotate(-90.0);
        p.drawText(QPointF(0.0, 0.0), m_yaxis.title);
        p.restore();
    }
    if (hasTitle) {
        const double w = fmT.horizontalAdvance(m_title);
        p.drawText(QPointF(plot.center().x() - 0.5 * w, target.top() + OUTER + fmT.ascent()),
                   m_title);
    }

    // series (clipped to the plot area)
    p.save();
    p.setClipRect(plot);
    for (const PlotSeries *s : m_series) {
        if (!s || !s->visible || s->points.isEmpty()) continue;
        if (s->type == PlotSeriesType::Line) {
            QPolygonF poly;
            poly.reserve(s->points.size());
            for (const QPointF &pt : s->points)
                poly << QPointF(mapX(pt.x()), mapY(pt.y()));
            QPen pen(s->color, s->width, s->style, Qt::RoundCap, Qt::RoundJoin);
            p.setPen(pen);
            p.setBrush(Qt::NoBrush);
            p.drawPolyline(poly);
        } else {
            const double r = 0.5 * s->markerSize;
            p.setPen(Qt::NoPen);
            p.setBrush(s->color);
            for (const QPointF &pt : s->points)
                p.drawEllipse(QPointF(mapX(pt.x()), mapY(pt.y())), r, r);
        }
    }
    p.restore();
}

// Local Variables:
// c-basic-offset: 4
// End:
