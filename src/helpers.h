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

#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <string>
#include <vector>

// duplicate string
extern char *mystrdup(const std::string &text);
extern char *mystrdup(const char *text);
extern char *mystrdup(const QString &text);

// compare date strings
extern int date_compare(const QString &one, const QString &two);

// split string into words while respecting quotes
extern std::vector<std::string> split_line(const std::string &text);

// get pointer to LAMMPS-GUI main widget
extern class QWidget *get_main_widget();

// find if executable is in path
extern bool has_exe(const QString &exe);

// recursively purge a directory
extern void purge_directory(const QString &dir);

// flag if light or dark theme
extern bool is_light_theme();

#endif
// Local Variables:
// c-basic-offset: 4
// End:
