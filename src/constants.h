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
constexpr int MIN_LAMMPS_VERSION =
    20260330; ///< Minimum LAMMPS version (30 March 2026) as YYYYMMDD format number
inline const QString MIN_LAMMPS_VERSION_STR =
    QStringLiteral("30 Mar 2026"); ///< Minimum LAMMPS version (30 March 2026) as string

// ---- Buffer thresholds ---------------------------------------------------
constexpr double BUFFER_WARNING_THRESHOLD = 0.333; ///< Warn when capture buffer exceeds this
constexpr int THERMO_SUGGEST_MULTIPLIER   = 5;     ///< Multiplier for thermo interval suggestion

// ---- Preferences dialog --------------------------------------------------
constexpr int PREFERENCES_WIDTH  = 700; ///< Preferences dialog default width in pixels
constexpr int PREFERENCES_HEIGHT = 500; ///< Preferences dialog default height in pixels

// ---- Update intervals (milliseconds) -------------------------------------
constexpr int DATA_UPDATE_INTERVAL_MIN      = 1;    ///< Min log/data update interval
constexpr int DATA_UPDATE_INTERVAL_MAX      = 1000; ///< Max log/data update interval
constexpr int DATA_UPDATE_INTERVAL_DEFAULT  = 10;   ///< Default log/data update interval
constexpr int CHART_UPDATE_INTERVAL_MIN     = 1;    ///< Min chart update interval
constexpr int CHART_UPDATE_INTERVAL_MAX     = 5000; ///< Max chart update interval
constexpr int CHART_UPDATE_INTERVAL_DEFAULT = 500;  ///< Default chart update interval

// ---- Chart dimension ranges (pixels) -------------------------------------
constexpr int CHART_WIDTH_MIN  = 400;   ///< Min configurable chart width
constexpr int CHART_WIDTH_MAX  = 40000; ///< Max configurable chart width
constexpr int CHART_HEIGHT_MIN = 300;   ///< Min configurable chart height
constexpr int CHART_HEIGHT_MAX = 30000; ///< Max configurable chart height

// ---- Chart smoothing (Savitzky-Golay) ------------------------------------
constexpr int SMOOTH_WINDOW_MIN     = 5;   ///< Min smoothing window size
constexpr int SMOOTH_WINDOW_MAX     = 999; ///< Max smoothing window size
constexpr int SMOOTH_WINDOW_DEFAULT = 10;  ///< Default smoothing window size
constexpr int SMOOTH_ORDER_MIN      = 1;   ///< Min smoothing polynomial order
constexpr int SMOOTH_ORDER_MAX      = 20;  ///< Max smoothing polynomial order
constexpr int SMOOTH_ORDER_DEFAULT  = 4;   ///< Default smoothing polynomial order

// ---- Auto-completion -----------------------------------------------------
constexpr int COMPLETION_CHARS_MIN = 1;  ///< Min characters before auto-completion triggers
constexpr int COMPLETION_CHARS_MAX = 32; ///< Max characters before auto-completion triggers

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
