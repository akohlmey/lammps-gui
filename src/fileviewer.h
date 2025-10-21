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

class FileViewer : public QPlainTextEdit {
    Q_OBJECT

public:
    FileViewer(const QString &filename, const QString &title = "", QWidget *parent = nullptr);
    ~FileViewer() override = default;

    FileViewer()                              = delete;
    FileViewer(const FileViewer &)            = delete;
    FileViewer(FileViewer &&)                 = delete;
    FileViewer &operator=(const FileViewer &) = delete;
    FileViewer &operator=(FileViewer &&)      = delete;

private slots:
    void quit();
    void stop_run();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QString fileName;
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
