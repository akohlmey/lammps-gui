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

/**
 * @brief Horizontal line widget for visual separation in dialogs
 * 
 * QHline provides a simple horizontal line widget that can be
 * used to visually separate sections in forms and dialogs.
 * It's essentially a styled QFrame with a horizontal line shape.
 */
class QHline : public QFrame {
public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    QHline(QWidget *parent = nullptr);
};

/**
 * @brief Auto-completer for color name inputs
 * 
 * QColorCompleter provides auto-completion for color names
 * in text input fields. It suggests valid color names from
 * Qt's color name list as the user types.
 */
class QColorCompleter : public QCompleter {
public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    QColorCompleter(QWidget *parent = nullptr);
};

/**
 * @brief Validator for color name inputs
 * 
 * QColorValidator validates color input fields to ensure they
 * contain valid color names or hex color codes. It can also
 * fix up partially entered color names to valid values.
 */
class QColorValidator : public QValidator {
public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    QColorValidator(QWidget *parent = nullptr);

    /**
     * @brief Attempt to fix invalid color input
     * @param input String to fix (modified in place)
     */
    void fixup(QString &input) const override;
    
    /**
     * @brief Validate color input string
     * @param input String to validate
     * @param pos Cursor position (unused)
     * @return Validation state (Invalid, Intermediate, Acceptable)
     */
    QValidator::State validate(QString &input, int &pos) const override;
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
