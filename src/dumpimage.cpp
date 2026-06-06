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

// Append the definition of the selected color map (color/amap arguments).
static void appendColorMapArgs(QString &cmd, const DumpImageParams &p)
{
    QString mmin = p.mapmin;
    if (mmin == "auto") mmin = "min";
    QString mmax = p.mapmax;
    if (mmax == "auto") mmax = "max";
    if (p.colormap == "RWB") {
        cmd += " color map1 0.459 0.055 0.075";
        cmd += " color map2 0.000 0.227 0.427";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "5 min map1 0.1 map1 0.5 white 0.9 map2 max map2";
    } else if (p.colormap == "PWT") {
        cmd += " color map1 0.286 0.114 0.553";
        cmd += " color map2 0.000 0.255 0.267";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "5 min map1 0.1 map1 0.5 white 0.9 map2 max map2";
    } else if (p.colormap == "BGR") {
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "5 min blue 0.05 blue 0.5 green 0.95 red max red";
    } else if (p.colormap == "BWG") {
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "5 min blue 0.1 blue 0.5 white 0.9 green max green";
    } else if (p.colormap == "Grayscale") {
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "2 min black max white";
    } else if (p.colormap == "Rainbow") {
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "6 min red 0.25 yellow 0.45 green 0.65 cyan 0.85 blue max purple";
    } else if (p.colormap == "Sequential") {
        cmd += " color map1 0.808 0.808 0.808";
        cmd += " color map2 0.647 0.349 0.667";
        cmd += " color map3 0.349 0.659 0.612";
        cmd += " color map4 0.941 0.772 0.443";
        cmd += " color map5 0.878 0.169 0.208";
        cmd += " color map6 0.031 0.165 0.329";
        cmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        cmd += "6 map1 map2 map3 map4 map5 map6";
    } else if (p.colormap == "Landscape") {
        cmd += " color map0 0.145 0.400 0.463";
        cmd += " color map1 0.392 0.867 0.588";
        cmd += " color map2 0.572 0.192 0.141";
        cmd += " color map3 0.392 0.831 0.992";
        cmd += " color map4 0.020 0.431 0.071";
        cmd += " color map5 0.992 0.349 0.145";
        cmd += " color map6 0.275 0.953 0.243";
        cmd += " color map7 0.729 0.525 0.361";
        cmd += " color map8 0.780 0.867 0.529";
        cmd += " color map9 0.243 0.298 0.078";
        cmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        cmd += "10 map0 map1 map2 map3 map4 map5 map6 map7 map8 map9";
    } else if (p.colormap == "Basic") {
        cmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        cmd += "10 red cyan green black magenta blue yellow purple white orange";
    } else if (p.colormap == "Teal") {
        cmd += " color map1 0.071 0.153 0.251";
        cmd += " color map2 0.106 0.282 0.369";
        cmd += " color map3 0.337 0.545 0.529";
        cmd += " color map4 0.710 0.820 0.682";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "4 min map1 0.25 map2 0.5 map3 max map4";
    } else if (p.colormap == "Viridis") {
        cmd += " color map1 0.282 0.129 0.451";
        cmd += " color map2 0.435 0.435 0.556";
        cmd += " color map3 0.161 0.686 0.498";
        cmd += " color map4 0.741 0.875 0.149";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "4 min map1 0.333 map2 0.667 map3 max map4";
    } else if (p.colormap == "Inferno") {
        cmd += " color map1 0.032 0.032 0.048";
        cmd += " color map2 0.318 0.071 0.486";
        cmd += " color map3 0.718 0.216 0.475";
        cmd += " color map4 0.988 0.537 0.380";
        cmd += " color map5 0.988 0.992 0.749";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "5 min map1 0.25 map2 0.5 map3 0.75 map4 max map5";
    } else if (p.colormap == "Plasma") {
        cmd += " color map1 0.051 0.031 0.529";
        cmd += " color map2 0.612 0.090 0.620";
        cmd += " color map3 0.929 0.475 0.325";
        cmd += " color map4 0.941 0.976 0.129";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "4 min map1 0.333 map2 0.667 map3 max map4";
    } else { // default is "BWR"
        cmd += " color map1 0.000 0.227 0.427";
        cmd += " color map2 0.459 0.055 0.075";
        cmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        cmd += "3 min map1 0.5 white max map2";
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

QString buildDumpImageCommand(const DumpImageParams &p)
{
    QString dumpcmd = QString("write_dump ") + p.group + " image ";
    dumpcmd += "'" + p.dumpfile + "'";

    const int hhrot   = (p.hrot > 180) ? 360 - p.hrot : p.hrot;
    const bool do_vdw = p.vdwfactor > VDW_CUT;

    // atom color for dump
    if (!p.atomcustom && p.useelements)
        dumpcmd += blank + "element";
    else
        dumpcmd += blank + p.atomcolor;

    // atom diameter for dump
    if (!p.atomcustom) {
        if (p.usediameter && do_vdw)
            dumpcmd += blank + "diameter";
        else
            dumpcmd += " type";
    } else {
        if ((p.atomdiam == "diameter") && p.usediameter && do_vdw)
            dumpcmd += blank + "diameter";
        else
            dumpcmd += " type";
    }

    if (!p.showatoms) dumpcmd += " atom no";
    if (p.showbodies && (p.body_flag == 1)) {
        dumpcmd += QString(" body %1 %2 %3").arg(p.bodycolor).arg(p.bodydiam).arg(p.bodyflag);
    } else if (p.showlines && (p.line_flag == 1))
        dumpcmd += QString(" line %1 %2").arg(p.linecolor).arg(p.linediam);
    else if (p.showtris && (p.tri_flag == 1))
        dumpcmd += QString(" tri %1 %2 %3").arg(p.tricolor).arg(p.triflag).arg(p.tridiam);
    else if (p.showellipsoids && (p.ellipsoid_flag == 1)) {
        dumpcmd += QString(" ellipsoid %1 %2 %3 %4")
                       .arg(p.ellipsoidcolor)
                       .arg(p.ellipsoidflag)
                       .arg(p.ellipsoidlevel)
                       .arg(p.ellipsoiddiam);
    }
    dumpcmd += QString(" size %1 %2").arg(p.xsize).arg(p.ysize);
    dumpcmd += QString(" zoom %1").arg(p.zoom);
    dumpcmd += QString(" shiny %1 ").arg(p.shinyfactor);
    dumpcmd += QString(" fsaa %1").arg(p.antialias ? "yes" : "no");
    if (p.nbondtypes > 0) {
        if (do_vdw || !p.showbonds)
            dumpcmd += " bond none none ";
        else
            dumpcmd += QString(" bond %1 %2 ").arg(p.bondcolor).arg(p.bonddiam);
    }
    if (p.dimension == 3) {
        dumpcmd += QString(" view %1 %2").arg(hhrot).arg(p.vrot);
    }
    if (p.usessao) dumpcmd += QString(" ssao yes 453983 %1").arg(p.ssaoval);
    if (p.showbox)
        dumpcmd += QString(" box yes %1").arg(p.boxdiam);
    else
        dumpcmd += " box no 0.0";
    if (p.showsubbox)
        dumpcmd += QString(" subbox yes %1").arg(p.subboxdiam);
    else
        dumpcmd += " subbox no 0.0";

    if (p.showaxes)
        dumpcmd += QString(" axes %1 %2 %3").arg(p.axesloc).arg(p.axeslen).arg(p.axesdiam);
    else
        dumpcmd += " axes no 0.0 0.0";

    if (p.autobond && p.haspairstyle) {
        // use custom bond diameter value, if present
        QRegularExpression validnum(R"((^\d+\.?\d*|^\d*\.?\d+))");
        auto match = validnum.match(p.bonddiam);
        if (match.hasMatch()) {
            dumpcmd +=
                blank + "autobond" + blank + QString::number(p.bondcutoff) + blank + p.bonddiam;
        } else {
            dumpcmd += blank + "autobond" + blank + QString::number(p.bondcutoff) + " 0.5";
        }
    }

    appendRegionArgs(dumpcmd, p);

    const bool dofixes = appendFixComputeStyles(dumpcmd, p);

    dumpcmd += QString(" center s %1 %2 %3").arg(p.xcenter).arg(p.ycenter).arg(p.zcenter);
    if (!dofixes) dumpcmd += " noinit";
    dumpcmd += " modify";

    // must change global colors first so they apply everywhere
    const int numcolors = p.color_list.size();

    for (int i = 0; i < numcolors; ++i) {
        dumpcmd += QString(" color %1 %2 %3 %4")
                       .arg(p.color_list[i].first)
                       .arg(p.color_list[i].second.redF())
                       .arg(p.color_list[i].second.greenF())
                       .arg(p.color_list[i].second.blueF());
    }
    for (int i = 1; i <= p.ntypes; ++i) {
        dumpcmd += QString(" acolor %1 %2").arg(i).arg(p.color_list[(i - 1) % numcolors].first);
    }

    dumpcmd += " boxcolor " + p.boxcolor;
    dumpcmd += " backcolor " + p.backcolor;
    dumpcmd += " backcolor2 " + p.backcolor2;
    dumpcmd += QString(" axestrans %1").arg(p.axestrans);
    dumpcmd += QString(" boxtrans %1").arg(p.boxtrans);
    dumpcmd += QString(" atrans * %1").arg(p.atomtrans);
    if (p.bond_flag == 1) dumpcmd += QString(" btrans * %1").arg(p.atomtrans);
    if (p.version > 20260330)
        dumpcmd += QString(" lights %1 %2 %3 %4")
                       .arg(p.ambientlight)
                       .arg(p.keylight)
                       .arg(p.filllight)
                       .arg(p.backlight);

    if (p.useelements) dumpcmd += blank + p.elements + blank + p.adiams + blank;
    if (p.usesigma) dumpcmd += blank + p.adiams + blank;
    if (!p.useelements && !p.usesigma && (p.atomSize != 1.0)) dumpcmd += blank + p.adiams + blank;
    // apply selected colormap
    appendColorMapArgs(dumpcmd, p);

    appendFixComputeColors(dumpcmd, p);

    return dumpcmd;
}

// Local Variables:
// c-basic-offset: 4
// End:
