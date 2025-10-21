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

#ifndef LAMMPSWRAPPER_H
#define LAMMPSWRAPPER_H

#include <QString>
#include <string>

class LammpsWrapper {
public:
    LammpsWrapper();
    ~LammpsWrapper() = default;

    LammpsWrapper(const LammpsWrapper &)            = delete;
    LammpsWrapper(LammpsWrapper &&)                 = delete;
    LammpsWrapper &operator=(const LammpsWrapper &) = delete;
    LammpsWrapper &operator=(LammpsWrapper &&)      = delete;

public:
    void open(int nargs, char **args);
    void close();
    void finalize();

    void file(const QString &fname) { file(fname.toStdString()); }
    void file(const std::string &fname) { file(fname.c_str()); }
    void file(const char *);
    void command(const QString &cmd) { command(cmd.toStdString()); }
    void command(const std::string &cmd) { command(cmd.c_str()); }
    void command(const char *);
    void commands_string(const QString &cmd) { commands_string(cmd.toStdString()); }
    void commands_string(const std::string &cmd) { commands_string(cmd.c_str()); }
    void commands_string(const char *);

    void force_timeout();

    int version();
    int extract_setting(const char *keyword);
    void *extract_global(const char *keyword);
    void *extract_pair(const char *keyword);
    void *extract_atom(const char *keyword);
    double extract_variable(const char *keyword);

    int has_id(const char *idtype, const char *id);
    int id_count(const char *idtype);
    int id_name(const char *idtype, int idx, char *buf, int buflen);
    int style_count(const char *keyword);
    int style_name(const char *keyword, int idx, char *buf, int buflen);
    int variable_info(int idx, char *buf, int buflen);

    double get_thermo(const char *keyword);
    void *last_thermo(const char *keyword, int idx);

    bool is_open() const { return lammps_handle != nullptr; }
    bool is_running();

    bool has_error() const;
    int get_last_error_message(char *errorbuf, int buflen);

    bool config_accelerator(const char *package, const char *category, const char *setting) const;
    bool config_has_package(const char *pkg) const;
    bool config_has_curl_support() const;
    bool config_has_omp_support() const;
    bool has_gpu_device() const;

    bool load_lib(const QString &fname) { return load_lib(fname.toStdString().c_str()); }
    bool load_lib(const char *lammpslib);
    bool has_plugin() const;

private:
    void *lammps_handle;
#if defined(LAMMPS_GUI_USE_PLUGIN)
    void *plugin_handle;
#endif
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
