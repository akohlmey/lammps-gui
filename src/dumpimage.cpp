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

#include "dumpimage.h"

#include <QRegularExpression>

// LAMMPS dump_image built-in defaults. The builder emits a color, color map,
// light, or transparency setting only when it differs from these, so the
// generated command stays compact without changing the rendered image: the
// GUI's own defaults were reconciled to match LAMMPS (the per-type color table
// in deftypecolors mirrors the LAMMPS color database). Deliberate GUI
// divergences (the backcolor2 gradient and shiny) are emitted unconditionally.
constexpr double DEF_TRANS     = 1.0; // fully opaque
constexpr double DEF_AMBIENT   = 0.0;
constexpr double DEF_KEYLIGHT  = 0.9;
constexpr double DEF_FILLLIGHT = 0.45;
constexpr double DEF_BACKLIGHT = 0.9;
const QString DEF_BOXCOLOR     = QStringLiteral("gold");
const QString DEF_BACKCOLOR    = QStringLiteral("black");

// Append region visualization arguments to the dump-image command.
static void appendRegionArgs(QString &cmd, const DumpImageParams &p)
{
    for (const auto &reg : p.regions) {
        if (reg.second->enabled) {
            QString id(reg.first.c_str());
            QString color(reg.second->color.c_str());
            switch (reg.second->style) {
                case FRAME:
                    cmd += " region " + id + blank + color;
                    cmd += " frame " + QString::number(reg.second->diameter);
                    if (p.version > 20260330)
                        cmd += " hull_points " + QString::number(reg.second->npoints);
                    break;
                case FILLED:
                    cmd += " region " + id + blank + color + " filled";
                    if (p.version > 20260330)
                        cmd += " hull_points " + QString::number(reg.second->npoints);
                    break;
                case TRANSPARENT:
                    cmd += " region " + id + blank + color;
                    cmd += " transparent " + QString::number(reg.second->opacity);
                    if (p.version > 20260330)
                        cmd += " hull_points " + QString::number(reg.second->npoints);
                    break;
                case POINTS:
                default:
                    cmd += " region " + id + blank + color;
                    cmd += " points " + QString::number(reg.second->npoints);
                    cmd += blank + QString::number(reg.second->diameter);
                    break;
            }
            cmd += blank;
        }
    }
}

// Append the per-fix/compute draw styles. Returns true if any fix or compute is
// active (the caller suppresses "noinit" in that case).
static bool appendFixComputeStyles(QString &cmd, const DumpImageParams &p)
{
    bool dofixes = false;
    for (const auto &comp : p.computes) {
        if (comp.second->enabled) {
            dofixes = true;
            QString id(comp.first.c_str());
            switch (comp.second->colorstyle) {
                case TYPE:
                    cmd += " compute " + id + blank + "type ";
                    break;
                case ELEMENT:
                    cmd += " compute " + id + blank + "element ";
                    break;
                case CONSTANT: // FALLTHROUGH
                default:
                    cmd += " compute " + id + blank + "const ";
                    break;
            }
            cmd += QString::number(comp.second->flag1) + blank +
                   QString::number(comp.second->flag2) + blank;
        }
    }
    for (const auto &fix : p.fixes) {
        if (fix.second->enabled) {
            dofixes = true;
            QString id(fix.first.c_str());
            switch (fix.second->colorstyle) {
                case TYPE:
                    cmd += " fix " + id + blank + "type ";
                    break;
                case ELEMENT:
                    cmd += " fix " + id + blank + "element ";
                    break;
                case CONSTANT: // FALLTHROUGH
                default:
                    cmd += " fix " + id + blank + "const ";
                    break;
            }
            cmd += QString::number(fix.second->flag1) + blank + QString::number(fix.second->flag2) +
                   blank;
        }
    }
    return dofixes;
}

// Append the definition of a color map. @p kw is the dump_modify keyword
// ("amap" for atoms, "bmap" for bonds); @p pfx prefixes the custom color-stop
// names so an atom map and a bond map with different gradients do not collide in
// the global color namespace (atoms use "map", bonds use "bm").
static void appendColorMapArgs(QString &cmd, const QString &kw, const QString &colormap,
                               const QString &mapmin, const QString &mapmax, const QString &pfx)
{
    const QString mmin = (mapmin == "auto") ? QStringLiteral("min") : mapmin;
    const QString mmax = (mapmax == "auto") ? QStringLiteral("max") : mapmax;
    auto M             = [&](int n) {
        return pfx + QString::number(n);
    };
    if (colormap == "RWB") {
        cmd += " color " + M(1) + " 0.459 0.055 0.075";
        cmd += " color " + M(2) + " 0.000 0.227 0.427";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "5 min " + M(1) + " 0.1 " + M(1) + " 0.5 white 0.9 " + M(2) + " max " + M(2);
    } else if (colormap == "PWT") {
        cmd += " color " + M(1) + " 0.286 0.114 0.553";
        cmd += " color " + M(2) + " 0.000 0.255 0.267";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "5 min " + M(1) + " 0.1 " + M(1) + " 0.5 white 0.9 " + M(2) + " max " + M(2);
    } else if (colormap == "BGR") {
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "5 min blue 0.05 blue 0.5 green 0.95 red max red";
    } else if (colormap == "BWG") {
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "5 min blue 0.1 blue 0.5 white 0.9 green max green";
    } else if (colormap == "Grayscale") {
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "2 min black max white";
    } else if (colormap == "Rainbow") {
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "6 min red 0.25 yellow 0.45 green 0.65 cyan 0.85 blue max purple";
    } else if (colormap == "Sequential") {
        cmd += " color " + M(1) + " 0.808 0.808 0.808";
        cmd += " color " + M(2) + " 0.647 0.349 0.667";
        cmd += " color " + M(3) + " 0.349 0.659 0.612";
        cmd += " color " + M(4) + " 0.941 0.772 0.443";
        cmd += " color " + M(5) + " 0.878 0.169 0.208";
        cmd += " color " + M(6) + " 0.031 0.165 0.329";
        cmd += QString(" %1 %2 %3 sa 1.0 ").arg(kw, mmin, mmax);
        cmd += "6 " + M(1) + " " + M(2) + " " + M(3) + " " + M(4) + " " + M(5) + " " + M(6);
    } else if (colormap == "Landscape") {
        cmd += " color " + M(0) + " 0.145 0.400 0.463";
        cmd += " color " + M(1) + " 0.392 0.867 0.588";
        cmd += " color " + M(2) + " 0.572 0.192 0.141";
        cmd += " color " + M(3) + " 0.392 0.831 0.992";
        cmd += " color " + M(4) + " 0.020 0.431 0.071";
        cmd += " color " + M(5) + " 0.992 0.349 0.145";
        cmd += " color " + M(6) + " 0.275 0.953 0.243";
        cmd += " color " + M(7) + " 0.729 0.525 0.361";
        cmd += " color " + M(8) + " 0.780 0.867 0.529";
        cmd += " color " + M(9) + " 0.243 0.298 0.078";
        cmd += QString(" %1 %2 %3 sa 1.0 ").arg(kw, mmin, mmax);
        cmd += "10 " + M(0) + " " + M(1) + " " + M(2) + " " + M(3) + " " + M(4) + " " + M(5) + " " +
               M(6) + " " + M(7) + " " + M(8) + " " + M(9);
    } else if (colormap == "Basic") {
        cmd += QString(" %1 %2 %3 sa 1.0 ").arg(kw, mmin, mmax);
        cmd += "10 red cyan green black magenta blue yellow purple white orange";
    } else if (colormap == "Teal") {
        cmd += " color " + M(1) + " 0.071 0.153 0.251";
        cmd += " color " + M(2) + " 0.106 0.282 0.369";
        cmd += " color " + M(3) + " 0.337 0.545 0.529";
        cmd += " color " + M(4) + " 0.710 0.820 0.682";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "4 min " + M(1) + " 0.25 " + M(2) + " 0.5 " + M(3) + " max " + M(4);
    } else if (colormap == "Viridis") {
        cmd += " color " + M(1) + " 0.282 0.129 0.451";
        cmd += " color " + M(2) + " 0.435 0.435 0.556";
        cmd += " color " + M(3) + " 0.161 0.686 0.498";
        cmd += " color " + M(4) + " 0.741 0.875 0.149";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "4 min " + M(1) + " 0.333 " + M(2) + " 0.667 " + M(3) + " max " + M(4);
    } else if (colormap == "Inferno") {
        cmd += " color " + M(1) + " 0.032 0.032 0.048";
        cmd += " color " + M(2) + " 0.318 0.071 0.486";
        cmd += " color " + M(3) + " 0.718 0.216 0.475";
        cmd += " color " + M(4) + " 0.988 0.537 0.380";
        cmd += " color " + M(5) + " 0.988 0.992 0.749";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd +=
            "5 min " + M(1) + " 0.25 " + M(2) + " 0.5 " + M(3) + " 0.75 " + M(4) + " max " + M(5);
    } else if (colormap == "Plasma") {
        cmd += " color " + M(1) + " 0.051 0.031 0.529";
        cmd += " color " + M(2) + " 0.612 0.090 0.620";
        cmd += " color " + M(3) + " 0.929 0.475 0.325";
        cmd += " color " + M(4) + " 0.941 0.976 0.129";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "4 min " + M(1) + " 0.333 " + M(2) + " 0.667 " + M(3) + " max " + M(4);
    } else { // default is "BWR"
        cmd += " color " + M(1) + " 0.000 0.227 0.427";
        cmd += " color " + M(2) + " 0.459 0.055 0.075";
        cmd += QString(" %1 %2 %3 cf 0.0 ").arg(kw, mmin, mmax);
        cmd += "3 min " + M(1) + " 0.5 white max " + M(2);
    }
}

// Append the per-fix/compute color and transparency overrides.
static void appendFixComputeColors(QString &cmd, const DumpImageParams &p)
{
    for (const auto &comp : p.computes) {
        if (comp.second->enabled) {
            QString id(comp.first.c_str());
            QString color(comp.second->color.c_str());
            cmd += " ccolor " + id + blank + color;
            cmd += " ctrans " + id + blank + QString::number(comp.second->opacity);
            cmd += blank;
        }
    }
    for (const auto &fix : p.fixes) {
        if (fix.second->enabled) {
            QString id(fix.first.c_str());
            QString color(fix.second->color.c_str());
            cmd += " fcolor " + id + blank + color;
            cmd += " ftrans " + id + blank + QString::number(fix.second->opacity);
            cmd += blank;
        }
    }
}

DumpImageCommand buildDumpImageCommand(const DumpImageParams &p)
{
    QString d; // render options (after "image <N> <file>")
    QString m; // dump_modify options

    const int hhrot   = (p.hrot > 180) ? 360 - p.hrot : p.hrot;
    const bool do_vdw = p.vdwfactor > VDW_CUT;

    // atom color
    const QString atomColorTok = (!p.atomcustom && p.useelements) ? QStringLiteral("element")
                                                                  : p.atomcolor;
    d += blank + atomColorTok;

    // atom diameter
    if (!p.atomcustom) {
        if (p.usediameter && do_vdw)
            d += blank + "diameter";
        else
            d += " type";
    } else {
        if ((p.atomdiam == "diameter") && p.usediameter && do_vdw)
            d += blank + "diameter";
        else
            d += " type";
    }

    if (!p.showatoms) d += " atom no";
    if (p.showbodies && (p.body_flag == 1)) {
        d += QString(" body %1 %2 %3").arg(p.bodycolor).arg(p.bodydiam).arg(p.bodyflag);
    } else if (p.showlines && (p.line_flag == 1))
        d += QString(" line %1 %2").arg(p.linecolor).arg(p.linediam);
    else if (p.showtris && (p.tri_flag == 1))
        d += QString(" tri %1 %2 %3").arg(p.tricolor).arg(p.triflag).arg(p.tridiam);
    else if (p.showellipsoids && (p.ellipsoid_flag == 1)) {
        d += QString(" ellipsoid %1 %2 %3 %4")
                 .arg(p.ellipsoidcolor)
                 .arg(p.ellipsoidflag)
                 .arg(p.ellipsoidlevel)
                 .arg(p.ellipsoiddiam);
    }
    d += QString(" size %1 %2").arg(p.xsize).arg(p.ysize);
    d += QString(" zoom %1").arg(p.zoom);
    d += QString(" shiny %1 ").arg(p.shinyfactor);
    d += QString(" fsaa %1").arg(p.antialias ? "yes" : "no");
    if (p.nbondtypes > 0) {
        if (do_vdw || !p.showbonds)
            d += " bond none none ";
        else
            d += QString(" bond %1 %2 ").arg(p.bondcolor).arg(p.bonddiam);
    }
    if (p.dimension == 3) {
        d += QString(" view %1 %2").arg(hhrot).arg(p.vrot);
    }
    if (p.usessao) d += QString(" ssao yes 453983 %1").arg(p.ssaoval);
    if (p.showbox)
        d += QString(" box yes %1").arg(p.boxdiam);
    else
        d += " box no 0.0";
    // subbox and axes default to "no" in LAMMPS, so emit them only when shown
    if (p.showsubbox) d += QString(" subbox yes %1").arg(p.subboxdiam);

    if (p.showaxes) d += QString(" axes %1 %2 %3").arg(p.axesloc).arg(p.axeslen).arg(p.axesdiam);

    if (p.autobond && p.haspairstyle) {
        // use custom bond diameter value, if present
        QRegularExpression validnum(R"((^\d+\.?\d*|^\d*\.?\d+))");
        auto match = validnum.match(p.bonddiam);
        if (match.hasMatch()) {
            d += blank + "autobond" + blank + QString::number(p.bondcutoff) + blank + p.bonddiam;
        } else {
            d += blank + "autobond" + blank + QString::number(p.bondcutoff) + " 0.5";
        }
    }

    appendRegionArgs(d, p);

    const bool dofixes = appendFixComputeStyles(d, p);

    // center defaults to the box center "s 0.5 0.5 0.5"; emit only when moved
    if ((p.xcenter != 0.5) || (p.ycenter != 0.5) || (p.zcenter != 0.5))
        d += QString(" center s %1 %2 %3").arg(p.xcenter).arg(p.ycenter).arg(p.zcenter);

    // ---- dump_modify options ----

    // change global color definitions first so they apply everywhere, but emit
    // only those that differ from the LAMMPS built-in defaults: deftypecolors
    // mirrors the LAMMPS color database, so an unmodified table emits nothing
    const int numcolors    = p.color_list.size();
    const int ndefcolors   = deftypecolors.size();
    const bool prunecolors = (numcolors == ndefcolors);

    for (int i = 0; i < numcolors; ++i) {
        if (prunecolors && (p.color_list[i].first == deftypecolors[i].first) &&
            (p.color_list[i].second == deftypecolors[i].second))
            continue;
        m += QString(" color %1 %2 %3 %4")
                 .arg(p.color_list[i].first)
                 .arg(p.color_list[i].second.redF())
                 .arg(p.color_list[i].second.greenF())
                 .arg(p.color_list[i].second.blueF());
    }
    // assign type colors only where the name differs from the default LAMMPS
    // assignment for that type (type i -> deftypecolors[(i - 1) % ndefcolors])
    for (int i = 1; i <= p.ntypes; ++i) {
        const QString curname = p.color_list[(i - 1) % numcolors].first;
        const QString defname = deftypecolors[(i - 1) % ndefcolors].first;
        if (curname == defname) continue;
        m += QString(" acolor %1 %2").arg(i).arg(curname);
    }

    if (p.boxcolor != DEF_BOXCOLOR) m += " boxcolor " + p.boxcolor;

    // background: with the gradient enabled (the GUI default, a deliberate
    // divergence from the solid LAMMPS default) the backcolor/backcolor2 pair is
    // emitted together. With the gradient off the background is solid and only
    // backcolor is emitted, and only when it differs from the LAMMPS default, so
    // backcolor2 is never emitted without backcolor.
    if (p.usegradient) {
        m += " backcolor " + p.backcolor;
        m += " backcolor2 " + p.backcolor2;
    } else if (p.backcolor != DEF_BACKCOLOR) {
        m += " backcolor " + p.backcolor;
    }

    if (p.axestrans != DEF_TRANS) m += QString(" axestrans %1").arg(p.axestrans);
    if (p.boxtrans != DEF_TRANS) m += QString(" boxtrans %1").arg(p.boxtrans);
    if (p.atomtrans != DEF_TRANS) m += QString(" atrans * %1").arg(p.atomtrans);
    if ((p.bond_flag == 1) && (p.atomtrans != DEF_TRANS))
        m += QString(" btrans * %1").arg(p.atomtrans);

    const bool lightsdefault = (p.ambientlight == DEF_AMBIENT) && (p.keylight == DEF_KEYLIGHT) &&
                               (p.filllight == DEF_FILLLIGHT) && (p.backlight == DEF_BACKLIGHT);
    if ((p.version > 20260330) && !lightsdefault)
        m += QString(" lights %1 %2 %3 %4")
                 .arg(p.ambientlight)
                 .arg(p.keylight)
                 .arg(p.filllight)
                 .arg(p.backlight);

    if (p.useelements) m += blank + p.elements + blank + p.adiams + blank;
    if (p.usesigma) m += blank + p.adiams + blank;
    if (!p.useelements && !p.usesigma && (p.atomSize != 1.0)) m += blank + p.adiams + blank;

    // apply the selected color map only when atoms are colored by a per-atom
    // value; for type/element coloring the map is unused, so omit it
    const bool atomByValue = (atomColorTok != "type") && (atomColorTok != "element");
    if (atomByValue) appendColorMapArgs(m, "amap", p.colormap, p.mapmin, p.mapmax, "map");
    if (p.bondbyvalue)
        appendColorMapArgs(m, "bmap", p.bondcolormap, p.bondmapmin, p.bondmapmax, "bm");

    appendFixComputeColors(m, p);

    return {d, m, dofixes};
}

QString toWriteDumpCommand(const DumpImageCommand &c, const QString &group, const QString &file)
{
    QString cmd = "write_dump " + group + " image '" + file + "'" + c.dumpargs;
    if (!c.dofixes) cmd += " noinit";
    cmd += " modify" + c.modifyargs;
    return cmd;
}

// Local Variables:
// c-basic-offset: 4
// End:
