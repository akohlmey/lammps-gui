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

// adapted from: https://stackoverflow.com/questions/5419356/redirect-stdout-stderr-to-a-string

#include "stdcapture.h"
#include "helpers.h"

#ifdef _WIN32
#include <io.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define dup _dup
#define dup2 _dup2
#define fileno _fileno
#define close _close
#define read _read
#else
#include <unistd.h>
#endif

#include <chrono>
#include <cstdio>
#include <fcntl.h>
#include <thread>

#ifdef _WIN32
// Check if a pipe has data available for reading without blocking.
// Uses PeekNamedPipe() instead of _eof() because _eof() relies on _lseek()
// internally to check the file position.  Since pipes are not seekable,
// _eof() returns -1 (error) on pipe file descriptors under the MSVC C runtime,
// which causes the caller to skip reading entirely and capture nothing.
// PeekNamedPipe() is the proper Win32 API for non-blocking pipe inspection.
static bool pipe_has_data(int fd)
{
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD available = 0;
    if (!PeekNamedPipe(h, nullptr, 0, nullptr, &available, nullptr)) return false;
    return available > 0;
}
#endif

namespace {
constexpr int bufSize = (1 << 16) + 1;
} // namespace

StdCapture::StdCapture() : m_oldStdOut(0), m_capturing(false), maxread(0), buf(new char[bufSize])
{
    // make stdout unbuffered so that we don't need to flush the stream
    setvbuf(stdout, nullptr, _IONBF, 0);

    m_pipe[READ]  = 0;
    m_pipe[WRITE] = 0;
#if _WIN32
    if (_pipe(m_pipe, 65536, O_BINARY) == -1) return;
#else
    if (pipe(m_pipe) == -1) return;
    fcntl(m_pipe[READ], F_SETFL, fcntl(m_pipe[READ], F_GETFL) | O_NONBLOCK);
#endif
    m_oldStdOut = dup(fileno(stdout));
    if (m_oldStdOut == -1) return;
}

StdCapture::~StdCapture()
{
    notify_capture_state(false);
    delete[] buf;
    if (m_oldStdOut > 0) close(m_oldStdOut);
    if (m_pipe[READ] > 0) close(m_pipe[READ]);
    if (m_pipe[WRITE] > 0) close(m_pipe[WRITE]);
}

void StdCapture::BeginCapture()
{
    if (m_capturing) EndCapture();
    if (is_stdout_silenced()) restore_stdout();
    dup2(m_pipe[WRITE], fileno(stdout));
    m_capturing = true;
    maxread     = 0;
    notify_capture_state(true);
}

bool StdCapture::EndCapture()
{
    if (!m_capturing) return false;
    notify_capture_state(false);
    dup2(m_oldStdOut, fileno(stdout));
    m_captured.clear();

    int bytesRead;
    bool fd_blocked;
    int maxwait = 100;

    do {
        bytesRead  = 0;
        fd_blocked = false;

#ifdef _WIN32
        if (pipe_has_data(m_pipe[READ])) {
            bytesRead = read(m_pipe[READ], buf, bufSize - 1);
        }
#else
        bytesRead = read(m_pipe[READ], buf, bufSize - 1);
#endif
        if (bytesRead > 0) {
            buf[bytesRead] = 0;
            m_captured += buf;
        } else if (bytesRead < 0) {
            fd_blocked =
                ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR)) && (maxwait > 0);

            if (fd_blocked) std::this_thread::sleep_for(std::chrono::milliseconds(10));
            --maxwait;
        }
    } while (fd_blocked || (bytesRead == (bufSize - 1)));
    m_capturing = false;
    return true;
}

std::string StdCapture::GetChunk()
{
    if (!m_capturing) return {};
    int bytesRead = 0;
    buf[0]        = '\0';

#ifdef _WIN32
    if (pipe_has_data(m_pipe[READ])) {
        bytesRead = read(m_pipe[READ], buf, bufSize - 1);
    }
#else
    bytesRead = read(m_pipe[READ], buf, bufSize - 1);
#endif
    if (bytesRead > 0) {
        buf[bytesRead] = '\0';
    }
    maxread = (maxread > bytesRead) ? maxread : bytesRead;
    return {buf};
}

double StdCapture::get_bufferuse() const
{
    return (double)maxread / (double)(bufSize - 1);
}

std::string StdCapture::GetCapture()
{
    std::string::size_type idx = m_captured.find_last_not_of("\r\n");
    if (idx == std::string::npos) {
        return m_captured;
    }
    return m_captured.substr(0, idx + 1);
}

// Local Variables:
// c-basic-offset: 4
// End:
