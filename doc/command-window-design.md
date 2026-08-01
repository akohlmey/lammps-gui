# Command window (shell console) -- design notes

**Status: implemented** on the `docked-layout` branch as
`src/commandwindow.{cpp,h}`, opened from *Run* > *Open Command Window*
(`Ctrl-Shift-X`) and tabbed with the Output panel at the bottom.  This file
remains the record of *why* it is shaped the way it is, and of the gaps that
were accepted rather than solved -- see "Known gaps" below, none of which have
changed.

## What it is

A dock panel in the output area with a shell prompt: a scrollback buffer, a
command line with history and command/filename completion. Typed lines are forwarded to a **persistent** shell process
(`$SHELL` on Unix, `cmd.exe` on Windows) whose output is streamed back into the
scrollback.

The purpose is running ordinary commands next to the simulation -- post-process
a dump file with a Python script, look at what a run just wrote, call a plotting
tool -- without leaving the GUI.

## What it is not

- **Not a terminal emulator.** No PTY, no ANSI/VT parsing, no cursor
  addressing, no color, no curses programs (`vim`, `top`, `less`).
- **Not a LAMMPS command console.** Passing commands to the running LAMMPS
  instance was considered first and is a *separate* idea with different
  problems (idle-only dispatch, cached GUI state going stale, a bad command
  poisoning the instance). Do not conflate the two.

## Architecture

`QProcess` holding one long-lived shell, started with `setWorkingDirectory()`
seeded from the GUI's current directory, and `MergedChannels` so stderr
interleaves with stdout in the right order. Typed lines are written to its
stdin. A persistent shell is what makes `cd`, `pushd`/`popd`, environment
variables and shell state work at all -- the panel does not implement any of
them, it only observes the result.

`QProcess` is already used in `movieimport`, `imagecache`, `slideshow`,
`fileviewer` and `helpers`, but always synchronously via `waitForFinished()`.
This panel needs the `readyReadStandardOutput` signal instead, so a long job
streams rather than blocking the UI.

The dock slot itself is cheap: a `ViewSlot::Command` in `windowlayout.h`, a
title, tabified with the Output panel at the bottom, plus a View menu entry.

### Command framing

A pipe is just a stream, so there is no way to tell where one command's output
ends. After each typed line, send a sentinel:

```sh
printf '\n__LGUI_DONE_%s_%s\n' "$?" "$PWD"
```

Watching for that line yields three things at once: the command finished, its
exit status, and the shell's current directory. Put `$PWD` last and read to end
of line so paths containing spaces survive. Send it once at startup too, so the
first prompt shows the right directory.

That line is POSIX, and shells do not agree on any of the three things it needs,
so the sentinel -- along with the start-up line that silences the shell's own
prompt, and the arguments it is started with -- is chosen by shell family
(`ShellKind` in `commandwindow.cpp`, decided from the program name):

| | exit status | directory | prompt off | echo off | alias |
|---|---|---|---|---|---|
| POSIX (`sh`, `bash`, `dash`) | `$?` | `$PWD` | `PS1=''` | `bash --noediting` | `alias n='v'` |
| zsh | `$?` | `$PWD` | `PS1=''` + below | not needed | `alias n='v'` |
| csh, tcsh | `$status` | `$cwd` | `set prompt = ""` | `unset edit` | `alias n 'v'` |
| `cmd.exe` | `%errorlevel%` | `%CD%` | -- | `@echo off` | none |

Three of these are not obvious.

In csh, `echo` is a builtin that sets a status of its own, so the status being
reported has to be saved into a variable before the first `echo` of the sentinel
runs. And csh's line editor, not any argument, is what echoes the command back
on a pipe; `unset edit` is the off switch that `--noediting` is for bash.
Getting either wrong looks the same from the outside: every command's output
appears one command late, with the line echoed in front of it.

zsh looks POSIX and mostly is, but `set +H` does **not** turn history expansion
off there -- `unsetopt banghist` does -- so with the POSIX line a command
containing a `!` fails with "event not found". It also prints from places bash
does not: `RPROMPT` on the right, `PROMPT_EOL_MARK` where output did not end in
a newline, and the `precmd`/`preexec` hooks a theme uses in place of
`PROMPT_COMMAND`. The last three are cosmetic here only because the scrollback's
carriage-return handling happens to swallow them; the history expansion is a
real failure. Neither zsh nor csh needs an argument to keep its line editor off,
because neither runs it when what it reads is not a terminal.

### Working directory

Tracked from the sentinel, never by parsing the typed line -- parsing would miss
`cd -`, `pushd`/`popd`, `$CDPATH` jumps, and any directory change made inside a
sourced script. The tracked directory is needed for two things: completing
relative paths against the right place, and showing it in the prompt.

`readlink /proc/<pid>/cwd` gives the same answer on Linux without the shell's
cooperation, but macOS needs `libproc` and Windows has no equivalent for another
process, so it is at best a cross-check and not the mechanism.

**Coupling decision:** seed the panel from the GUI's current directory at start,
then keep them independent. Add an explicit "cd to the input file's directory"
action rather than syncing silently -- automatic syncing would move the shell
out from under a running job.

## Front end

Mostly assembly out of existing pieces:

- scrollback: read-only `QPlainTextEdit` with `setMaximumBlockCount()`; needs
  carriage-return handling or `\r` progress bars fill it with junk
- history: `QStringList` with up/down, persisted through `QSettings`. A `Ctrl+R`
  reverse search was wanted and is **not** what was built: the lines already
  typed, sorted and deduplicated, are put at the front of the same completion
  model the command names are in, so typing the first characters of a long line
  offers it back. That reuses the completer already there, needs no key of its
  own, and needs no incremental-search mode with its own editing rules. The
  arrow keys keep the order the lines were typed in, which is the order they are
  wanted in there; the completion list is sorted, which is the order they are
  wanted in here
- command completion: a cached scan of `PATH`, refreshed on demand
- `CodeEditor` already runs several `QCompleter` instances -- copy that pattern
- filename completion: **not** `QFileSystemModel`, which was tried and did not
  work. `QLineEdit` hands its completer the *whole line*, so the file model was
  being asked to resolve `"cat /some/dir/parti"` as one path and matched
  nothing -- argument completion silently did nothing at all. Two things follow.
  First, the completer has to match and replace *the word the cursor is in*,
  which is what `WordCompleter` overrides `splitPath()` and `pathFromIndex()`
  for; the whole-line behavior is still what the first word wants, and falls
  out of the same override when there is no space yet. Second, with the word in
  hand the model can be a plain `QStringListModel` of the names in the tracked
  working directory -- no path traversal, no absolute paths, no separator
  question per platform. It is rebuilt when the shell changes directory and
  whenever completion starts on a new argument, so a file a command has just
  written is offered without reopening the panel.

**No Emacs-style line editing.** `QLineEdit` covers basic editing but not
`Ctrl+A/E/K/U/W/Y`, and on Linux `Ctrl+A` is *select all*. Adding those was
considered and dropped: keeping whatever Qt already provides is consistent with
the rest of the application, which is worth more here than readline muscle
memory.

## Known gaps -- decide before building

- **No interrupt, and no job control.** Without a PTY there is no `Ctrl+C` and
  no `Ctrl+Z`/`bg`: the shell says "no job control in this shell" at start-up and
  keeps no job table. *Interrupt Command* signals the shell's process group --
  the shell is given a session of its own with `setsid()` so this cannot reach
  the application -- but it is best effort, because without job control bash
  starts children with `SIGINT` ignored. Measured: the group is right, the
  signal is delivered, and a plain `sleep` sits through it. *Restart Shell* is
  the reliable recovery and is the practical equivalent of `Ctrl+Z` then `bg`:
  the prompt comes back in the same directory and the program keeps running,
  orphaned rather than backgrounded. *Kill Command* is the other half: the
  shell's direct children are asked of the operating system -- `/proc/<pid>/
  task/<pid>/children` on Linux, `pgrep -P` elsewhere, since without job control
  the shell keeps no job table -- and sent `SIGTERM` then `SIGKILL`. A command
  that started children of its own leaves those behind. Real job control is
  where PTY pressure comes back.
- **Start-up file sections gated on a terminal do not run.** The shell is
  started interactive, so it *does* read the user's rc file, but a fragment that
  opens with `[ ! -t 0 ] && return` stops there because stdin is a pipe. On
  Fedora that fragment is `/etc/profile.d/colorls.sh`, so `ls`, `ll` and `l.`
  are missing while every other alias is present -- confusing exactly because it
  looks like the rc file was not read at all. Anything of the user's own behind
  such a test is skipped the same way. Measured: giving the shell a pty as
  **stdin alone** restores them (3 of 3 versus 0 of 3), with stdout and stderr
  left as pipes and `TERM` still `dumb`, so no escape sequences reach the
  scrollback and the line discipline's echo goes to the pty master, which is
  never read. That is a real option if this turns out to matter -- it is not
  terminal emulation, only a terminal-shaped stdin -- and it was left undone
  deliberately, to keep a PTY out of the panel.

  What was done instead is `ShellAliases` (`src/shellaliases.{cpp,h}`), a table
  of aliases fed to every shell the panel starts, reachable from *File* >
  *Command Aliases...*. It covers the same ground from the other side and it
  covers more than the guard does: `ls` also drops its multi-column output when
  stdout is not a terminal, which no rc file would have fixed. The defaults are
  `ls` -> `ls -aCF` and `ll` -> `ls -laCF`. Editing the table applies to the
  running shell too -- the definitions are re-sent and a row that was removed is
  `unalias`ed, so the table means the same thing whenever it is edited.
- **Programs drop what they format only for a terminal.** Not the same gap as
  the one above, and one flavour of it is worth separating out: the *format*
  `ls` chooses is decided by `isatty(stdout)` and nothing else, but the *width*
  it formats to comes from `COLUMNS`, which needs no terminal at all. The panel
  therefore exports `COLUMNS` and `LINES`, computed from the scrollback's size
  in its fixed-width font and refreshed whenever the panel is resized or the
  font changes. Measured: 143 columns at 1400 pixels wide, 104 at 900. Anything
  written while a command is running would be read by that command rather than
  by the shell, so an update that falls in that window is held back until the
  sentinel says the shell is at a prompt again.
- **An unfinished multi-line construct wedges the prompt, and interrupting is
  the way out.** A line such as `if true; then` with nothing after it leaves the
  shell waiting for the rest, and what it reads next as part of that construct
  is the sentinel. Nothing then reports the command as done, so the prompt stays
  blocked -- and because it is blocked the closing `fi` cannot be typed either.
  Measured: *Interrupt Command* alone did not help and *Restart Shell* was the
  only way out, at the cost of the session. *Interrupt Command* therefore now
  follows its signal with a fresh sentinel, which a shell back at a prompt
  answers; if a command really was running and survived, its own sentinel is
  still queued ahead of this one, so the cost is a second, harmless report.
- **Ambiguous stdin -- resolved by refusing input.** A line typed while a
  command runs would go down the same pipe and be read by that command rather
  than by the shell. The prompt is therefore read-only while a command is
  running and says so. The cost is that a program cannot be fed from here at
  all; the benefit is that a line meant for the shell is never swallowed.
- **Buffering.** The shell is fine, but its children block-buffer when not on a
  tty, so output appears only at exit -- this is the child's C runtime, not the
  panel, which streams whatever it is given (verified: a slow producer shows its
  lines as they come). `PYTHONUNBUFFERED=1` is set in the shell's environment to
  cover the common case; anything else needs `stdbuf -oL` or its own flag.
- **macOS PATH.** An app launched from the Finder inherits a minimal PATH. The
  shell's environment needs the same fallback logic `findExe()` already applies.

## Portability

- `pushd`/`popd` are bash/zsh builtins, **not** POSIX. `dash`, which is
  `/bin/sh` on Debian and Ubuntu, does not have them -- so the default must be
  `$SHELL` with a bash fallback, not `/bin/sh`.
- csh and tcsh differ in every part of the framing; see the table above. Both
  also greet a pipe with "Inappropriate ioctl for device" and "no job control in
  this shell", which the existing start-up noise filter already drops.
- `cmd.exe` needs `@echo off` sent first or it echoes every line, uses
  `%errorlevel%` and `%CD%` for the sentinel, and emits CRLF. `pushd` onto a UNC
  path silently maps a drive letter.
- Put the shell command in a preference, defaulting to the user's preferred
  shell rather than a fixed name: `$SHELL` on Unix-like systems, falling back to
  `/bin/bash` and then `/bin/sh`. Windows has `%COMSPEC%`, which points at
  `cmd.exe`; the preference lets it be pointed at `pwsh` instead.

## Environment handed to the shell

- **`TERM` set to something minimal**, `dumb` being the conventional value. This
  is deliberate: a program that needs more than a stream of bytes should find out
  from `TERM` and say so -- `less` prints "terminal is not fully functional",
  editors refuse to start -- rather than emitting escape sequences into a
  scrollback that cannot interpret them. Failing clearly beats failing strangely.
- **`PYTHONUNBUFFERED=1`**, so a Python script's output appears as it is produced
  rather than all at once when it exits (see the buffering note above).

## Rejected alternatives

- **Real terminal emulator.** PTY plus VT parsing, scroll regions, alternate
  screen, `SIGWINCH`, process-group signals. Weeks of work. QTermWidget
  (Konsole's core, LGPL) is Unix-only with no Windows backend; `libvterm` (MIT)
  parses escapes but leaves the widget and still needs ConPTY on Windows. Since
  LAMMPS-GUI ships NSIS and DMG packages alongside Linux, this makes the feature
  first-class on two platforms and absent on the third.
- **One `QProcess` per command.** Simpler, but `cd` and every other piece of
  shell state evaporate between commands, which is most of what makes a console
  useful.

## Effort

Roughly 3-4 days, split about evenly between the front end (prompt, history,
completion) and the shell plumbing (persistent process, sentinel
framing, directory tracking, Windows differences), plus the usual docs and
tests.

## Safety note

Given the AI-assistant work (see `ai-assistant-design.md`, which treats all
model-generated content as untrusted), the command line must only ever be filled
by explicit user action. Nothing generated should reach it, or be executed on
its behalf, without the user typing or confirming it.
