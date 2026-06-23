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
class LammpsGui;
class PlotData;

/**
 * @brief A labeled vertical reference line for chart overlays
 *
 * Used by the "Reference Lines..." dialog to annotate charts with
 * vertical markers at specific x positions (e.g. high-symmetry k-points
 * in phonon-dispersion plots).
 */
/** @brief Orientation of a reference line: a vertical line at x, or a horizontal line at y */
enum class RefOrient { Vertical, Horizontal };

struct RefLine {
    RefOrient orient = RefOrient::Vertical; ///< vertical (fixed x) or horizontal (fixed y)
    double value;                           ///< x position (vertical) or y position (horizontal)
    QString label;                          ///< text label (in line color)
    QColor color;                           ///< line color (default: dark gray)
};

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
     * @param lammpsgui Pointer to LammpsGui for sending signals (optional;
     *                  nullptr for a standalone window with no live simulation)
     * @param parent Parent widget
     */
    explicit ChartWindow(const QString &filename, LammpsGui *lammpsgui = nullptr,
                         QWidget *parent = nullptr);

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

    /**
     * @brief Enable/Disable range sliders
     * @param enabled  true to enable sliders and false to disable them
     */
    void setRangeEnabled(bool enabled);

    /**
     * @brief Populate the window from an external data table (file plotting)
     * @param data  Parsed column data
     * @param xcol  Index of the column to use as the shared x axis
     * @param ycols Indices of the columns to plot, one chart each
     *
     * Replaces any existing charts; each selected y column becomes a chart
     * titled by its column name, with the x axis labeled by the x column.
     * Unlike the live thermo feed this loads all rows in one shot.
     */
    void loadData(const PlotData &data, int xcol, const QList<int> &ycols);

private slots:
    void quit();                          ///< Close window and quit
    void stopRun();                       ///< Stop running simulation
    void changeStyle();                   ///< Edit the current chart's display style
    void postProcess();                   ///< Run an analysis on the current chart's data
    void addDataFile();                   ///< Add data from another file as overlay series
    void referenceLines();                ///< Edit vertical reference lines for all charts
    void selectSmooth(int selection);     ///< Select smoothing algorithm
    void updateSmooth();                  ///< Update smoothing parameters
    void updateTLabel();                  ///< Update chart title
    void updateYLabel();                  ///< Update Y-axis label
    void updateXLabel();                  ///< Update X-axis label (standalone plot mode)
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
    /// Collect the displayed charts into a PlotData (column 0 "Step", then one
    /// column per chart) for the data exporters.
    PlotData chartsToPlotData() const;

    /// Return the chart currently selected in the columns combo box.
    ChartViewer *currentChart();

    LammpsGui *lammpsgui; ///< Main widget pointer for receiving signals
    bool doRaw, doSmooth; ///< Flags for displaying raw/smoothed data
    QMenuBar *menu;       ///< Menu bar
    QMenu *file;          ///< File menu
    QComboBox *columns;   ///< Dropdown for selecting chart
    QAction *saveAsAct, *copyAct, *exportCsvAct, *exportDatAct, *exportYamlAct; ///< Export actions
    QAction *closeAct, *stopAct, *quitAct; ///< Window control actions
    QAction *addDataAct;                   ///< "Add Data from File..." (standalone only)
    QAction *refLinesAct;                  ///< "Reference Lines..."
    QComboBox *smooth;                     ///< Smoothing algorithm selector
    QSpinBox *window, *order;              ///< Smoothing parameters
    QLineEdit *chartTitle, *chartYlabel,
        *chartXlabel;             ///< Chart labels (chartXlabel standalone only)
    QLabel *units;                ///< Units display
    QCheckBox *norm;              ///< Normalization checkbox
    RangeSlider *xrange, *yrange; ///< Range sliders for axes

    QString filename;            ///< Log file path
    QList<ChartViewer *> charts; ///< List of chart viewers
    QList<RefLine> refLines;     ///< Current set of reference lines (applied to all charts)
};

/* -------------------------------------------------------------------- */

#include "plotseries.h"

#include <QColor>

#include <memory>
#include <vector>

class PlotWidget;

/**
 * @brief How a chart's raw data series is drawn
 */
enum class ChartDisplayMode {
    Lines,          ///< connect data points with lines only
    Points,         ///< draw data points as markers only
    LinesAndPoints, ///< draw both lines and markers
};

/**
 * @brief Individual chart viewer for displaying a single time-series
 *
 * ChartViewer displays a single thermodynamic property as a function
 * of simulation time. It owns the neutral PlotSeries data objects and
 * renders them with a PlotWidget, supporting both raw and smoothed data
 * display, interactive zoom/pan, and data export.
 *
 * @see PlotWidget, PlotSeries
 */
class ChartViewer : public QWidget {
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
     * @brief Append an (x, y) data point to the chart
     * @param x X value (e.g. the simulation step, or an arbitrary abscissa)
     * @param y Y value
     *
     * The point is appended only if @p x is greater than the last appended x;
     * this keeps the live thermo feed monotonic in the step number.
     */
    void addPoint(double x, double y);

    /**
     * @brief Get the min/max bounds of the data
     * @return Rectangle containing data bounds
     */
    QRectF getMinMax() const;

    /** @brief Set the displayed X-axis range (used by the range sliders) */
    void setXAxisRange(double min, double max);
    /** @brief Set the displayed Y-axis range (used by the range sliders) */
    void setYAxisRange(double min, double max);

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
     * @brief Get the series (data column) name
     * @return The property/column name this chart was created with
     *
     * This is the per-column identifier (e.g. the thermo keyword), distinct
     * from the shared plot title returned by getTLabel(). Used for data-export
     * column headers.
     */
    QString getName() const;

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
     * @brief Set the X-axis label
     * @param xlabel New X-axis label
     */
    void setXLabel(const QString &xlabel);

    /**
     * @brief Set the X-axis tick label format
     * @param fmt printf-style format string (e.g. "%.6g" for floating-point,
     *            "%d" for integer steps)
     *
     * Call after setXLabel() when the x-axis carries non-integer data (e.g.
     * lattice constants from a Plot Data file).  The thermo live-feed path
     * leaves the default integer format in place.
     */
    void setXLabelFormat(const QString &fmt);

    /**
     * @brief Replace all chart data with the given points in one shot
     * @param points New (x, y) data points
     *
     * Bulk loader for file plotting; bypasses the monotonic-x guard and the
     * throttled live-update path used by addPoint().
     */
    void setPoints(const QList<QPointF> &points);

    /**
     * @brief Add an overlay data series from a second file
     * @param pts   (x, y) data points
     * @param name  Series name (shown as a tooltip / legend entry)
     * @param color Line color
     *
     * Overlay series are always shown in full (no smoothing); they are
     * included in the axis range calculation.
     */
    void addOverlaySeries(const QList<QPointF> &pts, const QString &name, const QColor &color);

    /** @brief Remove all overlay series added via addOverlaySeries() */
    void clearOverlaySeries();

    /** @brief Number of overlay series currently displayed */
    int overlaySeriesCount() const { return static_cast<int>(overlaySeries.size()); }

    /**
     * @brief Set vertical reference lines (replaces any existing set)
     * @param lines List of reference line descriptors
     *
     * Each line spans the full data y-range and is updated on every zoom reset.
     * Lines are identified by their position (x), label (series name), and color.
     */
    void setReferenceLines(const QList<RefLine> &lines);

    /** @brief Remove all vertical reference lines */
    void clearVerticalLines();

    /**
     * @brief Set how the raw data series is displayed
     * @param mode  Lines, points, or both
     * @param color Series color (invalid color falls back to the theme default)
     * @param width Line width (used for the line and lines+points modes)
     * @param pointSize Marker diameter (used for the points and lines+points modes)
     */
    void setDisplayStyle(ChartDisplayMode mode, const QColor &color, qreal width, qreal pointSize);

    /** @brief Current display mode */
    ChartDisplayMode displayMode() const { return dispmode; }
    /** @brief Current series color (may be invalid, meaning the theme default) */
    QColor displayColor() const { return rawColor; }
    /** @brief Current line width */
    qreal displayWidth() const { return rawWidth; }
    /** @brief Current marker diameter */
    qreal displayPointSize() const { return rawPointSize; }

    /**
     * @brief Set how the processed (smoothed) data series is displayed
     * @param mode  Lines, points, or both
     * @param color Series color (invalid color falls back to the theme default)
     * @param width Line width (used for the line and lines+points modes)
     * @param pointSize Marker diameter (used for the points and lines+points modes)
     */
    void setSmoothStyle(ChartDisplayMode mode, const QColor &color, qreal width, qreal pointSize);

