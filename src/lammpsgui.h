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

#ifndef LAMMPSGUI_H
#define LAMMPSGUI_H

#include <QMainWindow>

#include <QEvent>
#include <QGridLayout>
#include <QList>
#include <QPair>
#include <QSpacerItem>
#include <QString>
#include <QWizard>
#include <string>
#include <vector>

#include "lammpswrapper.h"

// identifier for LAMMPS restart files
#if !defined(LAMMPS_MAGIC)
#define LAMMPS_MAGIC "LammpS RestartT"
#endif

// forward declarations

class QFont;
class QLabel;
class QPlainTextEdit;
class QProgressBar;
class QTimer;
class QWidget;
class QWizardPage;

QT_BEGIN_NAMESPACE
namespace Ui {
class LammpsGui;
}
QT_END_NAMESPACE

class ChartWindow;
class GeneralTab;
class Highlighter;
class ImageViewer;
class LammpsRunner;
class LogWindow;
class Preferences;
class SlideShow;
class StdCapture;

/**
 * @brief Main application window for LAMMPS-GUI
 * 
 * LammpsGui is the central component of the LAMMPS-GUI application, serving as the main
 * window that coordinates all other components. It manages:
 * - The code editor for LAMMPS input scripts with syntax highlighting
 * - File operations (open, save, recent files)
 * - LAMMPS simulation execution and control
 * - Visualization windows (images, charts, log output)
 * - Application preferences and settings
 * - Tutorial wizard for interactive learning
 * 
 * The class integrates with Qt's main window framework and provides menu actions,
 * toolbars, and status bar components. It uses a LammpsWrapper to interface with
 * the LAMMPS library and LammpsRunner to execute simulations in a separate thread.
 * 
 * @see LammpsWrapper for LAMMPS library interface
 * @see LammpsRunner for threaded simulation execution
 * @see CodeEditor for the text editor component
 */
class LammpsGui : public QMainWindow {
    Q_OBJECT

    friend class CodeEditor;
    friend class AcceleratorTab;
    friend class GeneralTab;
    friend class TutorialWizard;
    friend class Preferences;

public:
    /**
     * @brief Construct the main application window
     * @param parent Parent widget (typically nullptr for main window)
     * @param filename Optional file to open on startup
     * 
     * Initializes the main window, sets up the UI components, loads preferences,
     * initializes the LAMMPS library, and optionally opens a file if provided.
     */
    LammpsGui(QWidget *parent = nullptr, const QString &filename = QString());
    
    /**
     * @brief Destructor
     * 
     * Cleans up resources including dynamically created widgets and LAMMPS instances.
     */
    ~LammpsGui() override;

    LammpsGui()                             = delete;
    LammpsGui(const LammpsGui &)            = delete;
    LammpsGui(LammpsGui &&)                 = delete;
    LammpsGui &operator=(const LammpsGui &) = delete;
    LammpsGui &operator=(LammpsGui &&)      = delete;

protected:
    /** @brief Open a file in the editor */
    void open_file(const QString &filename);
    
    /** @brief Open a file in a read-only viewer dialog */
    void view_file(const QString &filename);
    
    /** @brief Open a file for inspection (data files, etc.) */
    void inspect_file(const QString &filename);
    
    /** @brief Write current editor content to a file */
    void write_file(const QString &filename);
    
    /** @brief Update the recent files list */
    void update_recents(const QString &filename = "");
    
    /** @brief Clear the list of index-style variables */
    void clear_variables();
    
    /** @brief Update variables in LAMMPS from the variables list */
    void update_variables();
    
    /**
     * @brief Execute a LAMMPS simulation
     * @param use_buffer If true, runs from editor buffer; if false, saves and runs from file
     */
    void do_run(bool use_buffer);
    
    /** @brief Initialize and start a new LAMMPS instance */
    void start_lammps();
    
    /** @brief Handle completion of a LAMMPS run */
    void run_done();
    
    /** @brief Set the documentation version string for help links */
    void setDocver();
    
    /** @brief Perform an auto-save of the current file */
    void autoSave();
    
    /**
     * @brief Update the editor font
     * @param newfont The font to apply to the editor
     */
    void setFont(const QFont &newfont);
    
    /**
     * @brief Create an introduction page for a tutorial
     * @param ntutorial Tutorial number
     * @param infotext Information text to display
     * @return Wizard page with tutorial introduction
     */
    QWizardPage *tutorial_intro(const int ntutorial, const QString &infotext);
    
    /**
     * @brief Create a directory selection page for a tutorial
     * @param ntutorial Tutorial number
     * @return Wizard page for tutorial directory selection
     */
    QWizardPage *tutorial_directory(const int ntutorial);
    
    /**
     * @brief Set up files and resources for a tutorial
     * @param tutno Tutorial number
     * @param dir Directory to create files in
     * @param purgedir Whether to clean the directory first
     * @param getsolution Whether to include solution files
     * @param openwebpage Whether to open the tutorial web page
     */
    void setup_tutorial(int tutno, const QString &dir, bool purgedir, bool getsolution,
                        bool openwebpage);
    
    /** @brief Clean up the inspect file dialog list */
    void purge_inspect_list();
    
    /**
     * @brief Event filter for handling special events
     * @param watched Object being watched
     * @param event Event to filter
     * @return true if event was handled
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

public slots:
    /** @brief Quit the application */
    void quit();
    
    /** @brief Stop a running LAMMPS simulation */
    void stop_run();

private slots:
    /** @brief Create a new document */
    void new_document();
    
    /** @brief Open an existing file */
    void open();
    
    /** @brief View a file in read-only mode */
    void view();
    
    /** @brief Inspect a data file */
    void inspect();
    
    /** @brief Open a file from the recent files list */
    void open_recent();
    
    /** @brief Change the working directory */
    void get_directory();
    
    /** @brief Start external executable */
    void start_exe();
    
    /** @brief Save the current file */
    void save();
    
    /** @brief Save the current file with a new name */
    void save_as();
    
    /** @brief Copy selected text to clipboard */
    void copy();
    
    /** @brief Cut selected text to clipboard */
    void cut();
    
    /** @brief Paste text from clipboard */
    void paste();
    
    /** @brief Undo last edit action */
    void undo();
    
    /** @brief Redo previously undone action */
    void redo();
    
    /** @brief Open find and replace dialog */
    void findandreplace();
    
    /** @brief Run LAMMPS with content from editor buffer */
    void run_buffer() { do_run(true); }
    
    /** @brief Run LAMMPS from saved file */
    void run_file() { do_run(false); }
    
    /** @brief Restart LAMMPS with a new instance */
    void restart_lammps();

    /** @brief Open dialog to edit index-style variables */
    void edit_variables();
    
    /** @brief Render an image from a dump file */
    void render_image();
    
    /** @brief View a slideshow of images */
    void view_slides();
    
    /** @brief View a single image */
    void view_image();
    
    /** @brief View a chart from thermodynamic data */
    void view_chart();
    
    /** @brief View the log window */
    void view_log();
    
    /** @brief View current variable definitions */
    void view_variables();
    
    /** @brief Show about dialog */
    void about();
    
    /** @brief Show context-sensitive help */
    void help();
    
    /** @brief Open LAMMPS manual */
    void manual();
    
    /** @brief Open tutorial web page */
    void tutorial_web();
    
    /** @brief Start tutorial 1 */
    void start_tutorial1();
    
    /** @brief Start tutorial 2 */
    void start_tutorial2();
    
    /** @brief Start tutorial 3 */
    void start_tutorial3();
    
    /** @brief Start tutorial 4 */
    void start_tutorial4();
    
    /** @brief Start tutorial 5 */
    void start_tutorial5();
    
    /** @brief Start tutorial 6 */
    void start_tutorial6();
    
    /** @brief Start tutorial 7 */
    void start_tutorial7();
    
    /** @brief Start tutorial 8 */
    void start_tutorial8();
    
    /** @brief Open HOWTO documentation */
    void howto();
    
    /** @brief Update log window with new output */
    void logupdate();
    
    /** @brief Handle document modification */
    void modified();
    
    /** @brief Open preferences dialog */
    void preferences();
    
    /** @brief Reset settings to defaults */
    void defaults();

protected:
    Ui::LammpsGui *ui; ///< UI components generated from .ui file

private:
    Highlighter *highlighter;        ///< Syntax highlighter for LAMMPS input
    StdCapture *capturer;            ///< Captures stdout/stderr from LAMMPS
    QLabel *status;                  ///< Status bar label for general status
    QLabel *cpuuse;                  ///< Status bar label for CPU usage
    LogWindow *logwindow;            ///< Window displaying LAMMPS output log
    ImageViewer *imagewindow;        ///< Window for viewing single images
    ChartWindow *chartwindow;        ///< Window for displaying charts
    SlideShow *slideshow;            ///< Window for image slideshow
    QTimer *logupdater;              ///< Timer for periodic log updates
    QLabel *dirstatus;               ///< Status bar label showing current directory
    QProgressBar *progress;          ///< Progress bar for long operations
    Preferences *prefdialog;         ///< Preferences dialog
    QLabel *lammpsstatus;            ///< Status bar label for LAMMPS state
    QLabel *varwindow;               ///< Window showing variable definitions
    QWizard *wizard;                 ///< Tutorial wizard dialog

    /**
     * @brief Container for inspect dialog widgets
     * 
     * Holds references to the three tabs (info, data, image) in an inspect dialog
     */
    struct InspectData {
        QWidget *info;   ///< Information tab widget
        QWidget *data;   ///< Data viewing tab widget
        QWidget *image;  ///< Image rendering tab widget
    };
    QList<InspectData *> inspectList; ///< List of open inspect dialogs

    QString current_file;                       ///< Path to currently opened file
    QString current_dir;                        ///< Current working directory
    QList<QString> recent;                      ///< List of recently opened files
    QList<QPair<QString, QString>> variables;   ///< Index-style variable definitions

    LammpsWrapper lammps;            ///< Interface to LAMMPS library
    LammpsRunner *runner;            ///< Thread for running LAMMPS simulations
    QString docver;                  ///< LAMMPS documentation version string
    QString plugin_path;             ///< Path to LAMMPS shared library (plugin mode)
    bool is_running;                 ///< Whether a simulation is currently running
    int run_counter;                 ///< Counter for simulation runs
    std::vector<char *> lammps_args; ///< Command-line arguments for LAMMPS

protected:
    int nthreads; ///< Number of threads for parallel execution
};

/**
 * @brief Wizard dialog for interactive LAMMPS tutorials
 * 
 * TutorialWizard provides a step-by-step wizard interface for setting up
 * and running LAMMPS tutorials. It guides users through directory selection,
 * file preparation, and launching tutorial exercises.
 */
class TutorialWizard : public QWizard {
    Q_OBJECT

public:
    /**
     * @brief Construct a tutorial wizard
     * @param ntutorial Tutorial number (1-8)
     * @param parent Parent widget
     */
    TutorialWizard(int ntutorial, QWidget *parent = nullptr);
    
    /**
     * @brief Accept the wizard and set up the tutorial
     * 
     * Called when the user completes the wizard. Sets up tutorial files
     * and opens the tutorial in the main window.
     */
    void accept() override;

private:
    int _ntutorial; ///< Tutorial number identifier
};
#endif // LAMMPSGUI_H

// Local Variables:
// c-basic-offset: 4
// End:
