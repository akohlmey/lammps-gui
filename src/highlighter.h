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

/**
 * @brief Syntax highlighter for LAMMPS input scripts
 * 
 * This class provides syntax highlighting for LAMMPS input files in the
 * CodeEditor. It categorizes and colors different types of LAMMPS commands,
 * keywords, variables, numbers, strings, and comments.
 */
class Highlighter : public QSyntaxHighlighter {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent text document to highlight
     */
    explicit Highlighter(QTextDocument *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~Highlighter() override = default;

    Highlighter()                               = delete;
    Highlighter(const Highlighter &)            = delete;
    Highlighter(Highlighter &&)                 = delete;
    Highlighter &operator=(const Highlighter &) = delete;
    Highlighter &operator=(Highlighter &&)      = delete;

protected:
    /**
     * @brief Highlight a single block (line) of text
     * @param text The text to highlight
     */
    void highlightBlock(const QString &text) override;

private:
    // Regular expressions for different LAMMPS command categories
    QRegularExpression isLattice1, isLattice2, isLattice3;  ///< Lattice setup commands
    QRegularExpression isOutput1, isOutput2, isRead;        ///< Output and input commands
    QTextCharFormat formatOutput, formatRead, formatLattice, formatSetup;  ///< Formats for setup commands
    QRegularExpression isStyle, isForce, isDefine, isUndo;  ///< Style and force commands
    QRegularExpression isParticle, isRun, isSetup, isSetup1;  ///< Particle and run commands
    QTextCharFormat formatParticle, formatRun, formatDefine;   ///< Formats for various command types
    QRegularExpression isVariable, isReference;             ///< Variable definitions and references
    QTextCharFormat formatVariable;                         ///< Format for variables
    QRegularExpression isNumber1, isNumber2, isNumber3, isNumber4;  ///< Various number formats
    QTextCharFormat formatNumber;                           ///< Format for numbers
    QRegularExpression isSpecial, isContinue;               ///< Special keywords and line continuations
    QTextCharFormat formatSpecial;                          ///< Format for special keywords
    QRegularExpression isComment;                           ///< Comment patterns
    QRegularExpression isQuotedComment;                     ///< Quoted comment patterns
    QTextCharFormat formatComment;                          ///< Format for comments
    QRegularExpression isTriple;                            ///< Triple-quoted strings
    QRegularExpression isString;                            ///< Regular strings
    QTextCharFormat formatString;                           ///< Format for strings

    int in_triple;  ///< State flag for multi-line triple-quoted strings
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
