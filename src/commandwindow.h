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

#ifndef COMMANDWINDOW_H
#define COMMANDWINDOW_H

#include <QList>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QWidget>

class LammpsGui;
class QCompleter;
class QLabel;
class QLineEdit;
class QMenuBar;
class QPlainTextEdit;
class QPushButton;
class QStringListModel;

/**
 * @brief A shell prompt with a scrollback, next to the simulation
 *
 * CommandWindow forwards typed lines to one long-lived shell process and streams
 * what it writes back into a read-only scrollback.  It exists for the ordinary
 * work that surrounds a run -- post-process a dump file with a Python script,
 * look at what a run just wrote, call a plotting tool -- without leaving the GUI.
 *
 * It is deliberately **not a terminal emulator**: there is no pseudo terminal, so
 * no escape sequence interpretation, no cursor addressing, no color, and no
 * curses program.  @c TERM is set to @c dumb precisely so that a program needing
 * more than a stream of bytes finds out and says so, rather than writing escape
 * sequences into a scrollback that cannot interpret them.
 *
 * The shell is kept alive between commands, which is what makes @c cd,
 * @c pushd / @c popd, environment variables and the rest of the shell state work
 * at all.  This class does not implement any of them; it only observes the
 * result, by appending a sentinel to every command that reports the exit status
 * and the shell's working directory.
 *
 * @see doc/command-window-design.md for the reasoning and the known gaps
 */
class CommandWindow : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param lammpsgui Pointer to LammpsGui for sending signals (may be nullptr)
     * @param parent Parent widget
     */
    explicit CommandWindow(LammpsGui *lammpsgui, QWidget *parent = nullptr);

    /**
     * @brief Destructor; ends the shell process
     */
    ~CommandWindow() override;

    CommandWindow()                                 = delete;
    CommandWindow(const CommandWindow &)            = delete;
    CommandWindow(CommandWindow &&)                 = delete;
    CommandWindow &operator=(const CommandWindow &) = delete;
    CommandWindow &operator=(CommandWindow &&)      = delete;

    /**
     * @brief Move the shell to a directory
     * @param dir Directory to change to
     *
     * Sent as a command, so the shell stays the one place that knows where it
     * is and the prompt is updated from its answer like any other change.
     */
    void changeDirectory(const QString &dir);

    /**
     * @brief The command interpreter that will be run
     * @return Path of the user's preferred shell
     *
     * @c $SHELL on Unix-like systems, falling back to @c /bin/bash and then
     * @c /bin/sh; @c %COMSPEC% on Windows, falling back to @c cmd.exe.
     */
    static QString preferredShell();

private slots:
    void submit();          ///< Send the typed line to the shell
    void readOutput();      ///< Drain the shell's output into the scrollback
    void shellFinished();   ///< Report that the shell has gone away
    void interrupt();       ///< Send SIGINT to the shell and what it is running
    void killCommand();     ///< End the processes the shell is running
    void restartShell();    ///< Discard the shell and start a fresh one
    void clearScrollback(); ///< Empty the scrollback, keeping the shell
    void editAliases();     ///< Edit the aliases defined in every shell
    void quit();            ///< Quit the application (via LammpsGui::quit)
    void closeWindow();     ///< Close this window

protected:
    /**
     * @brief Filter the prompt's key presses for the history keys
     * @param watched Object being watched
     * @param event Event to filter
     * @return true if the event was consumed
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    /// Start the shell process and ask it where it is.
    void startShell();

    /// Build the File menu and the menu bar that holds it.
    void createMenuBar();

    /// Turn complete lines of shell output into scrollback text, picking the
    /// sentinel out of the stream as it goes.
    void consume(const QString &chunk);

    /// Append text to the scrollback, resolving carriage returns so that a
    /// progress bar overwrites its line instead of filling the buffer.
    void appendOutput(const QString &text);

    /// Show the working directory in front of the input line.
    void updatePrompt();

    /// Tell the shell how large the panel is, through COLUMNS and LINES, so
    /// that a program formatting its output to a width uses that rather than
    /// the 80 columns it falls back to when there is no terminal to ask.
    void sendTerminalSize();

    /// The processes the shell started directly, asked of the operating system
    /// because without job control the shell keeps no job table.
    QList<qint64> shellChildren() const;

    /// Executable names found in PATH, collected once and cached.
    QStringList pathCommands();

    /// Point the completer at commands for the first word and at file names
    /// after it.
    void updateCompleter(const QString &text);

    LammpsGui *lammpsgui;        ///< Main widget pointer for receiving signals
    QProcess *shell = nullptr;   ///< The long-lived interpreter
    QPlainTextEdit *scrollback;  ///< Read-only transcript of the session
    QLineEdit *prompt;           ///< The input line
    QLabel *cwdlabel;            ///< Working directory shown in front of it
    QMenuBar *menubar = nullptr; ///< Own menu bar; hidden in the combined layout
    QPushButton *killbutton;     ///< Ends the running command
    QCompleter *completer;       ///< Completes commands and file names
    QStringListModel *commands;  ///< Model behind the command completion

    QString shellprogram;     ///< The interpreter that was started
    QString pending;          ///< Output received so far that is not a complete line
    QString workingdir;       ///< Where the shell last reported itself to be
    QStringList history;      ///< Lines typed so far, oldest first
    int historypos   = 0;     ///< Position while walking the history, == size when idle
    bool running     = false; ///< A command was sent and its sentinel is outstanding
    bool priming     = false; ///< Still swallowing the shell's start-up chatter
    int termcols     = 0;     ///< Panel width in characters, as last told to the shell
    int termrows     = 0;     ///< Panel height in lines, as last told to the shell
    bool sizepending = true;  ///< The shell still has to be told the panel size
    QStringList pendingsetup; ///< Lines to send once the shell is at a prompt again
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
