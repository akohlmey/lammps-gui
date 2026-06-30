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

#include "chartviewer.h"

#include "analysis.h"
#include "constants.h"
#include "customfunc.h"
#include "fitting.h"
#include "helpers.h"
#include "lammpsgui.h"
#include "leastsquares.h"
#include "plotdata.h"
#include "plotdatadialog.h"
#include "qaddon.h"
#include "rangeslider.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QPen>
#include <QScrollArea>

#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>
#include <QVariant>
#include <algorithm>

#include "plotwidget.h"

#include <cmath>

namespace {

// Set RangeSlider resolution to 1000 steps
constexpr int SLIDER_RANGE       = 1000;
constexpr double SLIDER_FRACTION = 1.0 / static_cast<double>(SLIDER_RANGE);
constexpr int LAYOUT_SPACING     = 6;

// Widen a (near) empty [lo, hi] range to a small symmetric/relative band so the
// axis is never degenerate. Shared by the X and Y branches of getMinMax().
void padEmptyRange(double &lo, double &hi)
{
    const double delta = hi - lo;
    if ((delta / ((hi == 0.0) ? 1.0 : hi)) < 1.0e-10) {
        if ((lo == 0.0) || (hi == 0.0)) {
            lo = -0.025;
            hi = 0.025;
        } else {
            lo -= 0.025 * fabs(lo);
            hi += 0.025 * fabs(hi);
        }
    }
}

// brush color index must be kept in sync with preferences

const QList<QBrush> mybrushes = {
    QBrush(QColor(0, 0, 0)),       // black
    QBrush(QColor(100, 150, 255)), // blue
    QBrush(QColor(255, 125, 125)), // red
    QBrush(QColor(100, 200, 100)), // green
    QBrush(QColor(120, 120, 120)), // grey
};

// Parse a "name=value, name=value, ..." string of nonlinear-fit parameters and
// their initial guesses into an ordered list. Sets *ok to false on any empty or
// malformed token (so the caller can report a usage hint).
QList<FitParam> parseFitParams(const QString &text, bool *ok)
{
    QList<FitParam> params;
    *ok                      = true;
    const QStringList tokens = text.split(',', Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        const QStringList kv = token.split('=');
        if (kv.size() != 2) {
            *ok = false;
            return {};
        }
        const QString name = kv[0].trimmed();
        bool valueOk       = false;
        const double value = kv[1].trimmed().toDouble(&valueOk);
        if (name.isEmpty() || !valueOk) {
            *ok = false;
            return {};
        }
        params.append(FitParam{name, value});
    }
    if (params.isEmpty()) *ok = false;
    return params;
}

} // namespace

// Forward declarations of the data-only column helpers (defined in the column
// rendering-pipeline namespace further down) that ChartWindow needs before that
// block appears; they are pure (no PlotWidget), so no other helper is required.
namespace {
bool appendColumnPoint(ChartColumn &col, double x, double y);
void setColumnData(ChartColumn &col, const QList<QPointF> &points);
void setColumnSmoothFlags(ChartColumn &col, bool doRaw, bool doSmooth, int window, int order);
} // namespace

/* -------------------------------------------------------------------- */

ChartViewer *ChartWindow::currentChart()
{
    // a single view, kept bound by changeChart() to the combo's current column
    return cols.empty() ? nullptr : viewer;
}

// position in `cols` of the column whose index matches the combo selection (-1 if none)
int ChartWindow::activeIndex() const
{
    const int choice = columns->currentData().toInt();
    for (std::size_t i = 0; i < cols.size(); ++i)
        if (cols[i]->index == choice) return static_cast<int>(i);
    return cols.empty() ? -1 : 0;
}

void ChartWindow::setProcessedLabel(const QString &label)
{
    if (active >= 0) cols[active]->procLabel = label;
    smooth->setItemText(1, label);
}

void ChartWindow::resetRangeSliders()
{
    // setLow/setHigh only repaint the handles; callers update the plot separately
    xrange->setLow(0);
    xrange->setHigh(SLIDER_RANGE);
    yrange->setLow(0);
    yrange->setHigh(SLIDER_RANGE);
}

void ChartWindow::applySliderWindow()
{
    if (cols.empty()) return;
    updateXRange(xrange->low(), xrange->high());
    updateYRange(yrange->low(), yrange->high());
}

ChartWindow::ChartWindow(const QString &_filename, LammpsGui *_lammpsgui, QWidget *parent) :
    QWidget(parent), lammpsgui(_lammpsgui), menu(new QMenuBar), file(new QMenu("&File")),
    saveAsAct(nullptr), copyAct(nullptr), exportCsvAct(nullptr), exportDatAct(nullptr),
    exportYamlAct(nullptr), closeAct(nullptr), stopAct(nullptr), quitAct(nullptr),
    addDataAct(nullptr), refLinesAct(nullptr), smooth(nullptr), window(nullptr), order(nullptr),
    chartTitle(nullptr), chartYlabel(nullptr), chartXlabel(nullptr), units(nullptr), norm(nullptr),
    filename(_filename), viewer(nullptr), active(-1)
{
    QSettings settings;
    auto *top  = new QVBoxLayout;
    auto *row1 = new QHBoxLayout;
    auto *row2 = new QHBoxLayout;
    top->addLayout(row1);
    top->addWidget(new QHline);
    top->addLayout(row2);
    top->addWidget(new QHline);
    row1->setSpacing(LAYOUT_SPACING);
    row2->setSpacing(LAYOUT_SPACING);
    top->setSpacing(LAYOUT_SPACING);

    menu->addMenu(file);
    menu->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);

    // workaround for incorrect highlight bug on macOS
    auto *dummy = new QPushButton(QIcon(), "");
    dummy->hide();

    // plot title and axis labels
    settings.beginGroup(Keys::GROUP_CHARTS);
    QString mytitle;
    if (lammpsgui) {
        // live simulation: use the configured title template
        mytitle = settings.value(Keys::TITLE, "Thermo: %f").toString().replace("%f", filename);
    } else {
        // standalone/plot mode: just the base filename, no "Thermo:" prefix
        mytitle = QFileInfo(filename).fileName();
    }
    chartTitle  = new QLineEdit(mytitle);
    chartYlabel = new QLineEdit("");
    if (!lammpsgui) chartXlabel = new QLineEdit("");

    // plot smoothing
    int smoothchoice = settings.value(Keys::SMOOTHCHOICE, 0).toInt();
    switch (smoothchoice) {
        case 0:
            doRaw    = true;
            doSmooth = false;
            break;
        case 1:
            doRaw    = false;
            doSmooth = true;
            break;
        case 2: // fallthrough
        default:
            doRaw    = true;
            doSmooth = true;
            break;
    }
    // list of choices must be kepy in sync with list in preferences
    smooth = new QComboBox;
    smooth->addItem("Raw");
    // the processed-series slot always holds the smoothed data ("Smooth"); a
    // post-process fit/function replaces it and overrides the label with its name
    smooth->addItem("Smooth");
    smooth->addItem("Both");
    smooth->setCurrentIndex(smoothchoice);
    window = new QSpinBox;
    window->setRange(Cfg::SMOOTH_WINDOW_MIN, Cfg::SMOOTH_WINDOW_MAX);
    window->setValue(settings.value(Keys::SMOOTHWINDOW, Cfg::SMOOTH_WINDOW_DEFAULT).toInt());
    window->setEnabled(doSmooth);
    window->setToolTip("Smoothing Window Size");
    order = new QSpinBox;
    order->setRange(Cfg::SMOOTH_ORDER_MIN, Cfg::SMOOTH_ORDER_MAX);
    order->setValue(settings.value(Keys::SMOOTHORDER, Cfg::SMOOTH_ORDER_DEFAULT).toInt());
    order->setEnabled(doSmooth);
    order->setToolTip("Smoothing Order");
    settings.endGroup();

    columns = new QComboBox;
    row1->addWidget(menu);
    row1->addWidget(dummy);
    row2->addWidget(dummy);
    row1->addWidget(new QLabel("Title:"));
    // in standalone plot mode give the title half the stretch to make room for X-Axis label
    row1->addWidget(chartTitle, lammpsgui ? 2 : 1);
    if (!lammpsgui) {
        row1->addWidget(new QLabel("X-Axis:"));
        row1->addWidget(chartXlabel, 1);
    }
    row1->addWidget(new QLabel("Y-Axis:"));
    row1->addWidget(chartYlabel, 1);
    auto *unitsLabel = new QLabel("Units:");
    row1->addWidget(unitsLabel);
    units = new QLabel("[lj]");
    units->setFrameStyle(QFrame::Panel | QFrame::Raised);
    row1->addWidget(units);
    auto *normLabel = new QLabel("Norm:");
    row1->addWidget(normLabel);
    norm = new QCheckBox("");
    norm->setChecked(false);
    norm->setEnabled(false);
    row1->addWidget(norm);
    // units and normalization are LAMMPS thermo settings we do not know when
    // plotting external data files (no live simulation), so hide them then
    if (!lammpsgui) {
        unitsLabel->hide();
        units->hide();
        normLabel->hide();
        norm->hide();
    }
    row1->addWidget(new QLabel(" Data:"));
    row1->addWidget(columns, 1);

    xrange = new RangeSlider;
    xrange->setMinimum(0);
    xrange->setMaximum(SLIDER_RANGE);
    xrange->setLow(0);
    xrange->setHigh(SLIDER_RANGE);
    xrange->setToolTip("Adjust x-axis data range");
    xrange->setTickPosition(QSlider::TicksBothSides);
    xrange->setTickInterval(100);
    yrange = new RangeSlider;
    yrange->setMinimum(0);
    yrange->setMaximum(SLIDER_RANGE);
    yrange->setLow(0);
    yrange->setHigh(SLIDER_RANGE);
    yrange->setToolTip("Adjust y-axis data range");
    yrange->setTickPosition(QSlider::TicksBothSides);
    yrange->setTickInterval(100);
    auto makeToolBtn = [](const QString &icon, const QString &tip) {
        auto *btn = new QPushButton(QIcon(icon), "");
        btn->setToolTip(tip);
        return btn;
    };
    auto *styleBtn = makeToolBtn(":/icons/preferences-desktop-personal.svg", "Chart Style...");
    auto *refBtn   = makeToolBtn(":/icons/reference-lines.svg", "Reference Lines...");
    auto *ppBtn    = makeToolBtn(":/icons/chart-smooth.svg", "Postprocess...");
    // square toolbar buttons with a snug, uniform icon (shared policy)
    styleToolButtons(toolButtonSize(styleBtn), {styleBtn, refBtn, ppBtn});
    settings.beginGroup(Keys::GROUP_CHARTS);
    legendPos       = static_cast<LegendPos>(settings.value(Keys::LEGEND, 0).toInt());
    double defRefPt = font().pointSizeF();
    if (defRefPt <= 0.0) defRefPt = 9.0; // pixel-size app fonts report <= 0 pt
    refLabelSize  = settings.value(Keys::REFLABELSIZE, defRefPt).toDouble();
    refLabelDist  = settings.value(Keys::REFLABELDIST, 4.0).toDouble();
    refLabelBoxed = settings.value(Keys::REFLABELBOX, false).toBool();
    settings.endGroup();
    connect(styleBtn, &QPushButton::clicked, this, &ChartWindow::changeStyle);
    connect(refBtn, &QPushButton::clicked, this, &ChartWindow::referenceLines);
    connect(ppBtn, &QPushButton::clicked, this, &ChartWindow::postProcess);
    row2->addWidget(styleBtn);
    row2->addWidget(refBtn);
    row2->addWidget(ppBtn);
    row2->addWidget(new QLabel("X:"));
    row2->addWidget(xrange);
    row2->addWidget(new QLabel("Y:"));
    row2->addWidget(yrange);
    row2->addWidget(new QLabel("Plot:"));
    row2->addWidget(smooth);
    row2->addWidget(new QLabel(" Smooth:"));
    row2->addWidget(window);
    row2->addWidget(order);
    saveAsAct = addMenuAction(file, "&Save Graph As...", ":/icons/document-save-as.svg", this,
                              &ChartWindow::saveAs);
    copyAct   = addMenuAction(file, "Copy &Graph to Clipboard", ":/icons/edit-copy.svg", this,
                              &ChartWindow::copy);
    copyAct->setShortcut(QKeySequence(QKeySequence::Copy));
    exportCsvAct  = addMenuAction(file, "&Export data to CSV...", ":/icons/csv-file-icon.svg", this,
                                  &ChartWindow::exportCsv);
    exportDatAct  = addMenuAction(file, "Export data to &Gnuplot...", ":/icons/txt-file-icon.svg",
                                  this, &ChartWindow::exportDat);
    exportYamlAct = addMenuAction(file, "Export data to &YAML...", ":/icons/yaml-file-icon.svg",
                                  this, &ChartWindow::exportYaml);
    file->addSeparator();
    addMenuAction(file, "Chart &Style...", ":/icons/preferences-desktop-personal.svg", this,
                  &ChartWindow::changeStyle);
    refLinesAct = addMenuAction(file, "&Reference Lines...", ":/icons/reference-lines.svg", this,
                                &ChartWindow::referenceLines);
    addMenuAction(file, "&Postprocess...", ":/icons/chart-smooth.svg", this,
                  &ChartWindow::postProcess);
    // "Add Data from File..." is only relevant in standalone file-plot mode
    if (!lammpsgui) {
        addDataAct = addMenuAction(file, "&Add Data from File...", ":/icons/application-plot.svg",
                                   this, &ChartWindow::addDataFile);
    }
    file->addSeparator();
    stopAct =
        addMenuAction(file, "Stop &Run", ":/icons/process-stop.svg", this, &ChartWindow::stopRun);
    stopAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    // without a live simulation there is nothing to stop
    if (!lammpsgui) stopAct->setVisible(false); // no live simulation to stop
    closeAct = addMenuAction(file, "&Close", ":/icons/window-close.svg", this, &QWidget::close);
    closeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    quitAct =
        addMenuAction(file, "&Quit", ":/icons/application-exit.svg", this, &ChartWindow::quit);
    quitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    if (!lammpsgui) quitAct->setVisible(false); // quit == close in standalone mode
    auto *layout = new QVBoxLayout;
    layout->addLayout(top);
    layout->setSpacing(LAYOUT_SPACING);
    // the single shared chart view; it renders whichever column is active
    viewer = new ChartViewer;
    viewer->setLegendPos(legendPos);
    viewer->setRefLabelStyle(refLabelSize, refLabelDist, refLabelBoxed);
    layout->addWidget(viewer);
    setLayout(layout);

    connect(chartTitle, &QLineEdit::editingFinished, this, &ChartWindow::updateTLabel);
    connect(chartYlabel, &QLineEdit::editingFinished, this, &ChartWindow::updateYLabel);
    if (chartXlabel)
        connect(chartXlabel, &QLineEdit::editingFinished, this, &ChartWindow::updateXLabel);
    connect(smooth, &QComboBox::currentIndexChanged, this, &ChartWindow::selectSmooth);
    connect(window, &QAbstractSpinBox::editingFinished, this, &ChartWindow::updateSmooth);
    connect(order, &QAbstractSpinBox::editingFinished, this, &ChartWindow::updateSmooth);
    connect(window, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChartWindow::updateSmooth);
    connect(order, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChartWindow::updateSmooth);
    connect(columns, &QComboBox::currentIndexChanged, this, &ChartWindow::changeChart);
    connect(xrange, &RangeSlider::sliderMoved, this, &ChartWindow::updateXRange);
    connect(yrange, &RangeSlider::sliderMoved, this, &ChartWindow::updateYRange);

    applyWindowFlags(this);
    installEventFilter(this);
    resize(settings.value(Keys::CHARTX, Cfg::CHART_DEFAULT_WIDTH).toInt(),
           settings.value(Keys::CHARTY, Cfg::CHART_DEFAULT_HEIGHT).toInt());
}

int ChartWindow::getStep() const
{
    if (!cols.empty()) {
        const auto &series = cols[0]->series;
        if (series && series->count() > 0)
            return static_cast<int>(series->at(series->count() - 1).x());
    }
    return -1;
}

void ChartWindow::resetCharts()
{
    viewer->setColumn(nullptr); // unregister the active column's series from the plot
    cols.clear();
    columns->clear();
    active = -1;
}

void ChartWindow::resetZoom()
{
    if (!cols.empty()) viewer->resetZoom();
}

void ChartWindow::addChart(const QString &title, int index)
{
    auto c          = std::make_unique<ChartColumn>();
    c->index        = index;
    c->series       = std::make_unique<PlotSeries>();
    c->series->name = title;
    c->yTitle       = title;
    c->lastUpdate   = QTime::currentTime();
    cols.push_back(std::move(c));
    columns->addItem(title, index);
    columns->show();
    if (cols.size() == 1) {
        // first column: make it active, seed the Y-label field, and bind the view
        active = 0;
        chartYlabel->setText(title);
        viewer->setColumn(cols[0].get());
    }
    updateTLabel();
    selectSmooth(0);
}

void ChartWindow::addData(int step, double data, int index)
{
    for (std::size_t i = 0; i < cols.size(); ++i) {
        if (cols[i]->index != index) continue;
        if (static_cast<int>(i) == active)
            viewer->addPoint(step, data); // appends + throttled redraw of the active column
        else
            appendColumnPoint(*cols[i], step, data); // accumulate only; drawn when selected
        return;
    }
}

void ChartWindow::setUnits(const QString &_units)
{
    units->setText(_units);
}

void ChartWindow::setNorm(bool _norm)
{
    norm->setChecked(_norm);
}

void ChartWindow::setRangeEnabled(bool enabled)
{
    xrange->setEnabled(enabled);
    yrange->setEnabled(enabled);
    smooth->setEnabled(enabled);
    window->setEnabled(enabled && doSmooth);
    order->setEnabled(enabled && doSmooth);
}

void ChartWindow::loadData(const PlotData &data, int xcol, const QList<int> &ycols)
{
    resetCharts();
    if (data.isEmpty() || ycols.isEmpty()) return;
    if ((xcol < 0) || (xcol >= data.columnCount())) return;

    const std::vector<double> &xvals = data.column(xcol);
    const int nrow                   = data.rowCount();
    const QString xlabel             = data.columnName(xcol);

    int idx = 0;
    for (int ycol : ycols) {
        if ((ycol < 0) || (ycol >= data.columnCount())) continue;
        addChart(data.columnName(ycol), idx); // the first one binds the view
        const std::vector<double> &yvals = data.column(ycol);
        QList<QPointF> points;
        points.reserve(nrow);
        for (int r = 0; r < nrow; ++r)
            points.append(QPointF(xvals[r], yvals[r]));
        setColumnData(*cols.back(), points); // data only; the active one is drawn below
        ++idx;
    }
    // shared X-axis labeling on the single plot (standalone uses %.6g)
    viewer->setXLabel(xlabel);
    viewer->setXLabelFormat("%.6g");
    // now that data is loaded, (re)render the active column
    if (!cols.empty()) viewer->setColumn(cols[active >= 0 ? active : 0].get());
    if (!data.units().isEmpty()) setUnits(data.units());
    // pre-fill the X-axis label field in standalone plot mode
    if (chartXlabel) chartXlabel->setText(xlabel);
    setRangeEnabled(true);
    resetZoom();
}

void ChartWindow::copy()
{
#if QT_CONFIG(clipboard)
    auto *clip = QGuiApplication::clipboard();
    if (clip && !cols.empty()) {
        // a single view renders the active column
        QWidget *graph = viewer;
        if (graph) {
            auto image = graph->grab().toImage();
            if (!image.isNull()) {
                clip->setImage(image, QClipboard::Clipboard);
                if (clip->supportsSelection()) clip->setImage(image, QClipboard::Selection);
                return;
            }
        }
    }
    fprintf(stderr, "Copy graph to clipboard currently not available\n");
#else
    fprintf(stderr, "Copy graph to clipboard not supported on this platform\n");
#endif
}

void ChartWindow::quit()
{
    // in the live chart window Quit exits the whole application; a standalone
    // file-plot window (no LammpsGui) has nothing to quit, so just close it
    if (lammpsgui)
        lammpsgui->quit();
    else
        close();
}

void ChartWindow::stopRun()
{
    if (lammpsgui) lammpsgui->stopRun();
}

void ChartWindow::changeStyle()
{
    // the single view is bound to the currently selected column
    ChartViewer *chart = currentChart();
    if (!chart) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Chart Style");
    auto *layout = new QVBoxLayout(&dialog);

    // build a colored push button that edits the referenced color in place
    auto colorButton = [&dialog](QColor &chosen) {
        auto *btn        = new QPushButton;
        auto setBtnColor = [btn](const QColor &c) {
            btn->setText(c.name());
            btn->setStyleSheet(QString("background-color: %1; color: %2;")
                                   .arg(c.name(), (c.lightness() < 128) ? "white" : "black"));
        };
        setBtnColor(chosen);
        QObject::connect(btn, &QPushButton::clicked, &dialog, [&chosen, btn, setBtnColor]() {
            const QColor c = QColorDialog::getColor(chosen, btn, "Series Color");
            if (c.isValid()) {
                chosen = c;
                setBtnColor(c);
            }
        });
        return btn;
    };

    // build a display-mode selector preset to the given mode
    auto modeBox = [](ChartDisplayMode mode) {
        auto *mb = new QComboBox;
        mb->addItem("Lines", static_cast<int>(ChartDisplayMode::Lines));
        mb->addItem("Points", static_cast<int>(ChartDisplayMode::Points));
        mb->addItem("Lines + Points", static_cast<int>(ChartDisplayMode::LinesAndPoints));
        mb->setCurrentIndex(static_cast<int>(mode));
        return mb;
    };

    // build a line-width spin box preset to the given width
    auto widthBox = [](qreal width) {
        auto *w = new QDoubleSpinBox;
        w->setRange(0.5, 20.0);
        w->setSingleStep(0.5);
        w->setValue(width);
        return w;
    };

    // build a point-diameter spin box preset to the given size
    auto pointBox = [](qreal size) {
        auto *w = new QDoubleSpinBox;
        w->setRange(1.0, 40.0);
        w->setSingleStep(1.0);
        w->setValue(size);
        return w;
    };

    // raw data section
    QColor rawChosen = chart->displayColor();
    if (!rawChosen.isValid()) rawChosen = QColor(100, 150, 255);
    auto *rawMode      = modeBox(chart->displayMode());
    auto *rawColorBtn  = colorButton(rawChosen);
    auto *rawWidthSpin = widthBox(chart->displayWidth());
    auto *rawPointSpin = pointBox(chart->displayPointSize());
    auto *rawBox       = new QGroupBox("Raw data");
    auto *rawForm      = new QFormLayout(rawBox);
    rawForm->addRow("Display:", rawMode);
    rawForm->addRow("Color:", rawColorBtn);
    rawForm->addRow("Line width:", rawWidthSpin);
    rawForm->addRow("Point size:", rawPointSpin);
    layout->addWidget(rawBox);

    // processed data section
    QColor procChosen = chart->smoothColor();
    if (!procChosen.isValid()) procChosen = QColor(255, 125, 125);
    auto *procMode      = modeBox(chart->smoothMode());
    auto *procColorBtn  = colorButton(procChosen);
    auto *procWidthSpin = widthBox(chart->smoothWidth());
    auto *procPointSpin = pointBox(chart->smoothPointSize());
    auto *procBox       = new QGroupBox("Processed data");
    auto *procForm      = new QFormLayout(procBox);
    procForm->addRow("Display:", procMode);
    procForm->addRow("Color:", procColorBtn);
    procForm->addRow("Line width:", procWidthSpin);
    procForm->addRow("Point size:", procPointSpin);
    layout->addWidget(procBox);

    // in-plot legend section
    auto *legendCombo = new QComboBox;
    legendCombo->addItem("Off", static_cast<int>(LegendPos::Off));
    legendCombo->addItem("Top left", static_cast<int>(LegendPos::TopLeft));
    legendCombo->addItem("Top right", static_cast<int>(LegendPos::TopRight));
    legendCombo->addItem("Bottom right", static_cast<int>(LegendPos::BottomRight));
    legendCombo->addItem("Bottom left", static_cast<int>(LegendPos::BottomLeft));
    const int legendIdx = legendCombo->findData(static_cast<int>(legendPos));
    legendCombo->setCurrentIndex(legendIdx < 0 ? 0 : legendIdx);
    auto *legendBox  = new QGroupBox("Legend");
    auto *legendForm = new QFormLayout(legendBox);
    legendForm->addRow("Placement:", legendCombo);
    layout->addWidget(legendBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    styleDialogButtons(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        chart->setDisplayStyle(static_cast<ChartDisplayMode>(rawMode->currentData().toInt()),
                               rawChosen, rawWidthSpin->value(), rawPointSpin->value());
        chart->setSmoothStyle(static_cast<ChartDisplayMode>(procMode->currentData().toInt()),
                              procChosen, procWidthSpin->value(), procPointSpin->value());
        legendPos = static_cast<LegendPos>(legendCombo->currentData().toInt());
        if (viewer) viewer->setLegendPos(legendPos);
        QSettings settings;
        settings.beginGroup(Keys::GROUP_CHARTS);
        settings.setValue(Keys::LEGEND, static_cast<int>(legendPos));
        settings.endGroup();
        // a style change is view-only: restore the slider window the setters reset
        applySliderWindow();
    }
}

void ChartWindow::postProcess()
{
    // the single view is bound to the currently selected column
    ChartViewer *chart = currentChart();
    if (!chart) return;

    const int npoints = chart->getCount();
    if (npoints < 2) {
        warning(this, "Postprocess", "Not enough data points to analyze.");
        return;
    }

    // pre-compute x range for fit-range spinbox initialization
    double dataXmin = chart->getStep(0), dataXmax = chart->getStep(0);
    for (int i = 1; i < npoints; ++i) {
        const double x = chart->getStep(i);
        if (x < dataXmin) dataXmin = x;
        if (x > dataXmax) dataXmax = x;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Postprocess Chart Data");
    auto *form = new QFormLayout(&dialog);

    auto *analysisbox = new QComboBox;
    analysisbox->addItem("Autocorrelation");
    analysisbox->addItem("Polynomial fit");
    analysisbox->addItem("Birch-Murnaghan EOS fit");
    analysisbox->addItem("Custom function");
    analysisbox->addItem("Custom fit");
    form->addRow("Analysis:", analysisbox);

    auto *paramLabel = new QLabel;
    auto *paramSpin  = new QSpinBox;
    form->addRow(paramLabel, paramSpin);

    // expression field, shown for both the custom-function plot and fit
    auto *exprLabel = new QLabel("f(x) =");
    auto *exprEdit  = new QLineEdit;
    exprEdit->setPlaceholderText("e.g. 2*x^2 + 3*sin(x)");
    exprEdit->setMinimumWidth(Cfg::POSTPROCESS_EXPR_WIDTH);
    form->addRow(exprLabel, exprEdit);

    // parameter (initial-guess) and label fields, shown only for the custom fit
    auto *paramsLabel = new QLabel("Parameters:");
    auto *paramsEdit  = new QLineEdit;
    paramsEdit->setPlaceholderText("name=guess, e.g. a=1, b=0.5");
    paramsEdit->setMinimumWidth(Cfg::POSTPROCESS_EXPR_WIDTH);
    form->addRow(paramsLabel, paramsEdit);

    auto *fitLabelLabel = new QLabel("Label:");
    auto *fitLabelEdit  = new QLineEdit;
    fitLabelEdit->setPlaceholderText("optional name for the fitted curve");
    fitLabelEdit->setMinimumWidth(Cfg::POSTPROCESS_EXPR_WIDTH);
    form->addRow(fitLabelLabel, fitLabelEdit);

    // fit x-range (hidden for autocorrelation, shown for all fitting analyses)
    auto *fitRangeLabel  = new QLabel("Fit x-range:");
    auto *fitRangeWidget = new QWidget;
    auto *fitRangeRow    = new QHBoxLayout(fitRangeWidget);
    fitRangeRow->setContentsMargins(0, 0, 0, 0);
    auto *fitFromSpin = new QDoubleSpinBox;
    fitFromSpin->setDecimals(6);
    fitFromSpin->setRange(-1e15, 1e15);
    fitFromSpin->setValue(dataXmin);
    auto *fitToSpin = new QDoubleSpinBox;
    fitToSpin->setDecimals(6);
    fitToSpin->setRange(-1e15, 1e15);
    fitToSpin->setValue(dataXmax);
    fitRangeRow->addWidget(new QLabel("from"));
    fitRangeRow->addWidget(fitFromSpin, 1);
    fitRangeRow->addWidget(new QLabel("to"));
    fitRangeRow->addWidget(fitToSpin, 1);
    form->addRow(fitRangeLabel, fitRangeWidget);

    // swap the parameter widgets to match the selected analysis
    auto configure = [=, &dialog](int idx) {
        const bool plot      = (idx == 3); // custom-function plotting
        const bool fit       = (idx == 4); // custom-function nonlinear fit
        const bool expr      = plot || fit;
        const bool eos       = (idx == 2);
        const bool showRange = (idx != 0); // show for all except autocorrelation
        exprLabel->setVisible(expr);
        exprEdit->setVisible(expr);
        paramsLabel->setVisible(fit);
        paramsEdit->setVisible(fit);
        fitLabelLabel->setVisible(fit);
        fitLabelEdit->setVisible(fit);
        fitRangeLabel->setVisible(showRange);
        fitRangeWidget->setVisible(showRange);
        paramLabel->setVisible(!expr && !eos);
        if (idx == 1) { // polynomial degree
            paramLabel->setText("Degree:");
            paramSpin->setVisible(true);
            paramSpin->setRange(1, qMin(npoints - 1, 8));
            paramSpin->setValue(qMin(3, qMin(npoints - 1, 8)));
        } else if (eos) { // EOS: only show the x-axis confirmation
            paramSpin->setVisible(false);
        } else if (expr) { // custom function/fit: expression field(s) only
            paramSpin->setVisible(false);
        } else { // autocorrelation max lag
            paramLabel->setText("Max lag:");
            paramSpin->setVisible(true);
            paramSpin->setRange(1, npoints - 1);
            paramSpin->setValue(qMin(npoints - 1, npoints / 2));
        }
        dialog.adjustSize();
    };
    configure(0);
    connect(analysisbox, &QComboBox::currentIndexChanged, &dialog, configure);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    styleDialogButtons(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() != QDialog::Accepted) return;

    // gather the (x, y) data of the selected chart
    std::vector<double> xs, ys;
    xs.reserve(npoints);
    ys.reserve(npoints);
    for (int i = 0; i < npoints; ++i) {
        xs.push_back(chart->getStep(i));
        ys.push_back(chart->getData(i));
    }

    const int which = analysisbox->currentIndex();

    // filter to the user-specified x-range for fitting analyses (not autocorrelation)
    if (which != 0) {
        const double fitXmin = fitFromSpin->value();
        const double fitXmax = fitToSpin->value();
        if (fitXmin < fitXmax) {
            std::vector<double> fxs, fys;
            for (std::size_t i = 0; i < xs.size(); ++i) {
                if (xs[i] >= fitXmin && xs[i] <= fitXmax) {
                    fxs.push_back(xs[i]);
                    fys.push_back(ys[i]);
                }
            }
            if (fxs.size() >= 2) {
                xs = std::move(fxs);
                ys = std::move(fys);
            } else {
                warning(this, "Postprocess",
                        "Fewer than 2 data points in the selected x-range; using full data.");
            }
        }
    }

    if (which == 0) { // autocorrelation -> new window (the abscissa becomes lag)
        const std::vector<double> acf = autocorrelation(ys, paramSpin->value());
        if (acf.empty()) {
            warning(this, "Postprocess",
                    "Could not compute the autocorrelation (constant or insufficient data).");
            return;
        }
        PlotData result;
        result.setColumnNames({"lag", "ACF: " + chart->getName()});
        for (std::size_t k = 0; k < acf.size(); ++k)
            result.appendRow({static_cast<double>(k), acf[k]});

        auto *win = new ChartWindow(filename + " (ACF)", nullptr);
        win->setAttribute(Qt::WA_DeleteOnClose);
        win->setWindowTitle("Autocorrelation - LAMMPS-GUI");
        win->setWindowIcon(QIcon(Cfg::MAIN_ICON));
        win->setMinimumSize(Cfg::MINIMUM_WIDTH, Cfg::MINIMUM_HEIGHT);
        win->loadData(result, 0, {1});
        win->show();
        return;
    }

    // fits: build a smooth curve over the data x range and overlay it
    const auto mm        = std::minmax_element(xs.begin(), xs.end());
    const double xmin    = *mm.first;
    const double xmax    = *mm.second;
    constexpr int Ncurve = 200;

    if (which == 3) { // custom function f(x) evaluated over the data x range
        const QString expr       = exprEdit->text().trimmed();
        const CustomCurve result = evalCustomCurve(expr, xmin, xmax, Ncurve);
        if (!result.ok) {
            warning(this, "Custom Function",
                    QString("Could not evaluate the expression:\n%1").arg(result.error));
            return;
        }
        if (result.points.size() < 2) {
            warning(this, "Custom Function",
                    "The expression did not produce a usable curve over the data range.");
            return;
        }
        chart->setFitCurve(result.points, expr, /* eosMode= */ true);
        setProcessedLabel("Custom f(x)");
        resetRangeSliders();        // a fit re-fits to the whole data set; match the sliders
        smooth->setCurrentIndex(2); // "Both" = raw data + function overlay
        information(this, "Custom Function",
                    QString("Plotted f(x) = %1\nover x in [%2, %3].")
                        .arg(expr)
                        .arg(xmin, 0, 'g', 6)
                        .arg(xmax, 0, 'g', 6));
        return;
    }

    if (which == 4) { // custom nonlinear least-squares fit of f(x) to the data
        const QString expr            = exprEdit->text().trimmed();
        bool paramsOk                 = false;
        const QList<FitParam> initial = parseFitParams(paramsEdit->text(), &paramsOk);
        if (!paramsOk) {
            warning(this, "Custom Fit",
                    "Enter fit parameters as name=guess pairs, e.g. \"a=1, b=0.5\".");
            return;
        }
        const CustomFit fit = fitCustomCurve(expr, initial, xs, ys, xmin, xmax, Ncurve);
        if (!fit.ok) {
            warning(this, "Custom Fit",
                    QString("The fit could not be completed:\n%1").arg(fit.error));
            return;
        }
        const QString label   = fitLabelEdit->text().trimmed();
        const QString fitName = label.isEmpty() ? expr : label;
        chart->setFitCurve(fit.curve, fitName, /* eosMode= */ true);
        setProcessedLabel(fitName.length() > 12 ? "Custom fit" : fitName);
        resetRangeSliders();        // a fit re-fits to the whole data set; match the sliders
        smooth->setCurrentIndex(2); // "Both" = raw data + fit overlay

        QString report = QString("Custom fit of  f(x) = %1\n").arg(expr);
        if (!label.isEmpty()) report += QString("(%1)\n").arg(label);
        report += "\n";
        for (const auto &p : fit.params)
            report += QString("  %1 = %2\n").arg(p.name).arg(p.value, 0, 'g', 8);
        report += QString("\n  RMS residual = %1\n  iterations   = %2")
                      .arg(fit.rms, 0, 'g', 6)
                      .arg(fit.iterations);
        information(this, "Custom Fit", report);
        return;
    }

    if (which == 1) { // polynomial fit
        const PolynomialFit f = polynomialFit(xs, ys, paramSpin->value());
        if (!f.ok) {
            warning(this, "Postprocess", "Polynomial fit failed (too few points).");
            return;
        }
        QList<QPointF> curve;
        for (int k = 0; k <= Ncurve; ++k) {
            const double x = xmin + (xmax - xmin) * k / Ncurve;
            curve.append(QPointF(x, evalPolynomial(f.coeffs, x)));
        }
        const QString polyName = QString("Poly deg %1").arg(static_cast<int>(f.coeffs.size()) - 1);
        chart->setFitCurve(curve, polyName, /* eosMode= */ true);
        setProcessedLabel(polyName);
        resetRangeSliders();        // a fit re-fits to the whole data set; match the sliders
        smooth->setCurrentIndex(2); // "Both" = raw data + fit overlay

        QString report =
            QString("Polynomial fit of degree %1\n\n").arg(static_cast<int>(f.coeffs.size()) - 1);
        for (int i = 0; i < static_cast<int>(f.coeffs.size()); ++i)
            report += QString("  c[%1] = %2\n").arg(i).arg(f.coeffs[i], 0, 'g', 8);
        report += QString("\n  RMS residual = %1").arg(f.rms, 0, 'g', 6);
        information(this, "Polynomial Fit", report);
        return;
    }

    // Birch-Murnaghan EOS fit: second dialog — confirm columns + atoms per unit cell
    {
        const QString xLabel = chart->getXLabel();
        const QString yLabel = chart->getYLabel().isEmpty() ? chart->getName() : chart->getYLabel();

        QDialog eosConfirm(this);
        eosConfirm.setWindowTitle("Birch-Murnaghan EOS Fit — Column Setup");
        auto *eosLayout = new QVBoxLayout(&eosConfirm);
        eosLayout->addWidget(new QLabel(
            "The Birch-Murnaghan EOS fit expects volume on the x-axis and cohesive energy "
            "on the y-axis.\n\nThis chart has:"));
        auto *eosInfo = new QFormLayout;
        eosInfo->addRow("x-axis:", new QLabel("<b>" + xLabel + "</b>"));
        eosInfo->addRow("y-axis:", new QLabel("<b>" + yLabel + "</b>"));
        eosLayout->addLayout(eosInfo);
        eosLayout->addWidget(
            new QLabel("\nAtoms per unit cell N: the lattice constant is derived as\n"
                       "  a₀ = ∛(N × V₀)\n"
                       "Use the conventional unit cell (e.g. N=4 for FCC, N=2 for BCC/HCP).\n"
                       "Set N=1 only when the x-axis is already the conventional cell volume."));
        auto *natSpin = new QSpinBox;
        natSpin->setRange(1, 1000);
        natSpin->setValue(1);
        natSpin->setToolTip("Number of atoms in the conventional unit cell\n"
                            "(e.g. 4 for FCC, 2 for BCC/HCP).\n"
                            "Use N=1 when x is already the conventional cell volume.");
        auto *natForm = new QFormLayout;
        natForm->addRow("Atoms per unit cell N:", natSpin);
        eosLayout->addLayout(natForm);
        auto *eosBtns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        styleDialogButtons(eosBtns);
        connect(eosBtns, &QDialogButtonBox::accepted, &eosConfirm, &QDialog::accept);
        connect(eosBtns, &QDialogButtonBox::rejected, &eosConfirm, &QDialog::reject);
        eosLayout->addWidget(eosBtns);
        if (eosConfirm.exec() != QDialog::Accepted) return;

        const int natoms = natSpin->value();
        const EosFit f   = birchMurnaghanFit(xs, ys);
        if (!f.ok) {
            warning(this, "Postprocess",
                    "Birch-Murnaghan fit failed (needs >= 4 points, positive volumes, "
                    "and a minimum within the data).");
            return;
        }

        QList<QPointF> curve;
        for (int k = 0; k <= Ncurve; ++k) {
            const double x = xmin + (xmax - xmin) * k / Ncurve;
            if (x > 0.0) curve.append(QPointF(x, evalBirchMurnaghan(f, x)));
        }
        // EOS fit: hide in Raw mode, visible in EOS-fit/Both modes; raw data as points
        chart->setFitCurve(curve, "EOS fit", /* eosMode= */ true);
        chart->setDisplayStyle(ChartDisplayMode::Points, chart->displayColor(),
                               chart->displayWidth(), chart->displayPointSize());
        setProcessedLabel("EOS fit");
        resetRangeSliders();        // a fit re-fits to the whole data set; match the sliders
        smooth->setCurrentIndex(2); // "Both" = raw points + EOS fit line

        // derive lattice constant: a0 = cbrt(N * V0)
        const double a0 = std::cbrt(static_cast<double>(natoms) * f.v0);

        // Show the result in a dialog with the rendered formula
        auto *resultDlg = new QDialog(this);
        resultDlg->setWindowTitle("Birch-Murnaghan EOS Fit");
        resultDlg->setAttribute(Qt::WA_DeleteOnClose);
        auto *dlgLayout = new QVBoxLayout(resultDlg);

        auto *fmtLabel = new QLabel;
        fmtLabel->setPixmap(QPixmap(":/icons/birch-murnaghan-eos.png"));
        fmtLabel->setAlignment(Qt::AlignCenter);
        dlgLayout->addWidget(fmtLabel);

        auto *legend = new QLabel("where <i>V</i> is the unit cell volume "
                                  "and <i>V</i><sub>0</sub> the equilibrium volume.");
        legend->setAlignment(Qt::AlignCenter);
        dlgLayout->addWidget(legend);

        auto *resultForm = new QFormLayout;
        auto makeVal     = [](double v, int prec) {
            auto *l = new QLabel(QString::number(v, 'g', prec));
            l->setTextInteractionFlags(Qt::TextSelectableByMouse);
            return l;
        };
        resultForm->addRow("<b>V<sub>0</sub></b> &mdash; Equilibrium volume (from fit):",
                           makeVal(f.v0, 8));
        resultForm->addRow(
            QString("<b>a<sub>0</sub></b> &mdash; Lattice constant ∛(%1 &times; V<sub>0</sub>):")
                .arg(natoms),
            makeVal(a0, 8));
        resultForm->addRow("<b>E<sub>0</sub></b> &mdash; Cohesive energy at V<sub>0</sub>:",
                           makeVal(f.e0, 8));
        resultForm->addRow("<b>B<sub>0</sub></b> &mdash; Bulk modulus (&minus;V<sub>0</sub> dP/dV "
                           "at V<sub>0</sub>):",
                           makeVal(f.b0, 8));
        resultForm->addRow("<b>B<sub>0</sub>'</b> &mdash; Pressure derivative dB/dP at P=0:",
                           makeVal(f.b0prime, 6));
        resultForm->addRow("RMS residual:", makeVal(f.rms, 6));
        dlgLayout->addLayout(resultForm);

        auto *closeBtn = new QDialogButtonBox(QDialogButtonBox::Ok);
        styleDialogButtons(closeBtn);
        connect(closeBtn, &QDialogButtonBox::accepted, resultDlg, &QDialog::accept);
        dlgLayout->addWidget(closeBtn);
        resultDlg->exec();
    }
}

void ChartWindow::addDataFile()
{
    if (cols.empty()) return;

    const QString fileName = QFileDialog::getOpenFileName(
        this, "Add Data from File", QString(),
        "Data files (*.dat *.csv *.yaml *.yml *.json *.txt);;All files (*)");
    if (fileName.isEmpty()) return;

    QString error;
    PlotData data = loadPlotData(fileName, &error);
    if (data.isEmpty()) {
        critical(this, "Add Data from File",
                 "Could not read data from file:", error.isEmpty() ? fileName : error);
        return;
    }

    PlotDataDialog dialog(data, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const PlotData plotData = dialog.buildData();
    const QList<int> ycols  = dialog.yColumns();
    const int xcol          = dialog.xColumn();
    if (ycols.isEmpty() || xcol < 0 || xcol >= plotData.columnCount()) return;

    ChartViewer *chart = currentChart();
    if (!chart) return;

    const std::vector<double> &xvals = plotData.column(xcol);
    const int nrow                   = plotData.rowCount();

    // auto-color palette for overlay series (avoids primary raw/smooth colors)
    static const QList<QColor> palette = {
        QColor(220, 80, 40),  // red-orange
        QColor(40, 160, 40),  // green
        QColor(160, 40, 220), // purple
        QColor(180, 140, 0),  // amber
        QColor(0, 160, 180),  // teal
    };
    int colorIdx = chart->overlaySeriesCount();

    for (int ycol : ycols) {
        if (ycol < 0 || ycol >= plotData.columnCount()) continue;
        QList<QPointF> pts;
        pts.reserve(nrow);
        const std::vector<double> &yvals = plotData.column(ycol);
        for (int r = 0; r < nrow; ++r)
            pts.append(QPointF(xvals[r], yvals[r]));
        chart->addOverlaySeries(pts, plotData.columnName(ycol), palette[colorIdx % palette.size()]);
        ++colorIdx;
    }
    // new data was added (and re-fit to the full range): match the sliders to it
    resetRangeSliders();
}

void ChartWindow::referenceLines()
{
    if (cols.empty()) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Reference Lines");
    dialog.setMinimumWidth(680); // room for the label field plus the anchor selector
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(
        new QLabel("Reference lines (vertical at an x value or horizontal at a y value) are\n"
                   "applied to every chart. Labels are drawn next to the line."));

    // scrollable list of (x, label, color) rows
    auto *listWidget = new QWidget;
    auto *listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(4, 4, 4, 4);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(listWidget);
    scroll->setMinimumHeight(100);
    // keep rows within the viewport width; only scroll vertically as lines are added
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    layout->addWidget(scroll, 1);

    // helper to build one color-button (same pattern as changeStyle)
    struct RowData {
        QComboBox *orientCombo;
        QDoubleSpinBox *xSpin;
        QLineEdit *labelEdit;
        QComboBox *anchorCombo;
        QColor color;
    };
    QList<RowData *> rows;
    QList<QPushButton *> colorBtns;

    auto addRow = [&](RefOrient orient, double val, const QString &lbl, const QColor &col,
                      RefAnchor anchor) {
        auto *rd        = new RowData;
        rd->orientCombo = new QComboBox;
        rd->orientCombo->addItems({"Vertical", "Horizontal"});
        rd->orientCombo->setCurrentIndex(orient == RefOrient::Horizontal ? 1 : 0);
        rd->xSpin = new QDoubleSpinBox;
        rd->xSpin->setDecimals(6);
        rd->xSpin->setRange(-1e15, 1e15);
        rd->xSpin->setValue(val);
        // keep the value field compact so the label field has room
        rd->xSpin->setMaximumWidth(110);
        rd->labelEdit = new QLineEdit(lbl);
        rd->labelEdit->setPlaceholderText("label");
        rd->color = col.isValid() ? col : QColor(80, 80, 80);

        // position label tracks the orientation: "x =" for vertical, "y =" for horizontal
        auto *posLabel = new QLabel;
        auto updatePos = [posLabel](int idx) {
            posLabel->setText(idx == 1 ? "y =" : "x =");
        };
        updatePos(rd->orientCombo->currentIndex());
        QObject::connect(rd->orientCombo, &QComboBox::currentIndexChanged, &dialog, updatePos);

        // label anchor along the line; the item texts track the orientation
        rd->anchorCombo = new QComboBox;
        rd->anchorCombo->addItem("Top", static_cast<int>(RefAnchor::Start));
        rd->anchorCombo->addItem("Center", static_cast<int>(RefAnchor::Center));
        rd->anchorCombo->addItem("Bottom", static_cast<int>(RefAnchor::End));
        rd->anchorCombo->setCurrentIndex(static_cast<int>(anchor));
        auto *anchorCombo = rd->anchorCombo;
        auto updateAnchor = [anchorCombo](int idx) {
            const bool horiz = (idx == 1);
            anchorCombo->setItemText(0, horiz ? "Left" : "Top");
            anchorCombo->setItemText(2, horiz ? "Right" : "Bottom");
        };
        updateAnchor(rd->orientCombo->currentIndex());
        QObject::connect(rd->orientCombo, &QComboBox::currentIndexChanged, &dialog, updateAnchor);

        auto *colorBtn = new QPushButton;
        auto updateBtn = [colorBtn](const QColor &c) {
            colorBtn->setText(c.name());
            colorBtn->setStyleSheet(QString("background-color: %1; color: %2;")
                                        .arg(c.name(), c.lightness() < 128 ? "white" : "black"));
        };
        updateBtn(rd->color);
        QObject::connect(colorBtn, &QPushButton::clicked, &dialog, [rd, colorBtn, updateBtn]() {
            const QColor c = QColorDialog::getColor(rd->color, colorBtn, "Line Color");
            if (c.isValid()) {
                rd->color = c;
                updateBtn(c);
            }
        });

        auto *delBtn = new QPushButton("×");
        delBtn->setFixedWidth(24);

        auto *row = new QHBoxLayout;
        row->addWidget(rd->orientCombo);
        row->addWidget(posLabel);
        row->addWidget(rd->xSpin, 0);
        row->addWidget(new QLabel("Label:"));
        row->addWidget(rd->labelEdit, 1);
        row->addWidget(new QLabel("Pos:"));
        row->addWidget(rd->anchorCombo);
        row->addWidget(new QLabel("Color:"));
        row->addWidget(colorBtn);
        row->addWidget(delBtn);
        listLayout->addLayout(row);

        rows.append(rd);
        colorBtns.append(colorBtn);

        // remove this row when "×" is clicked
        QObject::connect(delBtn, &QPushButton::clicked, &dialog,
                         [rd, &rows, &colorBtns, colorBtn, row]() {
                             rows.removeOne(rd);
                             colorBtns.removeOne(colorBtn);
                             // hide all widgets in the row
                             QLayoutItem *item;
                             while ((item = row->takeAt(0)) != nullptr) {
                                 if (item->widget()) item->widget()->hide();
                                 delete item;
                             }
                             delete row;
                         });
    };

    // populate with existing lines
    for (const auto &rl : refLines)
        addRow(rl.orient, rl.value, rl.label, rl.color, rl.anchor);

    auto *addBtn = new QPushButton("Add line");
    QObject::connect(addBtn, &QPushButton::clicked, &dialog, [&]() {
        addRow(RefOrient::Vertical, 0.0, QString(), QColor(80, 80, 80), RefAnchor::Start);
    });
    layout->addWidget(addBtn);

    // window-wide label style: font size, gap from the line, and a framed/opaque background
    auto *styleRow = new QHBoxLayout;
    auto *fontSpin = new QDoubleSpinBox;
    fontSpin->setRange(5.0, 30.0);
    fontSpin->setSingleStep(0.5);
    fontSpin->setValue(refLabelSize);
    auto *distSpin = new QSpinBox;
    distSpin->setRange(0, 50);
    distSpin->setValue(static_cast<int>(refLabelDist));
    auto *boxedCheck = new QCheckBox("Boxed labels");
    boxedCheck->setChecked(refLabelBoxed);
    styleRow->addWidget(new QLabel("Label font:"));
    styleRow->addWidget(fontSpin);
    styleRow->addWidget(new QLabel("Gap:"));
    styleRow->addWidget(distSpin);
    styleRow->addWidget(boxedCheck);
    styleRow->addStretch(1);
    layout->addLayout(styleRow);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    styleDialogButtons(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return;

    // rebuild the refLines list (window-wide) and apply to the active column;
    // changeChart re-applies them when switching to another column
    refLines.clear();
    for (const auto *rd : rows) {
        const RefOrient o = rd->orientCombo->currentIndex() == 1 ? RefOrient::Horizontal
                                                                 : RefOrient::Vertical;
        const auto a      = static_cast<RefAnchor>(rd->anchorCombo->currentData().toInt());
        refLines.append({o, rd->xSpin->value(), rd->labelEdit->text().trimmed(), rd->color, a});
    }

    // store and apply the window-wide label style
    refLabelSize  = fontSpin->value();
    refLabelDist  = distSpin->value();
    refLabelBoxed = boxedCheck->isChecked();
    if (viewer) viewer->setRefLabelStyle(refLabelSize, refLabelDist, refLabelBoxed);
    QSettings rls;
    rls.beginGroup(Keys::GROUP_CHARTS);
    rls.setValue(Keys::REFLABELSIZE, refLabelSize);
    rls.setValue(Keys::REFLABELDIST, refLabelDist);
    rls.setValue(Keys::REFLABELBOX, refLabelBoxed);
    rls.endGroup();

    if (!cols.empty()) viewer->setReferenceLines(refLines);
}

void ChartWindow::selectSmooth(int)
{
    switch (smooth->currentIndex()) {
        case 0:
            doRaw    = true;
            doSmooth = false;
            break;
        case 1:
            doRaw    = false;
            doSmooth = true;
            break;
        case 2: // fallthrough
        default:
            doRaw    = true;
            doSmooth = true;
            break;
    }
    // the processed-slot label does not depend on the Raw/Smooth/Both choice; it
    // is "Smooth" unless a post-process fit overrode it (set in postProcess and
    // restored on column switch in changeChart)
    const bool isEos = currentChart() && currentChart()->isEosFit();
    // SG smooth parameters are only relevant when smoothing without a fit overlay
    const bool sgEnabled = doSmooth && !isEos;
    window->setEnabled(sgEnabled);
    order->setEnabled(sgEnabled);
    updateSmooth();
    // toggling Raw/Smooth/Both is a view-only change: keep the current slider
    // window, just re-derive the displayed range from it (the data range may have
    // grown/shrunk as the smoothed series was shown/hidden)
    applySliderWindow();
}

void ChartWindow::updateSmooth()
{
    int wval = window->value();
    int oval = order->value();

    // update every column's flags (so a hidden column is correct when selected),
    // but only the active column is on the plot and needs a redraw
    for (auto &c : cols)
        setColumnSmoothFlags(*c, doRaw, doSmooth, wval, oval);
    if (!cols.empty()) viewer->updateSmooth();
}

void ChartWindow::updateTLabel()
{
    // the chart title is shared by all columns on the single plot
    if (chartTitle && !cols.empty()) viewer->setTLabel(chartTitle->text());
}

void ChartWindow::updateYLabel()
{
    // the Y-axis label is per-column; update the active column and remember it
    if (active >= 0) {
        const QString label  = chartYlabel->text();
        cols[active]->yTitle = label;
        // The in-plot legend labels the raw series by its name; keep that in
        // sync with the editable Y-axis title rather than the fixed thermo
        // column id.  The raw points share the line's name so the two dedup
        // into a single legend entry.
        cols[active]->series->name = label;
        if (cols[active]->scatter) cols[active]->scatter->name = label;
        viewer->setYLabel(label);
    }
}

void ChartWindow::updateXLabel()
{
    // the X-axis label is shared by all columns on the single plot
    if (!chartXlabel || cols.empty()) return;
    viewer->setXLabel(chartXlabel->text());
}

void ChartWindow::updateXRange(int low, int high)
{
    if (cols.empty()) return;
    auto ranges = viewer->getMinMax();
    double xmin = ranges.left() + static_cast<double>(low) * SLIDER_FRACTION * ranges.width();
    double xmax = ranges.left() + static_cast<double>(high) * SLIDER_FRACTION * ranges.width();
    viewer->setXAxisRange(xmin, xmax);
}

void ChartWindow::updateYRange(int low, int high)
{
    if (cols.empty()) return;
    auto ranges = viewer->getMinMax();
    double ymin = ranges.bottom() - static_cast<double>(low) * SLIDER_FRACTION * ranges.height();
    double ymax = ranges.bottom() - static_cast<double>(high) * SLIDER_FRACTION * ranges.height();
    viewer->setYAxisRange(ymin, ymax);
}

void ChartWindow::saveAs()
{
    if (cols.empty()) return;
    QString defaultname = filename + "." + columns->currentText() + ".png";
    if (filename.isEmpty()) defaultname = columns->currentText() + ".png";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart as Image", defaultname,
                                                    "Image Files (*.jpg *.png *.bmp *.ppm)");
    if (!fileName.isEmpty()) viewer->grab().save(fileName);
}

PlotData ChartWindow::chartsToPlotData() const
{
    PlotData data;
    QStringList names;
    names << "Step";
    for (const auto &c : cols)
        names << c->series->name;
    data.setColumnNames(names);

    const int lines = cols.empty() ? 0 : cols[0]->series->count();
    for (int i = 0; i < lines; ++i) {
        std::vector<double> row;
        row.reserve(names.size());
        row.push_back(cols[0]->series->at(i).x());
        for (const auto &c : cols)
            row.push_back(c->series->at(i).y());
        data.appendRow(row);
    }
    return data;
}

// write the assembled chart data to a file using the given formatter
static void writeExport(QWidget *parent, const QString &caption, const QString &defaultname,
                        const QString &filter, const QString &text)
{
    const QString fileName = QFileDialog::getSaveFileName(parent, caption, defaultname, filter);
    if (fileName.isEmpty()) return;
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << text;
        file.close();
    }
}

void ChartWindow::exportDat()
{
    if (cols.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.dat" : filename + ".dat";
    writeExport(this, "Save Chart as Gnuplot data", defaultname, "Gnuplot data (*.dat)",
                writePlotDat(chartsToPlotData(), filename));
}

void ChartWindow::exportCsv()
{
    if (cols.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.csv" : filename + ".csv";
    writeExport(this, "Save Chart as CSV data", defaultname, "CSV data (*.csv)",
                writePlotCsv(chartsToPlotData()));
}

void ChartWindow::exportYaml()
{
    if (cols.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.yaml" : filename + ".yaml";
    writeExport(this, "Save Chart as YAML data", defaultname, "YAML data (*.yaml *.yml)",
                writePlotYaml(chartsToPlotData()));
}

void ChartWindow::changeChart(int)
{
    // bind the single view to the newly selected column and render it. The chart
    // title and X-axis label are window-wide and stay put; only the per-column
    // Y-axis label is restored here.
    active = activeIndex();
    if (active >= 0) {
        viewer->setColumn(cols[active].get());
        viewer->setReferenceLines(refLines); // re-apply window reference lines to this column
        chartYlabel->setText(cols[active]->yTitle);
        // restore this column's processed-slot label ("Smooth" or its fit name)
        smooth->setItemText(1, cols[active]->procLabel);
    }

    // sync the SG parameter spinbox state (irrelevant while a fit overrides the slot)
    const bool isEos     = currentChart() && currentChart()->isEosFit();
    const bool sgEnabled = doSmooth && !isEos;
    window->setEnabled(sgEnabled);
    order->setEnabled(sgEnabled);

    // a chart switch shows the new column at full range (setColumn re-fit it)
    resetRangeSliders();
}

void ChartWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    if (!isMaximized()) {
        settings.setValue(Keys::CHARTX, width());
        settings.setValue(Keys::CHARTY, height());
    }
    QWidget::closeEvent(event);
}

// event filter to handle "Ambiguous shortcut override" issues
bool ChartWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (!keyEvent) return QWidget::eventFilter(watched, event);
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == '/') {
            stopRun();
            event->accept();
            return true;
        }
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == 'W') {
            close();
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

/* -------------------------------------------------------------------- */

// ---- column rendering pipeline ------------------------------------------
// These free functions render a ChartColumn onto a given PlotWidget. They are
// deliberately independent of any particular ChartViewer instance so that, once
// the multi-view layout is collapsed, a single shared PlotWidget can be pointed
// at any column. ChartViewer's methods below are thin forwarders onto them.

namespace {

// Savitzky-Golay smoothing of an (x,y) point series: the y values are smoothed
// via the shared least-squares core while the x values are preserved.
QList<QPointF> calc_sgsmooth(const QList<QPointF> &input, std::size_t window, int order)
{
    const std::size_t ndat = input.count();
    if (ndat < ((2 * window) + 2)) window = (ndat / 2) - 1;

    if (window > 1) {
        float_vect in(ndat);
        QList<QPointF> rv(input);

        for (std::size_t i = 0; i < ndat; ++i)
            in[i] = input[i].y();

        float_vect out = sg_smooth(in, window, order);

        for (std::size_t i = 0; i < ndat; ++i)
            rv[i].setY(out[i]);

        return rv;
    }
    return input;
}

// Data-only min/max of a column: the cached raw bounds plus any smoothed, fit,
// or overlay curves. Pure -- touches no PlotWidget.
QRectF columnMinMax(const ChartColumn &col)
{
    qreal xmin = col.rawXmin;
    qreal xmax = col.rawXmax;
    qreal ymin = col.rawYmin;
    qreal ymax = col.rawYmax;

    // if plotting the smoothed data, include its range too
    if (col.doSmooth && col.smooth) {
        for (auto &p : col.smooth->points) {
            xmin = qMin(xmin, p.x());
            xmax = qMax(xmax, p.x());
            ymin = qMin(ymin, p.y());
            ymax = qMax(ymax, p.y());
        }
    }

    // include any visible fit/overlay curve (EOS, polynomial, custom)
    if (col.fit && col.fit->isVisible() && !col.fit->points.isEmpty()) {
        for (auto &p : col.fit->points) {
            xmin = qMin(xmin, p.x());
            xmax = qMax(xmax, p.x());
            ymin = qMin(ymin, p.y());
            ymax = qMax(ymax, p.y());
        }
    }

    // include extra overlay data series added from secondary files
    for (auto &s : col.overlaySeries) {
        if (s && s->isVisible()) {
            for (auto &p : s->points) {
                xmin = qMin(xmin, p.x());
                xmax = qMax(xmax, p.x());
                ymin = qMin(ymin, p.y());
                ymax = qMax(ymax, p.y());
            }
        }
    }
    // note: vlines (vertical reference lines) are decorative and excluded

    // avoid (nearly) empty ranges on either axis
    padEmptyRange(xmin, xmax);
    padEmptyRange(ymin, ymax);

    return {xmin, ymax, xmax - xmin, ymin - ymax};
}

// Register a series on the plot with the given color/width.
void addColumnSeries(PlotWidget *plot, PlotSeries *s, const QColor &color, qreal width)
{
    s->color = color;
    if (s->type == PlotSeriesType::Line) s->width = width;
    plot->addSeries(s);
}

// Restyle an already-registered series and repaint.
void styleColumnSeries(PlotWidget *plot, PlotSeries *s, const QColor &color, qreal width)
{
    s->color = color;
    if (s->type == PlotSeriesType::Line) s->width = width;
    plot->update();
}

// Draw a line series and, per the display mode, an accompanying scatter series
// (created on demand and kept in sync with the line).
void renderColumnSeries(PlotWidget *plot, PlotSeries *line, std::unique_ptr<PlotSeries> &points,
                        ChartDisplayMode mode, const QColor &color, qreal width, qreal pointSize)
{
    const bool wantLines  = (mode != ChartDisplayMode::Points);
    const bool wantPoints = (mode != ChartDisplayMode::Lines);

    // line series
    if (!plot->hasSeries(line))
        addColumnSeries(plot, line, color, width);
    else
        styleColumnSeries(plot, line, color, width);
    line->setVisible(wantLines);

    // matching points, created on demand and kept in sync with the line
    if (wantPoints) {
        if (!points) {
            points       = std::make_unique<PlotSeries>();
            points->type = PlotSeriesType::Scatter;
        }
        points->name = line->name; // share the line's name so the legend dedups them
        points->replace(line->points);
        if (!plot->hasSeries(points.get()))
            addColumnSeries(plot, points.get(), color, width);
        else
            styleColumnSeries(plot, points.get(), color, width);
        points->markerSize = pointSize;
        points->setVisible(true);
    } else if (points) {
        points->setVisible(false);
    }
}

// Recompute and (re)draw a column's raw and smoothed series onto the plot.
void refreshColumn(PlotWidget *plot, ChartColumn &col)
{
    QSettings settings;
    settings.beginGroup(Keys::GROUP_CHARTS);
    int rawidx    = settings.value(Keys::RAWBRUSH, 1).toInt();
    int smoothidx = settings.value(Keys::SMOOTHBRUSH, 2).toInt();
    if ((rawidx < 0) || (rawidx >= mybrushes.size())) rawidx = 0;
    if ((smoothidx < 0) || (smoothidx >= mybrushes.size())) smoothidx = 0;
    settings.endGroup();

    const QColor rawcol = col.rawColor.isValid() ? col.rawColor : mybrushes[rawidx].color();
    const QColor smcol = col.smoothcolor.isValid() ? col.smoothcolor : mybrushes[smoothidx].color();

    if (col.doRaw)
        renderColumnSeries(plot, col.series.get(), col.scatter, col.dispmode, rawcol, col.rawWidth,
                           col.rawPointSize);

    if (col.doSmooth) {
        if (col.eosMode && col.fit && !col.fit->points.isEmpty()) {
            // EOS fit acts as the "processed" series; suppress the SG smooth
            col.fit->setVisible(true);
            if (col.smooth) col.smooth->setVisible(false);
            if (col.smoothScatter) col.smoothScatter->setVisible(false);
        } else if (!col.eosMode && col.series->count() > (2 * col.window)) {
            if (col.fit) col.fit->setVisible(false);
            if (!col.smooth) {
                col.smooth       = std::make_unique<PlotSeries>();
                col.smooth->name = QStringLiteral("Smooth"); // legend label for the SG series
            }
            col.smooth->replace(calc_sgsmooth(col.series->points, col.window, col.order));
            renderColumnSeries(plot, col.smooth.get(), col.smoothScatter, col.smoothmode, smcol,
                               col.smoothwidth, col.smoothpointsize);
        }
    } else {
        if (col.eosMode && col.fit) col.fit->setVisible(false);
    }
    plot->update();
}

// Reset the plot ranges to fit the column's data and re-anchor its reference lines.
void resetColumnZoom(PlotWidget *plot, ChartColumn &col)
{
    auto ranges = columnMinMax(col);
    // update reference lines to span the current data range along their axis
    const double ybot = ranges.bottom();
    const double ytop = ranges.top();
    for (std::size_t i = 0; i < col.vlines.size(); ++i) {
        const RefLine &rl = col.reflineDefs[static_cast<int>(i)];
        if (rl.orient == RefOrient::Vertical)
            col.vlines[i]->replace(QList<QPointF>{{rl.value, ybot}, {rl.value, ytop}});
        else
            col.vlines[i]->replace(
                QList<QPointF>{{ranges.left(), rl.value}, {ranges.right(), rl.value}});
    }
    plot->setXRange(ranges.left(), ranges.right());
    plot->setYRange(ybot, ytop);
    plot->update();
}

// Append a point to a column's raw series (monotonic in x) and update its cached
// bounds. Pure data: returns true if the point was appended (x advanced).
bool appendColumnPoint(ChartColumn &col, double x, double y)
{
    if (col.lastX >= x) return false;
    col.lastX = x;
    col.series->append(x, y);
    col.rawXmin = qMin(col.rawXmin, x);
    col.rawXmax = qMax(col.rawXmax, x);
    col.rawYmin = qMin(col.rawYmin, y);
    col.rawYmax = qMax(col.rawYmax, y);
    return true;
}

// Replace a column's raw series with a full point list (file/standalone load),
// recompute its cached bounds, then redraw.
void setColumnPoints(PlotWidget *plot, ChartColumn &col, const QList<QPointF> &points)
{
    col.series->replace(points);
    col.lastX   = points.isEmpty() ? -1.0 : points.last().x();
    col.rawXmin = col.rawYmin = 1.0e100;
    col.rawXmax = col.rawYmax = -1.0e100;
    for (const auto &p : points) {
        col.rawXmin = qMin(col.rawXmin, p.x());
        col.rawXmax = qMax(col.rawXmax, p.x());
        col.rawYmin = qMin(col.rawYmin, p.y());
        col.rawYmax = qMax(col.rawYmax, p.y());
    }
    refreshColumn(plot, col);
    resetColumnZoom(plot, col);
}

// Set the raw-series display style and redraw.
void setColumnDisplayStyle(PlotWidget *plot, ChartColumn &col, ChartDisplayMode mode,
                           const QColor &color, qreal width, qreal pointSize)
{
    col.dispmode     = mode;
    col.rawColor     = color;
    col.rawWidth     = width;
    col.rawPointSize = pointSize;
    refreshColumn(plot, col);
    resetColumnZoom(plot, col);
}

// Set the processed-series display style and redraw.
void setColumnSmoothStyle(PlotWidget *plot, ChartColumn &col, ChartDisplayMode mode,
                          const QColor &color, qreal width, qreal pointSize)
{
    col.smoothmode      = mode;
    col.smoothcolor     = color;
    col.smoothwidth     = width;
    col.smoothpointsize = pointSize;
    refreshColumn(plot, col);
    resetColumnZoom(plot, col);
}

// Set or replace the column's fit-curve overlay (EOS, polynomial, custom).
void setColumnFitCurve(PlotWidget *plot, ChartColumn &col, const QList<QPointF> &points,
                       const QString &name, bool eos)
{
    col.eosMode = eos;
    if (!col.fit) {
        col.fit = std::make_unique<PlotSeries>();
        addColumnSeries(plot, col.fit.get(), QColor(220, 30, 30), 2.0); // distinct fit-curve color
    }
    if (!name.isEmpty()) col.fit->name = name;
    col.fit->replace(points);
    if (col.eosMode) {
        // visibility follows doSmooth: refreshColumn will show/hide it correctly
        refreshColumn(plot, col);
    } else {
        col.fit->setVisible(true);
    }
    resetColumnZoom(plot, col);
}

// Add an extra overlay data series (from a secondary file) to the column.
void addColumnOverlay(PlotWidget *plot, ChartColumn &col, const QList<QPointF> &pts,
                      const QString &name, const QColor &color)
{
    auto s  = std::make_unique<PlotSeries>();
    s->name = name;
    s->replace(pts);
    addColumnSeries(plot, s.get(), color, col.rawWidth);
    col.overlaySeries.push_back(std::move(s));
    resetColumnZoom(plot, col);
}

// Remove all overlay series from the column and the plot.
void clearColumnOverlay(PlotWidget *plot, ChartColumn &col)
{
    for (auto &s : col.overlaySeries)
        plot->removeSeries(s.get());
    col.overlaySeries.clear();
    resetColumnZoom(plot, col);
}

// Remove all reference lines from the column and the plot.
void clearColumnVerticalLines(PlotWidget *plot, ChartColumn &col)
{
    for (auto &s : col.vlines)
        plot->removeSeries(s.get());
    col.vlines.clear();
    col.reflineDefs.clear();
}

// Replace the column's reference lines with the given definitions.
void setColumnReferenceLines(PlotWidget *plot, ChartColumn &col, const QList<RefLine> &lines)
{
    clearColumnVerticalLines(plot, col);
    if (lines.isEmpty()) return;
    auto ranges = columnMinMax(col);
    for (const auto &rl : lines) {
        auto s  = std::make_unique<PlotSeries>();
        s->name = rl.label;
        if (rl.orient == RefOrient::Vertical)
            s->replace(QList<QPointF>{{rl.value, ranges.bottom()}, {rl.value, ranges.top()}});
        else
            s->replace(QList<QPointF>{{ranges.left(), rl.value}, {ranges.right(), rl.value}});
        const QColor c = rl.color.isValid() ? rl.color : QColor(80, 80, 80);
        // dashed reference line, with an optional label drawn next to it
        s->style = Qt::DashLine;
        if (!rl.label.isEmpty()) {
            s->isReference = true;
            s->refLabel    = rl.label;
            s->refAnchor   = rl.anchor;
        }
        addColumnSeries(plot, s.get(), c, 1.5);
        col.vlines.push_back(std::move(s));
        col.reflineDefs.append(rl);
    }
    // reference lines are annotations anchored to the full data extent (and
    // clipped to the view); they do not change the data range, so leave the
    // displayed range -- and the range sliders that drive it -- untouched
    plot->update();
}

// Apply smoothing flags/parameters to the column and redraw.
// Apply smoothing flags/parameters to the column WITHOUT redrawing (so a
// non-active column can be updated without touching the shared plot).
void setColumnSmoothFlags(ChartColumn &col, bool doRaw, bool doSmooth, int window, int order)
{
    // hide raw plot (keep the series alive; data is still needed for smoothing)
    if (!doRaw) {
        if (col.series) col.series->setVisible(false);
        if (col.scatter) col.scatter->setVisible(false);
    }
    // hide processed plot (keep the series alive for quick re-enable)
    if (!doSmooth) {
        if (col.smooth) col.smooth->setVisible(false);
        if (col.smoothScatter) col.smoothScatter->setVisible(false);
        if (col.eosMode && col.fit) col.fit->setVisible(false);
    }
    col.doRaw    = doRaw;
    col.doSmooth = doSmooth;
    col.window   = window;
    col.order    = order;
}

void setColumnSmoothParam(PlotWidget *plot, ChartColumn &col, bool doRaw, bool doSmooth, int window,
                          int order)
{
    setColumnSmoothFlags(col, doRaw, doSmooth, window, order);
    refreshColumn(plot, col);
}

// Replace a column's raw series with a full point list and recompute its cached
// bounds, WITHOUT redrawing (for loading non-active columns).
void setColumnData(ChartColumn &col, const QList<QPointF> &points)
{
    col.series->replace(points);
    col.lastX   = points.isEmpty() ? -1.0 : points.last().x();
    col.rawXmin = col.rawYmin = 1.0e100;
    col.rawXmax = col.rawYmax = -1.0e100;
    for (const auto &p : points) {
        col.rawXmin = qMin(col.rawXmin, p.x());
        col.rawXmax = qMax(col.rawXmax, p.x());
        col.rawYmin = qMin(col.rawYmin, p.y());
        col.rawYmax = qMax(col.rawYmax, p.y());
    }
}

} // namespace

/* -------------------------------------------------------------------- */

ChartViewer::ChartViewer(QWidget *parent) :
    QWidget(parent), plot(nullptr), updChart(Cfg::CHART_UPDATE_INTERVAL_DEFAULT), col(nullptr)
{
    plot = new PlotWidget(this);
    plot->setXTitle("Time step");
    plot->setXLabelFormat("%d");

    QSettings settings;
    // cache the live-update throttle interval once; re-reading it per appended
    // point would construct a QSettings object on the hot thermo path
    updChart = settings.value(Keys::UPDCHART, Cfg::CHART_UPDATE_INTERVAL_DEFAULT).toInt();
    settings.beginGroup(Keys::GROUP_CHARTS);
    plot->setGrid(settings.value(Keys::GRID, true).toBool(),
                  settings.value(Keys::MINORGRID, true).toBool());
    settings.endGroup();

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plot);
}

/* -------------------------------------------------------------------- */

ChartViewer::~ChartViewer() = default;

/* -------------------------------------------------------------------- */

void ChartViewer::setColumn(ChartColumn *c)
{
    plot->clearSeries();
    col = c;
    if (!col) {
        plot->update();
        return;
    }
    // re-register the column's persistent overlays/fit (they keep their styling);
    // the raw/smoothed series are (re)created and styled by refreshColumn, and the
    // reference lines are (re)applied by ChartWindow after binding.
    if (col->fit) plot->addSeries(col->fit.get());
    for (auto &s : col->overlaySeries)
        plot->addSeries(s.get());
    refreshColumn(plot, *col);
    plot->setYTitle(col->yTitle);
    resetColumnZoom(plot, *col);
}

/* -------------------------------------------------------------------- */

void ChartViewer::addPlotSeries(PlotSeries *s, const QColor &color, qreal width)
{
    addColumnSeries(plot, s, color, width);
}

/* -------------------------------------------------------------------- */

void ChartViewer::stylePlotSeries(PlotSeries *s, const QColor &color, qreal width)
{
    styleColumnSeries(plot, s, color, width);
}

/* -------------------------------------------------------------------- */

void ChartViewer::addPoint(double x, double y)
{
    if (appendColumnPoint(*col, x, y)) {
        // update the chart display only after at least updChart milliseconds have passed
        if (col->lastUpdate.msecsTo(QTime::currentTime()) > updChart) {
            col->lastUpdate = QTime::currentTime();
            refreshColumn(plot, *col);
            resetColumnZoom(plot, *col);
        }
    }
}

/* -------------------------------------------------------------------- */

void ChartViewer::setXAxisRange(double min, double max)
{
    plot->setXRange(min, max);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setYAxisRange(double min, double max)
{
    plot->setYRange(min, max);
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getName() const
{
    return col->series->name;
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getTLabel() const
{
    return plot->title();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getXLabel() const
{
    return plot->xTitle();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getYLabel() const
{
    return plot->yTitle();
}

/* -------------------------------------------------------------------- */

QRectF ChartViewer::getMinMax() const
{
    return columnMinMax(*col);
}

/* -------------------------------------------------------------------- */

void ChartViewer::resetZoom()
{
    resetColumnZoom(plot, *col);
}

/* -------------------------------------------------------------------- */

void ChartViewer::smoothParam(bool _doRaw, bool _doSmooth, int _window, int _order)
{
    setColumnSmoothParam(plot, *col, _doRaw, _doSmooth, _window, _order);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setTLabel(const QString &tlabel)
{
    plot->setTitle(tlabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setYLabel(const QString &ylabel)
{
    plot->setYTitle(ylabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setXLabel(const QString &xlabel)
{
    plot->setXTitle(xlabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setXLabelFormat(const QString &fmt)
{
    plot->setXLabelFormat(fmt);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setPoints(const QList<QPointF> &points)
{
    setColumnPoints(plot, *col, points);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setDisplayStyle(ChartDisplayMode mode, const QColor &color, qreal width,
                                  qreal pointSize)
{
    setColumnDisplayStyle(plot, *col, mode, color, width, pointSize);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setSmoothStyle(ChartDisplayMode mode, const QColor &color, qreal width,
                                 qreal pointSize)
{
    setColumnSmoothStyle(plot, *col, mode, color, width, pointSize);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setFitCurve(const QList<QPointF> &points, const QString &name, bool eos)
{
    setColumnFitCurve(plot, *col, points, name, eos);
}

/* -------------------------------------------------------------------- */

void ChartViewer::addOverlaySeries(const QList<QPointF> &pts, const QString &name,
                                   const QColor &color)
{
    addColumnOverlay(plot, *col, pts, name, color);
}

/* -------------------------------------------------------------------- */

void ChartViewer::clearOverlaySeries()
{
    clearColumnOverlay(plot, *col);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setReferenceLines(const QList<RefLine> &lines)
{
    setColumnReferenceLines(plot, *col, lines);
}

/* -------------------------------------------------------------------- */

void ChartViewer::clearVerticalLines()
{
    clearColumnVerticalLines(plot, *col);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setLegendPos(LegendPos pos)
{
    plot->setLegendPos(pos);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setRefLabelStyle(double pointSize, double distance, bool boxed)
{
    plot->setRefLabelStyle(pointSize, distance, boxed);
}

/* -------------------------------------------------------------------- */

void ChartViewer::renderSeries(PlotSeries *line, std::unique_ptr<PlotSeries> &points,
                               ChartDisplayMode mode, const QColor &color, qreal width,
                               qreal pointSize)
{
    renderColumnSeries(plot, line, points, mode, color, width, pointSize);
}

void ChartViewer::updateSmooth()
{
    refreshColumn(plot, *col);
}

// Local Variables:
// c-basic-offset: 4
// End:
