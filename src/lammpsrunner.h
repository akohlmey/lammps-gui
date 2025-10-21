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

class LammpsRunner : public QThread {
    Q_OBJECT

public:
    LammpsRunner(QObject *parent = nullptr) : QThread(parent), lammps(nullptr), input(nullptr) {}
    ~LammpsRunner() override = default;

    LammpsRunner()                                = delete;
    LammpsRunner(const LammpsRunner &)            = delete;
    LammpsRunner(LammpsRunner &&)                 = delete;
    LammpsRunner &operator=(const LammpsRunner &) = delete;
    LammpsRunner &operator=(LammpsRunner &&)      = delete;

public:
    // execute LAMMPS in runner thread
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

    // transfer info to worker thread and reset LAMMPS instance
    void setup_run(LammpsWrapper *_lammps, const char *_input, const char *_file = nullptr)
    {
        lammps = _lammps;
        input  = _input;
        file   = _file;
        lammps->command("clear");
    }

signals:
    void resultReady();

private:
    LammpsWrapper *lammps;
    const char *input;
    const char *file;
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
