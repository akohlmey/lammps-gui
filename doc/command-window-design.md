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

- **No interrupt.** Without a PTY there is no `Ctrl+C`: no terminal to deliver
  `SIGINT` to the foreground process group. `terminate()` kills the *shell* and
  loses the session state. A "restart shell" action is the cheap honest answer;
  a real interrupt is where PTY pressure comes back.
- **Ambiguous stdin.** If a child is reading stdin, the next typed line feeds
  the child rather than the shell, with nothing on screen to say so. Fine for
  `python script.py`, confusing for anything that prompts.
- **Buffering.** The shell is fine, but its children block-buffer when not on a
  tty, so output appears only at exit. Set `PYTHONUNBUFFERED=1` in the shell's
  environment to cover the common case; other tools need their own flag.
- **macOS PATH.** An app launched from the Finder inherits a minimal PATH. The
  shell's environment needs the same fallback logic `findExe()` already applies.

## Portability

- `pushd`/`popd` are bash/zsh builtins, **not** POSIX. `dash`, which is
  `/bin/sh` on Debian and Ubuntu, does not have them -- so the default must be
  `$SHELL` with a bash fallback, not `/bin/sh`.
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
