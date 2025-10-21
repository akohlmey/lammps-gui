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
     * @param parent Parent widget
     */
    LogWindow(const QString &filename, QWidget *parent = nullptr);
    
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
    void extract_yaml();   ///< Extract YAML data to separate file
    void quit();           ///< Close window
    void save_as();        ///< Save log to file
    void stop_run();       ///< Stop running simulation
    void next_warning();   ///< Navigate to next warning
    void open_errorurl();  ///< Open error documentation URL in browser

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
    bool check_yaml();

private:
    QString filename;                ///< Path to log file
    QString errorurl;                ///< URL of last detected error
    static const QString yaml_regex; ///< Regex for detecting YAML blocks
    static const QString url_regex;  ///< Regex for detecting URLs
    FlagWarnings *warnings;          ///< Warning highlighter
    QLabel *summary;                 ///< Summary label for warning count
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
