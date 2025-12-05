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

#ifndef LAMMPSRUNNER_H
#define LAMMPSRUNNER_H

#include <QThread>

/**
 * @brief Worker thread for executing LAMMPS simulations
 *
 * This class provides a separate thread for running LAMMPS simulations
 * so that the GUI remains responsive during long-running calculations.
 * It executes LAMMPS commands or input files in the background and
 * emits a signal when complete.
 */
class LammpsRunner : public QThread {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent QObject
     */
    LammpsRunner(QObject *parent = nullptr) : QThread(parent), lammps(nullptr), input(nullptr) {}

    /**
     * @brief Destructor
     */
    ~LammpsRunner() override = default;

    LammpsRunner()                                = delete;
    LammpsRunner(const LammpsRunner &)            = delete;
    LammpsRunner(LammpsRunner &&)                 = delete;
    LammpsRunner &operator=(const LammpsRunner &) = delete;
    LammpsRunner &operator=(LammpsRunner &&)      = delete;

public:
    /**
     * @brief Thread execution function - runs LAMMPS commands or input file
     *
     * This function executes in the worker thread. It processes either
     * a string of LAMMPS commands or an input file, then signals completion.
     */
    void run() override
    {
        if (input) {
            lammps->commands_string(input);
            delete[] input;
        } else if (file) {
            lammps->file(file);
            delete[] file;
        }
        emit resultReady();
    }

    /**
     * @brief Prepare the runner thread with LAMMPS instance and commands
     * @param _lammps Pointer to LammpsWrapper instance
     * @param _input String of LAMMPS commands to execute (can be nullptr)
     * @param _file Input file path to execute (can be nullptr)
     *
     * Sets up the runner with the LAMMPS instance and input. Clears any
     * previous LAMMPS state with the "clear" command. Either input or
     * file should be provided, not both.
     */
    void setup_run(LammpsWrapper *_lammps, const char *_input, const char *_file = nullptr)
    {
        lammps = _lammps;
        input  = _input;
        file   = _file;
        lammps->command("clear");
    }

signals:
    /**
     * @brief Signal emitted when LAMMPS execution completes
     */
    void resultReady();

private:
    LammpsWrapper *lammps; ///< Pointer to the LAMMPS wrapper instance
    const char *input;     ///< String of LAMMPS commands to execute
    const char *file;      ///< Input file path to execute
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
