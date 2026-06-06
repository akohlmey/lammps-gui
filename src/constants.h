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

/**
 * @brief Centralized QSettings key and group names
 *
 * One named constant per persisted QSettings key so a typo becomes a compile
 * error instead of a silently mismatched (and therefore lost) setting.  The
 * string value of each constant must match the original literal exactly.
 */
namespace SettingsKeys {

// ---- groups (QSettings::beginGroup) --------------------------------------
inline const QString GROUP_CHARTS   = QStringLiteral("charts");
inline const QString GROUP_REFORMAT = QStringLiteral("reformat");
inline const QString GROUP_SNAPSHOT = QStringLiteral("snapshot");
inline const QString GROUP_TUTORIAL = QStringLiteral("tutorial");

// ---- keys ----------------------------------------------------------------
inline const QString ACCELERATOR  = QStringLiteral("accelerator");
inline const QString ALLFAMILY    = QStringLiteral("allfamily");
inline const QString ALLSIZE      = QStringLiteral("allsize");
inline const QString ANTIALIAS    = QStringLiteral("antialias");
inline const QString AUTOBOND     = QStringLiteral("autobond");
inline const QString AUTOMATIC    = QStringLiteral("automatic");
inline const QString AUTOSAVE     = QStringLiteral("autosave");
inline const QString AXES         = QStringLiteral("axes");
inline const QString AXESDIAM     = QStringLiteral("axesdiam");
inline const QString AXESLEN      = QStringLiteral("axeslen");
inline const QString BACKCOLOR    = QStringLiteral("backcolor");
inline const QString BACKCOLOR2   = QStringLiteral("backcolor2");
inline const QString BACKGROUND   = QStringLiteral("background");
inline const QString BACKGROUND2  = QStringLiteral("background2");
inline const QString BONDCOLOR    = QStringLiteral("bondcolor");
inline const QString BONDCUT      = QStringLiteral("bondcut");
inline const QString BONDCUTOFF   = QStringLiteral("bondcutoff");
inline const QString BONDDIAM     = QStringLiteral("bonddiam");
inline const QString BOX          = QStringLiteral("box");
inline const QString BOXCOLOR     = QStringLiteral("boxcolor");
inline const QString BOXDIAM      = QStringLiteral("boxdiam");
inline const QString CHARTREPLACE = QStringLiteral("chartreplace");
inline const QString CHARTX       = QStringLiteral("chartx");
inline const QString CHARTY       = QStringLiteral("charty");
inline const QString CITE         = QStringLiteral("cite");
inline const QString COLOR        = QStringLiteral("color");
inline const QString COLORMAP     = QStringLiteral("colormap");
inline const QString COMMAND      = QStringLiteral("command");
inline const QString DIAMETER     = QStringLiteral("diameter");
inline const QString ECHO         = QStringLiteral("echo");
inline const QString GPUNEIGH     = QStringLiteral("gpuneigh");
inline const QString GPUPAIRONLY  = QStringLiteral("gpupaironly");
inline const QString GRID         = QStringLiteral("grid");
inline const QString HROT         = QStringLiteral("hrot");
inline const QString HTTPS_PROXY  = QStringLiteral("https_proxy");
inline const QString ID           = QStringLiteral("id");
inline const QString IMAGEREPLACE = QStringLiteral("imagereplace");
inline const QString INTELPREC    = QStringLiteral("intelprec");
inline const QString LOGREPLACE   = QStringLiteral("logreplace");
inline const QString LOGX         = QStringLiteral("logx");
inline const QString LOGY         = QStringLiteral("logy");
inline const QString MAINX        = QStringLiteral("mainx");
inline const QString MAINY        = QStringLiteral("mainy");
inline const QString MINORGRID    = QStringLiteral("minorgrid");
inline const QString MONOFAMILY   = QStringLiteral("monofamily");
inline const QString MONOSIZE     = QStringLiteral("monosize");
inline const QString NAME         = QStringLiteral("name");
inline const QString NTHREADS     = QStringLiteral("nthreads");
inline const QString PLUGIN_PATH  = QStringLiteral("plugin_path");
inline const QString RAWBRUSH     = QStringLiteral("rawbrush");
inline const QString RECENT       = QStringLiteral("recent");
inline const QString RETURN       = QStringLiteral("return");
inline const QString SHINYSTYLE   = QStringLiteral("shinystyle");
inline const QString SMOOTHBRUSH  = QStringLiteral("smoothbrush");
inline const QString SMOOTHCHOICE = QStringLiteral("smoothchoice");
inline const QString SMOOTHORDER  = QStringLiteral("smoothorder");
inline const QString SMOOTHWINDOW = QStringLiteral("smoothwindow");
inline const QString SOLUTION     = QStringLiteral("solution");
inline const QString SSAO         = QStringLiteral("ssao");
inline const QString TITLE        = QStringLiteral("title");
inline const QString TYPE         = QStringLiteral("type");
inline const QString UPDCHART     = QStringLiteral("updchart");
inline const QString UPDFREQ      = QStringLiteral("updfreq");
inline const QString VDWSTYLE     = QStringLiteral("vdwstyle");
inline const QString VIEWCHART    = QStringLiteral("viewchart");
inline const QString VIEWLOG      = QStringLiteral("viewlog");
inline const QString VIEWSLIDE    = QStringLiteral("viewslide");
inline const QString VROT         = QStringLiteral("vrot");
inline const QString WEBPAGE      = QStringLiteral("webpage");
inline const QString XSIZE        = QStringLiteral("xsize");
inline const QString YSIZE        = QStringLiteral("ysize");
inline const QString ZOOM         = QStringLiteral("zoom");

} // namespace SettingsKeys

#endif // CONSTANTS_H

// Local Variables:
// c-basic-offset: 4
// End:
