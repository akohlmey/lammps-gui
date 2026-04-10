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

#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <string>
#include <vector>

class QWidget;
class QImage;
class QFont;

// OS specific default fonts
extern QFont *GUI_MONOFONT;
extern QFont *GUI_ALLFONT;

/**
 * @brief Duplicate a string from std::string
 * @param text The string to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const std::string &text);

/**
 * @brief Duplicate a string from C-string
 * @param text The C-string to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const char *text);

/**
 * @brief Duplicate a string from QString
 * @param text The QString to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const QString &text);

/**
 * @brief Compare two date strings in "YYYY-MM-DD" format
 * @param one First date string
 * @param two Second date string
 * @return -1 if one < two, 0 if equal, 1 if one > two
 */
extern int dateCompare(const QString &one, const QString &two);

/**
 * @brief Split a string into words while respecting quotes
 * @param text The string to split
 * @return Vector of words extracted from the string
 */
extern std::vector<std::string> splitLine(const std::string &text);

/**
 * @brief Get pointer to the main LAMMPS-GUI widget
 * @return Pointer to the main widget (used for dialogs)
 */
extern class QWidget *getMainWidget();

/**
 * @brief Provide standardized critical error dialog
 * @param parent  Pointer to parent widget
 * @param title   Error dialog title
 * @param text1   Error message summary
 * @param text2   Detailed error message (optional)
 */
extern void critical(QWidget *parent, const QString &title, const QString &text1,
                     const QString &text2 = QString());

/**
 * @brief Provide standardized custom warning dialog
 * @param parent  Pointer to parent widget
 * @param title   Warning dialog title
 * @param text1   Warning message summary
 * @param text2   Detailed warning message (optional)
 */
extern void warning(QWidget *parent, const QString &title, const QString &text1,
                    const QString &text2 = QString());

/**
 * @brief Save image directly or convert with ImageMagick
 * @param parent  Pointer to parent widget
 * @param image   Pointer to image class
 * @param title   Warning dialog title if failed
 */
extern void exportImage(QWidget *parent, QImage *image, const QString &title);

/**
 * @brief Check if an executable is in the system PATH
 * @param exe The executable name to search for
 * @return true if executable is found in PATH, false otherwise
 */
extern bool hasExe(const QString &exe);

/**
 * @brief Recursively delete all files in a directory
 * @param dir The directory to purge
 */
extern void purgeDirectory(const QString &dir);

/**
 * @brief Determine if the current Qt theme is light or dark
 * @return true if light theme, false if dark theme
 */
extern bool isLightTheme();

/**
 * @brief Silence stdout by redirecting it to the null device
 *
 * Redirects stdout to /dev/null (Unix) or NUL: (Windows) to suppress
 * all output.  Does nothing if StdCapture is currently active or if
 * stdout is already silenced.
 *
 * @note Not thread-safe.  Must only be called from the main thread.
 */
extern void silenceStdout();

/**
 * @brief Restore stdout after it was silenced
 *
 * Restores the original stdout file descriptor that was saved by
 * silenceStdout().  Does nothing if stdout is not currently silenced.
 *
 * @note Not thread-safe.  Must only be called from the main thread.
 */
extern void restoreStdout();

/**
 * @brief Check if stdout is currently silenced
 * @return true if silenceStdout() is active, false otherwise
 */
extern bool isStdoutSilenced();

/**
 * @brief Notify the silence/restore system about StdCapture state changes
 *
 * Called by StdCapture to indicate whether it is actively capturing output.
 * While capture is active, silenceStdout() becomes a no-op to avoid
 * interfering with the capture pipe.
 *
 * @param active true when StdCapture starts capturing, false when it stops
 */
extern void notifyCaptureState(bool active);

#endif
// Local Variables:
// c-basic-offset: 4
// End:
