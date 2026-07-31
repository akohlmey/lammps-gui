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

#include "commandwindow.h"

#include "constants.h"
#include "helpers.h"
#include "lammpsgui.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCompleter>
#include <QDir>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStringListModel>
#include <QVBoxLayout>

namespace {

// A pipe is a stream with no record of where one command's output ends, so each
// command is followed by a line that says so.  It carries the exit status and
// the working directory as well, which is how the prompt learns about cd,
// pushd/popd, or a directory change made inside a sourced script -- parsing the
// typed line would miss all of those.
constexpr auto SENTINEL = "__LGUI_DONE_";

QString sentinelCommand()
{
#if defined(Q_OS_WIN32)
    return QStringLiteral("echo %s%%errorlevel%%_%%CD%%").arg(SENTINEL);
#else
    // the leading newline puts the sentinel on a line of its own even when the
    // command's output did not end with one; PWD last so a path with spaces in
    // it survives being read to the end of the line
    return QStringLiteral("printf '\\n%1%s_%s\\n' \"$?\" \"$PWD\"").arg(SENTINEL);
#endif
}

} // namespace

QString CommandWindow::preferredShell()
{
    QSettings settings;
    const QString configured = settings.value(Keys::SHELL, QString()).toString();
    if (!configured.isEmpty()) return configured;

#if defined(Q_OS_WIN32)
    const QString comspec = qEnvironmentVariable("COMSPEC");
    return comspec.isEmpty() ? QStringLiteral("cmd.exe") : comspec;
#else
    const QString shell = qEnvironmentVariable("SHELL");
    if (!shell.isEmpty()) return shell;
    // pushd/popd are not POSIX, so prefer a shell that has them
    if (QFileInfo::exists("/bin/bash")) return QStringLiteral("/bin/bash");
    return QStringLiteral("/bin/sh");
#endif
}

CommandWindow::CommandWindow(LammpsGui *_lammpsgui, QWidget *parent) :
    QWidget(parent), lammpsgui(_lammpsgui), scrollback(new QPlainTextEdit), prompt(new QLineEdit),
    cwdlabel(new QLabel), completer(new QCompleter(this)), commands(new QStringListModel(this)),
    workingdir(QDir::currentPath())
{
    scrollback->setReadOnly(true);
    scrollback->setLineWrapMode(QPlainTextEdit::NoWrap);
    scrollback->setMaximumBlockCount(Cfg::COMMAND_SCROLLBACK_LINES);
    scrollback->document()->setDefaultFont(monoFontFromSettings());

    prompt->setFont(monoFontFromSettings());
    prompt->setPlaceholderText("enter a command");
    // whoever hands the focus to this window -- the dock raising its tab, the
    // View menu, a click on a part of it that takes no focus itself -- means the
    // input line, not the container
    setFocusProxy(prompt);
    prompt->installEventFilter(this);
    connect(prompt, &QLineEdit::returnPressed, this, &CommandWindow::submit);
    connect(prompt, &QLineEdit::textEdited, this, &CommandWindow::updateCompleter);

    completer->setCompletionMode(QCompleter::PopupCompletion);
    completer->setCaseSensitivity(Qt::CaseSensitive);
    completer->setModel(commands);
    prompt->setCompleter(completer);

    cwdlabel->setFont(monoFontFromSettings());
    cwdlabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *promptrow = new QHBoxLayout;
    promptrow->addWidget(cwdlabel);
    promptrow->addWidget(prompt, 10);

    auto *top = new QVBoxLayout;
    top->addWidget(scrollback, 10);
    top->addLayout(promptrow);
    createMenuBar();
    top->setMenuBar(menubar);
    setLayout(top);

    QSettings settings;
    history    = settings.value(Keys::CMDHISTORY).toStringList();
    historypos = history.size();

    applyWindowFlags(this);
    resize(Cfg::COMMAND_DEFAULT_WIDTH, Cfg::COMMAND_DEFAULT_HEIGHT);
    startShell();
}

CommandWindow::~CommandWindow()
{
    QSettings settings;
    // keep the tail; a history file that grows without bound helps nobody
    while (history.size() > Cfg::COMMAND_HISTORY_MAX)
        history.removeFirst();
    settings.setValue(Keys::CMDHISTORY, history);

    if (shell && shell->state() != QProcess::NotRunning) {
        shell->closeWriteChannel();
        if (!shell->waitForFinished(Cfg::COMMAND_EXIT_TIMEOUT)) shell->kill();
    }
}

void CommandWindow::createMenuBar()
{
    menubar    = new QMenuBar;
    auto *file = new QMenu("&File", menubar);
    file->setObjectName(Cfg::VIEW_FILE_MENU);

    addMenuAction(file, "&Restart Shell", ":/icons/system-restart.svg", this,
                  &CommandWindow::restartShell);
    addMenuAction(file, "C&lear Output", ":/icons/edit-delete.svg", this,
                  &CommandWindow::clearScrollback);
    file->addSeparator();
    scopeShortcut(this,
                  addMenuAction(file, "&Close", ":/icons/window-close.svg", this,
                                &CommandWindow::closeWindow),
                  QKeySequence(Qt::CTRL | Qt::Key_W));
    auto *quitAct =
        addMenuAction(file, "&Quit", ":/icons/application-exit.svg", this, &CommandWindow::quit);
    scopeShortcut(this, quitAct, QKeySequence(Qt::CTRL | Qt::Key_Q));
    if (!lammpsgui) quitAct->setVisible(false);

    if (dockedLayout()) {
        // the main window shows this menu for us while the panel has the focus
        menubar->hide();
        return;
    }
    menubar->addMenu(file);
    if (lammpsgui)
        for (auto *shared : lammpsgui->sharedMenus())
            menubar->addMenu(shared);
}

void CommandWindow::startShell()
{
    delete shell;
    shell = new QProcess(this);
    // one stream, so what the command wrote to stderr appears where it happened
    shell->setProcessChannelMode(QProcess::MergedChannels);
    shell->setWorkingDirectory(workingdir);

    auto env = QProcessEnvironment::systemEnvironment();
    // Not a terminal: say so, so that a program wanting more than a stream of
    // bytes reports that its terminal is insufficient instead of writing escape
    // sequences into a scrollback that cannot interpret them.
    env.insert("TERM", "dumb");
    // a child of the shell block-buffers when it is not on a tty, which would
    // hold a script's output back until it exits
    env.insert("PYTHONUNBUFFERED", "1");
    shell->setProcessEnvironment(env);

    connect(shell, &QProcess::readyReadStandardOutput, this, &CommandWindow::readOutput);
    connect(shell, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &CommandWindow::shellFinished);

    const QString program = preferredShell();
    shell->start(program, {});
    if (!shell->waitForStarted(Cfg::COMMAND_START_TIMEOUT)) {
        appendOutput(QString("Cannot run \"%1\": %2\n").arg(program, shell->errorString()));
        return;
    }

    appendOutput(QString("%1\n").arg(program));
#if defined(Q_OS_WIN32)
    // cmd.exe otherwise echoes every line it is fed
    shell->write("@echo off\r\n");
#endif
    // ask where we are, so the prompt is right before anything is typed
    shell->write(qPrintable(sentinelCommand() + "\n"));
}

void CommandWindow::changeDirectory(const QString &dir)
{
    if (!shell || shell->state() != QProcess::Running) return;
    shell->write(qPrintable(QString("cd \"%1\"\n").arg(dir)));
    shell->write(qPrintable(sentinelCommand() + "\n"));
}

void CommandWindow::submit()
{
    const QString line = prompt->text();
    if (!shell || shell->state() != QProcess::Running) {
        appendOutput("No shell is running. Use File > Restart Shell.\n");
        return;
    }

    // the transcript reads like a session: the prompt, then what came back
    appendOutput(QString("%1$ %2\n").arg(workingdir, line));
    prompt->clear();

    if (!line.trimmed().isEmpty() && (history.isEmpty() || history.last() != line)) history << line;
    historypos = history.size();

    running = true;
    shell->write(qPrintable(line + "\n"));
    shell->write(qPrintable(sentinelCommand() + "\n"));
}

void CommandWindow::readOutput()
{
    consume(QString::fromLocal8Bit(shell->readAllStandardOutput()));
}

void CommandWindow::consume(const QString &chunk)
{
    pending += chunk;
    pending.replace("\r\n", "\n");

    // only whole lines can be examined for the sentinel
    int nl = pending.indexOf('\n');
    while (nl >= 0) {
        const QString line = pending.left(nl);
        pending.remove(0, nl + 1);

        const int mark = line.indexOf(SENTINEL);
        if (mark >= 0) {
            // "<status>_<directory>"; the status has no underscore in it, so the
            // first one separates them and the rest is the path, spaces and all
            const QString tail = line.mid(mark + int(qstrlen(SENTINEL)));
            const int sep      = tail.indexOf('_');
            if (sep > 0) {
                const int status = tail.left(sep).toInt();
                workingdir       = tail.mid(sep + 1);
                if (running && status != 0) appendOutput(QString("[exit status %1]\n").arg(status));
                updatePrompt();
            }
            running = false;
        } else {
            appendOutput(line + "\n");
        }
        nl = pending.indexOf('\n');
    }
}

void CommandWindow::appendOutput(const QString &text)
{
    QString out = text;
    // a progress bar rewrites its line with a carriage return; keep only what it
    // ended up showing rather than every intermediate state
    if (out.contains('\r')) {
        QStringList lines = out.split('\n');
        for (auto &line : lines) {
            const int cr = line.lastIndexOf('\r');
            if (cr >= 0) line = line.mid(cr + 1);
        }
        out = lines.join('\n');
    }
    if (out.endsWith('\n')) out.chop(1);

    scrollback->appendPlainText(out);
    scrollback->moveCursor(QTextCursor::End);
}

void CommandWindow::updatePrompt()
{
    cwdlabel->setText(workingdir + "$");
}

void CommandWindow::shellFinished()
{
    appendOutput("\n[the shell exited; use File > Restart Shell to start a new one]\n");
}

void CommandWindow::restartShell()
{
    appendOutput("\n[restarting the shell]\n");
    startShell();
}

void CommandWindow::clearScrollback()
{
    scrollback->clear();
}

void CommandWindow::quit()
{
    if (lammpsgui) lammpsgui->quit();
}

void CommandWindow::closeWindow()
{
    close();
}

QStringList CommandWindow::pathCommands()
{
    static QStringList cached;
    if (!cached.isEmpty()) return cached;

    const auto sep  = QDir::listSeparator();
    const auto dirs = qEnvironmentVariable("PATH").split(sep, Qt::SkipEmptyParts);
    for (const auto &dir : dirs) {
        const QFileInfoList entries =
            QDir(dir).entryInfoList(QDir::Files | QDir::Executable | QDir::NoDotAndDotDot);
        for (const auto &entry : entries)
            cached << entry.fileName();
    }
    cached.removeDuplicates();
    cached.sort();
    return cached;
}

void CommandWindow::updateCompleter(const QString &text)
{
    // the first word is a command, everything after it is most likely a path
    if (text.contains(' ')) {
        if (!qobject_cast<QFileSystemModel *>(completer->model())) {
            auto *files = new QFileSystemModel(completer);
            files->setRootPath(workingdir);
            completer->setModel(files);
        }
    } else if (completer->model() != commands) {
        completer->setModel(commands);
    }
    if (completer->model() == commands && commands->stringList().isEmpty())
        commands->setStringList(pathCommands());
}

bool CommandWindow::eventFilter(QObject *watched, QEvent *event)
{
    if ((watched == prompt) && (event->type() == QEvent::KeyPress)) {
        auto *key = static_cast<QKeyEvent *>(event);
        // walk the history; the completer popup uses the arrows itself, so this
        // only applies while it is not showing
        if (!completer->popup()->isVisible()) {
            if (key->key() == Qt::Key_Up) {
                if (historypos > 0) prompt->setText(history.value(--historypos));
                return true;
            }
            if (key->key() == Qt::Key_Down) {
                if (historypos < history.size()) ++historypos;
                prompt->setText(historypos < history.size() ? history.value(historypos)
                                                            : QString());
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Local Variables:
// c-basic-offset: 4
// End:
