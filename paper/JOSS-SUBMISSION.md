# JOSS submission checklist (LAMMPS-GUI)

Working notes for submitting the LAMMPS-GUI software paper to the
[Journal of Open Source Software](https://joss.theoj.org/). JOSS reviews the
*software*; the paper (`paper.md`) is only a short summary that points to it.
This file is a developer note and is not part of the published paper.

## Files in this folder

- `paper.md` -- JOSS paper (Pandoc Markdown + YAML header). Single author.
- `paper.bib` -- references for `paper.md` (BibTeX, DOIs where available).
- `images/lammps-gui-rhodo.png` -- Figure 1, referenced from `paper.md`.
- The abandoned long-form CPC draft (`lammps-gui-library.tex` / `.bib`) is
  **not** on this branch; it is archived on the `paper` branch as source
  material in case it is ever needed again.

To preview the paper locally (optional; the JOSS bot builds the real PDF):
`pandoc --citeproc -f markdown -t plain paper.md` -- should exit 0 with all
`@key` citations resolved.

## Pre-submission requirements

Author-side gates JOSS expects to be satisfied before review:

- [x] Open source, OSI-approved license -- GPLv2 (`LICENSE`).
- [x] Public version-controlled repository -- https://github.com/akohlmey/lammps-gui
- [x] Substantial scholarly effort (not a thin wrapper / trivial utility).
- [x] `paper.md` with Summary and Statement of need; `paper.bib`.
- [x] User documentation: install + usage -- `README.md` and the online
      manual at https://lammps-gui.lammps.org/
- [x] Example usage -- documented workflow + integrated LAMMPS tutorials.
- [x] Automated tests -- CTest suite (unit / command-line / GUI) run in CI.
- [x] Community guidelines -- `.github/CONTRIBUTING.md`,
      `.github/CODE_OF_CONDUCT.md`, issue/PR templates.
- [ ] **Tagged release** matching the submitted state (e.g. the current
      v2.1.x). Create the tag before/at submission.
- [ ] **Archive DOI** (Zenodo or similar) for the tagged release. JOSS asks
      for this during review; minting it early does no harm.
- [x] `CITATION.cff` at the repo root (GitHub shows a "Cite this
      repository" button). Update its `version`/`date-released` to match the
      release tag, and add a `preferred-citation` JOSS DOI once accepted.

## Submission steps

1. Push `paper.md` + `paper.bib` to the default branch (or a branch you name
   in the submission). Confirm the figure path resolves from `paper/`.
2. Cut a release tag and (optionally) archive it to get a DOI.
3. Submit the repository URL at https://joss.theoj.org/papers/new and point
   the form at the branch/path containing `paper.md`.
4. The Editorialbot compiles the PDF from `paper.md`; check the generated
   proof in the review issue and fix any Markdown/BibTeX warnings it reports.
5. Respond to the assigned reviewers in the GitHub review issue. Most fixes
   land in the software/docs, not the paper.

## What reviewers check (for reference)

Reviewers work through a checklist covering: a working install on their
platform, that the documented functionality actually works, the presence of
automated tests, community/contribution guidelines, and -- for the paper --
a clear Summary, an explicit Statement of need, references to the state of
the field, and correct/complete citations. Keeping the items above green is
the simplest way to make the review go quickly.
