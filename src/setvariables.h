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

#ifndef SET_VARIABLES_H
#define SET_VARIABLES_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QString>

/**
 * @brief Dialog for editing LAMMPS index-style variable definitions
 *
 * SetVariables provides a dialog for managing name-value pairs that
 * will be used as index-style variables in LAMMPS input scripts.
 * Users can add, delete, and edit variable definitions. The dialog
 * supports variable substitution using ${varname} syntax.
 */
class SetVariables : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param vars Reference to list of variable name-value pairs (modified in place)
     * @param parent Parent widget
     */
    explicit SetVariables(QList<QPair<QString, QString>> &vars, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~SetVariables() override = default;

    SetVariables()                                = delete;
    SetVariables(const SetVariables &)            = delete;
    SetVariables(SetVariables &&)                 = delete;
    SetVariables &operator=(const SetVariables &) = delete;
    SetVariables &operator=(SetVariables &&)      = delete;

private slots:
    /**
     * @brief Accept dialog and update variable list
     */
    void accept() override;

    /**
     * @brief Add a new empty variable row
     */
    void add_row();

    /**
     * @brief Delete the currently selected variable row
     */
    void del_row();

private:
    QList<QPair<QString, QString>> &vars; ///< Reference to variable list
    class QVBoxLayout *layout;            ///< Dialog layout
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
