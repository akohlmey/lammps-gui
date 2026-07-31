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

#ifndef WINDOWLAYOUT_H
#define WINDOWLAYOUT_H

#include <QObject>

#include <initializer_list>

class QDockWidget;
class QEvent;
class QMainWindow;
class QWidget;

/**
 * @brief The output views LammpsGui presents alongside the editor
 *
 * One enumerator per view that LammpsGui keeps for the lifetime of a session
 * (as opposed to the transient file viewers and inspection windows, which are
 * created and closed on demand).  Used to address a view in WindowLayout
 * without the layout having to know the concrete widget classes.
 */
enum class ViewSlot {
    Log,       ///< Output window with the captured LAMMPS log
    Chart,     ///< Charts window with the thermo data of the current run
    Image,     ///< Snapshot image viewer
    SlideShow, ///< Slide show viewer for dump image sequences
    Variables, ///< Variables window listing the active index variables
    Count      ///< Number of slots; not a view itself
};

/**
 * @brief How the output views are presented
 */
enum class LayoutMode {
    Windows, ///< Each view is an individual, freely placed top-level window
    Docked   ///< The views are docked into the main window around the editor
};

/**
 * @brief Presentation policy for the output views of the main window
 *
 * WindowLayout is the single place that decides *how* an output view is put
 * in front of the user.  LammpsGui creates and owns the view widgets and
 * keeps its typed pointers to them, but does not call show(), hide() or
 * isVisible() on them directly: it hands each widget to the layout with
 * place() and then addresses it by its ViewSlot.
 *
 * Two policies are available, chosen once at construction from the user
 * preference:
 *
 * - LayoutMode::Windows keeps every view an individual top-level window,
 *   freely placed and stacked, which is what the application has always done.
 * - LayoutMode::Docked puts the views into dock areas around the editor, which
 *   stays the central widget: the charts, image and slide show views share a
 *   tabbed group on the right, the log and the variables view share a group
 *   across the full width at the bottom.
 *
 * In docked mode the layout owns one QDockWidget per slot, created up front so
 * that a saved arrangement can be restored before the views themselves exist.
 * place() only swaps the content of the dock, so a view that is destroyed and
 * rebuilt (as the image viewer is on every render) keeps its position and its
 * place in the tab order.
 *
 * The layout never owns the view widgets themselves.  It watches them for
 * destruction, so a slot whose widget is deleted elsewhere empties itself and
 * never hands out a dangling pointer.
 *
 * @see LammpsGui for the owner of both the layout and the view widgets
 */
class WindowLayout : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param mainwindow Main window the views belong to; also becomes the
     *                   parent object, so the layout is deleted along with it
     * @param mode       Presentation policy to apply
     *
     * In docked mode the dock widgets are created and a previously saved
     * arrangement is restored here, before any view exists.
     */
    WindowLayout(QMainWindow *mainwindow, LayoutMode mode);

    /**
     * @brief Destructor
     */
    ~WindowLayout() override;

    WindowLayout()                                = delete;
    WindowLayout(const WindowLayout &)            = delete;
    WindowLayout(WindowLayout &&)                 = delete;
    WindowLayout &operator=(const WindowLayout &) = delete;
    WindowLayout &operator=(WindowLayout &&)      = delete;

    /**
     * @brief The policy this layout applies
     * @return The mode passed to the constructor
     */
    LayoutMode mode() const { return layoutmode; }

    /**
     * @brief Put a view widget into a slot
     * @param slot Slot the widget belongs to
     * @param view Widget to present; may be nullptr to empty the slot
     *
     * Replaces whatever the slot held before without deleting it -- the
     * widgets stay owned by LammpsGui.  Safe to call again with the same
     * widget, which is what a reused Output or Charts window does.
     */
    void place(ViewSlot slot, QWidget *view);

    /**
     * @brief Widget currently in a slot
     * @param slot Slot to query
     * @return The widget, or nullptr if the slot is empty
     */
    QWidget *view(ViewSlot slot) const;

    /**
     * @brief Show the view in a slot
     * @param slot Slot to show
     *
     * Does nothing when the slot is empty.
     */
    void show(ViewSlot slot);

    /**
     * @brief Hide the view in a slot
     * @param slot Slot to hide
     *
     * Does nothing when the slot is empty.
     */
    void hide(ViewSlot slot);

    /**
     * @brief Show the view in a slot and bring it to the front
     * @param slot Slot to raise
     *
     * Use for an explicit request from the user.  Unlike show(), this pulls the
     * view to the front of its tab group, which is not wanted for the periodic
     * updates during a run.
     */
    void raise(ViewSlot slot);

    /**
     * @brief Show or hide the view in a slot
     * @param slot Slot to update
     * @param visible true to show the view, false to hide it
     */
    void setVisible(ViewSlot slot, bool visible);

    /**
     * @brief Flip the visibility of the view in a slot
     * @param slot Slot to toggle
     * @return Visibility of the view after the call (false for an empty slot)
     *
     * Persists the new state for the slots that have a "show by default"
     * preference (Output and Charts), so the next session starts the way the
     * session ended.
     */
    bool toggle(ViewSlot slot);

    /**
     * @brief Check whether the view in a slot is visible
     * @param slot Slot to query
     * @return true if the slot holds a widget and that widget is visible
     */
    bool isVisible(ViewSlot slot) const;

    /**
     * @brief Store the current dock arrangement in the settings
     *
     * Does nothing in windowed mode, where the views carry their own geometry.
     * Call before the main window is destroyed.
     */
    void saveState() const;

protected:
    /**
     * @brief Keep the dock proportions when the main window is resized
     * @param watched Object being watched (the main window)
     * @param event Event to inspect
     * @return true if the event was consumed
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// Disable the shortcuts of a widget that just became a dock panel and
    /// whose sequences the main window menus already bind.
    void deferShortcutsToMainWindow(QWidget *view);

    /// Give a panel that is alone in its area its title bar back, and take it
    /// away again once a tab names it (Qt draws no tab bar for a single dock).
    void updateDockChrome();

    /// Drop the widget from whichever slot holds it (connected to its
    /// QObject::destroyed signal, so a slot never keeps a dangling pointer).
    void forget(QObject *view);

    /// Ask for the stored proportions to be applied at the end of the current
    /// event handling; coalesced, so placing several views costs one pass.
    void scheduleSplit();

    /// Apply the stored proportions.  Deferred through scheduleSplit(), because
    /// resizeDocks() does nothing before the docks are laid out.
    void applySplit();

    /// A visible dock of a group, or nullptr if the whole group is hidden;
    /// resizeDocks() needs one to set the size the group shares.
    QDockWidget *sizingDock(std::initializer_list<ViewSlot> group) const;

    /// Build the dock widgets, arrange them, and restore a saved arrangement.
    void createDocks();

    /// The dock holding a slot, or nullptr in windowed mode.
    QDockWidget *dock(ViewSlot slot) const { return docks[static_cast<int>(slot)]; }

    /// The widget whose visibility represents a slot: the dock when docked,
    /// the view itself otherwise.
    QWidget *presenter(ViewSlot slot) const;

    QMainWindow *mainwindow;   ///< Main window the views are shown in or docked into
    LayoutMode layoutmode;     ///< Presentation policy chosen at construction
    bool splitpending = false; ///< An application of the proportions is scheduled
    bool applying     = false; ///< Resizing the docks ourselves, so do not track it
    double hsplit     = 0.0;   ///< Fraction of the width held by the right hand group
    double vsplit     = 0.0;   ///< Fraction of the height held by the bottom group

    QWidget *views[static_cast<int>(ViewSlot::Count)]{};     ///< Widget in each slot
    QDockWidget *docks[static_cast<int>(ViewSlot::Count)]{}; ///< Dock per slot (docked mode only)
    /// Zero-height placeholder that collapses a dock's title bar while a tab names it
    QWidget *emptytitles[static_cast<int>(ViewSlot::Count)]{};
    /// Stand-in tab shown as the title bar of a panel that is alone in its area
    QWidget *tabtitles[static_cast<int>(ViewSlot::Count)]{};
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
