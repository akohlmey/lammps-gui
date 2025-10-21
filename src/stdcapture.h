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

#ifndef STDCAPTURE_H
#define STDCAPTURE_H

#include <string>

/**
 * @brief Capture stdout output to a string buffer
 * 
 * This class provides functionality to redirect and capture standard output
 * (stdout) into a string buffer. Used to capture output from LAMMPS library
 * calls for display in the GUI.
 */
class StdCapture {
public:
    /**
     * @brief Constructor - initializes capture buffers
     */
    StdCapture();
    StdCapture(const StdCapture &)            = delete;
    StdCapture(StdCapture &&)                 = delete;
    StdCapture &operator=(const StdCapture &) = delete;
    StdCapture &operator=(StdCapture &&)      = delete;
    
    /**
     * @brief Destructor - restores stdout and frees buffers
     */
    virtual ~StdCapture();

    /**
     * @brief Start capturing stdout
     * 
     * Redirects stdout to an internal pipe for capture
     */
    void BeginCapture();
    
    /**
     * @brief Stop capturing stdout and restore original stdout
     * @return true if capture was active, false otherwise
     */
    bool EndCapture();
    
    /**
     * @brief Get all captured output and clear the buffer
     * @return String containing all captured output
     */
    std::string GetCapture();
    
    /**
     * @brief Get a chunk of captured output without clearing
     * @return String containing new output since last GetChunk call
     */
    std::string GetChunk();

    /**
     * @brief Get the buffer usage as a fraction of max buffer size
     * @return Value between 0.0 and 1.0 indicating buffer fullness
     */
    double get_bufferuse() const;

private:
    /**
     * @brief Pipe file descriptors for capturing output
     */
    enum PIPES { READ, WRITE, PIPE_COUNT };
    int m_pipe[PIPE_COUNT];      ///< Pipe file descriptors
    int m_oldStdOut;             ///< Original stdout file descriptor
    bool m_capturing;            ///< Flag indicating if capture is active
    std::string m_captured;      ///< Buffer for captured output
    int maxread;                 ///< Maximum bytes to read at once

    char *buf;                   ///< Internal read buffer
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
