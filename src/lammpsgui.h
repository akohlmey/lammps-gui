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

#ifndef LAMMPSGUI_H
#define LAMMPSGUI_H

#include <QMainWindow>

#include <QList>
#include <QPair>
#include <QString>
#include <string>
#include <vector>

#include "constants.h"
#include "lammpswrapper.h"

// identifier for LAMMPS restart files
#if !defined(LAMMPS_MAGIC)
#define LAMMPS_MAGIC "LammpS RestartT"
#endif

// forward declarations

class QAction;
class QEvent;
class QFont;
class QLabel;
class QMenu;
class QMenuBar;
class QPlainTextEdit;
class QProgressBar;
class QStatusBar;
class QTimer;
class QWidget;
class QWizardPage;

class ChartWindow;
class CodeEditor;
class GeneralTab;
class Highlighter;
class ImageViewer;
class LammpsRunner;
class LogWindow;
class Preferences;
class SlideShow;
class StdCapture;
class TutorialWizard;

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
 * - Tutorial wizard for interactive LAMMPS learning
 *
 * The class integrates with Qt's main window framework and provides menu actions,
 * toolbars, and status bar components. It uses a LammpsWrapper to interface with
 * the LAMMPS library and LammpsRunner to execute simulations in a separate thread.
 *
 * @see CodeEditor for the text editor component
 * @see ChartWindow for the charts window component
 * @see LogWindow for the log output window component
 * @see ImageViewer for the snapshot image window component
 * @see SlideShow for the slide show viewer window component
 * @see Preferences for the preferences window component
 * @see LammpsRunner for simulation execution in a separate thread
 * @see LammpsWrapper for LAMMPS library interface
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
     * @param parent    Parent widget (typically nullptr for main window)
     * @param filename  Optional file to open on startup
     * @param width     Optional main editor window width override
     * @param height    Optional main editor window height override
     *
     * Initializes the main window, sets up the UI components, loads preferences,
     * initializes the LAMMPS library, and optionally opens a file if provided.
     */
    LammpsGui(QWidget *parent = nullptr, const QString &filename = QString(), int width = 0,
              int height = 0);

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
    void openFile(const QString &filename);

    /** @brief Open a file in a read-only viewer dialog */
    void viewFile(const QString &filename);

    /** @brief Open a file for inspection (data files, etc.) */
    void inspectFile(const QString &filename);

    /** @brief Write current editor content to a file */
    void writeFile(const QString &filename);

    /** @brief Update the recent files list */
    void updateRecents(const QString &filename = "");

    /** @brief Clear the list of index-style variables */
    void clearVariables();

    /** @brief Update variables in LAMMPS from the variables list */
    void updateVariables();

    /**
     * @brief Execute a LAMMPS simulation
     * @param use_buffer If true, runs from editor buffer; if false, saves and runs from file
     */
    void doRun(bool use_buffer);

    /** @brief Initialize and start a new LAMMPS instance */
    void startLammps();

    /** @brief Handle completion of a LAMMPS run */
    void runDone();

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
    QWizardPage *tutorialIntro(const int ntutorial, const QString &infotext);

    /**
     * @brief Create a directory selection page for a tutorial
     * @param ntutorial Tutorial number
     * @return Wizard page for tutorial directory selection
     */
    QWizardPage *tutorialDirectory(const int ntutorial);

    /**
     * @brief Set up files and resources for a tutorial
     * @param tutno Tutorial number
     * @param dir Directory to create files in
     * @param purgedir Whether to clean the directory first
     * @param getsolution Whether to include solution files
     * @param openwebpage Whether to open the tutorial web page
     */
    void setupTutorial(int tutno, const QString &dir, bool purgedir, bool getsolution,
                       bool openwebpage);

    /** @brief Clean up the inspect file dialog list */
    void purgeInspectList();

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
    void stopRun();

private slots:
    /** @brief Create a new document */
    void newDocument();

    /** @brief Open an existing file */
    void open();

    /** @brief View a file in read-only mode */
    void view();

    /** @brief Inspect a data file */
    void inspect();

    /** @brief Open a file from the recent files list */
    void openRecent();

    /** @brief Change the working directory */
    void getDirectory();

    /** @brief Start external executable */
    void startExe();

    /** @brief Save the current file */
    void save();

    /** @brief Save the current file with a new name */
    void saveAs();

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
    void findAndReplace();

    /** @brief Run LAMMPS with content from editor buffer */
    void runBuffer() { doRun(true); }

    /** @brief Run LAMMPS from saved file */
    void runFile() { doRun(false); }

    /** @brief Restart LAMMPS with a new instance */
    void restartLammps();

    /** @brief Open dialog to edit index-style variables */
    void editVariables();

    /** @brief Render an image from a dump file */
    void renderImage();

    /** @brief View a slideshow of images */
    void viewSlides();

    /** @brief View a single image */
    void viewImage();

    /** @brief View a chart from thermodynamic data */
    void viewChart();

    /** @brief View the log window */
    void viewLog();

    /** @brief View current variable definitions */
    void viewVariables();

    /** @brief Show about dialog */
    void about();

    /** @brief Show context-sensitive help */
    void help();

    /** @brief Open LAMMPS manual */
    void manual();

    /** @brief Open tutorial web page */
    void tutorialWeb();

    /**
     * @brief Start a tutorial wizard
     * @param tutno Tutorial number (1-8)
     */
    void startTutorial(int tutno);

    /** @brief Open HOWTO documentation */
    void howto();

    /** @brief Update log window with new output */
    void logUpdate();

    /** @brief Handle document modification */
    void modified();

    /** @brief Open preferences dialog */
    void preferences();

    /** @brief Reset settings to defaults */
    void defaults();

private:
    /** @brief Create all menu actions, menus, and status bar */
    void setupUi();

    /** @brief Create File menu actions and add them to the menu bar */
    void createFileMenu();

    /** @brief Create Edit menu actions and add them to the menu bar */
    void createEditMenu();

    /** @brief Create Run menu actions and add them to the menu bar */
    void createRunMenu();

    /** @brief Create View menu actions and add them to the menu bar */
    void createViewMenu();

    /** @brief Create Tutorials menu with tutorial actions */
    void createTutorialMenu();

    /** @brief Create About/Help menu actions and add them to the menu bar */
    void createAboutMenu();

    /** @brief Create the status bar and its widgets */
    void createStatusBar();

    /** @brief Connect all actions to their respective slots */
    void connectSignalsAndSlots();

    /**
     * @brief Configure a sub-window with standard icon, minimum size, and shortcuts
     * @param window       The widget to configure
     * @param windowTitle  Title to set on the window
     *
     * Sets the window icon to the standard LAMMPS-GUI icon, sets the minimum size
     * to MINIMUM_WIDTH x MINIMUM_HEIGHT, and adds Ctrl+W (close) and Ctrl+/ (stop run)
     * keyboard shortcuts.
     */
    void configureSubWindow(QWidget *window, const QString &windowTitle);

    // Central widget
    CodeEditor *textEdit;

    // Menu bar and menus
    QMenuBar *menubar;
    QMenu *menuFile;
    QMenu *menuEdit;
    QMenu *menuRun;
    QMenu *menuView;
    QMenu *menuTutorial;
    QMenu *menuAbout;
    QStatusBar *statusbar;

    // Actions - File menu
    QAction *actionNew;
    QAction *actionOpen;
    QAction *actionView;
    QAction *actionInspect;
    QAction *actionSave;
    QAction *actionSaveAs;
    QAction *actionQuit;
    QAction *recentActions[GuiConstants::NUM_RECENT_FILES]; ///< Recent file actions

    // Actions - Edit menu
    QAction *actionUndo;
    QAction *actionRedo;
    QAction *actionCopy;
    QAction *actionCut;
    QAction *actionPaste;
    QAction *actionSearchAndReplace;
    QAction *actionPreferences;
    QAction *actionDefaults;

    // Actions - Run menu
    QAction *actionRunBuffer;
    QAction *actionRunFile;
    QAction *actionStopLAMMPS;
    QAction *actionRestartLAMMPS;
    QAction *actionSetVariables;
    QAction *actionImage;
    QAction *actionViewInOVITO;
    QAction *actionViewInVMD;

    // Actions - View menu
    QAction *actionViewLogWindow;
    QAction *actionViewGraphWindow;
    QAction *actionViewImageWindow;
    QAction *actionViewSlideShow;
    QAction *actionViewVariableWindow;

    // Actions - Tutorials menu
    QAction *tutorialActions[GuiConstants::NUM_TUTORIALS]; ///< Tutorial 1-8

    // Actions - About menu
    QAction *actionAboutLAMMPSGUI;
    QAction *actionHelp;
    QAction *actionHowto;
    QAction *actionLAMMPSManual;
    QAction *actionLAMMPSTutorial;

    Highlighter *highlighter; ///< Syntax highlighter for LAMMPS input
    StdCapture *capturer;     ///< Captures stdout/stderr from LAMMPS
    QLabel *status;           ///< Status bar label for general status
    QLabel *cpuuse;           ///< Status bar label for CPU usage
    LogWindow *logwindow;     ///< Window displaying LAMMPS output log
    ImageViewer *imagewindow; ///< Window for viewing single images
    ChartWindow *chartwindow; ///< Window for displaying charts
    SlideShow *slideshow;     ///< Window for image slideshow
    QTimer *logupdater;       ///< Timer for periodic log updates
    QLabel *dirstatus;        ///< Status bar label showing current directory
    QProgressBar *progress;   ///< Progress bar for long operations
    Preferences *prefdialog;  ///< Preferences dialog
    QLabel *lammpsstatus;     ///< Status bar label for LAMMPS state
    QLabel *varwindow;        ///< Window showing variable definitions
    TutorialWizard *wizard;   ///< Tutorial wizard dialog

    /**
     * @brief Container for inspect dialog widgets
     *
     * Holds references to the three tabs (info, data, image) in an inspect dialog
     */
    struct InspectData {
        QWidget *info;  ///< Information tab widget
        QWidget *data;  ///< Data viewing tab widget
        QWidget *image; ///< Image rendering tab widget
    };
    QList<InspectData *> inspectList; ///< List of open inspect dialogs

    QString currentFile;                      ///< Path to currently opened file
    QString currentDir;                       ///< Current working directory
    QList<QString> recent;                    ///< List of recently opened files
    QList<QPair<QString, QString>> variables; ///< Index-style variable definitions

    LammpsWrapper lammps;                ///< Interface to LAMMPS library
    LammpsRunner *runner;                ///< Thread for running LAMMPS simulations
    QString docver;                      ///< LAMMPS documentation version string
    QString pluginPath;                  ///< Path to LAMMPS shared library (plugin mode)
    bool isRunning;                      ///< Whether a simulation is currently running
    int runCounter;                      ///< Counter for simulation runs
    std::vector<std::string> lammpsArgs; ///< Command-line arguments for LAMMPS

protected:
    int nthreads; ///< Number of threads for parallel execution
    int mainx;    ///< Override value for main editor window width or 0
    int mainy;    ///< Override value for main editor window height or 0
};

#endif // LAMMPSGUI_H

// Local Variables:
// c-basic-offset: 4
// End:
