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

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QSize>
#include <QString>
#include <QStringList>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

class QWidget;
class QImage;
class QFont;
class QAbstractButton;

// OS specific default fonts (managed via unique_ptr for automatic cleanup)
extern std::unique_ptr<QFont> GUI_MONOFONT;
extern std::unique_ptr<QFont> GUI_ALLFONT;

/**
 * @brief Compare two date strings in LAMMPS "DD MMM YYYY" format (e.g. "22 Jul 2025")
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
 * @brief Split a string into words while respecting quotes
 * @overload
 * @param text The string to split
 * @return List of words extracted from the string
 */
extern QStringList splitLine(const QString &text);

/**
 * @brief Provide standardized information dialog
 * @param parent  Pointer to parent widget
 * @param title   Information dialog title
 * @param text1   Information message part 1
 * @param text2   Information message part 2 (optional)
 */
extern void information(QWidget *parent, const QString &title, const QString &text1,
                        const QString &text2 = QString());

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
 * @brief Provide platform specific name of a LAMMPS shared library
 * @return String with the filename or an empty string if compiled without plugin support
 */
extern QString getLammpsLibName();

/**
 * @brief Provide platform specific URL for downloading a LAMMPS shared library
 * @return String with the URL or an empty string if compiled without plugin support
 */
extern QString getLammpsDownloadUrl();

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
[[nodiscard]] extern bool hasExe(const QString &exe);

/**
 * @brief Check whether a file is (likely) an image
 * @param filename Path to the file
 * @return true if the extension is a known image type, or the file exists and
 *         QImageReader recognizes its contents as an image
 *
 * Recognizes the formats Qt can decode plus common ImageMagick-only formats
 * (e.g. tga, eps, sgi) so callers can route them through a conversion step.
 */
[[nodiscard]] extern bool isImageFile(const QString &filename);

/**
 * @brief Check whether a file is a LAMMPS binary restart file
 * @param filename Path to the file
 * @return true if the file exists and begins with the LAMMPS restart magic string
 */
[[nodiscard]] extern bool isRestartFile(const QString &filename);

/**
 * @brief Heuristic check whether a file is binary rather than text
 * @param filename Path to the file
 * @return true if the first 8 KB of the file contains a null byte
 *
 * Null bytes are essentially absent from text (ASCII, UTF-8, Latin-1) but
 * ubiquitous in binary formats (IEEE floats, packed integers, padding).
 * This is the same heuristic used by git, grep, and the POSIX file utility.
 * Returns false for files that cannot be opened (callers handle that separately).
 */
[[nodiscard]] extern bool looksLikeBinaryFile(const QString &filename);

/**
 * @brief Re-exec the current LAMMPS-GUI process in place (e.g. to reload the plugin)
 *
 * Replaces the running process with a fresh launch of the same executable.
 * On success it does not return; it returns only if the re-exec failed, so
 * callers must handle that case (typically warn and/or exit).
 */
extern void relaunchApplication();

/**
 * @brief Recursively delete all files in a directory
 * @param dir The directory to purge
 */
extern void purgeDirectory(const QString &dir);

/**
 * @brief Determine if the current Qt theme is light or dark
 * @return true if light theme, false if dark theme
 */
[[nodiscard]] extern bool isLightTheme();

/**
 * @brief Show a standardized "unsaved changes" confirmation dialog
 * @param parent    Pointer to the parent widget
 * @param filename  Name of the file with unsaved changes
 * @param question  Informative text explaining the context (e.g. "save before opening?")
 * @return QMessageBox::Yes, QMessageBox::No, or QMessageBox::Cancel
 *
 * Provides a consistent confirmation dialog used whenever the user may lose
 * unsaved changes (opening a new file, quitting, running LAMMPS, etc.).
 */
extern int showUnsavedChangesDialog(QWidget *parent, const QString &filename,
                                    const QString &question);

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

/**
 * @brief RAII guard that silences stdout for the duration of its scope
 *
 * Calls silenceStdout() on construction and restoreStdout() on destruction,
 * so stdout is restored on every exit path from the enclosing scope, including
 * early returns and exceptions. Use in place of manual silenceStdout() /
 * restoreStdout() pairs.
 *
 * @note Not thread-safe. Must only be used from the main thread.
 */
class StdoutSilencer {
public:
    StdoutSilencer() { silenceStdout(); }
    ~StdoutSilencer() { restoreStdout(); }
    StdoutSilencer(const StdoutSilencer &)            = delete;
    StdoutSilencer(StdoutSilencer &&)                 = delete;
    StdoutSilencer &operator=(const StdoutSilencer &) = delete;
    StdoutSilencer &operator=(StdoutSilencer &&)      = delete;
};

/**
 * @brief Square size for a toolbar/status-bar button from a sample's size hint
 *
 * Implements the shared tool-button sizing policy: take the sample button's
 * minimum size hint height, enlarge it by Cfg::TOOLBAR_BUTTON_MARGIN, and
 * return a square of that side. Compute this once per button row and reuse it
 * for the row's buttons (via styleToolButtons()) and any adjacent widgets
 * (line edits, labels, spin boxes) that should share the button height.
 *
 * @param sample Representative button (typically the first in the row)
 * @return Square button size in logical pixels
 */
extern QSize toolButtonSize(const QAbstractButton *sample);

/**
 * @brief Apply the shared tool-button policy to a set of buttons
 *
 * Fixes every button to @p size (a square from toolButtonSize()) and gives it
 * the standard Cfg::TOOLBAR_ICON_SIZE icon, so only a small, uniform padding
 * remains between icon and frame. The image viewer, slide show, chart window,
 * and editor status bar all use this so their button rows look identical.
 *
 * @param size    Square button size (from toolButtonSize())
 * @param buttons Buttons to size and assign the standard icon size
 */
extern void styleToolButtons(const QSize &size, std::initializer_list<QAbstractButton *> buttons);

/**
 * @brief Append an action with an optional icon and a triggered() handler to a menu
 * @param menu     Menu to append the new action to
 * @param text     Action label text
 * @param icon     Resource path for the action icon (empty for no icon)
 * @param receiver Object that owns the slot/callable
 * @param slot     Member function pointer or callable invoked on trigger
 * @return The created action, for any further configuration by the caller (e.g. setData())
 *
 * Collapses the recurring "addAction() + setIcon() + connect()" idiom used when
 * building context menus and tool menus across the widget classes.
 */
template <typename Recv, typename Func>
QAction *addMenuAction(QMenu *menu, const QString &text, const QString &icon, Recv *receiver,
                       Func slot)
{
    auto *action = menu->addAction(text);
    if (!icon.isEmpty()) action->setIcon(QIcon(icon));
    QObject::connect(action, &QAction::triggered, receiver, slot);
    return action;
}

#endif
// Local Variables:
// c-basic-offset: 4
// End:
