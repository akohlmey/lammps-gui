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

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QPlainTextEdit>

class FlagWarnings;
class QLabel;

class LogWindow : public QPlainTextEdit {
    Q_OBJECT

public:
    LogWindow(const QString &filename, QWidget *parent = nullptr);
    ~LogWindow() override;

    LogWindow()                             = delete;
    LogWindow(const LogWindow &)            = delete;
    LogWindow(LogWindow &&)                 = delete;
    LogWindow &operator=(const LogWindow &) = delete;
    LogWindow &operator=(LogWindow &&)      = delete;

private slots:
    void extract_yaml();
    void quit();
    void save_as();
    void stop_run();
    void next_warning();
    void open_errorurl();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    bool check_yaml();

private:
    QString filename;
    QString errorurl;
    static const QString yaml_regex;
    static const QString url_regex;
    FlagWarnings *warnings;
    QLabel *summary;
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
