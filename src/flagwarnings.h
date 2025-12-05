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

/**
 * @brief Syntax highlighter for LAMMPS warning and error messages
 *
 * FlagWarnings extends QSyntaxHighlighter to detect and highlight
 * warning and error messages in LAMMPS log output. It also detects
 * and highlights URLs for documentation links. The class maintains
 * a count of warnings and updates a summary label.
 */
class FlagWarnings : public QSyntaxHighlighter {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param label Optional label to display warning count summary
     * @param parent Text document to apply highlighting to
     */
    explicit FlagWarnings(QLabel *label = nullptr, QTextDocument *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~FlagWarnings() override = default;

    FlagWarnings()                                = delete;
    FlagWarnings(const FlagWarnings &)            = delete;
    FlagWarnings(FlagWarnings &&)                 = delete;
    FlagWarnings &operator=(const FlagWarnings &) = delete;
    FlagWarnings &operator=(FlagWarnings &&)      = delete;

    /**
     * @brief Get the current number of warnings detected
     * @return Number of warnings found in the document
     */
    int get_nwarnings() const { return nwarnings; }

protected:
    /**
     * @brief Highlight a single block (line) of text
     * @param text Text to highlight
     *
     * Searches for warning/error patterns and URLs, applies formatting,
     * and updates warning count.
     */
    void highlightBlock(const QString &text) override;

private:
    QRegularExpression isWarning;  ///< Pattern for warning/error messages
    QRegularExpression isURL;      ///< Pattern for URLs
    QTextCharFormat formatWarning; ///< Format for warnings/errors
    QTextCharFormat formatURL;     ///< Format for URLs
    QLabel *summary;               ///< Label to display warning summary
    QTextDocument *document;       ///< Document being highlighted
    int nwarnings, oldwarnings;    ///< Current and previous warning count
    int nlines, oldlines;          ///< Current and previous line count
};
#endif
// Local Variables:
// c-basic-offset: 4
// End:
