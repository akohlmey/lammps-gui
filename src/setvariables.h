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

class SetVariables : public QDialog {
    Q_OBJECT

public:
    explicit SetVariables(QList<QPair<QString, QString>> &vars, QWidget *parent = nullptr);
    ~SetVariables() override = default;

    SetVariables()                                = delete;
    SetVariables(const SetVariables &)            = delete;
    SetVariables(SetVariables &&)                 = delete;
    SetVariables &operator=(const SetVariables &) = delete;
    SetVariables &operator=(SetVariables &&)      = delete;

private slots:
    void accept() override;
    void add_row();
    void del_row();

private:
    QList<QPair<QString, QString>> &vars;
    class QVBoxLayout *layout;
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
