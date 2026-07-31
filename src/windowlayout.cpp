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

#include "windowlayout.h"

#include "constants.h"

#include <QDockWidget>
#include <QMainWindow>
#include <QSettings>
#include <QString>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

namespace {

// Settings key recording the visibility of a slot, empty for the slots that
// have no "show by default" preference.  Only the keys listed here are written
// back when a view is toggled from the View menu.
QString visibilityKey(ViewSlot slot)
{
    switch (slot) {
        case ViewSlot::Log:
            return Keys::VIEWLOG;
        case ViewSlot::Chart:
            return Keys::VIEWCHART;
        default:
            return {};
    }
}

// Short label for the dock tab.  The views set a window title that names the
// input file and the run number, which is useful for a task bar entry but far
// too long for a tab, so the docked layout uses these instead.
QString dockTitle(ViewSlot slot)
{
    switch (slot) {
        case ViewSlot::Log:
            return QStringLiteral("Output");
        case ViewSlot::Chart:
            return QStringLiteral("Charts");
        case ViewSlot::Image:
            return QStringLiteral("Image");
        case ViewSlot::SlideShow:
            return QStringLiteral("Slide Show");
        case ViewSlot::Variables:
            return QStringLiteral("Variables");
        default:
            return {};
    }
}

// Stable object name; QMainWindow::saveState()/restoreState() match the docks
// of a saved arrangement to the existing ones by this name.
QString dockObjectName(ViewSlot slot)
{
    return QStringLiteral("dock_") + dockTitle(slot).remove(' ').toLower();
}

} // namespace

WindowLayout::WindowLayout(QMainWindow *_mainwindow, LayoutMode mode) :
    QObject(_mainwindow), mainwindow(_mainwindow), layoutmode(mode)
{
    if (layoutmode == LayoutMode::Docked && mainwindow) createDocks();
}

WindowLayout::~WindowLayout() = default;

void WindowLayout::createDocks()
{
    // the bottom area spans the full window width, so the log sits underneath
    // the editor *and* the right hand group rather than beside them
    mainwindow->setCorner(Qt::BottomLeftCorner, Qt::BottomDockWidgetArea);
    mainwindow->setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    // the tab already names the view, so the docks carry no title bar of their
    // own; put the tabs on top, where a tab bar is normally looked for
    mainwindow->setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

    auto make = [this](ViewSlot slot, Qt::DockWidgetArea area) {
        auto *d = new QDockWidget(dockTitle(slot), mainwindow);
        d->setObjectName(dockObjectName(slot));
        d->setAllowedAreas(Qt::AllDockWidgetAreas);
        // an empty widget collapses the title bar; the name lives on the tab and
        // the dock is still dragged, floated and re-docked by its tab
        d->setTitleBarWidget(new QWidget(d));
        mainwindow->addDockWidget(area, d);
        // an empty dock would be a blank panel; the views show themselves as
        // they are created, through place() and the usual show()/hide() calls
        d->hide();
        docks[static_cast<int>(slot)] = d;
        return d;
    };

    // charts, image and slide show share the tab group on the right
    auto *chartDock = make(ViewSlot::Chart, Qt::RightDockWidgetArea);
    auto *imageDock = make(ViewSlot::Image, Qt::RightDockWidgetArea);
    auto *slideDock = make(ViewSlot::SlideShow, Qt::RightDockWidgetArea);
    mainwindow->tabifyDockWidget(chartDock, imageDock);
    mainwindow->tabifyDockWidget(imageDock, slideDock);

    // log and variables share the group across the bottom
    auto *logDock = make(ViewSlot::Log, Qt::BottomDockWidgetArea);
    auto *varDock = make(ViewSlot::Variables, Qt::BottomDockWidgetArea);
    mainwindow->tabifyDockWidget(logDock, varDock);

    // a saved arrangement wins over the default one above; it is matched to
    // these docks by object name, which is why they all exist by now
    const QByteArray state = QSettings().value(Keys::DOCKSTATE).toByteArray();
    if (!state.isEmpty() && mainwindow->restoreState(state, Cfg::DOCK_STATE_VERSION)) {
        // restoreState() also restores visibility, but a dock that has no view
        // in it yet must not show as an empty panel
        for (auto *d : docks)
            if (d && !d->widget()) d->hide();
    } else {
        // resizeDocks() only has an effect once the docks are laid out and
        // visible, which they are not during construction, so the default split
        // is applied from show() the first time a view appears
        splitpending = true;
    }
}

void WindowLayout::applyDefaultSplit()
{
    if (!splitpending || !mainwindow) return;
    splitpending = false;

    // half the width for the right hand group -- the chart controls need most
    // of that to lay out without being clipped -- and a third of the height for
    // the log across the bottom
    auto *chartDock = dock(ViewSlot::Chart);
    auto *logDock   = dock(ViewSlot::Log);
    if (chartDock && chartDock->isVisible())
        mainwindow->resizeDocks({chartDock}, {mainwindow->width() / 2}, Qt::Horizontal);
    if (logDock && logDock->isVisible())
        mainwindow->resizeDocks({logDock}, {mainwindow->height() / 3}, Qt::Vertical);
}

void WindowLayout::saveState() const
{
    if (layoutmode != LayoutMode::Docked || !mainwindow) return;
    QSettings().setValue(Keys::DOCKSTATE, mainwindow->saveState(Cfg::DOCK_STATE_VERSION));
}

void WindowLayout::place(ViewSlot slot, QWidget *view)
{
    const int idx = static_cast<int>(slot);
    if (views[idx] == view) return;

    if (views[idx]) disconnect(views[idx], &QObject::destroyed, this, nullptr);
    views[idx] = view;
    // the widgets are owned by LammpsGui, so the slot has to be emptied when
    // one of them is deleted rather than at some point of our choosing
    if (view) connect(view, &QObject::destroyed, this, &WindowLayout::forget);

    if (auto *d = dock(slot)) {
        // only the content changes; the dock keeps its area and tab position,
        // so a view that is rebuilt does not move
        d->setWidget(view);
        if (!view) d->hide();
    }
}

QWidget *WindowLayout::view(ViewSlot slot) const
{
    return views[static_cast<int>(slot)];
}

QWidget *WindowLayout::presenter(ViewSlot slot) const
{
    if (auto *d = dock(slot)) return views[static_cast<int>(slot)] ? d : nullptr;
    return views[static_cast<int>(slot)];
}

void WindowLayout::forget(QObject *view)
{
    for (int i = 0; i < static_cast<int>(ViewSlot::Count); ++i) {
        if (views[i] != view) continue;
        views[i] = nullptr;
        // the dock's content went away with it; keep the dock but empty
        if (docks[i]) docks[i]->hide();
    }
}

void WindowLayout::show(ViewSlot slot)
{
    auto *w = presenter(slot);
    if (!w) return;
    w->show();

    // deliberately no raise() here: this runs on every periodic update during a
    // run (each new dump image shows the slide show view), and raising would
    // pull the tab group away from whatever the user is looking at
    if (splitpending)
        QTimer::singleShot(0, mainwindow, [this]() {
            applyDefaultSplit();
        });
}

void WindowLayout::raise(ViewSlot slot)
{
    auto *w = presenter(slot);
    if (!w) return;
    show(slot);
    // in a tab group showing a dock leaves it behind its siblings
    if (auto *d = dock(slot))
        d->raise();
    else
        w->raise();
}

void WindowLayout::hide(ViewSlot slot)
{
    if (auto *w = presenter(slot)) w->hide();
}

void WindowLayout::setVisible(ViewSlot slot, bool visible)
{
    if (visible)
        show(slot);
    else
        hide(slot);
}

bool WindowLayout::isVisible(ViewSlot slot) const
{
    const auto *w = presenter(slot);
    return w && w->isVisible();
}

bool WindowLayout::toggle(ViewSlot slot)
{
    if (!presenter(slot)) return false;

    const bool visible = !isVisible(slot);
    // an explicit request from the View menu: bring it to the front of its tab
    // group, otherwise turning it "on" would appear to do nothing
    if (visible)
        raise(slot);
    else
        hide(slot);

    const QString key = visibilityKey(slot);
    if (!key.isEmpty()) QSettings().setValue(key, visible);
    return visible;
}

// Local Variables:
// c-basic-offset: 4
// End:
