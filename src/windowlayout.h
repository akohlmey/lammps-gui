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
 * @brief Presentation policy for the output views of the main window
 *
 * WindowLayout is the single place that decides *how* an output view is put
 * in front of the user.  LammpsGui creates and owns the view widgets and
 * keeps its typed pointers to them, but no longer calls show(), hide() or
 * isVisible() on them directly: it hands each widget to the layout with
 * place() and then addresses it by its ViewSlot.
 *
 * At the moment there is a single policy -- every view is an individual
 * top-level window, which is what the application has always done.  Funneling
 * the calls through here is what makes a second policy (all views docked into
 * the main window) possible without spreading the distinction over every call
 * site.
 *
 * The layout does not take ownership of the widgets.  It watches them for
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
     * @param parent Parent object; normally the main window, which then owns
     *               the layout and deletes it along with itself
     */
    explicit WindowLayout(QObject *parent = nullptr);

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

private:
    /// Drop the widget from whichever slot holds it (connected to its
    /// QObject::destroyed signal, so a slot never keeps a dangling pointer).
    void forget(QObject *view);

    QWidget *views[static_cast<int>(ViewSlot::Count)]{}; ///< Widget in each slot, nullptr if empty
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
