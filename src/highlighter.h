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

#ifndef HIGHLIGHTER_H
#define HIGHLIGHTER_H

#include <QRegularExpression>
#include <QSyntaxHighlighter>
#include <QTextCharFormat>

class Highlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    explicit Highlighter(QTextDocument *parent = nullptr);
    ~Highlighter() override = default;

    Highlighter()                               = delete;
    Highlighter(const Highlighter &)            = delete;
    Highlighter(Highlighter &&)                 = delete;
    Highlighter &operator=(const Highlighter &) = delete;
    Highlighter &operator=(Highlighter &&)      = delete;

protected:
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression isLattice1, isLattice2, isLattice3;
    QRegularExpression isOutput1, isOutput2, isRead;
    QTextCharFormat formatOutput, formatRead, formatLattice, formatSetup;
    QRegularExpression isStyle, isForce, isDefine, isUndo;
    QRegularExpression isParticle, isRun, isSetup, isSetup1;
    QTextCharFormat formatParticle, formatRun, formatDefine;
    QRegularExpression isVariable, isReference;
    QTextCharFormat formatVariable;
    QRegularExpression isNumber1, isNumber2, isNumber3, isNumber4;
    QTextCharFormat formatNumber;
    QRegularExpression isSpecial, isContinue;
    QTextCharFormat formatSpecial;
    QRegularExpression isComment;
    QRegularExpression isQuotedComment;
    QTextCharFormat formatComment;
    QRegularExpression isTriple;
    QRegularExpression isString;
    QTextCharFormat formatString;

    int in_triple;
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
