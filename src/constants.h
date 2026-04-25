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

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <QString>

/**
 * @brief Application-wide constants for LAMMPS-GUI
 *
 * Centralizes magic numbers and repeated string literals that were previously
 * scattered across the codebase.  Grouping by category makes maintenance easier
 * and reduces the risk of typos from duplicated literals.
 */
namespace GuiConstants {

// ---- UI dimensions -------------------------------------------------------
constexpr int DEFAULT_BUFLEN      = 1024; ///< Default buffer length for error messages
constexpr int MAX_DEFAULT_THREADS = 16;   ///< Maximum default thread count
constexpr int MINIMUM_WIDTH       = 400;  ///< Minimum window width in pixels
constexpr int MINIMUM_HEIGHT      = 300;  ///< Minimum window height in pixels
constexpr int ICON_SCALE          = 22;   ///< Status bar icon dimension in pixels
constexpr int PROGRESS_MAXIMUM    = 1000; ///< Maximum value for QProgressBar

// ---- File limits ---------------------------------------------------------
constexpr int NUM_RECENT_FILES = 5; ///< Number of entries in the recent files list
constexpr int NUM_TUTORIALS    = 8; ///< Number of tutorials available

// ---- LAMMPS version requirement ------------------------------------------
constexpr int MIN_LAMMPS_VERSION = 20260330; ///< Minimum LAMMPS version (30 March 2026)
inline const QString MIN_LAMMPS_VERSION_STR = QStringLiteral("30 Mar 2026");

// ---- Buffer thresholds ---------------------------------------------------
constexpr double BUFFER_WARNING_THRESHOLD = 0.333; ///< Warn when capture buffer exceeds this
constexpr int THERMO_SUGGEST_MULTIPLIER   = 5;     ///< Multiplier for thermo interval suggestion

// ---- Resource paths ------------------------------------------------------
/** path to LAMMPS-GUI Window Icon resource */
inline const QString MAIN_ICON = QStringLiteral(":/icons/lammps-gui-icon-128x128.png");
/** path to LAMMPS Icon resource */
inline const QString LAMMPS_ICON = QStringLiteral(":/icons/lammps-icon-128x128.png");

// ---- Status messages -----------------------------------------------------
/** status string when LAMMPS-GUI is ready */
inline const QString STATUS_READY = QStringLiteral("Ready.");

// ---- Window title prefix -------------------------------------------------
/** window title prefix string for LAMMPS-GUI windows */
inline const QString TITLE_PREFIX = QStringLiteral("LAMMPS-GUI - ");

} // namespace GuiConstants

#endif // CONSTANTS_H

// Local Variables:
// c-basic-offset: 4
// End:
