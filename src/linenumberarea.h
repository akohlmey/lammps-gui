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

#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include "codeeditor.h"
#include <QWidget>

class LineNumberArea : public QWidget {
public:
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}
    ~LineNumberArea() override = default;

    LineNumberArea()                                  = delete;
    LineNumberArea(const LineNumberArea &)            = delete;
    LineNumberArea(LineNumberArea &&)                 = delete;
    LineNumberArea &operator=(const LineNumberArea &) = delete;
    LineNumberArea &operator=(LineNumberArea &&)      = delete;

    QSize sizeHint() const override { return {codeEditor->lineNumberAreaWidth(), 0}; }

protected:
    void paintEvent(QPaintEvent *event) override { codeEditor->lineNumberAreaPaintEvent(event); }

private:
    CodeEditor *codeEditor;
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
