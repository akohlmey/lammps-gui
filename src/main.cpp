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

#include "helpers.h"
#include "lammpsgui.h"

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFileInfo>
#include <QFont>
#include <QLocale>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleFactory>
#include <QtGlobal>

#include <cstdio>
#include <cstring>

#define stringify(x) myxstr(x)
#define myxstr(x) #x

int main(int argc, char *argv[])
{
#if defined(Q_OS_MACOS)
    // macOS does not support the "C" locale with UTF-8 encoding,
    // Since Qt requires UTF-8 we use "en_US" instead.
    qputenv("LC_ALL", "en_US.UTF-8");
#else
    // enforce using the plain ASCII C locale with UTF-8 encoding within the GUI.
    qputenv("LC_ALL", "C.UTF-8");
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
    parser.addOptions(
        {{{"x", "width"}, "Override LAMMPS-GUI editor window width", "width"},
         {{"y", "height"}, "Override LAMMPS-GUI editor window height", "height"},
         {{"s", "style"}, "Set LAMMPS-GUI's visual style (default: Fusion)", "style", "Fusion"}});
    parser.addPositionalArgument("file", "The LAMMPS input file to open (optional).");
    parser.process(app);

#if defined(LAMMPS_GUI_USE_PLUGIN)
    if (parser.isSet(plugindir)) {
        QStringList pluginPath = parser.values(plugindir);
        QSettings settings;
        if (pluginPath.length() > 0) {
            settings.setValue("plugin_path", QFileInfo(pluginPath.at(0)).canonicalFilePath());
            settings.sync();
        } else {
            // empty string provided -> delete any old setting
            settings.remove("plugin_path");
        }
    }
#endif

    QString infile;
    int width  = parser.value("width").toInt();
    int height = parser.value("height").toInt();

    auto *usestyle = QStyleFactory::create(parser.value("style"));
    if (usestyle) QApplication::setStyle(usestyle);

#if defined(Q_OS_MACOS)
    GUI_MONOFONT = new QFont("Menlo", -1, QFont::Normal);
    GUI_ALLFONT  = new QFont("Arial", -1, QFont::Normal);
#elif defined(Q_OS_WIN32)
    GUI_MONOFONT = new QFont("Consolas", -1, QFont::Normal);
    GUI_ALLFONT  = new QFont("Arial", -1, QFont::Normal);
#else
    GUI_MONOFONT = new QFont("Monospace", -1, QFont::Normal);
    GUI_ALLFONT  = new QFont("Arial", -1, QFont::Normal);
#endif
    GUI_MONOFONT->setStyleHint(QFont::Monospace, QFont::PreferQuality);
    GUI_MONOFONT->setFixedPitch(true);
    GUI_ALLFONT->setStyleHint(QFont::SansSerif, QFont::PreferQuality);

    QStringList args = parser.positionalArguments();
    if (!args.empty()) infile = args[0];

    Q_INIT_RESOURCE(lammpsgui);
    LammpsGui w(nullptr, infile, width, height);
    w.show();

    return app.exec();
}

// Local Variables:
// c-basic-offset: 4
// End:
