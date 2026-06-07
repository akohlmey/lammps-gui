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
#include "fitting.h"
#include "helpers.h"
#include "lammpsgui.h"
#include "leastsquares.h"
#include "plotdata.h"
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
#include <QFormLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>

#include <QLabel>
#include <QLayout>
#include <QList>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QSettings>
#include <QSpacerItem>
#include <QSpinBox>
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

} // namespace

ChartWindow::ChartWindow(const QString &_filename, LammpsGui *_lammpsgui, QWidget *parent) :
    QWidget(parent), lammpsgui(_lammpsgui), menu(new QMenuBar), file(new QMenu("&File")),
    saveAsAct(nullptr), copyAct(nullptr), exportCsvAct(nullptr), exportDatAct(nullptr),
    exportYamlAct(nullptr), closeAct(nullptr), stopAct(nullptr), quitAct(nullptr), smooth(nullptr),
    window(nullptr), order(nullptr), chartTitle(nullptr), chartYlabel(nullptr), units(nullptr),
    norm(nullptr), filename(_filename)
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
    auto mytitle = settings.value(Keys::TITLE, "Thermo: %f").toString().replace("%f", filename);
    chartTitle   = new QLineEdit(mytitle);
    chartYlabel  = new QLineEdit("");

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
    smooth->addItem("Smooth");
    smooth->addItem("Both");
    smooth->setCurrentIndex(smoothchoice);
    window = new QSpinBox;
    window->setRange(Cfg::SMOOTH_WINDOW_MIN, Cfg::SMOOTH_WINDOW_MAX);
    window->setValue(settings.value(Keys::SMOOTHWINDOW, Cfg::SMOOTH_WINDOW_DEFAULT).toInt());
    window->setEnabled(true);
    window->setToolTip("Smoothing Window Size");
    order = new QSpinBox;
    order->setRange(Cfg::SMOOTH_ORDER_MIN, Cfg::SMOOTH_ORDER_MAX);
    order->setValue(settings.value(Keys::SMOOTHORDER, Cfg::SMOOTH_ORDER_DEFAULT).toInt());
    order->setEnabled(true);
    order->setToolTip("Smoothing Order");
    settings.endGroup();

    columns = new QComboBox;
    row1->addWidget(menu);
    row1->addWidget(dummy);
    row2->addWidget(dummy);
    row1->addWidget(new QLabel("Title:"));
    row1->addWidget(chartTitle, 2);
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
    auto *layout = new QVBoxLayout;
    layout->addLayout(top);
    layout->setSpacing(LAYOUT_SPACING);
    setLayout(layout);

    connect(chartTitle, &QLineEdit::editingFinished, this, &ChartWindow::updateTLabel);
    connect(chartYlabel, &QLineEdit::editingFinished, this, &ChartWindow::updateYLabel);
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
        ++idx;
    }
    if (!data.units().isEmpty()) setUnits(data.units());
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
    auto *form = new QFormLayout(&dialog);

    auto *modebox = new QComboBox;
    modebox->addItem("Lines", static_cast<int>(ChartDisplayMode::Lines));
    modebox->addItem("Points", static_cast<int>(ChartDisplayMode::Points));
    modebox->addItem("Lines + Points", static_cast<int>(ChartDisplayMode::LinesAndPoints));
    modebox->setCurrentIndex(static_cast<int>(chart->displayMode()));
    form->addRow("Display:", modebox);

    QColor chosen = chart->displayColor();
    if (!chosen.isValid()) chosen = QColor(100, 150, 255); // default raw brush color
    auto *colorbtn   = new QPushButton;
    auto setBtnColor = [colorbtn](const QColor &c) {
        colorbtn->setText(c.name());
        colorbtn->setStyleSheet(QString("background-color: %1; color: %2;")
                                    .arg(c.name(), (c.lightness() < 128) ? "white" : "black"));
    };
    setBtnColor(chosen);
    connect(colorbtn, &QPushButton::clicked, &dialog, [&]() {
        const QColor c = QColorDialog::getColor(chosen, &dialog, "Series Color");
        if (c.isValid()) {
            chosen = c;
            setBtnColor(c);
        }
    });
    form->addRow("Color:", colorbtn);

    auto *widthspin = new QDoubleSpinBox;
    widthspin->setRange(0.5, 20.0);
    widthspin->setSingleStep(0.5);
    widthspin->setValue(chart->displayWidth());
    form->addRow("Line width:", widthspin);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    form->addRow(buttons);

    if (dialog.exec() == QDialog::Accepted) {
        const auto mode = static_cast<ChartDisplayMode>(modebox->currentData().toInt());
        chart->setDisplayStyle(mode, chosen, widthspin->value());
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

    QDialog dialog(this);
    dialog.setWindowTitle("Postprocess Chart Data");
    auto *form = new QFormLayout(&dialog);

    auto *analysisbox = new QComboBox;
    analysisbox->addItem("Autocorrelation");
    analysisbox->addItem("Polynomial fit");
    analysisbox->addItem("Birch-Murnaghan EOS fit");
    form->addRow("Analysis:", analysisbox);

    auto *paramLabel = new QLabel;
    auto *paramSpin  = new QSpinBox;
    form->addRow(paramLabel, paramSpin);

    // swap the parameter widget to match the selected analysis
    auto configure = [=](int idx) {
        if (idx == 1) { // polynomial degree
            paramLabel->setText("Degree:");
            paramSpin->setVisible(true);
            paramSpin->setRange(1, qMin(npoints - 1, 8));
            paramSpin->setValue(qMin(3, qMin(npoints - 1, 8)));
        } else if (idx == 2) { // EOS has no free parameters
            paramLabel->setText("(no parameters)");
            paramSpin->setVisible(false);
        } else { // autocorrelation max lag
            paramLabel->setText("Max lag:");
            paramSpin->setVisible(true);
            paramSpin->setRange(1, npoints - 1);
            paramSpin->setValue(qMin(npoints - 1, npoints / 2));
        }
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
        chart->setFitCurve(curve);

        QString report =
            QString("Polynomial fit of degree %1\n\n").arg(static_cast<int>(f.coeffs.size()) - 1);
        for (int i = 0; i < static_cast<int>(f.coeffs.size()); ++i)
            report += QString("  c[%1] = %2\n").arg(i).arg(f.coeffs[i], 0, 'g', 8);
        report += QString("\n  RMS residual = %1").arg(f.rms, 0, 'g', 6);
        information(this, "Polynomial Fit", report);
        return;
    }

    // Birch-Murnaghan EOS fit (x = volume, y = energy)
    const EosFit f = birchMurnaghanFit(xs, ys);
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
    chart->setFitCurve(curve);

    const QString report = QString("Birch-Murnaghan EOS fit\n\n"
                                   "  V0  = %1\n  E0  = %2\n  B0  = %3\n  B0' = %4\n\n"
                                   "  RMS residual = %5")
                               .arg(f.v0, 0, 'g', 8)
                               .arg(f.e0, 0, 'g', 8)
                               .arg(f.b0, 0, 'g', 8)
                               .arg(f.b0prime, 0, 'g', 6)
                               .arg(f.rms, 0, 'g', 6);
    information(this, "Birch-Murnaghan EOS Fit", report);
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
    window->setEnabled(doSmooth);
    order->setEnabled(doSmooth);
    updateSmooth();
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
    smooth(nullptr), scatter(nullptr), fit(nullptr), doRaw(true), doSmooth(false),
    dispmode(ChartDisplayMode::Lines), rawColor(), rawWidth(3.0)
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

    // if plotting the smoothed plot, check for its min/max values, too
    if (doSmooth && smooth) {
        auto spoints = smooth->points();
        for (auto &p : spoints) {
            xmin = qMin(xmin, p.x());
            xmax = qMax(xmax, p.x());
            ymin = qMin(ymin, p.y());
            ymax = qMax(ymax, p.y());
        }
    }

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
    backend->resetZoom(ranges.left(), ranges.right(), ranges.bottom(), ranges.top());
}

/* -------------------------------------------------------------------- */

void ChartViewer::smoothParam(bool _doRaw, bool _doSmooth, int _window, int _order)
{
    // hide raw plot (keep the series alive; data is still needed for smoothing)
    if (!_doRaw && series) {
        series->setVisible(false);
    }
    // hide smooth plot (keep the series alive for quick re-enable)
    if (!_doSmooth && smooth) {
        smooth->setVisible(false);
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
    if (backend->xAxis()) backend->xAxis()->setTitleText(xlabel);
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

void ChartViewer::setFitCurve(const QList<QPointF> &points)
{
    if (!fit) {
        fit = new QLineSeries;
        backend->addSeries(fit, QColor(220, 30, 30), 2.0); // distinct fit-curve color
    }
    fit->replace(points);
    fit->setVisible(true);
    resetZoom();
}

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

void ChartViewer::updateSmooth()
{
    QSettings settings;
    settings.beginGroup(Keys::GROUP_CHARTS);
    int rawidx    = settings.value(Keys::RAWBRUSH, 1).toInt();
    int smoothidx = settings.value(Keys::SMOOTHBRUSH, 2).toInt();
    if ((rawidx < 0) || (rawidx >= mybrushes.size())) rawidx = 0;
    if ((smoothidx < 0) || (smoothidx >= mybrushes.size())) smoothidx = 0;
    settings.endGroup();

    const QColor rawcolor = rawColor.isValid() ? rawColor : mybrushes[rawidx].color();
    const bool wantLines  = (dispmode != ChartDisplayMode::Points);
    const bool wantPoints = (dispmode != ChartDisplayMode::Lines);

    if (doRaw) {
        // raw data as a line series
        if (!backend->hasSeries(series))
            backend->addSeries(series, rawcolor, rawWidth);
        else
            backend->styleSeries(series, rawcolor, rawWidth);
        series->setVisible(wantLines);

        // raw data as points, created on demand and kept in sync with the line
        if (wantPoints) {
            if (!scatter) scatter = new QScatterSeries;
            scatter->replace(series->points());
            if (!backend->hasSeries(scatter))
                backend->addSeries(scatter, rawcolor, rawWidth);
            else
                backend->styleSeries(scatter, rawcolor, rawWidth);
            scatter->setVisible(true);
        } else if (scatter) {
            scatter->setVisible(false);
        }
    }

    if (doSmooth) {
        if (series->count() > (2 * window)) {
            if (!smooth) {
                smooth = new QLineSeries;
                backend->addSeries(smooth, mybrushes[smoothidx].color(), 3.0);
            }
            smooth->setVisible(true);
            smooth->replace(calc_sgsmooth(series->points(), window, order));
        }
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
