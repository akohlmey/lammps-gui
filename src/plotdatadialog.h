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

#ifndef PLOTDATADIALOG_H
#define PLOTDATADIALOG_H

#include "plotdata.h"

#include <QDialog>
#include <QList>
#include <QStringList>

class QButtonGroup;
class QCheckBox;
class QGridLayout;
class QLineEdit;

/**
 * @brief Dialog to choose which columns of a PlotData to plot
 *
 * Presents the parsed columns of an external data file and lets the user
 * assign a role to each column: exactly one column is the shared x-axis
 * (exclusive radio buttons), any number of columns are plotted on the
 * y-axis (checkboxes), and columns with neither selected are ignored.
 * When the x-axis selection moves to a different column, the previous
 * x-axis column becomes a y-axis column.
 * A small preview of the first rows is shown to help identify column content.
 *
 * A "Compute derived column" section at the bottom lets the user add derived columns
 * from expressions that reference the existing column names as variables
 * (e.g. @c nfcc/ntot or @c load_eV_per_Ang*1.602176634).  The column's
 * first-row value is also available under @c colname_first.
 */
class PlotDataDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param data   Parsed column data to choose from (stored as a working copy)
     * @param parent Parent widget
     */
    explicit PlotDataDialog(const PlotData &data, QWidget *parent = nullptr);
    ~PlotDataDialog() override = default;

    PlotDataDialog()                                  = delete;
    PlotDataDialog(const PlotDataDialog &)            = delete;
    PlotDataDialog(PlotDataDialog &&)                 = delete;
    PlotDataDialog &operator=(const PlotDataDialog &) = delete;
    PlotDataDialog &operator=(PlotDataDialog &&)      = delete;

    /**
     * @brief Index of the column used as the x-axis
     *
     * Returns the index of the column whose x-axis radio button is selected.
     * Falls back to 0 if no column is selected as the x-axis.
     * Indices refer to @ref buildData() columns.
     * @return Column index
     */
    int xColumn() const;

    /**
     * @brief Indices of the columns selected to plot on the y-axis
     *
     * Indices refer to @ref buildData() columns.
     * @return List of column indices (all columns with a checked y checkbox)
     */
    QList<int> yColumns() const;

    /**
     * @brief User-edited column names (may differ from the original parsed names)
     *
     * Each entry corresponds to a column by index in @ref buildData().
     * @return List of column name strings, one per column
     */
    QStringList columnNames() const;

    /**
     * @brief Return the working data with renames and derived columns applied
     *
     * Includes any columns added via the "Compute derived column" section and
     * applies the user's name edits.  Use this in place of calling
     * renameColumns() on the original data.
     * @return Updated PlotData ready for plotting
     */
    PlotData buildData() const;

private slots:
    /** @brief Evaluate the derived-column expression and append the new column */
    void computeColumn();

private:
    /** @brief Append one row (x radio button + y checkbox with @p checked state + name editor) */
    void appendColumnRow(const QString &name, bool checked);

    PlotData workingData;       ///< Working copy of the data; derived cols appended here
    QGridLayout *colsLayout;    ///< Layout of the per-column rows (for dynamic addition)
    QButtonGroup *xgroup;       ///< Exclusive group of x selection radio buttons (id = column)
    QList<QCheckBox *> ychecks; ///< per-column y selection checkboxes
    QList<QLineEdit *> ynames;  ///< per-column name editors
    QLineEdit *deriveNameEdit;  ///< Name field for the new derived column
    QLineEdit *deriveExprEdit;  ///< Expression field for the new derived column
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
