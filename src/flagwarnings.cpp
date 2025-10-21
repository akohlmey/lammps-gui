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

#include "flagwarnings.h"

#include "helpers.h"

#include <QColor>
#include <QFont>
#include <QLabel>
#include <QTextDocument>

FlagWarnings::FlagWarnings(QLabel *label, QTextDocument *parent) :
    QSyntaxHighlighter(parent), isWarning(QStringLiteral("^(ERROR|WARNING).*$")),
    isURL(QStringLiteral("^.*(https://docs.lammps.org/err[0-9]+).*$")), summary(label),
    document(parent)
{
    nwarnings = nlines = 0;
    oldwarnings = oldlines = -1;

    formatWarning.setForeground(QColorConstants::Red);
    formatWarning.setFontWeight(QFont::Bold);
    formatURL.setForeground(QColorConstants::Blue);
    formatURL.setFontWeight(QFont::Bold);
}

void FlagWarnings::highlightBlock(const QString &text)
{
    // nothing to do for empty lines
    if (text.isEmpty()) return;

    // highlight errors or warnings
    auto match = isWarning.match(text);
    if (match.hasMatch()) {
        ++nwarnings;
        setFormat(match.capturedStart(0), match.capturedLength(0), formatWarning);
    }

    // highlight ErrorURL links
    match = isURL.match(text);
    if (match.hasMatch()) {
        setFormat(match.capturedStart(1), match.capturedLength(1), formatURL);
    }

    // update error summary label when its content has changed
    if (document && summary) {
        nlines = document->lineCount();
        if ((nwarnings > oldwarnings) || (nlines > oldlines)) {
            oldwarnings = nwarnings;
            oldlines    = nlines;
            summary->setText(QString("%1 Warnings / Errors - %2 Lines").arg(nwarnings).arg(nlines));
            summary->repaint();
        }
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
