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

#include <QSettings>
#include <QString>
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

} // namespace

WindowLayout::WindowLayout(QObject *parent) : QObject(parent) {}

WindowLayout::~WindowLayout() = default;

void WindowLayout::place(ViewSlot slot, QWidget *view)
{
    const int idx = static_cast<int>(slot);
    if (views[idx] == view) return;

    if (views[idx]) disconnect(views[idx], &QObject::destroyed, this, nullptr);
    views[idx] = view;
    // the widgets are owned by LammpsGui, so the slot has to be emptied when
    // one of them is deleted rather than at some point of our choosing
    if (view) connect(view, &QObject::destroyed, this, &WindowLayout::forget);
}

QWidget *WindowLayout::view(ViewSlot slot) const
{
    return views[static_cast<int>(slot)];
}

void WindowLayout::forget(QObject *view)
{
    for (auto &slot : views)
        if (slot == view) slot = nullptr;
}

void WindowLayout::show(ViewSlot slot)
{
    if (auto *w = view(slot)) w->show();
}

void WindowLayout::hide(ViewSlot slot)
{
    if (auto *w = view(slot)) w->hide();
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
    const auto *w = view(slot);
    return w && w->isVisible();
}

bool WindowLayout::toggle(ViewSlot slot)
{
    if (!view(slot)) return false;

    const bool visible = !isVisible(slot);
    setVisible(slot, visible);

    const QString key = visibilityKey(slot);
    if (!key.isEmpty()) QSettings().setValue(key, visible);
    return visible;
}

// Local Variables:
// c-basic-offset: 4
// End:
