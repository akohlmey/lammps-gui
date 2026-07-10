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

class QCheckBox;
class QLineEdit;
class QVBoxLayout;

/**
 * @brief Dialog to choose which columns of a PlotData to plot
 *
 * Presents the parsed columns of an external data file and lets the user
 * select which columns to plot on the y-axis via checkboxes.  The first
 * unselected (unchecked) column is automatically used as the shared x-axis.
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
     * Returns the index of the first unchecked (unselected) column.
     * Falls back to 0 if all columns are checked.
     * Indices refer to @ref buildData() columns.
     * @return Column index
     */
    int xColumn() const;

    /**
     * @brief Indices of the columns selected to plot on the y-axis
     *
     * Indices refer to @ref buildData() columns.
     * @return List of column indices (all checked columns)
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
    /** @brief Append one row (checkbox with @p name + optional @p checked state) */
    void appendColumnRow(const QString &name, bool checked);

    PlotData workingData;       ///< Working copy of the data; derived cols appended here
    QVBoxLayout *colsLayout;    ///< Layout of the per-column rows (for dynamic addition)
    QList<QCheckBox *> ychecks; ///< per-column y selection checkboxes
    QList<QLineEdit *> ynames;  ///< per-column name editors
    QLineEdit *deriveNameEdit;  ///< Name field for the new derived column
    QLineEdit *deriveExprEdit;  ///< Expression field for the new derived column
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
