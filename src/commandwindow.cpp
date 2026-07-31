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
#include <QFile>
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
#include <QPushButton>
#include <QSettings>
#include <QStringListModel>
#include <QTimer>
#include <QVBoxLayout>

#if !defined(Q_OS_WIN32)
#include <csignal>
#include <unistd.h>
#endif

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

// An interactive shell without a terminal complains whenever it would otherwise
// hand one to a job -- when a command ends, and loudly when one is killed.  The
// message says nothing about the command and there is no terminal to be had, so
// it is dropped rather than shown after every job.
bool isShellJobControlNoise(const QString &line)
{
    return line.contains("Inappropriate ioctl for device") ||
           line.contains("no job control in this shell");
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
    // set on the widget, not only on the document: docked, this becomes a child
    // of the main window and would otherwise inherit its proportional font,
    // which QPlainTextEdit then adopts for the document as well
    scrollback->setFont(monoFontFromSettings());

    prompt->setFont(monoFontFromSettings());
    prompt->setPlaceholderText("enter a command");
    prompt->setToolTip("Commands run in the foreground and hold the prompt until they\n"
                       "finish, as they would in a terminal.  Start a graphical or\n"
                       "long-running program with a trailing \"&\" to keep the prompt\n"
                       "free -- there is no job control here to background it after\n"
                       "the fact with Ctrl-Z.");
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

    killbutton = new QPushButton(QIcon(":/icons/skull.svg"), "");
    killbutton->setToolTip("Kill the running command");
    killbutton->setEnabled(false);
    styleToolButtons(toolButtonSize(killbutton), {killbutton});
    connect(killbutton, &QPushButton::released, this, &CommandWindow::killCommand);

    auto *promptrow = new QHBoxLayout;
    promptrow->addWidget(cwdlabel);
    promptrow->addWidget(prompt, 10);
    promptrow->addWidget(killbutton);

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

    addMenuAction(file, "&Interrupt Command", ":/icons/process-stop.svg", this,
                  &CommandWindow::interrupt);
    addMenuAction(file, "&Kill Command", ":/icons/skull.svg", this, &CommandWindow::killCommand);
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
    if (shell) {
        // stop it reporting its own death: we are the ones ending it
        shell->disconnect(this);
        delete shell;
    }
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
    QStringList args;
#if !defined(Q_OS_WIN32)
    // An interactive shell is what reads the user's rc file, and that is where
    // aliases and shell functions live -- a non-interactive one skips it, and
    // the guard most rc files open with would bail out even if it did not.
    // --noediting keeps bash from running its line editor over input that is a
    // pipe, which would otherwise echo every line back at us and wrap it in the
    // escape sequences meant for a terminal.
    if (QFileInfo(program).fileName().contains("bash")) args << "--noediting";
    args << "-i";

    // put the shell in a session of its own, so a signal can be sent to it and
    // to whatever it is running rather than to this application
    shell->setChildProcessModifier([]() {
        setsid();
    });
#endif
    shell->start(program, args);
    if (!shell->waitForStarted(Cfg::COMMAND_START_TIMEOUT)) {
        appendOutput(QString("Cannot run \"%1\": %2\n").arg(program, shell->errorString()));
        return;
    }

    appendOutput(QString("%1\n").arg(program));

    // Everything the shell says before the first sentinel is its own start-up
    // noise -- the prompt it prints because it is interactive, and its complaint
    // about having no terminal to put a job in the foreground of.
    priming = true;
#if defined(Q_OS_WIN32)
    // cmd.exe otherwise echoes every line it is fed
    shell->write("@echo off\r\n");
#else
    // this window supplies the prompt, so the shell must not print one; and
    // history expansion is on in an interactive shell, which would turn a "!"
    // in an ordinary command line into an error
    // this window supplies the prompt, so the shell must not print one -- and
    // PROMPT_COMMAND is where a distribution's rc file hides the escape
    // sequence that sets a terminal's title.  History expansion is on in an
    // interactive shell, which would turn a "!" in an ordinary command into an
    // error.
    shell->write("PS1='' ; PS2='' ; unset PROMPT_COMMAND 2>/dev/null ;"
                 " set +H 2>/dev/null\n");
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
    // Return still arrives while the line is read-only, so the guard belongs
    // here and not only on the typing: sending anything now would queue it
    // behind the running command and leave the state waiting for a second
    // sentinel that answers nothing.
    if (running) return;

    const QString line = prompt->text();
    if (line.trimmed().isEmpty()) {
        prompt->clear();
        return;
    }
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
    updatePrompt();
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
            const QString tail  = line.mid(mark + int(qstrlen(SENTINEL)));
            const int sep       = tail.indexOf('_');
            const bool wasabout = running;
            // clear the state before the prompt is redrawn from it
            running = false;
            priming = false;
            if (sep > 0) {
                const int status = tail.left(sep).toInt();
                workingdir       = tail.mid(sep + 1);
                if (wasabout && status != 0)
                    appendOutput(QString("[exit status %1]\n").arg(status));
            }
            updatePrompt();
        } else if (!priming && !isShellJobControlNoise(line)) {
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
    // While a command holds the shell there is nothing useful to do with a typed
    // line: it would go down the same pipe and be read by the running program,
    // if it reads at all, and by the shell only once that had finished.  Refuse
    // it instead, and say why.
    prompt->setReadOnly(running);
    killbutton->setEnabled(running);
    if (running) {
        cwdlabel->setText("running >");
        prompt->setPlaceholderText("command running -- append \"&\" to background the next one");
    } else {
        cwdlabel->setText(workingdir + "$");
        prompt->setPlaceholderText("enter a command");
    }
}

void CommandWindow::shellFinished()
{
    running = false;
    updatePrompt();
    appendOutput("\n[the shell exited; use File > Restart Shell to start a new one]\n");
}

// The processes the shell started directly.  Without job control the shell has
// no job table to ask, so this goes to the operating system instead; a command
// that started children of its own leaves those behind, which is the price of
// not having a session to signal.
QList<qint64> CommandWindow::shellChildren() const
{
    QList<qint64> kids;
    if (!shell || shell->processId() <= 0) return kids;
    const qint64 pid = shell->processId();

#if defined(Q_OS_LINUX)
    QFile children(QString("/proc/%1/task/%1/children").arg(pid));
    if (children.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto parts = QString::fromLatin1(children.readAll()).split(' ', Qt::SkipEmptyParts);
        for (const auto &part : parts) {
            bool ok        = false;
            const qint64 c = part.trimmed().toLongLong(&ok);
            if (ok && (c > 0)) kids << c;
        }
    }
#elif !defined(Q_OS_WIN32)
    QProcess pgrep;
    pgrep.start("pgrep", {"-P", QString::number(pid)});
    if (pgrep.waitForFinished(Cfg::COMMAND_PGREP_TIMEOUT)) {
        const auto lines =
            QString::fromLatin1(pgrep.readAllStandardOutput()).split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            bool ok        = false;
            const qint64 c = line.trimmed().toLongLong(&ok);
            if (ok && (c > 0)) kids << c;
        }
    }
#endif
    return kids;
}

void CommandWindow::killCommand()
{
#if defined(Q_OS_WIN32)
    appendOutput("\n[killing is not supported on this platform;"
                 " use File > Restart Shell]\n");
#else
    if (!shell || (shell->state() != QProcess::Running) || !running) return;

    const auto kids = shellChildren();
    if (kids.isEmpty()) {
        // nothing identifiable to end; freeing the prompt is the next best thing
        appendOutput("\n[cannot identify the running command; restarting the shell]\n");
        restartShell();
        return;
    }

    // ask first, insist shortly afterwards
    appendOutput("\n[killing the running command]\n");
    for (const auto pid : kids)
        ::kill(static_cast<pid_t>(pid), SIGTERM);
    QTimer::singleShot(Cfg::COMMAND_KILL_GRACE, this, [kids]() {
        for (const auto pid : kids)
            if (::kill(static_cast<pid_t>(pid), 0) == 0) ::kill(static_cast<pid_t>(pid), SIGKILL);
    });
#endif
}

void CommandWindow::interrupt()
{
#if defined(Q_OS_WIN32)
    appendOutput("\n[interrupting is not supported on this platform;"
                 " use File > Restart Shell]\n");
#else
    if (!shell || shell->state() != QProcess::Running) return;

    // The shell was put in a session of its own so that its children share a
    // process group with it and nothing else does.  Ask for that group rather
    // than assuming the shell leads it, and refuse to signal our own group,
    // which would take this application down with the command.
    const pid_t group = ::getpgid(shell->processId());
    if ((group <= 0) || (group == ::getpgid(0))) {
        appendOutput("\n[cannot interrupt this command; use File > Restart Shell]\n");
        return;
    }
    // Best effort: bash has no job control here -- there is no terminal to hand
    // a foreground process group to -- and it starts children with SIGINT
    // ignored, so a program that does not install its own handler will sit
    // through this.  File > Restart Shell is the way out of those.
    ::kill(-group, SIGINT);
    appendOutput("\n[interrupt sent; use File > Restart Shell if it had no effect]\n");
#endif
}

void CommandWindow::restartShell()
{
    // Only the shell is ended.  Whatever it started keeps running, which is what
    // is wanted for a program launched with "&", and equally for the graphical
    // one that is holding the prompt: the point is to get a usable prompt back,
    // not to take the user's windows away.
    appendOutput("\n[restarting the shell]\n");
    running = false;
    updatePrompt();
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