    /** @brief Current processed-series display mode */
    ChartDisplayMode smoothMode() const { return smoothmode; }
    /** @brief Current processed-series color (may be invalid, meaning the theme default) */
    QColor smoothColor() const { return smoothcolor; }
    /** @brief Current processed-series line width */
    qreal smoothWidth() const { return smoothwidth; }
    /** @brief Current processed-series marker diameter */
    qreal smoothPointSize() const { return smoothpointsize; }

    /**
     * @brief Overlay a fit curve on the chart
     * @param points  Curve points (x, y) drawn as an overlay line; created on
     *                the first call and replaced on subsequent calls
     * @param name    Optional series name for the overlay (e.g. the fitted
     *                expression or a user label); shown wherever series names
     *                are surfaced
     * @param eosFit  When true the curve is treated as an EOS fit: its
     *                visibility follows the doSmooth flag (hidden in Raw mode,
     *                visible in Smoothed/Both mode) and it replaces the
     *                Savitzky-Golay series while active.
     */
    void setFitCurve(const QList<QPointF> &points, const QString &name = QString(),
                     bool eosFit = false);

    /** @brief True when the current fit overlay is a Birch-Murnaghan EOS fit */
    bool isEosFit() const { return eosMode && fit && !fit->points.isEmpty(); }

    /**
     * @brief Get current chart title
     * @return Chart title
     */
    QString getTLabel() const;

    /**
     * @brief Get X-axis label
     * @return X-axis label
     */
    QString getXLabel() const;

    /**
     * @brief Get Y-axis label
     * @return Y-axis label
     */
    QString getYLabel() const;

private:
    /// Add (or restyle) a line series and its on-demand scatter twin to show
    /// the data as lines, points, or both, in the given color and width.
    void renderSeries(PlotSeries *line, std::unique_ptr<PlotSeries> &points, ChartDisplayMode mode,
                      const QColor &color, qreal width, qreal pointSize);

    /// Register a series with the renderer in the given color and width.
    void addPlotSeries(PlotSeries *s, const QColor &color, qreal width);
    /// Restyle an already-registered series and request a repaint.
    void stylePlotSeries(PlotSeries *s, const QColor &color, qreal width);

    PlotWidget *plot;                          ///< Renderer (Qt child of this widget)
    double lastX;                              ///< Last (largest) x value appended
    int updChart;                              ///< Cached live-update throttle interval (ms)
    double rawXmin;                            ///< Running min x of the raw series (live path)
    double rawXmax;                            ///< Running max x of the raw series
    double rawYmin;                            ///< Running min y of the raw series
    double rawYmax;                            ///< Running max y of the raw series
    int index;                                 ///< Chart index
    int window, order;                         ///< Smoothing window and polynomial order
    std::unique_ptr<PlotSeries> series;        ///< Raw data series (always present)
    std::unique_ptr<PlotSeries> smooth;        ///< Smoothed data series (created on demand)
    std::unique_ptr<PlotSeries> scatter;       ///< Raw data as points (created on demand)
    std::unique_ptr<PlotSeries> smoothScatter; ///< Processed data as points (created on demand)
    std::unique_ptr<PlotSeries> fit;           ///< Optional fit-curve overlay (created on demand)
    QTime lastUpdate;                          ///< Time of last chart update
    bool doRaw, doSmooth;                      ///< Flags for showing raw/smoothed data
    bool eosMode;              ///< True when fit is a BM EOS overlay (visibility follows doSmooth)
    ChartDisplayMode dispmode; ///< How the raw series is drawn
    QColor rawColor;           ///< Raw series color override (invalid = theme default)
    qreal rawWidth;            ///< Raw series line width
    qreal rawPointSize;        ///< Raw series marker diameter
    ChartDisplayMode smoothmode; ///< How the processed series is drawn
    QColor smoothcolor;          ///< Processed series color (invalid = theme default)
    qreal smoothwidth;           ///< Processed series line width
    qreal smoothpointsize;       ///< Processed series marker diameter
    std::vector<std::unique_ptr<PlotSeries>> overlaySeries; ///< Extra series from secondary files
    std::vector<std::unique_ptr<PlotSeries>> vlines;        ///< Reference line series (decorative)
    QList<RefLine> reflineDefs; ///< Reference line definitions (parallel to vlines)
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
