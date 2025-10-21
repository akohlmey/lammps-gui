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

#ifndef QADDON_H
#define QADDON_H

#include <QCompleter>
#include <QFrame>
#include <QValidator>

// draw horizontal line
class QHline : public QFrame {
public:
    QHline(QWidget *parent = nullptr);
};

// complete color inputs
class QColorCompleter : public QCompleter {
public:
    QColorCompleter(QWidget *parent = nullptr);
};

// validate color inputs
class QColorValidator : public QValidator {
public:
    QColorValidator(QWidget *parent = nullptr);

    void fixup(QString &input) const override;
    QValidator::State validate(QString &input, int &pos) const override;
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
