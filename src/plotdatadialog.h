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

#include <QDialog>
#include <QList>

class QCheckBox;
class PlotData;

/**
 * @brief Dialog to choose which columns of a PlotData to plot
 *
 * Presents the parsed columns of an external data file and lets the user
 * select which columns to plot on the y-axis via checkboxes.  The first
 * unselected (unchecked) column is automatically used as the shared x-axis.
 * A small preview of the first rows is shown to help identify column content.
 */
class PlotDataDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param data   Parsed column data to choose from
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
     * @return Column index
     */
    int xColumn() const;

    /**
     * @brief Indices of the columns selected to plot on the y-axis
     * @return List of column indices (all checked columns)
     */
    QList<int> yColumns() const;

private:
    QList<QCheckBox *> ychecks; ///< per-column y selection checkboxes
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
