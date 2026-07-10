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
#include <QtGlobal>
#include <initializer_list>
#include <memory>

class QWidget;
class QImage;
class QPixmap;
class QFont;
class QAbstractButton;
class QDialogButtonBox;
class QScrollArea;

// OS specific default fonts (managed via unique_ptr for automatic cleanup)
extern std::unique_ptr<QFont> GUI_MONOFONT;
extern std::unique_ptr<QFont> GUI_ALLFONT;

/**
 * @brief Build the configured fixed-width font from the application settings
 * @return Fixed-pitch QFont with the family and point size stored in the
 *         settings, falling back to the platform default GUI_MONOFONT
 */
QFont monoFontFromSettings();

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
 * @brief Check whether a file is a movie (video) file
 * @param filename Path to the file
 * @return true if the extension is a known movie type, or the file is an
 *         animated GIF with more than one frame
 *
 * An animated GIF is both an image and a movie, and isImageFile() also claims
 * it.  Callers that route a file to either destination must therefore test
 * isMovieFile() first.
 */
[[nodiscard]] extern bool isMovieFile(const QString &filename);

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
 * @brief Apply the bundled SVG icons to a dialog button box's standard buttons
 * @param box The button box whose standard buttons should be re-iconed
 *
 * Qt fetches @c QDialogButtonBox standard-button icons (Ok, Cancel, ...) via
 * @c QIcon::fromTheme(), which falls back to the desktop theme because the
 * bundled @c lammpsgui icon theme only carries the editor context-menu actions.
 * This applies our own @c dialog-ok / @c dialog-cancel / @c dialog-no /
 * @c window-close SVGs to whichever standard buttons the box contains, so every
 * dialog button looks the same regardless of platform or desktop theme.
 */
extern void styleDialogButtons(QDialogButtonBox *box);

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
 * @brief RAII guard that collects Qt log messages instead of printing them
 *
 * Installs a Qt message handler on construction and restores the previous one
 * on destruction.  Debug, info, and warning messages emitted while the guard is
 * alive are collected and can be retrieved with messages(); they are never
 * printed.  Critical and fatal messages are passed on to the previous handler,
 * since those must not be swallowed.
 *
 * The intended use is a scope whose failure is expected and handled, such as
 * asking Qt to decode an image in a format it may not support: some of Qt's
 * image format plugins print a warning for every file they reject, which would
 * otherwise be repeated for each file and each attempt.  Retrieve the collected
 * text with messages() and report it once if the operation fails for good.
 *
 * @note The Qt message handler is process-wide, so the guard also captures
 *       messages emitted by other threads while it is alive.  Keep the guarded
 *       scope short and use it only from the main thread.
 */
class QtMessageSilencer {
public:
    QtMessageSilencer();
    ~QtMessageSilencer();
    QtMessageSilencer(const QtMessageSilencer &)            = delete;
    QtMessageSilencer(QtMessageSilencer &&)                 = delete;
    QtMessageSilencer &operator=(const QtMessageSilencer &) = delete;
    QtMessageSilencer &operator=(QtMessageSilencer &&)      = delete;

    /**
     * @brief The messages collected so far, one per line
     * @return Collected text, or an empty string if nothing was captured
     */
    [[nodiscard]] QString messages() const;

private:
    static void collect(QtMsgType type, const QMessageLogContext &context, const QString &message);

    static QtMessageSilencer *active; ///< Innermost live guard, nullptr if none
    QtMessageSilencer *outer;         ///< Guard this one replaced, for nesting
    QtMessageHandler previous;        ///< Message handler to restore, may be nullptr
    QStringList collected;            ///< Captured debug, info, and warning messages
};

/**
 * @brief Fade an image into an unmistakably inactive version of itself
 * @param src Image to convert
 * @return Grayscale, low-contrast copy of @p src with the original transparency
 *
 * Removes the color and, in addition, pulls the gray levels towards
 * Cfg::GRAYSCALE_MIDPOINT, keeping only Cfg::GRAYSCALE_CONTRAST of the original
 * contrast: a merely desaturated icon retains all of its structure and still
 * reads as active next to its colored counterpart.
 */
[[nodiscard]] extern QImage grayscaleImage(const QImage &src);

/**
 * @brief Fade a pixmap into an unmistakably inactive version of itself
 * @param src Pixmap to convert
 * @return Grayscale, low-contrast copy of @p src with the original transparency
 *
 * Applies grayscaleImage() to the pixmap.  Used for the "inactive" state of a
 * status icon, so the widget does not depend on the disabled-widget visual,
 * which does not refresh reliably on all platforms (e.g. macOS 12) and cannot
 * be applied to an enabled widget at all.
 */
[[nodiscard]] extern QPixmap grayscalePixmap(const QPixmap &src);

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
 * @brief Apply the shared window-manager hints to a top-level output window
 *
 * Strips the dialog property (so the window is an independent top-level window,
 * not a transient that stays above its parent) and removes the minimize button.
 * The maximize button is also removed, except on macOS where it is kept because
 * removing it makes the window non-resizable there. Used for the log, chart,
 * image, slide-show, file-viewer and variables windows so they share one frame
 * policy.
 *
 * @note setWindowFlags() re-shows a hidden widget, so call this before a final
 *       hide() when the window must start hidden.
 * @param window Top-level window to adjust (no-op if null)
 */
extern void applyWindowFlags(QWidget *window);

/**
 * @brief Compute the scroll area size that shows the given content, within a budget
 *
 * Pure size computation behind fitViewerWindow(). The natural size is the
 * content plus the scroll area frame, with no scroll bars. An axis whose
 * natural size exceeds the budget is clamped and gets a scroll bar, which
 * consumes @p sbext pixels of viewport on the *other* axis, so that axis is
 * enlarged accordingly (still within its budget). The result depends only on
 * the arguments -- never on the current scroll bar visibility -- so repeated
 * calls with the same input always yield the same size.
 *
 * @param content Size of the displayed content (image) in pixels
 * @param budget  Largest allowed outer scroll area size (e.g. a screen fraction)
 * @param frame   Total scroll area frame thickness (2 * frameWidth())
 * @param sbext   Scroll bar thickness (QStyle::PM_ScrollBarExtent)
 * @return Outer scroll area size that best fits the content within the budget
 */
[[nodiscard]] extern QSize viewerFitSize(const QSize &content, const QSize &budget, int frame,
                                         int sbext);

/**
 * @brief Resize a viewer window so its scroll area just fits the displayed image
 *
 * Shared auto-resize policy of the image viewer and the slide show: the scroll
 * area is sized via viewerFitSize() and the window is resized around it. The
 * scroll area's minimum size is pinned only for the duration of the resize, so
 * the user can freely shrink the window afterwards. When the computed size
 * equals @p lastFit, the window is left untouched; passing the previous return
 * value back in keeps navigating a sequence of equally-sized images from ever
 * moving or resizing the window.
 *
 * While @p window is still hidden its layout uses unpolished style metrics, so
 * the applied size is only approximate: the fit is applied but an invalid
 * QSize is returned instead of the memoized size. The viewers use that in
 * their showEvent() overrides to apply the fit once more on the shown window.
 *
 * @param window  Top-level viewer window to resize
 * @param area    Scroll area inside @p window that shows the content
 * @param content Size of the displayed content (image) in pixels
 * @param budget  Largest allowed outer scroll area size (e.g. a screen fraction)
 * @param lastFit Return value of the previous call (default QSize() initially)
 * @return The scroll area size applied, or an invalid QSize while @p window is
 *         hidden; pass this value back as @p lastFit on the next call
 */
extern QSize fitViewerWindow(QWidget *window, QScrollArea *area, const QSize &content,
                             const QSize &budget, const QSize &lastFit);

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
