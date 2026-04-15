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

#include "helpers.h"
#include "lammpsgui.h"
#include "qaddon.h"
#include "rangeslider.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QCloseEvent>
#include <QComboBox>
#include <QEvent>
#include <QFileDialog>
#include <QHBoxLayout>
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
constexpr double SLIDER_FRACTION = 1.0 / (double)SLIDER_RANGE;
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
    settings.beginGroup("charts");
    auto mytitle = settings.value("title", "Thermo: %f").toString().replace("%f", filename);
    chartTitle   = new QLineEdit(mytitle);
    chartYlabel  = new QLineEdit("");

    // plot smoothing
    int smoothchoice = settings.value("smoothchoice", 0).toInt();
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
    window->setRange(5, 999);
    window->setValue(settings.value("smoothwindow", 10).toInt());
    window->setEnabled(true);
    window->setToolTip("Smoothing Window Size");
    order = new QSpinBox;
    order->setRange(1, 20);
    order->setValue(settings.value("smoothorder", 4).toInt());
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
    row1->addWidget(new QLabel("Units:"));
    units = new QLabel("[lj]");
    units->setFrameStyle(QFrame::Panel | QFrame::Raised);
    row1->addWidget(units);
    row1->addWidget(new QLabel("Norm:"));
    norm = new QCheckBox("");
    norm->setChecked(false);
    norm->setEnabled(false);
    row1->addWidget(norm);
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
    saveAsAct = file->addAction("&Save Graph As...", this, &ChartWindow::saveAs);
    saveAsAct->setIcon(QIcon(":/icons/document-save-as.png"));
    copyAct = file->addAction("Copy &Graph to Clipboard", this, &ChartWindow::copy);
    copyAct->setIcon(QIcon(":/icons/edit-copy.png"));
    copyAct->setShortcut(QKeySequence(QKeySequence::Copy));
    exportCsvAct = file->addAction("&Export data to CSV...", this, &ChartWindow::exportCsv);
    exportCsvAct->setIcon(QIcon(":/icons/application-calc.png"));
    exportDatAct = file->addAction("Export data to &Gnuplot...", this, &ChartWindow::exportDat);
    exportDatAct->setIcon(QIcon(":/icons/application-plot.png"));
    exportYamlAct = file->addAction("Export data to &YAML...", this, &ChartWindow::exportYaml);
    exportYamlAct->setIcon(QIcon(":/icons/yaml-file-icon.png"));
    file->addSeparator();
    stopAct = file->addAction("Stop &Run", this, &ChartWindow::stopRun);
    stopAct->setIcon(QIcon(":/icons/process-stop.png"));
    stopAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash));
    closeAct = file->addAction("&Close", this, &QWidget::close);
    closeAct->setIcon(QIcon(":/icons/window-close.png"));
    closeAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    quitAct = file->addAction("&Quit", this, &ChartWindow::quit);
    quitAct->setIcon(QIcon(":/icons/application-exit.png"));
    quitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    auto *layout = new QVBoxLayout;
    layout->addLayout(top);
    layout->setSpacing(LAYOUT_SPACING);
    setLayout(layout);

    connect(chartTitle, &QLineEdit::editingFinished, this, &ChartWindow::updateTLabel);
    connect(chartYlabel, &QLineEdit::editingFinished, this, &ChartWindow::updateYLabel);
    connect(smooth, SIGNAL(currentIndexChanged(int)), this, SLOT(selectSmooth(int)));
    connect(window, &QAbstractSpinBox::editingFinished, this, &ChartWindow::updateSmooth);
    connect(order, &QAbstractSpinBox::editingFinished, this, &ChartWindow::updateSmooth);
    connect(window, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChartWindow::updateSmooth);
    connect(order, QOverload<int>::of(&QSpinBox::valueChanged), this, &ChartWindow::updateSmooth);
    connect(columns, SIGNAL(currentIndexChanged(int)), this, SLOT(changeChart(int)));
    connect(xrange, &RangeSlider::sliderMoved, this, &ChartWindow::updateXRange);
    connect(yrange, &RangeSlider::sliderMoved, this, &ChartWindow::updateYRange);

    installEventFilter(this);
    resize(settings.value("chartx", 640).toInt(), settings.value("charty", 480).toInt());
}

int ChartWindow::getStep() const
{
    if (!charts.empty()) {
        auto *v = charts[0];
        if (v) {
            return (int)v->getStep(v->getCount() - 1);
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
        if (c->getIndex() == index) c->addData(step, data);
}

void ChartWindow::setUnits(const QString &_units)
{
    units->setText(_units);
}

void ChartWindow::setNorm(bool _norm)
{
    norm->setChecked(_norm);
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
    if (lammpsgui) lammpsgui->quit();
}

void ChartWindow::stopRun()
{
    if (lammpsgui) lammpsgui->stopRun();
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
            double xmin = ranges.left() + (double)low * SLIDER_FRACTION * ranges.width();
            double xmax = ranges.left() + (double)high * SLIDER_FRACTION * ranges.width();
            axes[0]->setRange(xmin, xmax);
            c->refreshView();
        }
    }
}

void ChartWindow::updateYRange(int low, int high)
{
    for (auto &c : charts) {
        if (c->isVisible()) {
            auto axes   = c->getAxes();
            auto ranges = c->getMinMax();
            double ymin = ranges.bottom() - (double)low * SLIDER_FRACTION * ranges.height();
            double ymax = ranges.bottom() - (double)high * SLIDER_FRACTION * ranges.height();
            axes[1]->setRange(ymin, ymax);
            c->refreshView();
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

void ChartWindow::exportDat()
{
    if (charts.empty()) return;
    QString defaultname = filename + ".dat";
    if (filename.isEmpty()) defaultname = "lammpsdata.dat";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart as Gnuplot data", defaultname,
                                                    "Image Files (*.dat)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            constexpr int fw = 16;
            out.setFieldAlignment(QTextStream::AlignRight);
            out.setRealNumberPrecision(8);

            out << "# Thermodynamic data from " << filename << "\n";
            out << "#          Step";
            for (auto &c : charts)
                out << qSetFieldWidth(0) << ' ' << qSetFieldWidth(fw) << c->getTitle();
            out << qSetFieldWidth(0) << '\n';

            int lines = charts[0]->getCount();
            for (int i = 0; i < lines; ++i) {
                // timestep
                out << qSetFieldWidth(0) << ' ' << qSetFieldWidth(fw) << charts[0]->getStep(i);
                for (auto &c : charts)
                    out << qSetFieldWidth(0) << ' ' << qSetFieldWidth(fw) << c->getData(i);
                out << qSetFieldWidth(0) << '\n';
            }
            file.close();
        }
    }
}

void ChartWindow::exportCsv()
{
    if (charts.empty()) return;
    QString defaultname = filename + ".csv";
    if (filename.isEmpty()) defaultname = "lammpsdata.csv";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart as CSV data", defaultname,
                                                    "Image Files (*.csv)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setRealNumberPrecision(8);

            out << "Step";
            for (auto &c : charts)
                out << ',' << c->getTitle();
            out << '\n';

            int lines = charts[0]->getCount();
            for (int i = 0; i < lines; ++i) {
                // timestep
                out << charts[0]->getStep(i);
                for (auto &c : charts)
                    out << ',' << c->getData(i);
                out << '\n';
            }
            file.close();
        }
    }
}
void ChartWindow::exportYaml()
{
    if (charts.empty()) return;
    QString defaultname = filename + ".yaml";
    if (filename.isEmpty()) defaultname = "lammpsdata.yaml";
    QString fileName = QFileDialog::getSaveFileName(this, "Save Chart as YAML data", defaultname,
                                                    "Image Files (*.yaml, *.yml)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out.setRealNumberPrecision(8);
            out << "---\n";

            out << "keywords: ['Step'";
            for (auto &c : charts)
                out << ", " << c->getTitle();
            out << "]\n";

            out << "data: \n";
            int lines = charts[0]->getCount();
            for (int i = 0; i < lines; ++i) {
                // timestep
                out << "  - [" << charts[0]->getStep(i);
                // data
                for (auto &c : charts)
                    out << ", " << c->getData(i);
                out << "]\n";
            }
            out << "...\n";
            file.close();
        }
    }
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
        settings.setValue("chartx", width());
        settings.setValue("charty", height());
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
    QWidget(parent), lastStep(-1), index(_index), window(10), order(4), series(new QLineSeries),
    smooth(nullptr), doRaw(true), doSmooth(false)
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
    delete smooth;
    delete series;
}

/* -------------------------------------------------------------------- */

void ChartViewer::addData(int step, double data)
{
    if (lastStep < step) {
        lastStep = step;
        series->append(step, data);

        QSettings settings;
        // update the chart display only after at least updchart milliseconds have passed
        if (lastUpdate.msecsTo(QTime::currentTime()) > settings.value("updchart", "500").toInt()) {
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

QString ChartViewer::getTitle() const
{
    return backend->getTLabel();
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
    if (smooth) {
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
    // turn off raw plot
    if (!_doRaw) {
        if (doRaw) backend->removeSeries(series);
    }
    // turn off smooth plot
    if (!_doSmooth) {
        if (smooth) {
            backend->removeSeries(smooth);
            delete smooth;
            smooth = nullptr;
        }
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

void ChartViewer::refreshView()
{
    backend->refreshSeries();
}

// local implementation of Savitzky-Golay filter

namespace {

//! array of doubles
using float_vect = std::vector<double>;

//! array of ints;
using int_vect = std::vector<int>;

// forward declaration
float_vect sg_smooth(const float_vect &v, const std::size_t w, const int deg);

// savitzky golay smoothing.
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

/*! matrix class.
 *
 * This is a matrix class derived from a vector of float_vects.  Note that
 * the matrix elements indexed [row][column] with indices starting at 0 (c
 * style). Also note that because of its design looping through rows should
 * be faster than looping through columns.
 *
 * \brief two dimensional floating point array
 */
class float_mat : public std::vector<float_vect> {

public:
    // disable selected default constructors and assignment operators
    float_mat()                             = delete;
    float_mat(float_mat &&)                 = default;
    ~float_mat()                            = default;
    float_mat &operator=(const float_mat &) = delete;
    float_mat &operator=(float_mat &&)      = delete;

    //! constructor with sizes
    float_mat(const std::size_t rows, const std::size_t cols, const double def = 0.0);
    //! copy constructor for matrix
    float_mat(const float_mat &m);
    //! copy constructor for vector
    float_mat(const float_vect &v);

    //! get size
    std::size_t nr_rows() const { return size(); };
    //! get size
    std::size_t nr_cols() const { return front().size(); };
};

// constructor with sizes
float_mat::float_mat(const std::size_t rows, const std::size_t cols, const double defval) :
    std::vector<float_vect>(rows)
{
    for (std::size_t i = 0; i < rows; ++i) {
        (*this)[i].resize(cols, defval);
    }
}

// copy constructor for matrix
float_mat::float_mat(const float_mat &m) : std::vector<float_vect>(m.size())
{

    auto inew = begin();
    auto iold = m.begin();
    for (/* empty */; iold < m.end(); ++inew, ++iold) {
        const auto oldsz = iold->size();
        inew->resize(oldsz);
        const float_vect &oldvec(*iold);
        *inew = oldvec;
    }
}

// copy constructor for vector
float_mat::float_mat(const float_vect &v) : std::vector<float_vect>(1)
{

    const auto oldsz = v.size();
    front().resize(oldsz);
    front() = v;
}

//////////////////////
// Helper functions //
//////////////////////

//! permute() orders the rows of A to match the integers in the index array.
void permute(float_mat &A, int_vect &idx)
{
    int_vect i(idx.size());

    for (std::size_t j = 0; j < A.nr_rows(); ++j) {
        i[j] = j;
    }

    // loop over permuted indices
    for (std::size_t j = 0; j < A.nr_rows(); ++j) {
        if (i[j] != idx[j]) {

            // search only the remaining indices
            for (std::size_t k = j + 1; k < A.nr_rows(); ++k) {
                if (i[k] == idx[j]) {
                    std::swap(A[j], A[k]); // swap the rows and
                    i[k] = i[j];           // the elements of
                    i[j] = idx[j];         // the ordered index.
                    break;                 // next j
                }
            }
        }
    }
}

/*! \brief Implicit partial pivoting.
 *
 * The function looks for pivot element only in rows below the current
 * element, A[idx[row]][column], then swaps that row with the current one in
 * the index map. The algorithm is for implicit pivoting (i.e., the pivot is
 * chosen as if the max coefficient in each row is set to 1) based on the
 * scaling information in the vector scale. The map of swapped indices is
 * recorded in swp. The return value is +1 or -1 depending on whether the
 * number of row swaps was even or odd respectively. */
int partial_pivot(float_mat &A, const std::size_t row, const std::size_t col, float_vect &scale,
                  int_vect &idx)
{
    int swapNum = 1;

    // default pivot is the current position, [row,col]
    std::size_t pivot = row;
    double piv_elem   = fabs(A[idx[row]][col]) * scale[idx[row]];

    // loop over possible pivots below current
    for (std::size_t j = row + 1; j < A.nr_rows(); ++j) {

        const double tmp = fabs(A[idx[j]][col]) * scale[idx[j]];

        // if this elem is larger, then it becomes the pivot
        if (tmp > piv_elem) {
            pivot    = j;
            piv_elem = tmp;
        }
    }

    if (pivot > row) {         // bring the pivot to the diagonal
        int j      = idx[row]; // reorder swap array
        idx[row]   = idx[pivot];
        idx[pivot] = j;
        swapNum    = -swapNum; // keeping track of odd or even swap
    }
    return swapNum;
}

/*! \brief Perform backward substitution.
 *
 * Solves the system of equations A*b=a, ASSUMING that A is upper
 * triangular. If diag==1, then the diagonal elements are additionally
 * assumed to be 1.  Note that the lower triangular elements are never
 * checked, so this function is valid to use after a LU-decomposition in
 * place.  A is not modified, and the solution, b, is returned in a. */
void lu_backsubst(float_mat &A, float_mat &a, bool diag = false)
{
    for (int r = (A.nr_rows() - 1); r >= 0; --r) {
        for (int c = (A.nr_cols() - 1); c > r; --c) {
            for (std::size_t k = 0; k < A.nr_cols(); ++k) {
                a[r][k] -= A[r][c] * a[c][k];
            }
        }
        if (!diag) {
            for (std::size_t k = 0; k < A.nr_cols(); ++k) {
                a[r][k] /= A[r][r];
            }
        }
    }
}

/*! \brief Perform forward substitution.
 *
 * Solves the system of equations A*b=a, ASSUMING that A is lower
 * triangular. If diag==1, then the diagonal elements are additionally
 * assumed to be 1.  Note that the upper triangular elements are never
 * checked, so this function is valid to use after a LU-decomposition in
 * place.  A is not modified, and the solution, b, is returned in a. */
void lu_forwsubst(float_mat &A, float_mat &a, bool diag = true)
{
    for (int r = 0; r < (int)A.nr_rows(); ++r) {
        for (int c = 0; c < r; ++c) {
            for (std::size_t k = 0; k < A.nr_cols(); ++k) {
                a[r][k] -= A[r][c] * a[c][k];
            }
        }
        if (!diag) {
            for (std::size_t k = 0; k < A.nr_cols(); ++k) {
                a[r][k] /= A[r][r];
            }
        }
    }
}

/*! \brief Performs LU factorization in place.
 *
 * This is Crout's algorithm (cf., Num. Rec. in C, Section 2.3).  The map of
 * swapped indeces is recorded in idx. The return value is +1 or -1
 * depending on whether the number of row swaps was even or odd
 * respectively.  idx must be preinitialized to a valid set of indices
 * (e.g., {1,2, ... ,A.nr_rows()}). */
int lu_factorize(float_mat &A, int_vect &idx)
{
    float_vect scale(A.nr_rows()); // implicit pivot scaling
    for (std::size_t i = 0; i < A.nr_rows(); ++i) {
        double maxval = 0.0;
        for (std::size_t j = 0; j < A.nr_cols(); ++j) {
            maxval = std::max(fabs(A[i][j]), maxval);
        }
        if (maxval == 0.0) {
            return 0;
        }
        scale[i] = 1.0 / maxval;
    }

    int swapNum = 1;
    for (std::size_t c = 0; c < A.nr_cols(); ++c) {     // loop over columns
        swapNum *= partial_pivot(A, c, c, scale, idx);  // bring pivot to diagonal
        for (std::size_t r = 0; r < A.nr_rows(); ++r) { //  loop over rows
            std::size_t lim = (r < c) ? r : c;
            for (std::size_t j = 0; j < lim; ++j) {
                A[idx[r]][c] -= A[idx[r]][j] * A[idx[j]][c];
            }
            if (r > c) A[idx[r]][c] /= A[idx[c]][c];
        }
    }
    permute(A, idx);
    return swapNum;
}

/*! \brief Solve a system of linear equations.
 * Solves the inhomogeneous matrix problem with lu-decomposition. Note that
 * inversion may be accomplished by setting a to the identity_matrix. */
float_mat lin_solve(const float_mat &A, const float_mat &a)
{
    float_mat B(A);
    float_mat b(a);
    int_vect idx(B.nr_rows());

    for (std::size_t j = 0; j < B.nr_rows(); ++j) {
        idx[j] = j; // init row swap label array
    }
    lu_factorize(B, idx); // get the lu-decomp.
    permute(b, idx);      // sort the inhomogeneity to match the lu-decomp
    lu_forwsubst(B, b);   // solve the forward problem
    lu_backsubst(B, b);   // solve the backward problem
    return b;
}

///////////////////////
// related functions //
///////////////////////

//! Returns the inverse of a matrix using LU-decomposition.
float_mat invert(const float_mat &A)
{
    const std::size_t n = A.size();
    float_mat E(n, n, 0.0);
    const float_mat &B(A);

    for (std::size_t i = 0; i < n; ++i) {
        E[i][i] = 1.0;
    }

    return lin_solve(B, E);
}

//! returns the transposed matrix.
float_mat transpose(const float_mat &a)
{
    float_mat res(a.nr_cols(), a.nr_rows());

    for (std::size_t i = 0; i < a.nr_rows(); ++i) {
        for (std::size_t j = 0; j < a.nr_cols(); ++j) {
            res[j][i] = a[i][j];
        }
    }
    return res;
}

//! matrix multiplication.
float_mat operator*(const float_mat &a, const float_mat &b)
{
    float_mat res(a.nr_rows(), b.nr_cols());
    for (std::size_t i = 0; i < a.nr_rows(); ++i) {
        for (std::size_t j = 0; j < b.nr_cols(); ++j) {
            double sum(0.0);
            for (std::size_t k = 0; k < a.nr_cols(); ++k) {
                sum += a[i][k] * b[k][j];
            }
            res[i][j] = sum;
        }
    }
    return res;
}

//! calculate savitzky golay coefficients.
float_vect sg_coeff(const float_vect &b, const std::size_t deg)
{
    const std::size_t rows(b.size());
    const std::size_t cols(deg + 1);
    float_mat A(rows, cols);
    float_vect res(rows);

    // generate input matrix for least squares fit
    for (std::size_t i = 0; i < rows; ++i) {
        for (std::size_t j = 0; j < cols; ++j) {
            A[i][j] = pow(double(i), double(j));
        }
    }

    float_mat c(invert(transpose(A) * A) * (transpose(A) * transpose(b)));

    for (std::size_t i = 0; i < b.size(); ++i) {
        res[i] = c[0][0];
        for (std::size_t j = 1; j <= deg; ++j) {
            res[i] += c[j][0] * pow(double(i), double(j));
        }
    }
    return res;
}

/*! \brief savitzky golay smoothing.
 *
 * This method means fitting a polynome of degree 'deg' to a sliding window
 * of width 2w+1 throughout the data.  The needed coefficients are
 * generated dynamically by doing a least squares fit on a "symmetric" unit
 * vector of size 2w+1, e.g. for w=2 b=(0,0,1,0,0). evaluating the polynome
 * yields the sg-coefficients.  at the border non symmectric vectors b are
 * used. */
float_vect sg_smooth(const float_vect &v, const std::size_t width, const int deg)
{
    float_vect res(v.size(), 0.0);
    const std::size_t window = (2 * (std::size_t)width) + 1;
    const int endidx         = v.size() - 1;

    // do a regular sliding window average
    if (deg == 0) {
        // handle border cases first because we need different coefficients
        for (std::size_t i = 0; i < width; ++i) {
            const double scale = 1.0 / double(i + 1);
            const float_vect c1(width, scale);
            for (std::size_t j = 0; j <= i; ++j) {
                res[i] += c1[j] * v[j];
                res[endidx - i] += c1[j] * v[endidx - j];
            }
        }

        // now loop over rest of data. reusing the "symmetric" coefficients.
        const double scale = 1.0 / double(window);
        const float_vect c2(window, scale);
        for (std::size_t i = 0; i <= (v.size() - window); ++i) {
            for (std::size_t j = 0; j < window; ++j) {
                res[i + width] += c2[j] * v[i + j];
            }
        }
        return res;
    }

    // handle border cases first because we need different coefficients
    for (std::size_t i = 0; i < width; ++i) {
        float_vect b1(window, 0.0);
        b1[i] = 1.0;

        const float_vect c1(sg_coeff(b1, deg));
        for (std::size_t j = 0; j < window; ++j) {
            res[i] += c1[j] * v[j];
            res[endidx - i] += c1[j] * v[endidx - j];
        }
    }

    // now loop over rest of data. reusing the "symmetric" coefficients.
    float_vect b2(window, 0.0);
    b2[width] = 1.0;
    const float_vect c2(sg_coeff(b2, deg));

    for (std::size_t i = 0; i <= (v.size() - window); ++i) {
        for (std::size_t j = 0; j < window; ++j) {
            res[i + width] += c2[j] * v[i + j];
        }
    }
    return res;
}
} // namespace

/* -------------------------------------------------------------------- */

// update smooth plot data

void ChartViewer::updateSmooth()
{
    QSettings settings;
    settings.beginGroup("charts");
    int rawidx    = settings.value("rawbrush", 1).toInt();
    int smoothidx = settings.value("smoothbrush", 2).toInt();
    if ((rawidx < 0) || (rawidx >= mybrushes.size())) rawidx = 0;
    if ((smoothidx < 0) || (smoothidx >= mybrushes.size())) smoothidx = 0;
    settings.endGroup();

    if (doRaw) {
        // add raw data if not in chart
        if (!backend->hasSeries(series)) backend->addSeries(series, mybrushes[rawidx].color(), 3.0);
    }

    if (doSmooth) {
        if (series->count() > (2 * window)) {
            if (!smooth) {
                smooth = new QLineSeries;
                backend->addSeries(smooth, mybrushes[smoothidx].color(), 3.0);
            }
            smooth->replace(calc_sgsmooth(series->points(), window, order));
        }
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
