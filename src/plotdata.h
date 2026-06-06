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

#ifndef PLOTDATA_H
#define PLOTDATA_H

// Column-oriented numeric data model and file parsers used to plot data from
// external structured files (whitespace/.dat, CSV, LAMMPS YAML, JSON). The
// numeric payload is stored as std::vector<double> so it can be fed directly
// to the leastsquares toolkit for the planned polynomial / EOS fits.

#include <QByteArray>
#include <QString>
#include <QStringList>
#include <vector>

/**
 * @brief Column-oriented table of named numeric columns
 *
 * Holds a set of equally long columns of doubles, each with a name, plus an
 * optional units string. It is the GUI-free data model shared by the file
 * parsers and the chart window when plotting external data.
 */
class PlotData {
public:
    PlotData()                            = default;
    ~PlotData()                           = default;
    PlotData(const PlotData &)            = default; ///< Copy constructor
    PlotData(PlotData &&)                 = default; ///< Move constructor
    PlotData &operator=(const PlotData &) = default; ///< Copy assignment
    PlotData &operator=(PlotData &&)      = default; ///< Move assignment

    /** @brief Number of columns */
    int columnCount() const { return static_cast<int>(cols.size()); }
    /** @brief Number of rows (length of the columns; 0 if empty) */
    int rowCount() const { return cols.empty() ? 0 : static_cast<int>(cols.front().size()); }
    /** @brief True if there are no columns or no rows */
    bool isEmpty() const { return rowCount() == 0; }

    /**
     * @brief Name of a column
     * @param c Column index
     * @return Column name
     */
    const QString &columnName(int c) const { return names[c]; }

    /** @brief All column names */
    const QStringList &columnNames() const { return names; }

    /**
     * @brief Read access to a column's values
     * @param c Column index
     * @return Reference to the column data
     */
    const std::vector<double> &column(int c) const { return cols[c]; }

    /** @brief Optional units string (e.g. the LAMMPS unit system); may be empty */
    const QString &units() const { return unitstr; }
    /**
     * @brief Set the optional units string
     * @param u Units string
     */
    void setUnits(const QString &u) { unitstr = u; }

    /**
     * @brief Reset the table to a fresh set of (empty) named columns
     * @param columnNames Names of the columns to create
     */
    void setColumnNames(const QStringList &columnNames);

    /**
     * @brief Append one row of values, one per column
     * @param row Values to append (size must equal columnCount())
     * @return true on success, false if the size does not match
     */
    bool appendRow(const std::vector<double> &row);

    /**
     * @brief Append a complete named column
     * @param name Column name
     * @param data Column values
     *
     * Used by the column-wise parsers (e.g. JSON object-of-arrays).
     */
    void addColumn(const QString &name, std::vector<double> data);

    /** @brief Remove all columns and data */
    void clear();

private:
    QStringList names;                     ///< per-column names
    std::vector<std::vector<double>> cols; ///< column-major numeric payload
    QString unitstr;                       ///< optional units string
};

/**
 * @brief Parse comma-separated values into a PlotData
 * @param text  File contents
 * @param error Optional out-parameter set to a message on failure
 * @return Parsed table (empty on failure)
 *
 * The first line is treated as a header of column names unless every field
 * parses as a number, in which case generic names are generated and the line
 * is treated as data.
 */
PlotData parsePlotCsv(const QString &text, QString *error = nullptr);

/**
 * @brief Parse whitespace-separated columns (gnuplot / LAMMPS `.dat`) into a PlotData
 * @param text  File contents
 * @param error Optional out-parameter set to a message on failure
 * @return Parsed table (empty on failure)
 *
 * Lines beginning with `#` are comments; the last comment line before the
 * data is used for column names if its field count matches the data.
 */
PlotData parsePlotWhitespace(const QString &text, QString *error = nullptr);

/**
 * @brief Parse the LAMMPS thermo YAML form (`keywords:` + `data:`) into a PlotData
 * @param text  File contents
 * @param error Optional out-parameter set to a message on failure
 * @return Parsed table (empty on failure)
 */
PlotData parsePlotYaml(const QString &text, QString *error = nullptr);

/**
 * @brief Parse JSON into a PlotData
 * @param bytes File contents
 * @param error Optional out-parameter set to a message on failure
 * @return Parsed table (empty on failure)
 *
 * Supports two simple shapes only: an array of equally long numeric rows
 * (`[[...],[...]]`, generic column names) and an object mapping names to
 * numeric arrays (`{"a":[...],"b":[...]}`).
 */
PlotData parsePlotJson(const QByteArray &bytes, QString *error = nullptr);

/**
 * @brief Load a file into a PlotData, choosing the parser by file extension
 * @param filename Path to the data file (.csv, .yaml/.yml, .json, else whitespace)
 * @param error    Optional out-parameter set to a message on failure
 * @return Parsed table (empty on failure)
 */
PlotData loadPlotData(const QString &filename, QString *error = nullptr);

#endif

// Local Variables:
// c-basic-offset: 4
// End:
