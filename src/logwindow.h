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

#ifndef LOGWINDOW_H
#define LOGWINDOW_H

#include <QPlainTextEdit>

class FlagWarnings;
class LammpsGui;
class QLabel;

/**
 * @brief Window for displaying LAMMPS log output with warning detection
 *
 * LogWindow provides a specialized text viewer for LAMMPS log files.
 * It highlights warnings and errors using FlagWarnings, supports
 * extraction of embedded YAML data, navigation to warnings, and
 * opening error documentation URLs. The window shows a summary
 * of detected warnings in the title bar.
 */
class LogWindow : public QPlainTextEdit {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param filename Path to log file to display
     * @param lammpsgui Pointer to LammpsGui for sending signals
     * @param parent Parent widget
     */
    LogWindow(const QString &filename, LammpsGui *lammpsgui, QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~LogWindow() override;

    LogWindow()                             = delete;
    LogWindow(const LogWindow &)            = delete;
    LogWindow(LogWindow &&)                 = delete;
    LogWindow &operator=(const LogWindow &) = delete;
    LogWindow &operator=(LogWindow &&)      = delete;

private slots:
    void extractYaml();  ///< Extract YAML data to separate file
    void quit();         ///< Close window
    void saveAs();       ///< Save log to file
    void stopRun();      ///< Stop running simulation
    void nextWarning();  ///< Navigate to next warning
    void openErrorUrl(); ///< Open error documentation URL in browser

protected:
    /**
     * @brief Handle window close event
     * @param event Close event
     */
    void closeEvent(QCloseEvent *event) override;

    /**
     * @brief Handle double-click to open URLs
     * @param event Mouse event
     */
    void mouseDoubleClickEvent(QMouseEvent *event) override;

    /**
     * @brief Show context menu with log-specific actions
     * @param event Context menu event
     */
    void contextMenuEvent(QContextMenuEvent *event) override;

    /**
     * @brief Event filter for keyboard shortcuts
     * @param watched Object being watched
     * @param event Event to filter
     * @return true if event handled, false otherwise
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /**
     * @brief Check if log contains embedded YAML data
     * @return true if YAML data detected, false otherwise
     */
    bool checkYaml();

private:
    QString filename;       ///< Path to log file
    LammpsGui *lammpsgui;   ///< Main widget pointer for receiving signals
    QString errorurl;       ///< URL of last detected error
    FlagWarnings *warnings; ///< Warning highlighter
    QLabel *summary;        ///< Summary label for warning count
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
