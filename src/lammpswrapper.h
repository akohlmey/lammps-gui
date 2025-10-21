/* -*- c++ -*- ----------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#ifndef LAMMPSWRAPPER_H
#define LAMMPSWRAPPER_H

#include <QString>
#include <string>

/**
 * @brief C++ wrapper for the LAMMPS C library interface
 * 
 * This class provides a C++ interface to the LAMMPS library. It wraps
 * the C library API functions and handles dynamic loading of the LAMMPS
 * library when built in plugin mode. All LAMMPS library function calls
 * are routed through this wrapper class.
 */
class LammpsWrapper {
public:
    /**
     * @brief Constructor - initializes wrapper
     */
    LammpsWrapper();
    
    /**
     * @brief Destructor - cleans up LAMMPS instance if open
     */
    ~LammpsWrapper() = default;

    LammpsWrapper(const LammpsWrapper &)            = delete;
    LammpsWrapper(LammpsWrapper &&)                 = delete;
    LammpsWrapper &operator=(const LammpsWrapper &) = delete;
    LammpsWrapper &operator=(LammpsWrapper &&)      = delete;

public:
    /**
     * @brief Create a new LAMMPS instance
     * @param nargs Number of command-line arguments
     * @param args Command-line arguments array
     */
    void open(int nargs, char **args);
    
    /**
     * @brief Close the LAMMPS instance
     */
    void close();
    
    /**
     * @brief Finalize MPI (if used) and close LAMMPS
     */
    void finalize();

    /**
     * @brief Process commands from a LAMMPS input file
     * @param fname Filename (QString version)
     */
    void file(const QString &fname) { file(fname.toStdString()); }
    
    /**
     * @brief Process commands from a LAMMPS input file
     * @param fname Filename (std::string version)
     */
    void file(const std::string &fname) { file(fname.c_str()); }
    
    /**
     * @brief Process commands from a LAMMPS input file
     * @param fname Filename (C-string version)
     */
    void file(const char *);
    
    /**
     * @brief Execute a single LAMMPS command
     * @param cmd Command string (QString version)
     */
    void command(const QString &cmd) { command(cmd.toStdString()); }
    
    /**
     * @brief Execute a single LAMMPS command
     * @param cmd Command string (std::string version)
     */
    void command(const std::string &cmd) { command(cmd.c_str()); }
    
    /**
     * @brief Execute a single LAMMPS command
     * @param cmd Command string (C-string version)
     */
    void command(const char *);
    
    /**
     * @brief Execute multiple LAMMPS commands from a string
     * @param cmd Commands string with newlines (QString version)
     */
    void commands_string(const QString &cmd) { commands_string(cmd.toStdString()); }
    
    /**
     * @brief Execute multiple LAMMPS commands from a string
     * @param cmd Commands string with newlines (std::string version)
     */
    void commands_string(const std::string &cmd) { commands_string(cmd.c_str()); }
    
    /**
     * @brief Execute multiple LAMMPS commands from a string
     * @param cmd Commands string with newlines (C-string version)
     */
    void commands_string(const char *);

    /**
     * @brief Force a timeout condition in LAMMPS
     */
    void force_timeout();

    /**
     * @brief Get LAMMPS version number
     * @return Version number as integer (YYYYMMDD format)
     */
    int version();
    
    /**
     * @brief Extract a global setting from LAMMPS
     * @param keyword Setting name to extract
     * @return Integer value of the setting
     */
    int extract_setting(const char *keyword);
    
    /**
     * @brief Extract a pointer to global data from LAMMPS
     * @param keyword Name of global data to extract
     * @return Pointer to the data
     */
    void *extract_global(const char *keyword);
    
    /**
     * @brief Extract pair style data from LAMMPS
     * @param keyword Name of pair data to extract
     * @return Pointer to the pair data
     */
    void *extract_pair(const char *keyword);
    
    /**
     * @brief Extract atom data from LAMMPS
     * @param keyword Name of atom data to extract
     * @return Pointer to the atom data
     */
    void *extract_atom(const char *keyword);
    
    /**
     * @brief Extract a variable value from LAMMPS
     * @param keyword Variable name to extract
     * @return Value of the variable as double
     */
    double extract_variable(const char *keyword);

    /**
     * @brief Check if a compute/fix/variable ID exists
     * @param idtype Type of ID ("compute", "fix", "variable")
     * @param id The ID to check
     * @return 1 if exists, 0 otherwise
     */
    int has_id(const char *idtype, const char *id);
    
    /**
     * @brief Get count of IDs of a specific type
     * @param idtype Type of ID ("compute", "fix", "variable", "group")
     * @return Number of IDs of that type
     */
    int id_count(const char *idtype);
    
    /**
     * @brief Get name of an ID by index
     * @param idtype Type of ID
     * @param idx Index of the ID
     * @param buf Buffer to store the name
     * @param buflen Length of buffer
     * @return 0 on success, -1 on error
     */
    int id_name(const char *idtype, int idx, char *buf, int buflen);
    
    /**
     * @brief Get count of styles of a specific type
     * @param keyword Type of style ("compute", "fix", "pair", etc.)
     * @return Number of available styles
     */
    int style_count(const char *keyword);
    
    /**
     * @brief Get name of a style by index
     * @param keyword Type of style
     * @param idx Index of the style
     * @param buf Buffer to store the name
     * @param buflen Length of buffer
     * @return 0 on success, -1 on error
     */
    int style_name(const char *keyword, int idx, char *buf, int buflen);
    
    /**
     * @brief Get information about a variable by index
     * @param idx Variable index
     * @param buf Buffer to store variable info
     * @param buflen Length of buffer
     * @return Variable type code
     */
    int variable_info(int idx, char *buf, int buflen);

    /**
     * @brief Get current value of a thermodynamic quantity
     * @param keyword Thermo keyword
     * @return Value of the thermo quantity
     */
    double get_thermo(const char *keyword);
    
    /**
     * @brief Get a specific value from last thermo output
     * @param keyword Thermo keyword
     * @param idx Index for vector quantities
     * @return Pointer to the value
     */
    void *last_thermo(const char *keyword, int idx);

    /**
     * @brief Check if LAMMPS instance is open
     * @return true if LAMMPS is initialized, false otherwise
     */
    bool is_open() const { return lammps_handle != nullptr; }
    
    /**
     * @brief Check if LAMMPS is currently executing a run
     * @return true if running, false otherwise
     */
    bool is_running();

    /**
     * @brief Check if LAMMPS has encountered an error
     * @return true if error occurred, false otherwise
     */
    bool has_error() const;
    
    /**
     * @brief Get the last error message from LAMMPS
     * @param errorbuf Buffer to store error message
     * @param buflen Length of buffer
     * @return Error type code
     */
    int get_last_error_message(char *errorbuf, int buflen);

    /**
     * @brief Check if an accelerator package is available
     * @param package Package name
     * @param category Category name
     * @param setting Setting name
     * @return true if available, false otherwise
     */
    bool config_accelerator(const char *package, const char *category, const char *setting) const;
    
    /**
     * @brief Check if a package is included in LAMMPS build
     * @param pkg Package name
     * @return true if included, false otherwise
     */
    bool config_has_package(const char *pkg) const;
    
    /**
     * @brief Check if LAMMPS was built with CURL support
     * @return true if CURL is available, false otherwise
     */
    bool config_has_curl_support() const;
    
    /**
     * @brief Check if LAMMPS was built with OpenMP support
     * @return true if OpenMP is available, false otherwise
     */
    bool config_has_omp_support() const;
    
    /**
     * @brief Check if GPU device is available for GPU package
     * @return true if GPU device found, false otherwise
     */
    bool has_gpu_device() const;

    /**
     * @brief Load LAMMPS shared library (plugin mode)
     * @param fname Library filename (QString version)
     * @return true on success, false on failure
     */
    bool load_lib(const QString &fname) { return load_lib(fname.toStdString().c_str()); }
    
    /**
     * @brief Load LAMMPS shared library (plugin mode)
     * @param lammpslib Library filename (C-string version)
     * @return true on success, false on failure
     */
    bool load_lib(const char *lammpslib);
    
    /**
     * @brief Check if running in plugin mode
     * @return true if plugin mode enabled, false if linked mode
     */
    bool has_plugin() const;

private:
    void *lammps_handle;  ///< Handle to LAMMPS instance
#if defined(LAMMPS_GUI_USE_PLUGIN)
    void *plugin_handle;  ///< Handle to dynamically loaded LAMMPS library
#endif
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
