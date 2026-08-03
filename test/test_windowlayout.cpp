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

#include <gtest/gtest.h>

#include <QApplication>
#include <QDockWidget>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QSettings>
#include <QTemporaryDir>
#include <QWidget>

// WindowLayout is the presentation policy for the output views.  Docked, a view
// is a child of a dock widget rather than a window of its own, and two things
// that are obvious for a window are not obvious there: closing the view has to
// take the dock with it, and the dock's title bar is a widget we supply.  These
// tests pin both down.
class WindowLayoutTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        if (!QApplication::instance()) {
            qputenv("QT_QPA_PLATFORM", "offscreen");
            static int argc     = 1;
            static char *argv[] = {(char *)"test_windowlayout"};
            app                 = new QApplication(argc, argv);
        }
        // WindowLayout reads and writes QSettings; send those to a throw-away
        // directory so a test run cannot touch the real configuration
        QCoreApplication::setOrganizationName("LAMMPS-GUI-Test");
        QCoreApplication::setApplicationName("test_windowlayout");
        QSettings::setDefaultFormat(QSettings::IniFormat);
        settingsdir = new QTemporaryDir;
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settingsdir->path());
    }

    static void TearDownTestSuite()
    {
        delete settingsdir;
        settingsdir = nullptr;
    }

    static QApplication *app;
    static QTemporaryDir *settingsdir;
};

QApplication *WindowLayoutTest::app          = nullptr;
QTemporaryDir *WindowLayoutTest::settingsdir = nullptr;

// A bare QWidget has no layout and therefore an *invalid* size hint of (-1,-1).
// Qt takes the height of a dock's title bar straight from that hint, so using
// one as the "no title bar" placeholder puts a -1 into the dock's own minimum
// height, where it surfaces as "QWidget::setMinimumSize: Negative sizes (0,-1)
// are not possible".
TEST_F(WindowLayoutTest, DockTitleBarsHaveAValidSizeHint)
{
    QMainWindow window;
    window.show();
    WindowLayout layout(&window, LayoutMode::Docked);

    // two panels of one group, so they are tabbed: a dock that is alone keeps a
    // title bar naming it, and only a tabbed one is given the empty placeholder
    layout.place(ViewSlot::Log, new QPlainTextEdit);
    layout.place(ViewSlot::Command, new QPlainTextEdit);
    layout.show(ViewSlot::Log);
    layout.show(ViewSlot::Command);
    QApplication::processEvents();

    const auto docks = window.findChildren<QDockWidget *>();
    ASSERT_FALSE(docks.isEmpty());
    for (const auto *dock : docks) {
        const auto *title = dock->titleBarWidget();
        if (!title) continue;
        EXPECT_TRUE(title->sizeHint().isValid())
            << "dock " << qPrintable(dock->objectName()) << " has a title bar of size ("
            << title->sizeHint().width() << "," << title->sizeHint().height() << ")";
        EXPECT_GE(dock->minimumSizeHint().height(), 0);
    }
}

// Closing a docked view used to hide the widget and leave the dock -- and with
// it the tab naming it, and the space it holds -- behind and empty.
TEST_F(WindowLayoutTest, ClosingADockedViewClosesItsDock)
{
    QMainWindow window;
    // a dock of a window that was never shown reports itself hidden either way
    window.show();
    WindowLayout layout(&window, LayoutMode::Docked);

    auto *view = new QPlainTextEdit;
    layout.place(ViewSlot::Log, view);
    layout.show(ViewSlot::Log);
    QApplication::processEvents();
    ASSERT_TRUE(layout.isVisible(ViewSlot::Log));

    view->close();
    QApplication::processEvents();

    EXPECT_FALSE(layout.isVisible(ViewSlot::Log));
    // the widget itself stays, so showing the panel again brings it back
    EXPECT_FALSE(view->isHidden());
}

// Undocked, a view is a window and closing it is Qt's business, not ours.
TEST_F(WindowLayoutTest, ClosingAFloatingViewIsLeftAlone)
{
    WindowLayout layout(nullptr, LayoutMode::Windows);

    auto *view = new QPlainTextEdit;
    layout.place(ViewSlot::Log, view);
    layout.show(ViewSlot::Log);
    ASSERT_TRUE(layout.isVisible(ViewSlot::Log));

    view->close();

    EXPECT_FALSE(layout.isVisible(ViewSlot::Log));
    EXPECT_TRUE(view->isHidden());
    delete view;
}

// The saved dock arrangement is a QMainWindow::saveState() blob whose format
// belongs to the Qt release that reads it back, and restoring one written by a
// different feature release can crash.  The settings key therefore carries the
// running Qt's feature version -- and only that, so a patch-level update does
// not silently discard the arrangement the user set up.
TEST_F(WindowLayoutTest, DockStateKeyIsQualifiedByQtFeatureVersion)
{
    const QString version = QString::fromLatin1(qVersion());
    const QString feature = version.section('.', 0, 1); // "6.9" out of "6.9.1"
    ASSERT_FALSE(feature.isEmpty());

    EXPECT_EQ(Keys::DOCKSTATE, QString("dockstate_%1").arg(feature));

    // it must not be the unqualified key any more, nor carry the patch level
    EXPECT_NE(Keys::DOCKSTATE, Keys::DOCKSTATE_LEGACY);
    if (version != feature) {
        EXPECT_FALSE(Keys::DOCKSTATE.contains(version));
    }
}

// The arrangement has to go out under the versioned key, and an unversioned one
// left over from before -- written by an unknown Qt, so unsafe to restore -- has
// to be gone rather than sit there waiting to be picked up.
TEST_F(WindowLayoutTest, SavingUsesTheVersionedKeyAndDropsTheLegacyOne)
{
    {
        QSettings settings;
        settings.setValue(Keys::DOCKSTATE_LEGACY, QByteArray("stale-blob-from-some-other-qt"));
        settings.remove(Keys::DOCKSTATE);
        settings.sync();
    }

    QMainWindow window;
    window.show();
    WindowLayout layout(&window, LayoutMode::Docked); // reads, and drops the legacy key
    layout.place(ViewSlot::Log, new QPlainTextEdit);
    layout.show(ViewSlot::Log);
    QApplication::processEvents();
    layout.saveState();

    QSettings settings;
    settings.sync();
    EXPECT_FALSE(settings.contains(Keys::DOCKSTATE_LEGACY));
    EXPECT_FALSE(settings.value(Keys::DOCKSTATE).toByteArray().isEmpty());
}

// Local Variables:
// c-basic-offset: 4
// End:
