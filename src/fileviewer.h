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

#ifndef FILEVIEWER_H
#define FILEVIEWER_H

#include <QPlainTextEdit>

/**
 * @brief Read-only text viewer for displaying file contents
 *
 * FileViewer provides a simple read-only text window for viewing
 * file contents. It's used in the context menu of the code editor
 * to view files referenced in LAMMPS input scripts (data files,
 * potential files, etc.). The viewer supports keyboard shortcuts
 * for closing and stopping the simulation.
 */
class FileViewer : public QPlainTextEdit {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param filename Path to file to display
     * @param title Window title (defaults to filename if empty)
     * @param parent Parent widget
     */
    FileViewer(const QString &filename, const QString &title = "", QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~FileViewer() override = default;

    FileViewer()                              = delete;
    FileViewer(const FileViewer &)            = delete;
    FileViewer(FileViewer &&)                 = delete;
    FileViewer &operator=(const FileViewer &) = delete;
    FileViewer &operator=(FileViewer &&)      = delete;

private slots:
    void quit();     ///< Close the viewer window
    void stop_run(); ///< Stop the running simulation

protected:
    /**
     * @brief Event filter for keyboard shortcuts
     * @param watched Object being watched
     * @param event Event to filter
     * @return true if event handled, false otherwise
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QString fileName; ///< Path to the displayed file
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
