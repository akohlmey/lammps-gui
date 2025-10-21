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

#ifndef STDCAPTURE_H
#define STDCAPTURE_H

#include <string>

class StdCapture {
public:
    StdCapture();
    StdCapture(const StdCapture &)            = delete;
    StdCapture(StdCapture &&)                 = delete;
    StdCapture &operator=(const StdCapture &) = delete;
    StdCapture &operator=(StdCapture &&)      = delete;
    virtual ~StdCapture();

    void BeginCapture();
    bool EndCapture();
    std::string GetCapture();
    std::string GetChunk();

    double get_bufferuse() const;

private:
    enum PIPES { READ, WRITE, PIPE_COUNT };
    int m_pipe[PIPE_COUNT];
    int m_oldStdOut;
    bool m_capturing;
    std::string m_captured;
    int maxread;

    char *buf;
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
