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

#ifndef FLAGWARNINGS_H
#define FLAGWARNINGS_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class QLabel;
class QTextDocument;

class FlagWarnings : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit FlagWarnings(QLabel *label = nullptr, QTextDocument *parent = nullptr);
    ~FlagWarnings() override = default;

    FlagWarnings()                                = delete;
    FlagWarnings(const FlagWarnings &)            = delete;
    FlagWarnings(FlagWarnings &&)                 = delete;
    FlagWarnings &operator=(const FlagWarnings &) = delete;
    FlagWarnings &operator=(FlagWarnings &&)      = delete;

    int get_nwarnings() const { return nwarnings; }

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression isWarning;
    QRegularExpression isURL;
    QTextCharFormat formatWarning;
    QTextCharFormat formatURL;
    QLabel *summary;
    QTextDocument *document;
    int nwarnings, oldwarnings, nlines, oldlines;
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
