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

#ifndef LINENUMBERAREA_H
#define LINENUMBERAREA_H

#include "codeeditor.h"
#include <QWidget>

/**
 * @brief Widget for displaying line numbers alongside the code editor
 *
 * This class provides a custom widget that displays line numbers in the
 * margin of the CodeEditor. It delegates painting to the CodeEditor class.
 */
class LineNumberArea : public QWidget {
public:
    /**
     * @brief Constructor
     * @param editor Pointer to the associated CodeEditor
     */
    explicit LineNumberArea(CodeEditor *editor) : QWidget(editor), codeEditor(editor) {}

    /**
     * @brief Destructor
     */
    ~LineNumberArea() override = default;

    LineNumberArea()                                  = delete;
    LineNumberArea(const LineNumberArea &)            = delete;
    LineNumberArea(LineNumberArea &&)                 = delete;
    LineNumberArea &operator=(const LineNumberArea &) = delete;
    LineNumberArea &operator=(LineNumberArea &&)      = delete;

    /**
     * @brief Get the ideal size for the line number area
     * @return QSize with width from editor, height 0 (fills available height)
     */
    QSize sizeHint() const override { return {codeEditor->lineNumberAreaWidth(), 0}; }

protected:
    /**
     * @brief Paint event handler - delegates to CodeEditor
     * @param event The paint event
     */
    void paintEvent(QPaintEvent *event) override { codeEditor->lineNumberAreaPaintEvent(event); }

private:
    CodeEditor *codeEditor; ///< Pointer to the associated code editor
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
