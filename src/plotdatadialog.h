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
class QComboBox;
class PlotData;

/**
 * @brief Dialog to choose which columns of a PlotData to plot
 *
 * Presents the parsed columns of an external data file and lets the user pick
 * one column for the shared x axis and any number of columns to plot against
 * it, with a small preview of the first rows.
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
     * @brief Index of the column chosen for the x axis
     * @return Column index
     */
    int xColumn() const;

    /**
     * @brief Indices of the columns selected to plot (excluding the x column)
     * @return List of column indices
     */
    QList<int> yColumns() const;

private:
    QComboBox *xcombo;          ///< x-axis column selector
    QList<QCheckBox *> ychecks; ///< per-column y selection checkboxes
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
