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
#include "helpers.h"

#include <QAction>
#include <QDockWidget>
#include <QEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayout>
#include <QMainWindow>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QString>
#include <QTabWidget>
#include <QTimer>
#include <QWidget>

namespace {

constexpr int TAB_TITLE_MARGIN = 2;

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

// Qt draws a tab bar only once two docks share an area.  A panel that is alone
// therefore gets this instead of the plain title bar: a label drawn like the
// single tab it stands in for, with the frame open at the bottom towards the
// panel it labels.
QWidget *makeTabTitle(QWidget *parent, const QString &title)
{
    auto *bar    = new QWidget(parent);
    auto *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(TAB_TITLE_MARGIN, TAB_TITLE_MARGIN, TAB_TITLE_MARGIN, 0);
    layout->setSpacing(0);

    auto *label = new QLabel(title, bar);
    label->setObjectName("dockTabTitle");
    label->setStyleSheet("QLabel#dockTabTitle {"
                         "  border: 1px solid palette(dark);"
                         "  border-bottom: none;"
                         "  border-top-left-radius: 4px;"
                         "  border-top-right-radius: 4px;"
                         "  padding: 3px 12px;"
                         "  background: palette(button);"
                         "}");
    layout->addWidget(label);
    layout->addStretch();
    return bar;
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
        // the panels have fixed places, so they are neither dragged nor floated;
        // the View menu shows and hides them
        d->setFeatures(QDockWidget::NoDockWidgetFeatures);
        // updateDockChrome() decides per dock whether its name is carried by a
        // tab or by a title bar, and needs this widget to collapse the latter
        const int idx    = static_cast<int>(slot);
        emptytitles[idx] = new QWidget(d);
        tabtitles[idx]   = makeTabTitle(d, dockTitle(slot));
        d->setTitleBarWidget(emptytitles[idx]);
        connect(d, &QDockWidget::visibilityChanged, this, [this]() {
            updateDockChrome();
        });
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

    mainwindow->installEventFilter(this);
    // watch the docks, so the cached fractions follow a splitter the user drags
    // and not just a resize of the window
    for (auto *d : docks)
        if (d) d->installEventFilter(this);

    // a saved arrangement wins over the default one above; it is matched to
    // these docks by object name, which is why they all exist by now
    QSettings settings;
    const QByteArray state = settings.value(Keys::DOCKSTATE).toByteArray();
    if (!state.isEmpty() && mainwindow->restoreState(state, Cfg::DOCK_STATE_VERSION)) {
        // restoreState() also restores visibility, but a dock that has no view
        // in it yet must not show as an empty panel
        for (auto *d : docks)
            if (d && !d->widget()) d->hide();
    }

    // The proportions are kept separately rather than left to restoreState():
    // that runs while the docks are still empty, so the sizes it restores are
    // replaced by the size hint of each view as soon as one is put in.  They are
    // re-applied from show() once a view is actually there -- resizeDocks() has
    // no effect before the docks are laid out and visible anyway.
    hsplit = settings.value(Keys::DOCKSPLITH, Cfg::DOCK_SPLIT_HORIZONTAL).toDouble();
    vsplit = settings.value(Keys::DOCKSPLITV, Cfg::DOCK_SPLIT_VERTICAL).toDouble();
    scheduleSplit();
}

// The docks of one group share a size, so any visible one of them can be
// resized to set it -- but only a visible one: resizeDocks() ignores a hidden
// dock, and which member of a group is up varies with what the run produced.
QDockWidget *WindowLayout::sizingDock(std::initializer_list<ViewSlot> group) const
{
    // note: "slots" is a Qt keyword macro and cannot be used as a name here
    for (auto slot : group) {
        auto *d = dock(slot);
        if (d && d->isVisible()) return d;
    }
    return nullptr;
}

// Coalesce into a single application at the end of the current event handling:
// several views can be placed in one go, and resizeDocks() has no effect until
// the docks holding them are laid out.
void WindowLayout::scheduleSplit()
{
    if (layoutmode != LayoutMode::Docked || !mainwindow || splitpending) return;
    splitpending = true;
    QTimer::singleShot(0, mainwindow, [this]() {
        applySplit();
    });
}

void WindowLayout::applySplit()
{
    splitpending = false;
    if (!mainwindow) return;

    // the resizes below are not the user changing the split, so keep the event
    // filter from recording them as a new target
    applying         = true;
    auto *rightDock  = sizingDock({ViewSlot::Chart, ViewSlot::Image, ViewSlot::SlideShow});
    auto *bottomDock = sizingDock({ViewSlot::Log, ViewSlot::Variables});
    if (rightDock)
        mainwindow->resizeDocks({rightDock}, {int(mainwindow->width() * hsplit)}, Qt::Horizontal);
    if (bottomDock)
        mainwindow->resizeDocks({bottomDock}, {int(mainwindow->height() * vsplit)}, Qt::Vertical);
    applying = false;
}

// Qt only draws a tab bar once two dock widgets share an area, so a panel that
// is alone would end up with no visible name at all.  Give such a panel its
// title bar back and take it away again as soon as a tab names it.
void WindowLayout::updateDockChrome()
{
    if (layoutmode != LayoutMode::Docked || !mainwindow) return;

    for (int i = 0; i < static_cast<int>(ViewSlot::Count); ++i) {
        auto *d = docks[i];
        if (!d) continue;

        bool tabbed = false;
        for (const auto *sibling : mainwindow->tabifiedDockWidgets(d)) {
            if (sibling && sibling->isVisible()) {
                tabbed = true;
                break;
            }
        }
        // the placeholder stays owned by the dock either way, so it can be
        // handed back and forth without leaking
        QWidget *wanted = tabbed ? emptytitles[i] : tabtitles[i];
        if (d->titleBarWidget() != wanted) {
            // the one being replaced stays owned by the dock, so it can be
            // handed back and forth without leaking
            if (auto *previous = d->titleBarWidget()) previous->hide();
            d->setTitleBarWidget(wanted);
            if (wanted) wanted->show();
        }
    }
}

// keep the dock proportions across a resize of the main window: the sizes
// before the resize are relative to the old size, so re-applying the same
// fractions preserves whatever split the user last set.
bool WindowLayout::eventFilter(QObject *watched, QEvent *event)
{
    // Only once the window is on screen and the default proportions have been
    // applied: during start-up the docks are not laid out yet, so their size is
    // still zero and the fraction computed from it would be zero too -- which
    // this would then enforce, collapsing the panel for good.
    if (event->type() == QEvent::Resize && layoutmode == LayoutMode::Docked && !splitpending &&
        !applying && mainwindow && mainwindow->isVisible()) {
        // a dock changed size under the user's hands: record what fraction of
        // the window its group now holds
        for (int i = 0; i < static_cast<int>(ViewSlot::Count); ++i) {
            const auto *d = docks[i];
            if (d != watched || !d || d->isHidden()) continue;
            const auto slot = static_cast<ViewSlot>(i);
            if (slot == ViewSlot::Log || slot == ViewSlot::Variables) {
                if (d->height() > 0 && mainwindow->height() > 0)
                    vsplit = double(d->height()) / mainwindow->height();
            } else {
                if (d->width() > 0 && mainwindow->width() > 0)
                    hsplit = double(d->width()) / mainwindow->width();
            }
            break;
        }
    }

    if (watched == mainwindow && event->type() == QEvent::Resize &&
        layoutmode == LayoutMode::Docked && !splitpending && mainwindow->isVisible()) {
        auto *re            = static_cast<QResizeEvent *>(event);
        const QSize oldsize = re->oldSize();
        const QSize newsize = re->size();

        applying         = true;
        auto *rightDock  = sizingDock({ViewSlot::Chart, ViewSlot::Image, ViewSlot::SlideShow});
        auto *bottomDock = sizingDock({ViewSlot::Log, ViewSlot::Variables});
        if (oldsize.width() > 0 && newsize.width() != oldsize.width() && rightDock)
            mainwindow->resizeDocks({rightDock}, {int(newsize.width() * hsplit)}, Qt::Horizontal);
        if (oldsize.height() > 0 && newsize.height() != oldsize.height() && bottomDock)
            mainwindow->resizeDocks({bottomDock}, {int(newsize.height() * vsplit)}, Qt::Vertical);
        applying = false;
    }
    return QObject::eventFilter(watched, event);
}

void WindowLayout::saveState() const
{
    if (layoutmode != LayoutMode::Docked || !mainwindow) return;
    QSettings settings;
    settings.setValue(Keys::DOCKSTATE, mainwindow->saveState(Cfg::DOCK_STATE_VERSION));

    // Store the proportions alongside it; see createDocks() for why.  The cached
    // values are used rather than the current geometry: this runs while the main
    // window is on its way out, where the widget sizes no longer reflect the
    // layout the user was looking at.
    if (hsplit > 0.0) settings.setValue(Keys::DOCKSPLITH, hsplit);
    if (vsplit > 0.0) settings.setValue(Keys::DOCKSPLITV, vsplit);
}

// A dock panel lives inside the main window, so a sequence it binds is in scope
// at the same time as the main window's own binding for it and Qt fires neither.
// Leave those to the menu: the panel's menu entry still works, only its
// accelerator goes.  This is decided here, when a widget actually becomes a
// panel, rather than when it is built -- a view that stays a window of its own
// (the file viewers, the find dialog, the standalone viewer modes) has no
// ambiguity and keeps everything.
void WindowLayout::deferShortcutsToMainWindow(QWidget *view)
{
    if (!view) return;
    for (auto *shortcut : view->findChildren<QShortcut *>())
        if (isMainWindowShortcut(shortcut->key())) shortcut->setEnabled(false);
    for (auto *action : view->findChildren<QAction *>())
        if (isMainWindowShortcut(action->shortcut())) action->setShortcut(QKeySequence());
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
        // A dock area sizes its panel, so the view must be able to follow it
        // down.  Its layout would otherwise impose the combined minimum of all
        // the controls -- for the charts view that is wide enough to push the
        // editor to its own minimum and make the requested split unreachable.
        if (view) {
            view->setMinimumSize(0, 0);
            if (auto *l = view->layout()) l->setSizeConstraint(QLayout::SetNoConstraint);
        }
        // only the content changes; the dock keeps its area and tab position,
        // so a view that is rebuilt does not move
        d->setWidget(view);
        if (!view) d->hide();
        // the panels are never undocked in this layout, so this is one-way
        deferShortcutsToMainWindow(view);
        updateDockChrome();
        // a newly built view brings its own size hint into the dock area, which
        // would otherwise take the split with it
        scheduleSplit();
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
    scheduleSplit();
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
