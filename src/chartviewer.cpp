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
#include <QSpacerItem>
#include <QSpinBox>
#include <QStringList>
#include <QTextStream>
#include <QTime>
#include <QVBoxLayout>
#include <QVariant>
#include <algorithm>

#include "chartbackend.h"
#ifdef LAMMPS_GUI_USE_QTGRAPHS
#include "qtgraphsbackend.h"
#else
#include "qtchartsbackend.h"
#endif

#include <cmath>

namespace {

// Set RangeSlider resolution to 1000 steps
constexpr int SLIDER_RANGE       = 1000;
constexpr double SLIDER_FRACTION = 1.0 / static_cast<double>(SLIDER_RANGE);
constexpr int LAYOUT_SPACING     = 6;

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

/* -------------------------------------------------------------------- */

ChartViewer *ChartWindow::currentChart()
{
    const int choice = columns->currentData().toInt();
    for (auto &c : charts)
        if (c->getIndex() == choice) return c;
    return charts.isEmpty() ? nullptr : charts.first();
}

ChartWindow::ChartWindow(const QString &_filename, LammpsGui *_lammpsgui, QWidget *parent) :
    QWidget(parent), lammpsgui(_lammpsgui), menu(new QMenuBar), file(new QMenu("&File")),
    saveAsAct(nullptr), copyAct(nullptr), exportCsvAct(nullptr), exportDatAct(nullptr),
    exportYamlAct(nullptr), closeAct(nullptr), stopAct(nullptr), quitAct(nullptr),
    addDataAct(nullptr), refLinesAct(nullptr), smooth(nullptr), window(nullptr), order(nullptr),
    chartTitle(nullptr), chartYlabel(nullptr), chartXlabel(nullptr), units(nullptr), norm(nullptr),
    filename(_filename)
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
    smooth->addItem("Smoothed");
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
        btn->setFixedWidth(32);
        return btn;
    };
    auto *styleBtn = makeToolBtn(":/icons/preferences-desktop-personal.png", "Chart Style...");
    auto *ppBtn    = makeToolBtn(":/icons/application-plot.png", "Postprocess...");
    connect(styleBtn, &QPushButton::clicked, this, &ChartWindow::changeStyle);
    connect(ppBtn, &QPushButton::clicked, this, &ChartWindow::postProcess);
    row2->addWidget(styleBtn);
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
    saveAsAct = addMenuAction(file, "&Save Graph As...", ":/icons/document-save-as.png", this,
                              &ChartWindow::saveAs);
    copyAct   = addMenuAction(file, "Copy &Graph to Clipboard", ":/icons/edit-copy.png", this,
                              &ChartWindow::copy);
    copyAct->setShortcut(QKeySequence(QKeySequence::Copy));
    exportCsvAct = addMenuAction(file, "&Export data to CSV...", ":/icons/application-calc.png",
                                 this, &ChartWindow::exportCsv);
    exportDatAct = addMenuAction(file, "Export data to &Gnuplot...", ":/icons/application-plot.png",
                                 this, &ChartWindow::exportDat);
    exportYamlAct = addMenuAction(file, "Export data to &YAML...", ":/icons/yaml-file-icon.png",
                                  this, &ChartWindow::exportYaml);
    file->addSeparator();
    addMenuAction(file, "Chart &Style...", ":/icons/preferences-desktop-personal.png", this,
                  &ChartWindow::changeStyle);
    addMenuAction(file, "&Postprocess...", ":/icons/application-plot.png", this,
                  &ChartWindow::postProcess);
    // "Add Data from File..." is only relevant in standalone file-plot mode
    if (!lammpsgui) {
        addDataAct = addMenuAction(file, "&Add Data from File...", ":/icons/application-plot.png",
                                   this, &ChartWindow::addDataFile);
    }
    refLinesAct = addMenuAction(file, "&Reference Lines...", ":/icons/preferences-desktop.png",
                                this, &ChartWindow::referenceLines);
    file->addSeparator();
    stopAct =
        addMenuAction(file, "Stop &Run", ":/icons/process-stop.png", this, &ChartWindow::stopRun);
    stopAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    // without a live simulation there is nothing to stop
    if (!lammpsgui) stopAct->setVisible(false); // no live simulation to stop
    closeAct = addMenuAction(file, "&Close", ":/icons/window-close.png", this, &QWidget::close);
    closeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    quitAct =
        addMenuAction(file, "&Quit", ":/icons/application-exit.png", this, &ChartWindow::quit);
    quitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    if (!lammpsgui) quitAct->setVisible(false); // quit == close in standalone mode
    auto *layout = new QVBoxLayout;
    layout->addLayout(top);
    layout->setSpacing(LAYOUT_SPACING);
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

    installEventFilter(this);
    resize(settings.value(Keys::CHARTX, Cfg::CHART_DEFAULT_WIDTH).toInt(),
           settings.value(Keys::CHARTY, Cfg::CHART_DEFAULT_HEIGHT).toInt());
}

int ChartWindow::getStep() const
{
    if (!charts.empty()) {
        auto *v = charts[0];
        if (v) {
            return static_cast<int>(v->getStep(v->getCount() - 1));
        }
    }
    return -1;
}

void ChartWindow::resetCharts()
{
    while (layout()->count() > 1) {
        auto *item = layout()->takeAt(1);
        if (item) {
            layout()->removeItem(item);
            delete item->widget();
            delete item;
        }
    }
    charts.clear();
    columns->clear();
}

void ChartWindow::resetZoom()
{
    for (auto &c : charts)
        c->resetZoom();
}

void ChartWindow::addChart(const QString &title, int index)
{
    auto *chart = new ChartViewer(title, index);
    layout()->addWidget(chart);
    columns->addItem(title, index);
    columns->show();
    // hide all but the first chart added
    if (!charts.empty()) {
        chart->hide();
    } else {
        // must initialize QLineEdit with first title
        // will be automatically updated when changing charts.
        chartYlabel->setText(title);
    }
    charts.append(chart);
    updateTLabel();
    selectSmooth(0);
}

void ChartWindow::addData(int step, double data, int index)
{
    for (auto &c : charts)
        if (c->getIndex() == index) c->addPoint(step, data);
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
        addChart(data.columnName(ycol), idx);
        auto *chart                      = charts.last();
        const std::vector<double> &yvals = data.column(ycol);
        QList<QPointF> points;
        points.reserve(nrow);
        for (int r = 0; r < nrow; ++r)
            points.append(QPointF(xvals[r], yvals[r]));
        chart->setPoints(points);
        chart->setXLabel(xlabel);
        chart->setXLabelFormat("%.6g");
        ++idx;
    }
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
    if (clip) {
        int choice     = columns->currentData().toInt();
        QWidget *graph = nullptr;
        for (auto &c : charts)
            if (choice == c->getIndex()) graph = c;

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
    if (charts.empty()) return;

    // the chart currently shown in the combo box
    const int choice   = columns->currentData().toInt();
    ChartViewer *chart = nullptr;
    for (auto &c : charts)
        if (c->getIndex() == choice) chart = c;
    if (!chart) chart = charts.first();

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

    // raw data section
    QColor rawChosen = chart->displayColor();
    if (!rawChosen.isValid()) rawChosen = QColor(100, 150, 255);
    auto *rawMode      = modeBox(chart->displayMode());
    auto *rawColorBtn  = colorButton(rawChosen);
    auto *rawWidthSpin = widthBox(chart->displayWidth());
    auto *rawBox       = new QGroupBox("Raw data");
    auto *rawForm      = new QFormLayout(rawBox);
    rawForm->addRow("Display:", rawMode);
    rawForm->addRow("Color:", rawColorBtn);
    rawForm->addRow("Line width:", rawWidthSpin);
    layout->addWidget(rawBox);

    // processed data section
    QColor procChosen = chart->smoothColor();
    if (!procChosen.isValid()) procChosen = QColor(255, 125, 125);
    auto *procMode      = modeBox(chart->smoothMode());
    auto *procColorBtn  = colorButton(procChosen);
    auto *procWidthSpin = widthBox(chart->smoothWidth());
    auto *procBox       = new QGroupBox("Smoothed data");
    auto *procForm      = new QFormLayout(procBox);
    procForm->addRow("Display:", procMode);
    procForm->addRow("Color:", procColorBtn);
    procForm->addRow("Line width:", procWidthSpin);
    layout->addWidget(procBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        chart->setDisplayStyle(static_cast<ChartDisplayMode>(rawMode->currentData().toInt()),
                               rawChosen, rawWidthSpin->value());
        chart->setSmoothStyle(static_cast<ChartDisplayMode>(procMode->currentData().toInt()),
                              procChosen, procWidthSpin->value());
    }
}

void ChartWindow::postProcess()
{
    if (charts.empty()) return;

    // the chart currently shown in the combo box
    const int choice   = columns->currentData().toInt();
    ChartViewer *chart = nullptr;
    for (auto &c : charts)
        if (c->getIndex() == choice) chart = c;
    if (!chart) chart = charts.first();

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
        smooth->setItemText(1, "Custom f(x)");
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
        smooth->setItemText(1, fitName.length() > 12 ? "Custom fit" : fitName);
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
        smooth->setItemText(1, polyName);
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
                               chart->displayWidth());
        smooth->setItemText(1, "EOS fit");
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

        auto *closeBtn = new QDialogButtonBox(QDialogButtonBox::Close);
        connect(closeBtn, &QDialogButtonBox::rejected, resultDlg, &QDialog::accept);
        dlgLayout->addWidget(closeBtn);
        resultDlg->exec();
    }
}

void ChartWindow::addDataFile()
{
    if (charts.empty()) return;

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
}

void ChartWindow::referenceLines()
{
    if (charts.empty()) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Reference Lines");
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(
        new QLabel("Vertical reference lines are applied to all charts.\n"
                   "Example: annotate high-symmetry k-points in phonon-dispersion plots."));

    // scrollable list of (x, label, color) rows
    auto *listWidget = new QWidget;
    auto *listLayout = new QVBoxLayout(listWidget);
    listLayout->setContentsMargins(4, 4, 4, 4);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(listWidget);
    scroll->setMinimumHeight(100);
    layout->addWidget(scroll, 1);

    // helper to build one color-button (same pattern as changeStyle)
    struct RowData {
        QDoubleSpinBox *xSpin;
        QLineEdit *labelEdit;
        QColor color;
    };
    QList<RowData *> rows;
    QList<QPushButton *> colorBtns;

    auto addRow = [&](double xval, const QString &lbl, const QColor &col) {
        auto *rd  = new RowData;
        rd->xSpin = new QDoubleSpinBox;
        rd->xSpin->setDecimals(6);
        rd->xSpin->setRange(-1e15, 1e15);
        rd->xSpin->setValue(xval);
        rd->labelEdit = new QLineEdit(lbl);
        rd->labelEdit->setPlaceholderText("label");
        rd->color = col.isValid() ? col : QColor(80, 80, 80);

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
        row->addWidget(new QLabel("x ="));
        row->addWidget(rd->xSpin, 1);
        row->addWidget(new QLabel(" Label:"));
        row->addWidget(rd->labelEdit, 2);
        row->addWidget(new QLabel(" Color:"));
        row->addWidget(colorBtn);
        row->addWidget(delBtn);
        listLayout->addLayout(row);

        rows.append(rd);
        colorBtns.append(colorBtn);

        // remove this row when "×" is clicked
        QObject::connect(delBtn, &QPushButton::clicked, &dialog,
                         [rd, &rows, &colorBtns, colorBtn, row, listLayout]() {
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
        addRow(rl.x, rl.label, rl.color);

    auto *addBtn = new QPushButton("Add line");
    QObject::connect(addBtn, &QPushButton::clicked, &dialog, [&]() {
        addRow(0.0, QString(), QColor(80, 80, 80));
    });
    layout->addWidget(addBtn);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);

    if (dialog.exec() != QDialog::Accepted) return;

    // rebuild the refLines list and apply to all charts
    refLines.clear();
    for (const auto *rd : rows)
        refLines.append({rd->xSpin->value(), rd->labelEdit->text().trimmed(), rd->color});

    for (auto *c : charts)
        c->setVerticalLines(refLines);
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
    const bool isEos = currentChart() && currentChart()->isEosFit();
    // only reset label to "Smoothed" when no fit overlay is active; otherwise
    // preserve whatever name was set when the fit was applied (EOS fit, Poly deg N, etc.)
    if (!isEos) smooth->setItemText(1, "Smoothed");
    // SG smooth parameters are only relevant when smoothing without a fit overlay
    const bool sgEnabled = doSmooth && !isEos;
    window->setEnabled(sgEnabled);
    order->setEnabled(sgEnabled);
    updateSmooth();
    // re-fit the axes to the now-displayed series so the range covers the whole
    // data (e.g. a smoothed curve that overshoots the raw range is not clipped)
    resetZoom();
}

void ChartWindow::updateSmooth()
{
    int wval = window->value();
    int oval = order->value();

    for (auto &c : charts)
        c->smoothParam(doRaw, doSmooth, wval, oval);
}

void ChartWindow::updateTLabel()
{
    if (chartTitle) {
        for (auto &c : charts)
            c->setTLabel(chartTitle->text());
    }
}

void ChartWindow::updateYLabel()
{
    for (auto &c : charts) {
        if (c->isVisible()) c->setYLabel(chartYlabel->text());
    }
}

void ChartWindow::updateXLabel()
{
    if (!chartXlabel) return;
    for (auto &c : charts)
        c->setXLabel(chartXlabel->text());
}

void ChartWindow::updateXRange(int low, int high)
{
    for (auto &c : charts) {
        if (c->isVisible()) {
            auto axes   = c->getAxes();
            auto ranges = c->getMinMax();
            double xmin =
                ranges.left() + static_cast<double>(low) * SLIDER_FRACTION * ranges.width();
            double xmax =
                ranges.left() + static_cast<double>(high) * SLIDER_FRACTION * ranges.width();
            axes[0]->setRange(xmin, xmax);
        }
    }
}

void ChartWindow::updateYRange(int low, int high)
{
    for (auto &c : charts) {
        if (c->isVisible()) {
            auto axes   = c->getAxes();
            auto ranges = c->getMinMax();
            double ymin =
                ranges.bottom() - static_cast<double>(low) * SLIDER_FRACTION * ranges.height();
            double ymax =
                ranges.bottom() - static_cast<double>(high) * SLIDER_FRACTION * ranges.height();
            axes[1]->setRange(ymin, ymax);
        }
    }
}

void ChartWindow::saveAs()
{
    if (charts.empty()) return;
    QString defaultname = filename + "." + columns->currentText() + ".png";
    if (filename.isEmpty()) defaultname = columns->currentText() + ".png";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart as Image", defaultname,
                                                    "Image Files (*.jpg *.png *.bmp *.ppm)");
    if (!fileName.isEmpty()) {
        int choice = columns->currentData().toInt();
        for (auto &c : charts)
            if (choice == c->getIndex()) c->grab().save(fileName);
    }
}

PlotData ChartWindow::chartsToPlotData() const
{
    PlotData data;
    QStringList names;
    names << "Step";
    for (auto *c : charts)
        names << c->getName();
    data.setColumnNames(names);

    const int lines = charts.isEmpty() ? 0 : charts[0]->getCount();
    for (int i = 0; i < lines; ++i) {
        std::vector<double> row;
        row.reserve(names.size());
        row.push_back(charts[0]->getStep(i));
        for (auto *c : charts)
            row.push_back(c->getData(i));
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
    if (charts.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.dat" : filename + ".dat";
    writeExport(this, "Save Chart as Gnuplot data", defaultname, "Gnuplot data (*.dat)",
                writePlotDat(chartsToPlotData(), filename));
}

void ChartWindow::exportCsv()
{
    if (charts.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.csv" : filename + ".csv";
    writeExport(this, "Save Chart as CSV data", defaultname, "CSV data (*.csv)",
                writePlotCsv(chartsToPlotData()));
}

void ChartWindow::exportYaml()
{
    if (charts.empty()) return;
    const QString defaultname = filename.isEmpty() ? "lammpsdata.yaml" : filename + ".yaml";
    writeExport(this, "Save Chart as YAML data", defaultname, "YAML data (*.yaml *.yml)",
                writePlotYaml(chartsToPlotData()));
}

void ChartWindow::changeChart(int)
{
    int choice = columns->currentData().toInt();
    for (auto &c : charts) {
        if (choice == c->getIndex()) {
            c->show();
            chartTitle->setText(c->getTLabel());
            chartYlabel->setText(c->getYLabel());
        } else {
            c->hide();
        }
    }

    // sync "Smoothed"/"EOS fit" label and SG parameter spinbox state
    const bool isEos = currentChart() && currentChart()->isEosFit();
    smooth->setItemText(1, isEos ? "EOS fit" : "Smoothed");
    const bool sgEnabled = doSmooth && !isEos;
    window->setEnabled(sgEnabled);
    order->setEnabled(sgEnabled);

    // reset plot range selection
    xrange->setLow(0);
    xrange->setHigh(SLIDER_RANGE);
    yrange->setLow(0);
    yrange->setHigh(SLIDER_RANGE);
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

ChartViewer::ChartViewer(const QString &title, int _index, QWidget *parent) :
    QWidget(parent), lastX(-1.0), index(_index), window(10), order(4), series(new QLineSeries),
    smooth(nullptr), scatter(nullptr), smoothScatter(nullptr), fit(nullptr), doRaw(true),
    doSmooth(false), eosMode(false), dispmode(ChartDisplayMode::Lines), rawColor(), rawWidth(3.0),
    smoothmode(ChartDisplayMode::Lines), smoothcolor(), smoothwidth(3.0)
{
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    backend = std::make_unique<QtGraphsBackend>();
#else
    backend = std::make_unique<QtChartsBackend>();
#endif
    backend->init(this, title, series);

    // embed the backend widget into our layout
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    if (auto *widget = backend->widget()) {
        layout->addWidget(widget);
    } else {
        auto *error = new QLabel(tr("Unable to initialize chart display."), this);
        error->setAlignment(Qt::AlignCenter);
        error->setWordWrap(true);
        layout->addWidget(error);
    }

    lastUpdate = QTime::currentTime();
    updateSmooth();
}

/* -------------------------------------------------------------------- */

ChartViewer::~ChartViewer()
{
    // series and smooth are owned by the chart backend (QChart or GraphsView)
    // and will be cleaned up when the backend widget is destroyed as a child
    // of this widget. Do not delete them manually to avoid double-free crashes,
    // particularly with the QtGraphs backend where removeSeries() and widget
    // destruction both delete attached series objects.
}

/* -------------------------------------------------------------------- */

void ChartViewer::addPoint(double x, double y)
{
    if (lastX < x) {
        lastX = x;
        series->append(x, y);

        QSettings settings;
        // update the chart display only after at least updchart milliseconds have passed
        if (lastUpdate.msecsTo(QTime::currentTime()) >
            settings.value(Keys::UPDCHART, Cfg::CHART_UPDATE_INTERVAL_DEFAULT).toInt()) {
            lastUpdate = QTime::currentTime();
            updateSmooth();
            resetZoom();
        }
    }
}

/* -------------------------------------------------------------------- */

QList<QAbstractAxis *> ChartViewer::getAxes() const
{
    return backend->getAxes();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getName() const
{
    return series->name();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getTLabel() const
{
    return backend->getTLabel();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getXLabel() const
{
    return backend->xAxis()->titleText();
}

/* -------------------------------------------------------------------- */

QString ChartViewer::getYLabel() const
{
    return backend->yAxis()->titleText();
}

/* -------------------------------------------------------------------- */

QRectF ChartViewer::getMinMax() const
{
    auto points = series->points();

    // get min/max for plot
    qreal xmin = 1.0e100;
    qreal xmax = -1.0e100;
    qreal ymin = 1.0e100;
    qreal ymax = -1.0e100;
    for (auto &p : points) {
        xmin = qMin(xmin, p.x());
        xmax = qMax(xmax, p.x());
        ymin = qMin(ymin, p.y());
        ymax = qMax(ymax, p.y());
    }

    // if plotting the smoothed data, include its range too
    if (doSmooth && smooth) {
        auto spoints = smooth->points();
        for (auto &p : spoints) {
            xmin = qMin(xmin, p.x());
            xmax = qMax(xmax, p.x());
            ymin = qMin(ymin, p.y());
            ymax = qMax(ymax, p.y());
        }
    }

    // include any visible fit/overlay curve (EOS, polynomial, custom)
    if (fit && fit->isVisible() && !fit->points().isEmpty()) {
        for (auto &p : fit->points()) {
            xmin = qMin(xmin, p.x());
            xmax = qMax(xmax, p.x());
            ymin = qMin(ymin, p.y());
            ymax = qMax(ymax, p.y());
        }
    }

    // include extra overlay data series added from secondary files
    for (auto *s : overlaySeries) {
        if (s && s->isVisible()) {
            for (auto &p : s->points()) {
                xmin = qMin(xmin, p.x());
                xmax = qMax(xmax, p.x());
                ymin = qMin(ymin, p.y());
                ymax = qMax(ymax, p.y());
            }
        }
    }
    // note: vlines (vertical reference lines) are decorative and excluded from range

    // avoid (nearly) empty ranges
    double deltax = xmax - xmin;
    if ((deltax / ((xmax == 0.0) ? 1.0 : xmax)) < 1.0e-10) {
        if ((xmin == 0.0) || (xmax == 0.0)) {
            xmin = -0.025;
            xmax = 0.025;
        } else {
            xmin -= 0.025 * fabs(xmin);
            xmax += 0.025 * fabs(xmax);
        }
    }

    double deltay = ymax - ymin;
    if ((deltay / ((ymax == 0.0) ? 1.0 : ymax)) < 1.0e-10) {
        if ((ymin == 0.0) || (ymax == 0.0)) {
            ymin = -0.025;
            ymax = 0.025;
        } else {
            ymin -= 0.025 * fabs(ymin);
            ymax += 0.025 * fabs(ymax);
        }
    }

    return {xmin, ymax, xmax - xmin, ymin - ymax};
}

/* -------------------------------------------------------------------- */

void ChartViewer::resetZoom()
{
    auto ranges = getMinMax();
    // update vertical reference lines to span the current data y range
    const double ybot = ranges.bottom();
    const double ytop = ranges.top();
    for (int i = 0; i < vlines.size(); ++i) {
        const double x = vlinePositions[i];
        vlines[i]->replace(QList<QPointF>{{x, ybot}, {x, ytop}});
    }
    backend->resetZoom(ranges.left(), ranges.right(), ybot, ytop);
}

/* -------------------------------------------------------------------- */

void ChartViewer::smoothParam(bool _doRaw, bool _doSmooth, int _window, int _order)
{
    // hide raw plot (keep the series alive; data is still needed for smoothing)
    if (!_doRaw) {
        if (series) series->setVisible(false);
        if (scatter) scatter->setVisible(false);
    }
    // hide processed plot (keep the series alive for quick re-enable)
    if (!_doSmooth) {
        if (smooth) smooth->setVisible(false);
        if (smoothScatter) smoothScatter->setVisible(false);
        if (eosMode && fit) fit->setVisible(false);
    }
    doRaw    = _doRaw;
    doSmooth = _doSmooth;
    window   = _window;
    order    = _order;
    updateSmooth();
}

/* -------------------------------------------------------------------- */

void ChartViewer::setTLabel(const QString &tlabel)
{
    backend->setTLabel(tlabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setYLabel(const QString &ylabel)
{
    backend->setYLabel(ylabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setXLabel(const QString &xlabel)
{
    backend->setXLabel(xlabel);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setXLabelFormat(const QString &fmt)
{
    backend->setXLabelFormat(fmt);
}

/* -------------------------------------------------------------------- */

void ChartViewer::setPoints(const QList<QPointF> &points)
{
    series->replace(points);
    lastX = points.isEmpty() ? -1.0 : points.last().x();
    updateSmooth();
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::setDisplayStyle(ChartDisplayMode mode, const QColor &color, qreal width)
{
    dispmode = mode;
    rawColor = color;
    rawWidth = width;
    updateSmooth();
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::setSmoothStyle(ChartDisplayMode mode, const QColor &color, qreal width)
{
    smoothmode  = mode;
    smoothcolor = color;
    smoothwidth = width;
    updateSmooth();
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::setFitCurve(const QList<QPointF> &points, const QString &name, bool eos)
{
    eosMode = eos;
    if (!fit) {
        fit = new QLineSeries;
        backend->addSeries(fit, QColor(220, 30, 30), 2.0); // distinct fit-curve color
    }
    if (!name.isEmpty()) fit->setName(name);
    fit->replace(points);
    if (eosMode) {
        // visibility follows doSmooth: updateSmooth will show/hide it correctly
        updateSmooth();
    } else {
        fit->setVisible(true);
    }
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::addOverlaySeries(const QList<QPointF> &pts, const QString &name,
                                   const QColor &color)
{
    auto *s = new QLineSeries;
    s->setName(name);
    s->replace(pts);
    backend->addSeries(s, color, rawWidth);
    overlaySeries.append(s);
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::clearOverlaySeries()
{
    for (auto *s : overlaySeries)
        backend->removeSeries(s);
    overlaySeries.clear();
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::setVerticalLines(const QList<RefLine> &lines)
{
    clearVerticalLines();
    if (lines.isEmpty()) return;
    auto ranges       = getMinMax();
    const double ybot = ranges.bottom();
    const double ytop = ranges.top();
    for (const auto &rl : lines) {
        auto *s = new QLineSeries;
        s->setName(rl.label);
        s->replace(QList<QPointF>{{rl.x, ybot}, {rl.x, ytop}});
        const QColor col = rl.color.isValid() ? rl.color : QColor(80, 80, 80);
        backend->addSeries(s, col, 1.5);
#ifndef LAMMPS_GUI_USE_QTGRAPHS
        // dashed style is only supported by the QtCharts backend
        QPen pen = s->pen();
        pen.setStyle(Qt::DashLine);
        s->setPen(pen);
#endif
        vlines.append(s);
        vlinePositions.append(rl.x);
    }
    resetZoom();
}

/* -------------------------------------------------------------------- */

void ChartViewer::clearVerticalLines()
{
    for (auto *s : vlines)
        backend->removeSeries(s);
    vlines.clear();
    vlinePositions.clear();
}

/* -------------------------------------------------------------------- */

// local Savitzky-Golay smoothing wrapper around the least-squares core

namespace {

// savitzky golay smoothing of an (x,y) point series: the y values are smoothed
// in place via the shared least-squares core while the x values are preserved.
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

} // namespace

/* -------------------------------------------------------------------- */

// update smooth plot data

void ChartViewer::renderSeries(QLineSeries *line, QScatterSeries *&points, ChartDisplayMode mode,
                               const QColor &color, qreal width)
{
    const bool wantLines  = (mode != ChartDisplayMode::Points);
    const bool wantPoints = (mode != ChartDisplayMode::Lines);

    // line series
    if (!backend->hasSeries(line))
        backend->addSeries(line, color, width);
    else
        backend->styleSeries(line, color, width);
    line->setVisible(wantLines);

    // matching points, created on demand and kept in sync with the line
    if (wantPoints) {
        if (!points) points = new QScatterSeries;
        points->replace(line->points());
        if (!backend->hasSeries(points))
            backend->addSeries(points, color, width);
        else
            backend->styleSeries(points, color, width);
        points->setVisible(true);
    } else if (points) {
        points->setVisible(false);
    }
}

void ChartViewer::updateSmooth()
{
    QSettings settings;
    settings.beginGroup(Keys::GROUP_CHARTS);
    int rawidx    = settings.value(Keys::RAWBRUSH, 1).toInt();
    int smoothidx = settings.value(Keys::SMOOTHBRUSH, 2).toInt();
    if ((rawidx < 0) || (rawidx >= mybrushes.size())) rawidx = 0;
    if ((smoothidx < 0) || (smoothidx >= mybrushes.size())) smoothidx = 0;
    settings.endGroup();

    const QColor rawcol = rawColor.isValid() ? rawColor : mybrushes[rawidx].color();
    const QColor smcol  = smoothcolor.isValid() ? smoothcolor : mybrushes[smoothidx].color();

    if (doRaw) renderSeries(series, scatter, dispmode, rawcol, rawWidth);

    if (doSmooth) {
        if (eosMode && fit && !fit->points().isEmpty()) {
            // EOS fit acts as the "processed" series; suppress the SG smooth
            fit->setVisible(true);
            if (smooth) smooth->setVisible(false);
            if (smoothScatter) smoothScatter->setVisible(false);
        } else if (!eosMode && series->count() > (2 * window)) {
            if (fit) fit->setVisible(false);
            if (!smooth) smooth = new QLineSeries;
            smooth->replace(calc_sgsmooth(series->points(), window, order));
            renderSeries(smooth, smoothScatter, smoothmode, smcol, smoothwidth);
        }
    } else {
        if (eosMode && fit) fit->setVisible(false);
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
