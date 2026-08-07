# Importing fix ave/* output files into the chart facility

**Status: implemented** on branch `ave-file-import` (off `develop`), except
for the follow-ups listed at the end.  What remains durable here is the
ground truth about the file formats, the reasoning behind the choices that
were made, and the recommendations for a LAMMPS-side YAML effort.

## Goal

Load the output files written by `fix ave/time` (vector mode), `fix
ave/histo`, and `fix ave/correlate` (plus variants) into the existing chart
window via the existing "plot external data" path, with sensible per-format
defaults, so that the common cases are two clicks: a converged histogram or
correlation function with error bars, or a snapshot of a vector at a chosen
timestep.

## Non-goals

- No new chart window type.  Everything reduces to a flat `PlotData`
  (plus optional per-point errors) fed to `ChartWindow::loadData()`.
- No generic YAML parser.  The hand-rolled subset parser is extended only
  for the specific shapes LAMMPS writes.
- Post-processing beyond block averaging (e.g. blocking analysis for
  correlated data, histogram re-binning) stays out; the existing
  post-process/fit facilities apply to the imported curves as usual.

## Ground truth: the file formats

All formats share the skeleton *three `#` header lines, then repeated
blocks of one block-header line followed by N uniform data rows* --
except where noted.  Verified against the LAMMPS sources (2026-08).

| Style | Header comment 1 (default) | Block header | Data row |
|---|---|---|---|
| ave/time (scalar) | `# Time-averaged data for fix ID` | -- (flat table) | `step v1 ... vN` |
| ave/time (vector) | `# Time-averaged data for fix ID` | `step nrows` | `row v1 ... vN` |
| ave/histo | `# Histogrammed data for fix ID` | `step nbins total missing min max` | `bin coord count count/total` |
| ave/correlate | `# Time-correlated data for fix ID` | `step nwindows` | `index timedelta ncount c1 ...` |
| ave/correlate/long | `# Time-correlated data for fix ID` | `# Timestep: N` (a *comment*) | `time c1 ...` |
| ave/chunk | `# Chunk-averaged data for fix ID and group G` | `step nchunk total-count` | `chunk [coords] count v1 ...` |

Facts that shape the design:

- **Field two of every native block header is the row count**
  (`nrows`/`nbins`/`nwindows`/`nchunk`), and the data rows always start
  with a row index counting `1, 2, ... n`.  This is what makes block
  delimiting exact rather than a line-width heuristic, and it matters:
  `fix ave/time` in vector mode with a *single* value writes a block
  header exactly as wide as its data rows.
- **Column names** come from the last default header comment
  (`# Row ...`, `# Bin Coord Count Count/Total`,
  `# Index TimeDelta Ncount c_a*c_a ...`).
- **`title1/2/3` keywords** let users replace all header comments, so
  header-based detection needs a structural fallback and a manual
  override.
- **`overwrite`** truncates to a single (latest) block -- a well-formed
  special case, not an anomaly.
- **Multiple `run` commands** append more blocks to the same open file;
  a second header is not written.  A *new* fix instance re-opens and
  appends headers mid-file, so the parser must re-sync on comment lines
  anywhere.
- **ave/histo/weight** subclasses `FixAveHisto` and only changes the
  binning; the output format is *identical*.  Nothing to do.
- **ave/correlate/long is genuinely different**: only two title lines;
  blocks are delimited by a `# Timestep: N` *comment*; there are no
  index/ncount columns; the first column is already *simulation time*
  (`t[i]*dt*nevery`) on a non-uniform (multiple-tau) grid.  The
  correlator accumulates over the whole run, so each block is a
  successive snapshot of one improving estimate -- **averaging across
  blocks is statistically wrong there; the last block is the answer.**
  The same caveat applies to plain ave/correlate with `ave running`.
- **ave/time already writes YAML** (suffix-triggered on the `file`
  argument, both modes); ave/histo and ave/correlate do not.  The
  vector-mode YAML shape is `keywords: [...]` then `data:` as a map of
  `step:` keys each holding a list of flow-style rows -- and those rows
  carry *no* row index, unlike the native format.

## What was fixed along the way

Before this, a native multi-block file **silently parsed wrong**: the
whitespace parser locked its column count onto the first numeric line --
the block header -- and dropped every data row as ragged.  The user got a
plausible "Number-of-rows vs. TimeStep" plot with no error.  An ave/time
vector-mode YAML file parsed with all blocks silently concatenated into one
table.  Both now go through the block path instead.

ave/time in *scalar* mode was and remains a flat table handled by the
existing parsers.

## Data model (`src/plotblockdata.{h,cpp}`)

```
enum class AveFileKind { Unknown, AveTimeVector, AveHisto,
                         AveCorrelate, AveCorrelateLong };

struct PlotDataBlock {
    long long step;               // block timestep (correlate/long: from comment)
    std::vector<double> scalars;  // per-block extras (histo: total missing min max)
    PlotData rows;                // the block's table, named columns
};

struct PlotBlockData {
    AveFileKind kind;             // detected, user-overridable in the dialog
    QString fixId;                // from the default header, if present
    QStringList scalarNames;      // names for PlotDataBlock::scalars
    std::vector<PlotDataBlock> blocks;
};
```

`parseAveBlocks()` handles all native kinds with one scanner: a block
opens on a numeric header line and closes after its announced row count
(or early, if a row of unexpected width shows up -- an interrupted run),
and a `# Timestep: N` comment opens one for correlate/long.  Any comment
line closes the open block and starts a fresh header run, which is what
makes a file a second fix instance appended its headers to parse.

The scanner needs **no per-style tables**: column names are taken from
whichever header comment is as wide as a data row, and the per-block
scalar names from the one as wide as a block header.  Both fall back to
generic names when `title1/2/3` replaced the headers.

`parseAveBlocksYaml()` handles the vector-mode `step:`-keyed map shape and
synthesizes the leading `Row` column, so that the YAML and native readings
of the same data produce identical tables.

`looksLikeAveBlocks()` insists on *structure*, not just a matching header
comment: the row index must count `1, 2, ... n` in every block.  A flat
ave/time scalar file, whose first header comment reads exactly like a
vector-mode one, is therefore never mistaken for a block file.

## Reduction modes

**A. Single block** (`singleBlock()`).  Default for ave/time vector,
correlate/long, and running-average correlate.

**B. Block-range average with error bars** (`averageBlocks()`).  Default
for ave/histo and non-running ave/correlate.  Default range is *all*
blocks -- equilibration trimming is a deliberate act, not a default.
Only blocks whose shape matches the *last selected* block participate;
mismatching blocks are dropped and counted in the visible status line.
Deviations are summed in a second pass, so a small spread on top of a
large mean keeps its digits.

Error type defaults to the **standard deviation**, named as such in the
dialog, with the standard error of the mean, the min/max range of the
blocks, and "none" available.  None of them is the whole truth --
successive Nfreq windows are not strictly independent, so the standard
error is a lower bound -- which is a reason to name what is drawn rather
than to pick for the user.  The min/max range makes no statistical claim at
all; it is also the one type whose bars are **asymmetric**, since the
extremes of a set of blocks need not straddle their mean evenly.

`aveImportDefaults()` picks the mode and the preselected columns per kind,
and detects the running-average case from an `Ncount` column that grows
from block to block.

## Error-bar rendering

`PlotSeries` carries `QList<double> yerr` (parallel to `points`; empty =
none) plus `yerrLo` for the lower half of asymmetric bars (empty = the bar
reaches `yerr` in both directions), and `PlotErrors` mirrors that split
column-wise as `upper`/`lower`.  `PlotWidget` draws capped vertical bars
before any curve, so they stay behind the data.

The bars have a **color and line width of their own** (`errColor`,
`errWidth`), not a faded copy of the series color: a bar that is a shade of
the curve it crosses is unreadable exactly where it matters.  An invalid
`errColor` means "use the configured default", which is what lets a color
changed in the preferences reach charts that already exist; the *Chart
Style* dialog sets the style per chart, for every series of that chart that
carries bars, and the charts preferences tab holds the defaults.

Cached column bounds cover the bar ends so they fit inside the plot.
Smoothing operates on the values alone.  In points-only display mode the
bars move to the visible marker series, and only one of the two ever
carries them.  Chart export writes a `<name>-err` column, or `-errlo` plus
`-errhi` for asymmetric bars; the importers stay oblivious and read them
back as ordinary columns.

## Import dialog

`PlotDataDialog` stays the single entry point; a second constructor takes a
`PlotBlockData` and adds the "Data blocks" group above the familiar column
grid, which operates on the *reduced* table and is rebuilt when the
reduction changes -- preserving column roles, edited names, and derived
columns (re-evaluated against the new table).

The detected format sits in a combo the user can correct; correcting it
moves the preselected reduction and columns and never reinterprets the
data.

Menu wiring: the existing *File > Plot Data File...* action detects block
files itself, so the command window's `plot` and the `-c` command-line flag
get it for free, and flat files take exactly the path they took before.

## X axis

The files only contain the step.  In modes A and B the x axis is a
*within-block* column (Row/Coord/TimeDelta/Time), so no step-to-time
conversion is needed:

1. Any column may be chosen as x (the dialog already does this).
2. The "compute derived column" section converts a column in timesteps to
   one in time units (`TimeDelta*0.001`).  Not valid under
   variable-timestep runs.
3. ave/correlate/long needs nothing: its x column *is* simulation time.

A dedicated Delta-t field only becomes relevant for the deferred
across-block evolution mode, whose x axis is the block timestep.

## Upstream YAML recommendations (for a LAMMPS-side effort)

Native-format parsing is required regardless (existing files, old
versions), so YAML output for ave/histo/ave/correlate is *orthogonal*,
not a prerequisite.  If it is added upstream, these choices would make
import trivial and self-describing (and are worth retrofitting into
ave/time's YAML where backward-compatible):

1. **Provenance keys.**  The current ave/time YAML carries *no*
   indication of what wrote it (the native title1 does!).  Emit e.g.
   `style: 'ave/histo'`, `fix-id: 'myhisto'`, `mode: 'vector'` before
   `keywords:` -- detection becomes exact, no heuristics, no title1
   fragility.
2. **Complete keywords.**  Vector-mode `keywords:` omits the implicit
   row/bin/index column, and so do the rows; the list should name every
   column of a row and the rows should carry it.
3. **Per-block metadata as keys, not positions.**  Blocks as
   `data:` -> `step:` maps (as ave/time vector already does), with
   histo's stats as a nested map (`stats: {total: ..., missing: ...,
   min: ..., max: ...}`) instead of a positional header line.
4. **Emit `units:`, `dt:` (timestep size), and ideally a per-block
   `time:`** -- this is what makes a simulation-time x axis work without
   asking the user for Delta-t.
5. **Keep the ave/time conventions**: suffix-triggered activation
   (`.yaml`/`.yml` on the `file` argument), one `---`/`...` document
   per run section, quoted keywords, flow-style rows.
6. **Drop the trailing comma** LAMMPS writes inside flow sequences
   (`..., 3.14, ]`); the GUI tolerates it but strict YAML parsers may
   not.

## Deferred follow-ups

- **Across-block evolution** (x = block timestep, y = one chosen cell or
  one of the per-block scalars), with the step-to-time Delta-t field it
  needs.
- **fix ave/chunk defaults.**  It already parses and imports through the
  generic path without losing data; what is missing is a kind of its own
  with preselected columns (first coord column as x) and reduction.
- **Overlay a few blocks** in mode A: multi-select and load the extras
  through the existing overlay-series mechanism (histogram evolution as
  3-4 overlaid snapshots).
- **Shaded error-band style**, nicer than bars for correlation functions.
- **Generic "this column is the error of that column" role** in the
  dialog for arbitrary flat files -- the plumbing for it already exists.
- **Step/bar histogram drawing style.**
