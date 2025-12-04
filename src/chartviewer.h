// -*- c++ -*- /////////////////////////////////////////////////////////////////////////
// LAMMPS-GUI - A Graphical Tool to Learn and Explore the LAMMPS MD Simulation Software
//
// Copyright (c) 2023, 2024, 2025  Axel Kohlmeyer
//
// Documentation: https://lammps-gui.lammps.org/
// Contact: akohlmey@gmail.com
//
// This software is distributed under the GNU General Public License version 2 or later.
////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHARTVIEWER_H
#define CHARTVIEWER_H

#include <QComboBox>
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
class QLabel;
class QMenuBar;
class QMenu;
class QSpinBox;
class RangeSlider;

namespace QtCharts {
class ChartViewer;
}

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
    int num_charts() const { return charts.size(); }

    /**
     * @brief Check if a chart at given index has the specified title
     * @param title Title to check
     * @param index Chart index
     * @return true if chart has the title, false otherwise
     */
    bool has_title(const QString &title, int index) const
    {
        return (columns->itemText(index) == title);
    }

    /**
     * @brief Get the current simulation step number
     * @return Current step
     */
    int get_step() const;

    /**
     * @brief Reset all charts to initial state
     */
    void reset_charts();

    /**
     * @brief Add a new chart to the window
     * @param title Chart title (thermodynamic property name)
     * @param index Chart index
     */
    void add_chart(const QString &title, int index);

    /**
     * @brief Add a data point to a chart
     * @param step Simulation step number
     * @param data Data value
     * @param index Chart index
     */
    void add_data(int step, double data, int index);

    /**
     * @brief Set the units displayed for thermodynamic quantities
     * @param _units Units string (e.g., "real", "metal", "lj")
     */
    void set_units(const QString &_units);

    /**
     * @brief Enable/disable data normalization
     * @param norm true to normalize data, false otherwise
     */
    void set_norm(bool norm);

private slots:
    void quit();                           ///< Close window and quit
    void stop_run();                       ///< Stop running simulation
    void select_smooth(int selection);     ///< Select smoothing algorithm
    void update_smooth();                  ///< Update smoothing parameters
    void update_tlabel();                  ///< Update chart title
    void update_ylabel();                  ///< Update Y-axis label
    void update_xrange(int low, int high); ///< Update X-axis range
    void update_yrange(int low, int high); ///< Update Y-axis range

    void saveAs();     ///< Save chart as image
    void exportDat();  ///< Export data in DAT format
    void exportCsv();  ///< Export data in CSV format
    void exportYaml(); ///< Export data in YAML format

    void change_chart(int index); ///< Switch to different chart

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
    bool do_raw, do_smooth; ///< Flags for displaying raw/smoothed data
    QMenuBar *menu;         ///< Menu bar
    QMenu *file;            ///< File menu
    QComboBox *columns;     ///< Dropdown for selecting chart
    QAction *saveAsAct, *exportCsvAct, *exportDatAct, *exportYamlAct; ///< Export actions
    QAction *closeAct, *stopAct, *quitAct;                            ///< Window control actions
    QComboBox *smooth;                   ///< Smoothing algorithm selector
    QSpinBox *window, *order;            ///< Smoothing parameters
    QLineEdit *chartTitle, *chartYlabel; ///< Chart labels
    QLabel *units;                       ///< Units display
    QCheckBox *norm;                     ///< Normalization checkbox
    RangeSlider *xrange, *yrange;        ///< Range sliders for axes

    QString filename;                      ///< Log file path
    QList<QtCharts::ChartViewer *> charts; ///< List of chart viewers
};

/* -------------------------------------------------------------------- */

#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
class QChart;

namespace QtCharts {
/**
 * @brief Individual chart viewer for displaying a single time-series
 *
 * ChartViewer extends QChartView to display a single thermodynamic
 * property as a function of simulation time. It supports both raw
 * and smoothed data display, interactive zoom/pan, and provides
 * accessors for data export.
 */
class ChartViewer : public QChartView {
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
    void add_data(int step, double data);

    /**
     * @brief Get the min/max bounds of the data
     * @return Rectangle containing data bounds
     */
    QRectF get_minmax() const;

    /**
     * @brief Get list of chart axes
     * @return List of axes (X and Y)
     */
    QList<QAbstractAxis *> get_axes() const { return chart->axes(); }

    /**
     * @brief Reset zoom to show all data
     */
    void reset_zoom();

    /**
     * @brief Set smoothing parameters
     * @param _do_raw Show raw data series
     * @param _do_smooth Show smoothed data series
     * @param _window Smoothing window size
     * @param _order Polynomial order for Savitzky-Golay
     */
    void smooth_param(bool _do_raw, bool _do_smooth, int _window, int _order);

    /**
     * @brief Recalculate and update smoothed data
     */
    void update_smooth();

    /**
     * @brief Get chart index
     * @return Index of this chart
     */
    int get_index() const { return index; };

    /**
     * @brief Get number of data points
     * @return Number of points in series
     */
    int get_count() const { return series->count(); }

    /**
     * @brief Get chart title
     * @return Title string
     */
    QString get_title() const { return series->name(); }

    /**
     * @brief Get step number at given index
     * @param index Data point index
     * @return Step number (X value)
     */
    double get_step(int index) const { return (index < 0) ? 0.0 : series->at(index).x(); }

    /**
     * @brief Get data value at given index
     * @param index Data point index
     * @return Data value (Y value)
     */
    double get_data(int index) const { return (index < 0) ? 0.0 : series->at(index).y(); }

    /**
     * @brief Set chart title
     * @param tlabel New title
     */
    void set_tlabel(const QString &tlabel);

    /**
     * @brief Set Y-axis label
     * @param ylabel New Y-axis label
     */
    void set_ylabel(const QString &ylabel);

    /**
     * @brief Get current chart title
     * @return Chart title
     */
    QString get_tlabel() const { return chart->title(); }

    /**
     * @brief Get X-axis label
     * @return X-axis label
     */
    QString get_xlabel() const { return xaxis->titleText(); }

    /**
     * @brief Get Y-axis label
     * @return Y-axis label
     */
    QString get_ylabel() const { return yaxis->titleText(); }

private:
    int last_step, index;         ///< Last step processed, chart index
    int window, order;            ///< Smoothing window and polynomial order
    QChart *chart;                ///< The chart object
    QLineSeries *series, *smooth; ///< Raw and smoothed data series
    QValueAxis *xaxis;            ///< X-axis (time/step)
    QValueAxis *yaxis;            ///< Y-axis (property value)
    QTime last_update;            ///< Time of last chart update
    bool do_raw, do_smooth;       ///< Flags for showing raw/smoothed data
};
} // namespace QtCharts
#endif

// Local Variables:
// c-basic-offset: 4
// End:
