// -*- c++ -*- /////////////////////////////////////////////////////////////////////////
// LAMMPS-GUI - A Graphical Tool to Learn and Explore the LAMMPS MD Simulation Software
//
// Copyright (c) 2023, 2024, 2025  Axel Kohlmeyer
//
// Documentation: https://lammps-gui.lammps.org/
// Contact: akohlmey@gmail.com
//
// This software is distributed under the GNU General Public License version 2 or later.
////////////////////////////////////////////////////////////////////////////////////////

#include "lammpsgui.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <cstdio>
#include <cstring>

#define stringify(x) myxstr(x)
#define myxstr(x) #x

int main(int argc, char *argv[])
{
    Q_INIT_RESOURCE(lammpsgui);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // register QList<QString> only needed for Qt5
    qRegisterMetaTypeStreamOperators<QList<QString>>("QList<QString>");
#endif

#ifndef Q_OS_MACOS
    // enforce using the plain ASCII C locale with UTF-8 encoding within the GUI.
    qputenv("LC_ALL", "C.UTF-8");
#else
    // macOS does not support "C" locale with UTF-8 encoding, but Qt requires UTF-8
    qputenv("LC_ALL", "en_US.UTF-8");
#endif

    // disable processor affinity for threads by default
    qputenv("OMP_PROC_BIND", "false");

    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("The LAMMPS Developers");
    QCoreApplication::setOrganizationDomain("lammps.org");
    QCoreApplication::setApplicationName("LAMMPS-GUI (QT" stringify(QT_VERSION_MAJOR) ")");
    QCoreApplication::setApplicationVersion(LAMMPS_GUI_VERSION);
    QCommandLineParser parser;
    QString description(
        "\nThis is LAMMPS-GUI v" LAMMPS_GUI_VERSION "\n"
        "\nA graphical editor for LAMMPS input files with syntax highlighting and\n"
        "auto-completion that can run LAMMPS directly. It has built-in capabilities\n"
        "for monitoring, visualization, plotting, and capturing console output.");
#if defined(LAMMPS_GUI_USE_PLUGIN)
    description += QString("\n\nCurrent LAMMPS plugin path setting:\n  %1")
                       .arg(QSettings().value("plugin_path", "").toString());
#endif
    parser.setApplicationDescription(description);

#if defined(LAMMPS_GUI_USE_PLUGIN)
    QCommandLineOption plugindir(QStringList() << "p"
                                               << "pluginpath",
                                 "Set path to LAMMPS shared library", "path");
    parser.addOption(plugindir);
#endif

    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("file", "The LAMMPS input file to open (optional).");
    parser.process(app);

#if defined(LAMMPS_GUI_USE_PLUGIN)
    if (parser.isSet(plugindir)) {
        QStringList pluginpath = parser.values(plugindir);
        if (pluginpath.length() > 0) {
            QSettings settings;
            settings.setValue("plugin_path", QFileInfo(pluginpath.at(0)).canonicalFilePath());
            settings.sync();
        }
    }
#endif

    QString infile;
    QStringList args = parser.positionalArguments();
    if (!args.empty()) infile = args[0];
    LammpsGui w(nullptr, infile);
    w.show();
    return app.exec();
}

// Local Variables:
// c-basic-offset: 4
// End:
