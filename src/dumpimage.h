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

#ifndef DUMPIMAGE_H
#define DUMPIMAGE_H

#include "imageviewer_internal.h" // ImageInfo, RegionInfo, region/color-style enums

#include <QColor>
#include <QList>
#include <QPair>
#include <QString>
#include <map>
#include <string>

/**
 * @brief Pre-resolved inputs for assembling a LAMMPS `dump image` command
 *
 * `ImageViewer::createImage()` populates this struct from its widget state and
 * from a set of LAMMPS library queries (atom/setting/global data) before the
 * image is rendered.  All LAMMPS-derived values are captured here as plain
 * data so that buildDumpImageCommand() can run as a pure function -- it never
 * touches the GUI or a live LAMMPS instance and is therefore unit-testable on
 * its own.
 *
 * The `computes`/`fixes`/`regions` maps hold non-owning pointers to ImageInfo
 * and RegionInfo objects owned elsewhere (by ImageViewer at runtime, or by the
 * test fixture in unit tests); they only need to outlive the call.
 */
struct DumpImageParams {
    // ---- output target ----
    QString group;    ///< atom group to render
    QString dumpfile; ///< full path of the temporary `.ppm` output file

    // ---- atom appearance ----
    bool atomcustom;   ///< use custom atom color/diameter properties
    bool useelements;  ///< element data available (color/diameter by element)
    bool usediameter;  ///< per-atom diameter (radius) attribute available
    bool usesigma;     ///< Lennard-Jones sigma usable as atom radius
    bool showatoms;    ///< draw atoms
    QString atomcolor; ///< custom atom color property
    QString atomdiam;  ///< custom atom diameter property
    double vdwfactor;  ///< van der Waals radius scaling factor
    double atomSize;   ///< explicit atom size (radius)
    QString elements;  ///< pre-built `element <X> <Y> ...` argument string
    QString adiams;    ///< pre-built `adiam <i> <d> ...` argument string

    // ---- shaped particles ----
    bool showbodies;        ///< draw body particles
    int body_flag;          ///< LAMMPS body_flag setting
    QString bodycolor;      ///< body color property
    double bodydiam;        ///< body diameter
    int bodyflag;           ///< body draw flag (triangle, cylinder, or both)
    bool showlines;         ///< draw line particles
    int line_flag;          ///< LAMMPS line_flag setting
    QString linecolor;      ///< line color property
    double linediam;        ///< line diameter
    bool showtris;          ///< draw triangle particles
    int tri_flag;           ///< LAMMPS tri_flag setting
    QString tricolor;       ///< triangle color property
    int triflag;            ///< triangle draw flag (triangle, cylinder, or both)
    double tridiam;         ///< triangle diameter
    bool showellipsoids;    ///< draw ellipsoid particles
    int ellipsoid_flag;     ///< LAMMPS ellipsoid_flag setting
    QString ellipsoidcolor; ///< ellipsoid color property
    int ellipsoidflag;      ///< ellipsoid draw flag (triangle or cylinder)
    int ellipsoidlevel;     ///< ellipsoid refinement level
    double ellipsoiddiam;   ///< ellipsoid diameter

    // ---- bonds ----
    int nbondtypes;    ///< number of bond types
    int bond_flag;     ///< LAMMPS bond_flag setting
    bool showbonds;    ///< draw bonds
    QString bondcolor; ///< bond color property (or "c_<id>" when bondbyvalue)
    bool bondbyvalue;  ///< color bonds by a per-bond compute value (emit bmap)
    QString bonddiam;  ///< bond diameter property
    bool autobond;     ///< derive bonds from a distance cutoff
    bool haspairstyle; ///< a pair style other than "none" is defined
    double bondcutoff; ///< autobond distance cutoff

    // ---- view / image ----
    int xsize;          ///< rendered image width in pixels
    int ysize;          ///< rendered image height in pixels
    double zoom;        ///< zoom level
    double shinyfactor; ///< shininess / specular factor
    bool antialias;     ///< enable full-scene antialiasing
    int dimension;      ///< system dimension (2 or 3)
    int hrot;           ///< horizontal rotation angle
    int vrot;           ///< vertical rotation angle
    bool usessao;       ///< enable screen-space ambient occlusion
    double ssaoval;     ///< SSAO strength

    // ---- box / axes ----
    bool showbox;      ///< draw simulation box
    double boxdiam;    ///< box edge diameter
    bool showsubbox;   ///< draw subdomain boxes
    double subboxdiam; ///< subbox edge diameter
    bool showaxes;     ///< draw coordinate axes
    QString axesloc;   ///< axes location
    double axeslen;    ///< axes length
    double axesdiam;   ///< axes diameter

    // ---- view center ----
    double xcenter; ///< view center x coordinate
    double ycenter; ///< view center y coordinate
    double zcenter; ///< view center z coordinate

    // ---- colors / lighting ----
    int ntypes;                               ///< number of atom types
    QList<QPair<QString, QColor>> color_list; ///< per-type atom color table
    QString boxcolor;                         ///< box / subbox color
    QString backcolor;                        ///< lower background color
    QString backcolor2;                       ///< upper background color
    bool usegradient;                         ///< draw a vertical gradient
    double axestrans;                         ///< axes transparency
    double boxtrans;                          ///< box / subbox transparency
    double atomtrans;                         ///< atom transparency
    double bondtrans;                         ///< bond transparency
    double ambientlight;                      ///< ambient light setting
    double keylight;                          ///< key light setting
    double filllight;                         ///< fill light setting
    double backlight;                         ///< back light setting
    int version;                              ///< LAMMPS version (date) id

    // ---- color maps (atoms / bonds) ----
    QString colormap;     ///< name of the selected atom color map
    QString mapmin;       ///< minimum-value choice for the atom color map
    QString mapmax;       ///< maximum-value choice for the atom color map
    bool revcolormap;     ///< reverse (mirror) the atom color map
    QString bondcolormap; ///< name of the selected bond color map
    QString bondmapmin;   ///< minimum-value choice for the bond color map
    QString bondmapmax;   ///< maximum-value choice for the bond color map
    bool revbondcolormap; ///< reverse (mirror) the bond color map

    // ---- regions / fixes / computes ----
    std::map<std::string, ImageInfo *> computes; ///< per-compute graphics settings
    std::map<std::string, ImageInfo *> fixes;    ///< per-fix graphics settings
    std::map<std::string, RegionInfo *> regions; ///< per-region display settings
};

/**
 * @brief The two argument strings of an assembled `dump image` command
 *
 * The render options (everything that follows `... image <N> <file>`) and the
 * `dump_modify` options (colors, color maps, lighting, ...) are kept separate so
 * the caller can compose either an explicit `dump`/`dump_modify` pair or a
 * one-shot `write_dump` (via toWriteDumpCommand()), and so each builder step can
 * append to whichever string is logical. Both strings begin with a leading space.
 */
struct DumpImageCommand {
    QString dumpargs;   ///< render options after `image <N> <file>`
    QString modifyargs; ///< options for `dump_modify <id>` / after `modify`
    bool dofixes;       ///< a fix/compute graphic is active (write_dump omits `noinit`)
};

/**
 * @brief Assemble the render and dump_modify argument strings for a dump image
 * @param p Fully populated parameter struct (no GUI or LAMMPS access required)
 * @return The two argument strings (see DumpImageCommand)
 *
 * Pure function: depends only on @p p, performs no I/O and no LAMMPS calls, and
 * is therefore exercised directly by the unit tests.
 */
DumpImageCommand buildDumpImageCommand(const DumpImageParams &p);

/**
 * @brief Compose a one-shot `write_dump ... image ...` command from the pieces
 * @param c     the two argument strings from buildDumpImageCommand()
 * @param group atom group to render
 * @param file  output image file name
 * @return A complete `write_dump` command string
 */
QString toWriteDumpCommand(const DumpImageCommand &c, const QString &group, const QString &file);

#endif

// Local Variables:
// c-basic-offset: 4
// End:
