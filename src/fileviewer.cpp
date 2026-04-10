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

#include "fileviewer.h"

#include "helpers.h"
#include "lammpsgui.h"

#include <QApplication>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QFontInfo>
#include <QIcon>
#include <QKeySequence>
#include <QProcess>
#include <QSettings>
#include <QShortcut>
#include <QString>
#include <QStringList>
#include <QTextCursor>
#include <QTextStream>

FileViewer::FileViewer(const QString &_filename, const QString &title, QWidget *parent) :
    QPlainTextEdit(parent), fileName(_filename)
{
    auto *action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    connect(action, &QShortcut::activated, this, &FileViewer::quit);
    action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    connect(action, &QShortcut::activated, this, &FileViewer::stopRun);

    installEventFilter(this);

    // open and read file. Set editor to read-only.
    QFile file(fileName);
    QFileInfo finfo(file);
    QString command;
    QString content;
    QProcess decomp;
    QStringList args = {"-cdf", fileName};
    bool compressed  = false;

    // match suffix with decompression program
    if (finfo.suffix() == "gz") {
        command    = "gzip";
        compressed = true;
    } else if (finfo.suffix() == "bz2") {
        command    = "bzip2";
        compressed = true;
    } else if (finfo.suffix() == "zst") {
        command    = "zstd";
        compressed = true;
    } else if (finfo.suffix() == "xz") {
        command    = "xz";
        compressed = true;
    } else if (finfo.suffix() == "lzma") {
        command = "xz";
        args.insert(1, "--format=lzma");
        compressed = true;
    } else if (finfo.suffix() == "lz4") {
        command    = "lz4";
        compressed = true;
    }

    // read compressed file from pipe
    if (compressed) {
        decomp.start(command, args, QIODevice::ReadOnly);
        if (decomp.waitForStarted()) {
            while (decomp.waitForReadyRead())
                content += decomp.readAll();
        } else {
            content = "\nCould not open compressed file %1 with decompression program %2\n";
            content = content.arg(fileName).arg(command);
        }
        decomp.close();
    } else if (file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        // read plain text
        QTextStream in(&file);
        content = in.readAll();
        file.close();
    }

    QSettings settings;
    QFont mono_font;
    QFontInfo mono_info(*GUI_MONOFONT);
    mono_font.setFamily(settings.value("monofamily", mono_info.family()).toString());
    mono_font.setPointSize(settings.value("monosize", mono_info.pointSize()).toInt());
    mono_font.setStyleHint(GUI_MONOFONT->styleHint());
    mono_font.setFixedPitch(true);
    document()->setDefaultFont(mono_font);

    document()->setPlainText(content);
    moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
    setReadOnly(true);
    setLineWrapMode(NoWrap);
    setMinimumSize(800, 500);
    setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    if (title.isEmpty())
        setWindowTitle("LAMMPS-GUI - Viewer - " + fileName);
    else
        setWindowTitle(title);

    // set window flags for window manager
    auto flags = windowFlags();
    flags &= ~Qt::Dialog;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowMinimizeButtonHint;
    // must add maximize button for macOS to allow resizing, but remove on other platforms
#if defined(Q_OS_MACOS)
    flags |= Qt::WindowMaximizeButtonHint;
#else
    flags &= ~Qt::WindowMaximizeButtonHint;
#endif
    setWindowFlags(flags);
}

void FileViewer::quit()
{
    auto *main = dynamic_cast<LammpsGui *>(getMainWidget());
    if (main) main->quit();
}

void FileViewer::stopRun()
{
    auto *main = dynamic_cast<LammpsGui *>(getMainWidget());
    if (main) main->stopRun();
}

// event filter to handle "Ambiguous shortcut override" issues
bool FileViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (!keyEvent) return QAbstractScrollArea::eventFilter(watched, event);
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == '/') {
            stopRun();
            event->accept();
            return true;
        }
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == 'W') {
            close();
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Local Variables:
// c-basic-offset: 4
// End:
