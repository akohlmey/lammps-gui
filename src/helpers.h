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

#ifndef HELPERS_H
#define HELPERS_H

#include <QString>
#include <string>
#include <vector>

/**
 * @brief Duplicate a string from std::string
 * @param text The string to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const std::string &text);

/**
 * @brief Duplicate a string from C-string
 * @param text The C-string to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const char *text);

/**
 * @brief Duplicate a string from QString
 * @param text The QString to duplicate
 * @return Pointer to newly allocated C-string (caller must free)
 */
extern char *mystrdup(const QString &text);

/**
 * @brief Compare two date strings in "YYYY-MM-DD" format
 * @param one First date string
 * @param two Second date string
 * @return -1 if one < two, 0 if equal, 1 if one > two
 */
extern int date_compare(const QString &one, const QString &two);

/**
 * @brief Split a string into words while respecting quotes
 * @param text The string to split
 * @return Vector of words extracted from the string
 */
extern std::vector<std::string> split_line(const std::string &text);

/**
 * @brief Get pointer to the main LAMMPS-GUI widget
 * @return Pointer to the main widget (used for dialogs)
 */
extern class QWidget *get_main_widget();

/**
 * @brief Check if an executable is in the system PATH
 * @param exe The executable name to search for
 * @return true if executable is found in PATH, false otherwise
 */
extern bool has_exe(const QString &exe);

/**
 * @brief Recursively delete all files in a directory
 * @param dir The directory to purge
 */
extern void purge_directory(const QString &dir);

/**
 * @brief Silence output to stdout
 * @return old file descriptor
 */
extern int silence_stdout();

/**
 * @brief Restore output to stdout
 * @param fd file descriptor returned by silence_stdout()
 */
extern void restore_stdout(int fd);

/**
 * @brief Determine if the current Qt theme is light or dark
 * @return true if light theme, false if dark theme
 */
extern bool is_light_theme();

#endif
// Local Variables:
// c-basic-offset: 4
// End:
