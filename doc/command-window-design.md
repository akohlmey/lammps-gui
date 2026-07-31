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

| | exit status | directory | prompt off | echo off |
|---|---|---|---|---|
| POSIX (`sh`, `bash`, `zsh`, `dash`) | `$?` | `$PWD` | `PS1=''` | `bash --noediting` |
| csh, tcsh | `$status` | `$cwd` | `set prompt = ""` | `unset edit` |
| `cmd.exe` | `%errorlevel%` | `%CD%` | -- | `@echo off` |

Two of these are not obvious. In csh, `echo` is a builtin that sets a status of
its own, so the status being reported has to be saved into a variable before the
first `echo` of the sentinel runs. And csh's line editor, not any argument, is
what echoes the command back on a pipe; `unset edit` is the off switch that
`--noediting` is for bash. Getting either wrong looks the same from the outside:
every command's output appears one command late, with the line echoed in front
of it.

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
- history: `QStringList` with up/down, persisted through `QSettings`; `Ctrl+R`
  reverse search is a nice-to-have
- filename completion: `QCompleter` over a `QFileSystemModel`, rooted at the
  tracked working directory
- command completion: a cached scan of `PATH`, refreshed on demand
- `CodeEditor` already runs several `QCompleter` instances -- copy that pattern

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
