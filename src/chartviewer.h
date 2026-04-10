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

#ifndef CHARTVIEWER_H
#define CHARTVIEWER_H

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QRectF>
#include <QString>
#include <QTime>
#include <QWidget>

class QAction;
class QCheckBox;
class QCloseEvent;
class QEvent;
class QMenuBar;
class QMenu;
class QSpinBox;
class RangeSlider;

class ChartViewer;

/**
 * @brief Window for displaying and managing multiple time-series charts
 *
 * ChartWindow provides a GUI for displaying and manipulating multiple
 * charts showing time-series data from LAMMPS simulations (thermodynamic
 * output). It supports features like data smoothing, normalization,
 * zoom/pan, and export to various formats (PNG, CSV, YAML, DAT).
 */
class ChartWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param filename Path to the log file containing the data
     * @param parent Parent widget
     */
    ChartWindow(const QString &filename, QWidget *parent = nullptr);

    /**
     * @brief Get the number of charts currently displayed
     * @return Number of charts
     */
    int numCharts() const { return charts.size(); }

    /**
     * @brief Check if a chart at given index has the specified title
     * @param title Title to check
     * @param index Chart index
     * @return true if chart has the title, false otherwise
     */
    bool hasTitle(const QString &title, int index) const
    {
        return (columns->itemText(index) == title);
    }

    /**
     * @brief Get the current simulation step number
     * @return Current step
     */
    int getStep() const;

    /**
     * @brief Reset all charts to initial state
     */
    void resetCharts();

    /**
     * @brief Manually update chart display zoom status
     *
     * This is needed at an end of a run when the run finishes too quickly
     * and the regular chart update has not yet triggered.
     */
    void resetZoom();

    /**
     * @brief Add a new chart to the window
     * @param title Chart title (thermodynamic property name)
     * @param index Chart index
     */
    void addChart(const QString &title, int index);

    /**
     * @brief Add a data point to a chart
     * @param step Simulation step number
     * @param data Data value
     * @param index Chart index
     */
    void addData(int step, double data, int index);

    /**
     * @brief Set the units displayed for thermodynamic quantities
     * @param _units Units string (e.g., "real", "metal", "lj")
     */
    void setUnits(const QString &_units);

    /**
     * @brief Enable/disable data normalization
     * @param norm true to normalize data, false otherwise
     */
    void setNorm(bool norm);

private slots:
    void quit();                          ///< Close window and quit
    void stopRun();                       ///< Stop running simulation
    void selectSmooth(int selection);     ///< Select smoothing algorithm
    void updateSmooth();                  ///< Update smoothing parameters
    void updateTLabel();                  ///< Update chart title
    void updateYLabel();                  ///< Update Y-axis label
    void updateXRange(int low, int high); ///< Update X-axis range
    void updateYRange(int low, int high); ///< Update Y-axis range

    void copy();       ///< Copy image to clipboard
    void saveAs();     ///< Save chart as image
    void exportDat();  ///< Export data in DAT format
    void exportCsv();  ///< Export data in CSV format
    void exportYaml(); ///< Export data in YAML format

    void changeChart(int index); ///< Switch to different chart

protected:
    /**
     * @brief Handle window close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Event filter for keyboard shortcuts
     * @param watched Object being watched
     * @param event Event to filter
     * @return true if event handled, false otherwise
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    bool doRaw, doSmooth; ///< Flags for displaying raw/smoothed data
    QMenuBar *menu;       ///< Menu bar
    QMenu *file;          ///< File menu
    QComboBox *columns;   ///< Dropdown for selecting chart
    QAction *saveAsAct, *copyAct, *exportCsvAct, *exportDatAct, *exportYamlAct; ///< Export actions
    QAction *closeAct, *stopAct, *quitAct; ///< Window control actions
    QComboBox *smooth;                     ///< Smoothing algorithm selector
    QSpinBox *window, *order;              ///< Smoothing parameters
    QLineEdit *chartTitle, *chartYlabel;   ///< Chart labels
    QLabel *units;                         ///< Units display
    QCheckBox *norm;                       ///< Normalization checkbox
    RangeSlider *xrange, *yrange;          ///< Range sliders for axes

    QString filename;            ///< Log file path
    QList<ChartViewer *> charts; ///< List of chart viewers
};

/* -------------------------------------------------------------------- */

#ifdef LAMMPS_GUI_USE_QTGRAPHS

#include <QtGraphs/QAbstractAxis>
#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

class QLabel;
class QQuickWidget;
class QQuickItem;
class VerticalLabel;

#else

#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>

class QChart;

#endif

/**
 * @brief Individual chart viewer for displaying a single time-series
 *
 * ChartViewer wraps a QGraphsView via QQuickWidget to display a single
 * to display a single thermodynamic property as a function of simulation
 * time. It supports both raw and smoothed data display, interactive
 * zoom/pan, and provides accessors for data export.
 */
#ifdef LAMMPS_GUI_USE_QTGRAPHS
class ChartViewer : public QWidget {
#else
class ChartViewer : public QChartView {
#endif
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param title Chart title (property name)
     * @param index Chart index
     * @param parent Parent widget
     */
    explicit ChartViewer(const QString &title, int index, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ChartViewer() override;

    ChartViewer()                               = delete;
    ChartViewer(const ChartViewer &)            = delete;
    ChartViewer(ChartViewer &&)                 = delete;
    ChartViewer &operator=(const ChartViewer &) = delete;
    ChartViewer &operator=(ChartViewer &&)      = delete;

    /**
     * @brief Add a data point to the chart
     * @param step Simulation step number
     * @param data Data value for this step
     */
    void addData(int step, double data);

    /**
     * @brief Get the min/max bounds of the data
     * @return Rectangle containing data bounds
     */
    QRectF getMinMax() const;

    /**
     * @brief Get list of chart axes
     * @return List of axes (X and Y)
     */
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    QList<QAbstractAxis *> getAxes() const { return {xaxis, yaxis}; }
#else
    QList<QAbstractAxis *> getAxes() const { return chart->axes(); }
#endif

    /**
     * @brief Reset zoom to show all data
     */
    void resetZoom();

    /**
     * @brief Set smoothing parameters
     * @param _doRaw Show raw data series
     * @param _doSmooth Show smoothed data series
     * @param _window Smoothing window size
     * @param _order Polynomial order for Savitzky-Golay
     */
    void smoothParam(bool _doRaw, bool _doSmooth, int _window, int _order);

    /**
     * @brief Recalculate and update smoothed data
     */
    void updateSmooth();

    /**
     * @brief Get chart index
     * @return Index of this chart
     */
    int getIndex() const { return index; };

    /**
     * @brief Get number of data points
     * @return Number of points in series
     */
    int getCount() const { return series->count(); }

    /**
     * @brief Get chart title
     * @return Title string
     */
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    QString getTitle() const { return titleWidget->text(); }
#else
    QString getTitle() const { return chart->title(); }
#endif

    /**
     * @brief Get step number at given index
     * @param index Data point index
     * @return Step number (X value)
     */
    double getStep(int index) const { return (index < 0) ? 0.0 : series->at(index).x(); }

    /**
     * @brief Get data value at given index
     * @param index Data point index
     * @return Data value (Y value)
     */
    double getData(int index) const { return (index < 0) ? 0.0 : series->at(index).y(); }

    /**
     * @brief Set chart title
     * @param tlabel New title
     */
    void setTLabel(const QString &tlabel);

    /**
     * @brief Set Y-axis label
     * @param ylabel New Y-axis label
     */
    void setYLabel(const QString &ylabel);

    /**
     * @brief Get current chart title
     * @return Chart title
     */
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    QString getTLabel() const { return titleWidget->text(); }
#else
    QString getTLabel() const { return chart->title(); }
#endif

    /**
     * @brief Get X-axis label
     * @return X-axis label
     */
    QString getXLabel() const { return xaxis->titleText(); }

    /**
     * @brief Get Y-axis label
     * @return Y-axis label
     */
    QString getYLabel() const { return yaxis->titleText(); }

private:
    int lastStep, index; ///< Last step processed, chart index
    int window, order;   ///< Smoothing window and polynomial order
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    QQuickWidget *quickWidget;   ///< Widget hosting the QGraphsView QML item
    QQuickItem *graphsView;      ///< Root QGraphsView QML item
    VerticalLabel *ylabelWidget; ///< External y-axis title label (avoids overlap)
    QLabel *xlabelWidget;        ///< External x-axis title label (with spacing)
    QLabel *titleWidget;         ///< Chart title (with spacing)
#else
    QChart *chart; ///< The chart object
#endif
    QLineSeries *series, *smooth; ///< Raw and smoothed data series
    QValueAxis *xaxis;            ///< X-axis (time/step)
    QValueAxis *yaxis;            ///< Y-axis (property value)
    QTime lastUpdate;             ///< Time of last chart update
    bool doRaw, doSmooth;         ///< Flags for showing raw/smoothed data
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
