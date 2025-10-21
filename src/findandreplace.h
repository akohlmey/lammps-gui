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

#ifndef FIND_AND_REPLACE_H
#define FIND_AND_REPLACE_H

#include "codeeditor.h"
#include <QDialog>

class QLineEdit;
class QCheckBox;

class FindAndReplace : public QDialog {
    Q_OBJECT

public:
    explicit FindAndReplace(CodeEditor *_editor, QWidget *parent = nullptr);
    ~FindAndReplace() override = default;

    FindAndReplace()                                  = delete;
    FindAndReplace(const FindAndReplace &)            = delete;
    FindAndReplace(FindAndReplace &&)                 = delete;
    FindAndReplace &operator=(const FindAndReplace &) = delete;
    FindAndReplace &operator=(FindAndReplace &&)      = delete;

private slots:
    void find_next();
    void replace_next();
    void replace_all();
    void quit();

private:
    CodeEditor *editor;
    QLineEdit *search, *replace;
    QCheckBox *withcase, *wrap, *whole;
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
