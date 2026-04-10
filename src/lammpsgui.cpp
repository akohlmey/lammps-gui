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

#include "lammpsgui.h"

#include "aboutdialog.h"
#include "chartviewer.h"
#include "codeeditor.h"
#include "fileviewer.h"
#include "findandreplace.h"
#include "helpers.h"
#include "highlighter.h"
#include "imageviewer.h"
#include "lammpsrunner.h"
#include "logwindow.h"
#include "preferences.h"
#include "setvariables.h"
#include "slideshow.h"
#include "stdcapture.h"

#include <QAction>
#include <QByteArray>
#include <QCheckBox>
#include <QClipboard>
#include <QCoreApplication>
#include <QDataStream>
#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontInfo>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QShortcut>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QWizard>
#include <QWizardPage>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <process.h>
#define execl(exe, arg0, arg1) _execl(exe, arg0, arg1)
#else
#include <unistd.h>
#endif

namespace {
constexpr int DEFAULT_BUFLEN      = 1024;
constexpr int MAX_DEFAULT_THREADS = 16;
constexpr int MINIMUM_WIDTH       = 400;
constexpr int MINIMUM_HEIGHT      = 300;

const QString blank(" ");
const QString citeme("# When using LAMMPS-GUI in your project, please cite: "
                     "https://doi.org/10.33011/livecoms.6.1.3037\n");
const QString bannerstyle("CodeEditor {background-position: center center; "
                          "padding: 0px; "
                          "background-repeat: no-repeat; "
                          "background-image: url(:/icons/lammps-gui-banner.png);}");
} // namespace

void LammpsGui::setupUi()
{
    setWindowTitle("LAMMPS-GUI");
    resize(600, 400);

    // Central widget
    textEdit = new CodeEditor(this);
    textEdit->setEnabled(true);
    textEdit->setAcceptDrops(true);
    setCentralWidget(textEdit);

    // File actions
    actionNew = new QAction(QIcon(":/icons/document-new.png"), "&New", this);
    actionNew->setShortcut(QKeySequence("Ctrl+N"));

    actionOpen = new QAction(QIcon(":/icons/document-open.png"), "&Open", this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));

    actionView = new QAction(QIcon(":/icons/document-open.png"), "&View", this);
    actionView->setShortcut(QKeySequence("Ctrl+Shift+F"));

    actionInspect = new QAction(QIcon(":/icons/document-open.png"), "Inspect &Restart", this);
    actionInspect->setShortcut(QKeySequence("Ctrl+Shift+R"));

    actionSave = new QAction(QIcon(":/icons/document-save.png"), "&Save", this);
    actionSave->setShortcut(QKeySequence("Ctrl+S"));

    actionSaveAs = new QAction(QIcon(":/icons/document-save-as.png"), "Save &As", this);
    actionSaveAs->setShortcut(QKeySequence("Ctrl+Shift+S"));

    actionQuit = new QAction(QIcon(":/icons/application-exit.png"), "&Quit", this);
    actionQuit->setShortcut(QKeySequence("Ctrl+Q"));

    for (int i = 0; i < 5; ++i) {
        recentActions[i] =
            new QAction(QIcon(":/icons/document-open-recent.png"),
                        QString("&%1.").arg(i + 1), this);
    }

    // Edit actions
    actionUndo = new QAction(QIcon(":/icons/edit-undo.png"), "&Undo", this);
    actionUndo->setShortcut(QKeySequence("Ctrl+Z"));

    actionRedo = new QAction(QIcon(":/icons/edit-redo.png"), "&Redo", this);
    actionRedo->setShortcut(QKeySequence("Ctrl+Shift+Z"));

    actionCopy = new QAction(QIcon(":/icons/edit-copy.png"), "&Copy", this);
    actionCopy->setShortcut(QKeySequence("Ctrl+C"));

    actionCut = new QAction(QIcon(":/icons/edit-cut.png"), "Cu&t", this);
    actionCut->setShortcut(QKeySequence("Ctrl+X"));

    actionPaste = new QAction(QIcon(":/icons/edit-paste.png"), "&Paste", this);
    actionPaste->setShortcut(QKeySequence("Ctrl+V"));

    actionSearchAndReplace =
        new QAction(QIcon(":/icons/search.png"), "&Find and Replace...", this);
    actionSearchAndReplace->setShortcut(QKeySequence("Ctrl+F"));

    actionPreferences =
        new QAction(QIcon(":/icons/preferences-desktop.png"), "P&references...", this);
    actionPreferences->setShortcut(QKeySequence("Ctrl+P"));

    actionDefaults =
        new QAction(QIcon(":/icons/document-revert.png"), "Reset Preferences to &Defaults", this);

    // Run actions
    actionRunBuffer =
        new QAction(QIcon(":/icons/system-run.png"), "&Run LAMMPS from Editor Buffer", this);
    actionRunBuffer->setShortcut(QKeySequence("Ctrl+Return"));

    actionRunFile =
        new QAction(QIcon(":/icons/run-file.png"), "Run LAMMPS from &File", this);
    actionRunFile->setShortcut(QKeySequence("Ctrl+Shift+Return"));

    actionStopLAMMPS =
        new QAction(QIcon(":/icons/process-stop.png"), "&Stop LAMMPS", this);
    actionStopLAMMPS->setShortcut(QKeySequence("Ctrl+/"));

    actionRestartLAMMPS =
        new QAction(QIcon(":/icons/system-restart.png"), "Relaunch &LAMMPS Instance", this);

    actionSetVariables = new QAction(QIcon(":/icons/preferences-desktop-personal.png"),
                                     "Set &Variables...", this);
    actionSetVariables->setShortcut(QKeySequence("Ctrl+Shift+V"));

    actionImage = new QAction(QIcon(":/icons/emblem-photos.png"), "Create &Image", this);
    actionImage->setShortcut(QKeySequence("Ctrl+I"));

    actionViewInOVITO = new QAction(QIcon(":/icons/ovito.png"), "View in &OVITO", this);
    actionViewInOVITO->setShortcut(QKeySequence("Ctrl+Shift+O"));

    actionViewInVMD = new QAction(QIcon(":/icons/vmd.png"), "View in VM&D", this);
    actionViewInVMD->setShortcut(QKeySequence("Ctrl+Shift+D"));

    // View actions
    actionViewLogWindow =
        new QAction(QIcon(":/icons/utilities-terminal.png"), "&Output Window", this);
    actionViewLogWindow->setShortcut(QKeySequence("Ctrl+Shift+L"));

    actionViewGraphWindow =
        new QAction(QIcon(":/icons/x-office-drawing.png"), "&Charts Window", this);
    actionViewGraphWindow->setShortcut(QKeySequence("Ctrl+Shift+C"));

    actionViewImageWindow =
        new QAction(QIcon(":/icons/emblem-photos.png"), "&Image Window", this);
    actionViewImageWindow->setShortcut(QKeySequence("Ctrl+Shift+I"));

    actionViewSlideShow =
        new QAction(QIcon(":/icons/image-x-generic.png"), "&Slide Show Window", this);
    actionViewSlideShow->setShortcut(QKeySequence("Ctrl+L"));

    actionViewVariableWindow =
        new QAction(QIcon(":/icons/preferences-desktop-personal.png"), "&Variables Window", this);
    actionViewVariableWindow->setShortcut(QKeySequence("Ctrl+Shift+W"));

    // Tutorial actions
    for (int i = 0; i < 8; ++i) {
        tutorialActions[i] =
            new QAction(QIcon(":/icons/tutorial-logo.png"),
                        QString("Start LAMMPS Tutorial &%1").arg(i + 1), this);
    }

    // About actions
    actionAboutLAMMPSGUI =
        new QAction(QIcon(":/icons/help-about.png"), "&About LAMMPS-GUI", this);
    actionAboutLAMMPSGUI->setShortcut(QKeySequence("Ctrl+Shift+A"));

    actionHelp = new QAction(QIcon(":/icons/help-faq.png"), "Quick &Help", this);
    actionHelp->setShortcut(QKeySequence("Ctrl+Shift+H"));

    actionHowto =
        new QAction(QIcon(":/icons/system-help.png"), "LAMMPS-&GUI Documentation", this);
    actionHowto->setShortcut(QKeySequence("Ctrl+Shift+G"));

    actionLAMMPSManual =
        new QAction(QIcon(":/icons/help-browser.png"), "LAMMPS Online &Manual", this);
    actionLAMMPSManual->setShortcut(QKeySequence("Ctrl+Shift+M"));

    actionLAMMPSTutorial =
        new QAction(QIcon(":/icons/help-tutorial.png"), "LAMMPS &Tutorial Website", this);
    actionLAMMPSTutorial->setShortcut(QKeySequence("Ctrl+Shift+T"));

    // Menu bar
    menubar = new QMenuBar(this);
    setMenuBar(menubar);

    // File menu
    menuFile = menubar->addMenu("&File");
    menuFile->addAction(actionNew);
    menuFile->addSeparator();
    menuFile->addAction(actionOpen);
    menuFile->addAction(actionView);
    menuFile->addAction(actionInspect);
    menuFile->addSeparator();
    for (int i = 0; i < 5; ++i)
        menuFile->addAction(recentActions[i]);
    menuFile->addSeparator();
    menuFile->addAction(actionSave);
    menuFile->addAction(actionSaveAs);
    menuFile->addSeparator();
    menuFile->addAction(actionQuit);

    // Edit menu
    menuEdit = menubar->addMenu("&Edit");
    menuEdit->addAction(actionUndo);
    menuEdit->addAction(actionRedo);
    menuEdit->addSeparator();
    menuEdit->addAction(actionCopy);
    menuEdit->addAction(actionCut);
    menuEdit->addAction(actionPaste);
    menuEdit->addSeparator();
    menuEdit->addAction(actionSearchAndReplace);
    menuEdit->addSeparator();
    menuEdit->addAction(actionPreferences);
    menuEdit->addAction(actionDefaults);

    // Run menu
    menuRun = menubar->addMenu("&Run");
    menuRun->addAction(actionRunBuffer);
    menuRun->addAction(actionRunFile);
    menuRun->addAction(actionStopLAMMPS);
    menuRun->addSeparator();
    menuRun->addAction(actionRestartLAMMPS);
    menuRun->addSeparator();
    menuRun->addAction(actionSetVariables);
    menuRun->addSeparator();
    menuRun->addAction(actionImage);
    menuRun->addAction(actionViewInOVITO);
    menuRun->addAction(actionViewInVMD);

    // View menu
    menuView = menubar->addMenu("&View");
    menuView->addAction(actionViewLogWindow);
    menuView->addAction(actionViewGraphWindow);
    menuView->addAction(actionViewImageWindow);
    menuView->addAction(actionViewSlideShow);
    menuView->addAction(actionViewVariableWindow);

    // Tutorials menu
    menuTutorial = menubar->addMenu("&Tutorials");
    for (int i = 0; i < 8; ++i)
        menuTutorial->addAction(tutorialActions[i]);

    // About menu
    menuAbout = menubar->addMenu("&About");
    menuAbout->addAction(actionAboutLAMMPSGUI);
    menuAbout->addAction(actionHelp);
    menuAbout->addAction(actionHowto);
    menuAbout->addAction(actionLAMMPSManual);
    menuAbout->addAction(actionLAMMPSTutorial);

    // Status bar
    statusbar = new QStatusBar(this);
    setStatusBar(statusbar);
}

LammpsGui::LammpsGui(QWidget *parent, const QString &filename, int width, int height) :
    QMainWindow(parent), highlighter(nullptr), capturer(nullptr),
    status(nullptr), cpuuse(nullptr), logwindow(nullptr), imagewindow(nullptr),
    chartwindow(nullptr), slideshow(nullptr), logupdater(nullptr), dirstatus(nullptr),
    progress(nullptr), prefdialog(nullptr), lammpsstatus(nullptr), varwindow(nullptr),
    wizard(nullptr), runner(nullptr), is_running(false), run_counter(0), nthreads(1), mainx(width),
    mainy(height)
{
    docver = "";
    setupUi();
    textEdit->document()->setPlainText(citeme);
    textEdit->document()->setModified(false);
    textEdit->setStyleSheet(bannerstyle);
    highlighter = new Highlighter(textEdit->document());
    capturer    = new StdCapture;
    current_file.clear();
    current_dir = QDir(".").absolutePath();
    // use $HOME if we get dropped to "/" like on macOS or the installation folder or
    // system folder like on Windows
    if ((current_dir == "/") || (current_dir.contains("AppData")) ||
        (current_dir.contains("system32")))
        current_dir = QDir::homePath();
    QDir::setCurrent(current_dir);

    inspectList.clear();
    setAutoFillBackground(true);

    // restore and initialize settings
    QSettings settings;

#if defined(LAMMPS_GUI_USE_PLUGIN)
    // first try to load from existing setting
    plugin_path = settings.value("plugin_path", "").toString();
    if (!plugin_path.isEmpty()) {
        // make canonical and try loading; reset to empty string if loading failed
        plugin_path = QFileInfo(plugin_path).canonicalFilePath();
        if (!lammps.load_lib(plugin_path)) {
            plugin_path.clear();
            // could not load successfully -> remove any existing setting.
            settings.remove("plugin_path");
        }
    }

    if (plugin_path.isEmpty()) {
        // construct list of possible standard choices for the shared library file
        // we prefer the current directory, then the dynamic library path, then some system folders
        // adapt file pattern and paths to the different operating systems
        QStringList dirlist{"."};
#if defined(Q_OS_MACOS)
        QStringList filter("liblammps*.dylib");
        dirlist.append(
            QString::fromLocal8Bit(qgetenv("DYLD_LIBRARY_PATH")).split(":", Qt::SkipEmptyParts));
        // library may be included in an application bundle:
        dirlist.append({"/Applications/LAMMPS-GUI.app/Contents/Frameworks",
                        "/Applications/LAMMPS.app/Contents/Frameworks"});
#elif defined(Q_OS_WIN32)
        QStringList filter("liblammps*.dll");
        dirlist.append(QString::fromLocal8Bit(qgetenv("PATH")).split(";", Qt::SkipEmptyParts));
#else
        // for Linux and other unix-like systems
        QStringList filter("liblammps*.so*");
        dirlist.append(
            QString::fromLocal8Bit(qgetenv("LD_LIBRARY_PATH")).split(":", Qt::SkipEmptyParts));
#endif
        dirlist.append({"/usr/lib", "/usr/lib64", "/usr/local/lib", "/usr/local/lib64"});

        // construct list of matching files
        QFileInfoList entries;
        for (const auto &dir : dirlist)
            entries.append(QDir(dir).entryInfoList(filter));

        // convert list of paths to list of canonical file names
        QStringList choices;
        for (const auto &fn : entries)
            choices.append(fn.canonicalFilePath());
        choices.removeDuplicates();
        for (const auto &libpath : choices) {
            if (lammps.load_lib(libpath)) {
                plugin_path = libpath;
                settings.setValue("plugin_path", plugin_path);
                settings.sync();
                break;
            }
        }

        // plugin path has been reset. Open file browser to select a file interactively.
        if (plugin_path.isEmpty()) {
#if defined(Q_OS_MACOS)
            const QString pattern = "LAMMPS shared library (liblammps*.dylib)";
#elif defined(Q_OS_WIN32)
            const QString pattern = "LAMMPS shared library (liblammps*.dll)";
#else
            const QString pattern = "LAMMPS shared library (liblammps*.so*)";
#endif
            QString pluginfile = QFileDialog::getOpenFileName(
                this, "Select LAMMPS shared library to use", ".", pattern, nullptr,
                QFileDialog::DontResolveSymlinks | QFileDialog::ReadOnly);
            if (!pluginfile.isEmpty() && pluginfile.contains("liblammps", Qt::CaseSensitive)) {
                auto canonical = QFileInfo(pluginfile).canonicalFilePath();
                settings.setValue("plugin_path", canonical);
                settings.sync();
                // must re-launch LAMMPS-GUI to cleanly load the selected new plugin
                // without overlaps from other load attempts
                const char *path = mystrdup(QCoreApplication::applicationFilePath());
                const char *arg0 = mystrdup(QCoreApplication::arguments().at(0));
                execl(path, arg0, (char *)nullptr);
                critical(this, "LAMMPS-GUI Error", "Relaunching LAMMPS-GUI failed.",
                         "LAMMPS-GUI must be restarted to correctly load the selected "
                         "LAMMPS shared library. Click on 'Close' to exit.");
                exit(1);
            }
        }

        // plugin_path was not changed interactively and not suitable plugin exists.
        // print warning dialog and exit.
        if (plugin_path.isEmpty()) {
            // remove key so we won't get stuck in a loop reading a bad file
            settings.remove("plugin_path");
            critical(this, "LAMMPS-GUI Error", "No suitable LAMMPS shared library file loaded.",
                     "<p align=\"justify\">Either no LAMMPS shared library file has been "
                     "selected, or no compatible LAMMPS shared library file path has been "
                     "provided, or the provided path has a file with an incompatible LAMMPS "
                     "version, or some dependent shared library files are not found.</p>"
                     "<p align=\"justify\">Please try again and either use the -p command line "
                     "flag to specify a path to a suitable LAMMPS shared library file or select "
                     "one from the file browser dialog.");
            exit(1);
        }
    }
#endif

    // default accelerator package is OPENMP, but we switch the configured accelerator to
    // "none" if the selected package is not available to have an option that always works
    int accel = settings.value("accelerator", AcceleratorTab::OpenMP).toInt();
    switch (accel) {
        case AcceleratorTab::Opt:
            if (!lammps.config_has_package("OPT")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::OpenMP:
            if (!lammps.config_has_package("OPENMP")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Intel:
            if (!lammps.config_has_package("INTEL")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Gpu:
            if (!lammps.config_has_package("GPU")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Kokkos:
            if (!lammps.config_has_package("KOKKOS")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::None: // fallthrough
        default:                   // do nothing
            break;
    }
    settings.setValue("accelerator", accel);

    // Check and initialize some settings for individual accelerator packages and commit
    // GPU neighbor list on GPU versus host
    bool gpuneigh = settings.value("gpuneigh", true).toBool();
    settings.setValue("gpuneigh", gpuneigh);
    // accelerate only pair style (i.e. run PPPM completely on host)
    bool gpupaironly = settings.value("gpupaironly", false).toBool();
    settings.setValue("gpupaironly", gpupaironly);
    // INTEL package precision
    int intelprec = settings.value("intelprec", AcceleratorTab::Mixed).toInt();
    settings.setValue("intelprec", intelprec);

    // Check and initialize nthreads setting for when OpenMP support is compiled in.
    // Default is to use OMP_NUM_THREADS setting, if that is not available, thenhalf of max
    // (assuming hyper-threading is enabled) and no more than MAX_DEFAULT_THREADS (=16).
    // This is only if there is no preference set but do not override OMP_NUM_THREADS
    int default_threads = std::min(QThread::idealThreadCount() / 2, MAX_DEFAULT_THREADS);
    default_threads     = std::max(default_threads, 1);
    if (qEnvironmentVariableIsSet("OMP_NUM_THREADS"))
        default_threads = qEnvironmentVariable("OMP_NUM_THREADS").toInt();
    nthreads = settings.value("nthreads", default_threads).toInt();

    // reset nthreads if accelerator does not support threads
    if ((accel == AcceleratorTab::Opt) || (accel == AcceleratorTab::None)) nthreads = 1;

    // set OMP_NUM_THREADS environment variable, if not set
    if (!qEnvironmentVariableIsSet("OMP_NUM_THREADS"))
        qputenv("OMP_NUM_THREADS", QByteArray::number(nthreads));

    // set up default LAMMPS thread arguments
    lammps_args.clear();
    lammps_args.push_back(mystrdup("LAMMPS-GUI"));
    lammps_args.push_back(mystrdup("-log"));
    lammps_args.push_back(mystrdup("none"));

    installEventFilter(this);

    setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));

    QFont all_font;
    QFontInfo all_info(*GUI_ALLFONT);
    all_font.setFamily(settings.value("allfamily", all_info.family()).toString());
    all_font.setPointSize(settings.value("allsize", all_info.pointSize()).toInt());
    all_font.setStyleHint(GUI_ALLFONT->styleHint());
    settings.setValue("allfamily", all_font.family());
    settings.setValue("allsize", all_font.pointSize());
    setFont(all_font);

    QFont mono_font;
    QFontInfo mono_info(*GUI_MONOFONT);
    mono_font.setFamily(settings.value("monofamily", mono_info.family()).toString());
    mono_font.setPointSize(settings.value("monosize", mono_info.pointSize()).toInt());
    mono_font.setStyleHint(GUI_MONOFONT->styleHint());
    mono_font.setFixedPitch(true);
    settings.setValue("monofamily", mono_font.family());
    settings.setValue("monosize", mono_font.pointSize());
    textEdit->setFont(mono_font);
    textEdit->document()->setDefaultFont(mono_font);
    textEdit->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    settings.sync();

    varwindow = new QLabel(QString());
    varwindow->setWindowTitle(QString("LAMMPS-GUI - Current Variables"));
    varwindow->setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    varwindow->setMinimumSize(100, 50);
    varwindow->setText("(none)");
    varwindow->setFont(mono_font);
    varwindow->setFrameStyle(QFrame::Sunken);
    varwindow->setFrameShape(QFrame::Panel);
    varwindow->setAlignment(Qt::AlignVCenter);
    varwindow->setContentsMargins(5, 5, 5, 5);
    varwindow->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    varwindow->hide();

    update_recents();

    // check if we have OVITO and VMD installed and deactivate actions if not
    actionViewInOVITO->setEnabled(has_exe("ovito"));
    actionViewInOVITO->setData("ovito");
    actionViewInVMD->setEnabled(has_exe("vmd"));
    actionViewInVMD->setData("vmd");

    connect(actionNew, &QAction::triggered, this, &LammpsGui::new_document);
    connect(actionOpen, &QAction::triggered, this, &LammpsGui::open);
    connect(actionSave, &QAction::triggered, this, &LammpsGui::save);
    connect(actionSaveAs, &QAction::triggered, this, &LammpsGui::save_as);
    connect(actionView, &QAction::triggered, this, &LammpsGui::view);
    connect(actionInspect, &QAction::triggered, this, &LammpsGui::inspect);
    connect(actionQuit, &QAction::triggered, this, &LammpsGui::quit);
    connect(actionCopy, &QAction::triggered, this, &LammpsGui::copy);
    connect(actionCut, &QAction::triggered, this, &LammpsGui::cut);
    connect(actionPaste, &QAction::triggered, this, &LammpsGui::paste);
    connect(actionUndo, &QAction::triggered, this, &LammpsGui::undo);
    connect(actionRedo, &QAction::triggered, this, &LammpsGui::redo);
    connect(actionSearchAndReplace, &QAction::triggered, this, &LammpsGui::findandreplace);
    connect(actionRunBuffer, &QAction::triggered, this, &LammpsGui::run_buffer);
    connect(actionRunFile, &QAction::triggered, this, &LammpsGui::run_file);
    connect(actionStopLAMMPS, &QAction::triggered, this, &LammpsGui::stop_run);
    connect(actionRestartLAMMPS, &QAction::triggered, this, &LammpsGui::restart_lammps);
    connect(actionSetVariables, &QAction::triggered, this, &LammpsGui::edit_variables);
    connect(actionImage, &QAction::triggered, this, &LammpsGui::render_image);
    connect(actionLAMMPSTutorial, &QAction::triggered, this, &LammpsGui::tutorial_web);
    connect(tutorialActions[0], &QAction::triggered, this, &LammpsGui::start_tutorial1);
    connect(tutorialActions[1], &QAction::triggered, this, &LammpsGui::start_tutorial2);
    connect(tutorialActions[2], &QAction::triggered, this, &LammpsGui::start_tutorial3);
    connect(tutorialActions[3], &QAction::triggered, this, &LammpsGui::start_tutorial4);
    connect(tutorialActions[4], &QAction::triggered, this, &LammpsGui::start_tutorial5);
    connect(tutorialActions[5], &QAction::triggered, this, &LammpsGui::start_tutorial6);
    connect(tutorialActions[6], &QAction::triggered, this, &LammpsGui::start_tutorial7);
    connect(tutorialActions[7], &QAction::triggered, this, &LammpsGui::start_tutorial8);
    connect(actionAboutLAMMPSGUI, &QAction::triggered, this, &LammpsGui::about);
    connect(actionHelp, &QAction::triggered, this, &LammpsGui::help);
    connect(actionHowto, &QAction::triggered, this, &LammpsGui::howto);
    connect(actionLAMMPSManual, &QAction::triggered, this, &LammpsGui::manual);
    connect(actionPreferences, &QAction::triggered, this, &LammpsGui::preferences);
    connect(actionDefaults, &QAction::triggered, this, &LammpsGui::defaults);
    connect(actionViewInOVITO, &QAction::triggered, this, &LammpsGui::start_exe);
    connect(actionViewInVMD, &QAction::triggered, this, &LammpsGui::start_exe);
    connect(actionViewLogWindow, &QAction::triggered, this, &LammpsGui::view_log);
    connect(actionViewGraphWindow, &QAction::triggered, this, &LammpsGui::view_chart);
    connect(actionViewImageWindow, &QAction::triggered, this, &LammpsGui::view_image);
    connect(actionViewSlideShow, &QAction::triggered, this, &LammpsGui::view_slides);
    connect(actionViewVariableWindow, &QAction::triggered, this, &LammpsGui::view_variables);
    connect(recentActions[0], &QAction::triggered, this, &LammpsGui::open_recent);
    connect(recentActions[1], &QAction::triggered, this, &LammpsGui::open_recent);
    connect(recentActions[2], &QAction::triggered, this, &LammpsGui::open_recent);
    connect(recentActions[3], &QAction::triggered, this, &LammpsGui::open_recent);
    connect(recentActions[4], &QAction::triggered, this, &LammpsGui::open_recent);

    connect(textEdit->document(), &QTextDocument::modificationChanged, this,
            &LammpsGui::modified);

#if !QT_CONFIG(clipboard)
    actionCut->setEnabled(false);
    actionCopy->setEnabled(false);
    actionPaste->setEnabled(false);
#endif

    lammpsstatus = new QLabel(QString());
    auto pix     = QPixmap(":/icons/lammps-icon-128x128.png");
    lammpsstatus->setPixmap(pix.scaled(22, 22, Qt::KeepAspectRatio));
    statusbar->addWidget(lammpsstatus);
    lammpsstatus->setToolTip("LAMMPS instance is active");
    lammpsstatus->hide();

    auto *lammpssave  = new QPushButton(QIcon(":/icons/document-save.png"), "");
    auto *lammpsrun   = new QPushButton(QIcon(":/icons/system-run.png"), "");
    auto *lammpsstop  = new QPushButton(QIcon(":/icons/process-stop.png"), "");
    auto *lammpsimage = new QPushButton(QIcon(":/icons/emblem-photos.png"), "");
    lammpssave->setToolTip("Save edit buffer to file");
    lammpsrun->setToolTip("Run LAMMPS on input");
    lammpsstop->setToolTip("Stop LAMMPS");
    lammpsimage->setToolTip("Create snapshot image");
    auto buttonhint = lammpssave->minimumSizeHint();
    buttonhint.setWidth(buttonhint.height() * 4 / 3);
    lammpssave->setMinimumSize(buttonhint);
    lammpssave->setMaximumSize(buttonhint);
    lammpsrun->setMinimumSize(buttonhint);
    lammpsrun->setMaximumSize(buttonhint);
    lammpsstop->setMinimumSize(buttonhint);
    lammpsstop->setMaximumSize(buttonhint);
    lammpsimage->setMinimumSize(buttonhint);
    lammpsimage->setMaximumSize(buttonhint);
    statusbar->addWidget(lammpssave);
    statusbar->addWidget(lammpsrun);
    statusbar->addWidget(lammpsstop);
    statusbar->addWidget(lammpsimage);
    connect(lammpssave, &QPushButton::released, this, &LammpsGui::save);
    connect(lammpsrun, &QPushButton::released, this, &LammpsGui::run_buffer);
    connect(lammpsstop, &QPushButton::released, this, &LammpsGui::stop_run);
    connect(lammpsimage, &QPushButton::released, this, &LammpsGui::render_image);

    cpuuse = new QLabel("   0%CPU");
    cpuuse->setFixedWidth(90);
    statusbar->addWidget(cpuuse);
    cpuuse->hide();
    status = new QLabel("Ready.");
    status->setFixedWidth(300);
    statusbar->addWidget(status);
    dirstatus = new QLabel(QString(" Directory: ") + current_dir);
    dirstatus->setMinimumWidth(MINIMUM_WIDTH);
    statusbar->addWidget(dirstatus);
    progress = new QProgressBar();
    progress->setRange(0, 1000);
    progress->setMinimumWidth(MINIMUM_WIDTH);
    progress->hide();
    dirstatus->show();
    statusbar->addWidget(progress);

    if ((filename.size() > 0) && !filename.endsWith("lammps-gui.exe")) {
        open_file(filename);
    } else {
        setWindowTitle("LAMMPS-GUI - Editor - *unknown*");
    }

    // set width and height of main window.
    // use last values unless overridden from command-line
    // do not accept an geometry smaller than minimum
    if (mainx < MINIMUM_WIDTH)
        mainx = settings.value("mainx", QString::number(MINIMUM_WIDTH)).toInt();
    if (mainy < MINIMUM_HEIGHT)
        mainy = settings.value("mainy", QString::number(MINIMUM_HEIGHT)).toInt();
    resize(mainx, mainy);

    // start LAMMPS and initialize command completion
    start_lammps();
    QStringList style_list;
    char buf[DEFAULT_BUFLEN];
    QFile internal_commands(":/lammps_internal_commands.txt");
    if (internal_commands.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!internal_commands.atEnd()) {
            style_list << QString(internal_commands.readLine()).trimmed();
        }
    }
    internal_commands.close();
    int ncmds = lammps.style_count("command");
    for (int i = 0; i < ncmds; ++i) {
        if (lammps.style_name("command", i, buf, DEFAULT_BUFLEN)) {
            // skip suffixed names
            const QString style(buf);
            if (style.endsWith("/kk/host") || style.endsWith("/kk/device") || style.endsWith("/kk"))
                continue;
            style_list << style;
        }
    }
    style_list.sort();
    textEdit->setCommandList(style_list);

    style_list.clear();
    const char *varstyles[] = {"delete",   "atomfile", "file",   "format", "getenv", "index",
                               "internal", "loop",     "python", "string", "timer",  "uloop",
                               "universe", "world",    "equal",  "vector", "atom"};
    for (const auto *const var : varstyles)
        style_list << var;
    style_list.sort();
    textEdit->setVariableList(style_list);

    style_list.clear();
    const char *unitstyles[] = {"lj", "real", "metal", "si", "cgs", "electron", "micro", "nano"};
    for (const auto *const unit : unitstyles)
        style_list << unit;
    style_list.sort();
    textEdit->setUnitsList(style_list);

    style_list.clear();
    const char *extraargs[] = {"extra/atom/types",        "extra/bond/types",
                               "extra/angle/types",       "extra/dihedral/types",
                               "extra/improper/types",    "extra/bond/per/atom",
                               "extra/angle/per/atom",    "extra/dihedral/per/atom",
                               "extra/improper/per/atom", "extra/special/per/atom"};
    for (const auto *const extra : extraargs)
        style_list << extra;
    textEdit->setExtraList(style_list);

    textEdit->setFileList();

#define ADD_STYLES(keyword, Type)                                                              \
    style_list.clear();                                                                        \
    if ((std::string(#keyword) == "pair") || (std::string(#keyword) == "bond") ||              \
        (std::string(#keyword) == "angle") || (std::string(#keyword) == "dihedral") ||         \
        (std::string(#keyword) == "improper") || (std::string(#keyword) == "kspace"))          \
        style_list << QString("none");                                                         \
    ncmds = lammps.style_count(#keyword);                                                      \
    for (int i = 0; i < ncmds; ++i) {                                                          \
        if (lammps.style_name(#keyword, i, buf, DEFAULT_BUFLEN)) {                             \
            const QString style(buf);                                                          \
            if (style.endsWith("/gpu") || style.endsWith("/intel") || style.endsWith("/kk") || \
                style.endsWith("/kk/device") || style.endsWith("/kk/host") ||                  \
                style.endsWith("/omp") || style.endsWith("/opt"))                              \
                continue;                                                                      \
            style_list << style;                                                               \
        }                                                                                      \
    }                                                                                          \
    style_list.sort();                                                                         \
    textEdit->set##Type##List(style_list)

    ADD_STYLES(fix, Fix);
    ADD_STYLES(compute, Compute);
    ADD_STYLES(dump, Dump);
    ADD_STYLES(atom, Atom);
    ADD_STYLES(pair, Pair);
    ADD_STYLES(bond, Bond);
    ADD_STYLES(angle, Angle);
    ADD_STYLES(dihedral, Dihedral);
    ADD_STYLES(improper, Improper);
    ADD_STYLES(kspace, Kspace);
    ADD_STYLES(region, Region);
    ADD_STYLES(integrate, Integrate);
    ADD_STYLES(minimize, Minimize);
#undef ADD_STYLES

    settings.beginGroup("reformat");
    textEdit->setReformatOnReturn(settings.value("return", false).toBool());
    textEdit->setAutoComplete(settings.value("automatic", true).toBool());
    settings.endGroup();

    // apply https proxy setting: prefer environment variable or fall back to preferences value
    auto https_proxy = QString::fromLocal8Bit(qgetenv("https_proxy"));
    if (https_proxy.isEmpty()) https_proxy = settings.value("https_proxy", "").toString();
    if (!https_proxy.isEmpty()) lammps.command(QString("shell putenv https_proxy=") + https_proxy);

    // set window flags for window manager
    auto flags = windowFlags();
    flags &= ~Qt::Dialog;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowMinimizeButtonHint;
    flags |= Qt::WindowMaximizeButtonHint;
    setWindowFlags(flags);
}

LammpsGui::~LammpsGui()
{
    delete highlighter;
    delete capturer;
    delete status;
    delete cpuuse;
    delete logwindow;
    delete imagewindow;
    delete chartwindow;
    delete dirstatus;
    delete varwindow;
    delete slideshow;
}

void LammpsGui::new_document()
{
    current_file.clear();
    textEdit->document()->setPlainText(citeme);
    textEdit->document()->setModified(false);
    textEdit->setStyleSheet(bannerstyle);

    if (lammps.is_running()) {
        stop_run();
        runner->wait();
        delete runner;
        runner = nullptr;
    }
    // close windows
    delete chartwindow;
    delete logwindow;
    delete slideshow;
    delete imagewindow;
    delete varwindow;
    chartwindow = nullptr;
    logwindow   = nullptr;
    slideshow   = nullptr;
    imagewindow = nullptr;
    varwindow   = nullptr;

    silence_stdout();
    lammps.close();
    restore_stdout();
    lammpsstatus->hide();
    setWindowTitle("LAMMPS-GUI - Editor - *unknown*");
    run_counter = 0;
}

void LammpsGui::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    open_file(fileName);
}

void LammpsGui::view()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    view_file(fileName);
}

void LammpsGui::inspect()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the restart file");
    inspect_file(fileName);
}

void LammpsGui::open_recent()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act) open_file(act->data().toString());
}

void LammpsGui::get_directory()
{
    if (wizard) {
        auto *line = wizard->findChild<QLineEdit *>("t_directory");
        if (line) {
            auto curdir = line->text();
            QFileDialog dialog(this, "Choose Directory for Tutorial Files", curdir);
            dialog.setFileMode(QFileDialog::Directory);
            dialog.setOption(QFileDialog::ShowDirsOnly, false);
            dialog.exec();
            line->setText(dialog.directory().path());
        }
    }
}

void LammpsGui::start_exe()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act) {
        auto exe = act->data().toString();
        QStringList args;
        if (lammps.extract_setting("box_exist")) {
            QString datacmd = "write_data '";
            QDir datadir(QDir::tempPath());
            QFile datafile(datadir.absoluteFilePath(current_file + ".data"));
            datacmd += datafile.fileName() + "'";
            if (exe == "vmd") {
                QFile vmdfile(datadir.absoluteFilePath("tmp-loader.vmd"));
                if (vmdfile.open(QIODevice::WriteOnly)) {
                    vmdfile.write("package require topotools\n");
                    vmdfile.write("topo readlammpsdata {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write("}\ntopo guessatom lammps data\n");
                    vmdfile.write("animate write psf {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".psf}\nanimate write dcd {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".dcd}\nmol delete top\nmol new {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".psf} type psf waitfor all\nmol addfile {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".dcd} type dcd waitfor all\nfile delete {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write("} {");
                    vmdfile.write(vmdfile.fileName().toLocal8Bit());
                    vmdfile.write("} {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".dcd} {");
                    vmdfile.write(datafile.fileName().toLocal8Bit());
                    vmdfile.write(".psf}\n");
                    vmdfile.close();
                    args << "-e" << vmdfile.fileName();
                    silence_stdout();
                    lammps.command(datacmd);
                    restore_stdout();
                    auto *vmd = new QProcess(this);
                    vmd->start(exe, args);
                } else {
                    warning(this, "LAMMPS-GUI Error",
                            "Cannot create temporary file for loading system in VMD",
                            vmdfile.errorString());
                }
            }
            if (exe == "ovito") {
                QStringList args;
                args << datafile.fileName();
                silence_stdout();
                lammps.command(datacmd);
                restore_stdout();
                auto *ovito = new QProcess(this);
                ovito->start(exe, args);
            }
        } else {
            // launch program without arguments when no system exists (yet)
            auto *proc = new QProcess(this);
            proc->start(exe, args);
        }
    }
}

void LammpsGui::update_recents(const QString &filename)
{
    QSettings settings;
    if (settings.contains("recent")) recent = settings.value("recent").value<QList<QString>>();

    for (int i = 0; i < recent.size(); ++i) {
        QFileInfo fi(recent[i]);
        if (!fi.isReadable()) {
            recent.removeAt(i);
            i = 0;
        }
    }

    if (!filename.isEmpty() && !recent.contains(filename)) recent.prepend(filename);
    if (recent.size() > 5) recent.removeLast();
    if (!recent.empty())
        settings.setValue("recent", QVariant::fromValue(recent));
    else
        settings.remove("recent");

    recentActions[0]->setVisible(false);
    if ((!recent.empty()) && !recent[0].isEmpty()) {
        QFileInfo fi(recent[0]);
        recentActions[0]->setText(QString("&1. ") + fi.fileName());
        recentActions[0]->setData(recent[0]);
        recentActions[0]->setVisible(true);
    }
    recentActions[1]->setVisible(false);
    if ((recent.size() > 1) && !recent[1].isEmpty()) {
        QFileInfo fi(recent[1]);
        recentActions[1]->setText(QString("&2. ") + fi.fileName());
        recentActions[1]->setData(recent[1]);
        recentActions[1]->setVisible(true);
    }
    recentActions[2]->setVisible(false);
    if ((recent.size() > 2) && !recent[2].isEmpty()) {
        QFileInfo fi(recent[2]);
        recentActions[2]->setText(QString("&3. ") + fi.fileName());
        recentActions[2]->setData(recent[2]);
        recentActions[2]->setVisible(true);
    }
    recentActions[3]->setVisible(false);
    if ((recent.size() > 3) && !recent[3].isEmpty()) {
        QFileInfo fi(recent[3]);
        recentActions[3]->setText(QString("&4. ") + fi.fileName());
        recentActions[3]->setData(recent[3]);
        recentActions[3]->setVisible(true);
    }
    recentActions[4]->setVisible(false);
    if ((recent.size() > 4) && !recent[4].isEmpty()) {
        QFileInfo fi(recent[4]);
        recentActions[4]->setText(QString("&5. ") + fi.fileName());
        recentActions[4]->setData(recent[4]);
        recentActions[4]->setVisible(true);
    }
}

// delete all current variables in the LAMMPS instance
void LammpsGui::clear_variables()
{
    int nvar = lammps.id_count("variable");
    char buffer[DEFAULT_BUFLEN];

    // delete from back so they are not re-indexed
    for (int i = nvar - 1; i >= 0; --i) {
        memset(buffer, 0, DEFAULT_BUFLEN);
        if (lammps.id_name("variable", i, buffer, DEFAULT_BUFLEN))
            lammps.command(QString("variable %1 delete").arg(buffer));
    }
}

void LammpsGui::update_variables()
{
    const auto doc = textEdit->toPlainText().replace('\t', ' ').split('\n');
    QStringList known;
    QRegularExpression indexvar(R"(^\s*variable\s+(\w+)\s+index\s+(.*))");
    QRegularExpression anyvar(R"(^\s*variable\s+(\w+)\s+(\w+)\s+(.*))");
    QRegularExpression usevar(R"((\$(\w)|\${(\w+)}))");
    QRegularExpression refvar(R"(v_(\w+))");

    // forget previously listed variables
    variables.clear();

    for (const auto &line : doc) {

        if (line.isEmpty()) continue;

        // first find variable definitions.
        // index variables are special since they can be overridden from the command line
        auto index = indexvar.match(line);
        auto any   = anyvar.match(line);

        if (index.hasMatch()) {
            if (index.lastCapturedIndex() >= 2) {
                auto name = index.captured(1);
                if (!known.contains(name)) {
                    variables.append(qMakePair(name, index.captured(2)));
                    known.append(name);
                }
            }
        } else if (any.hasMatch()) {
            if (any.lastCapturedIndex() >= 3) {
                auto name = any.captured(1);
                if (!known.contains(name)) known.append(name);
            }
        }

        // now split line into words and search for use of undefined variables
        auto words = line.split(' ');
        for (const auto &word : words) {
            auto use = usevar.match(word);
            auto ref = refvar.match(word);
            if (use.hasMatch()) {
                auto name = use.captured(use.lastCapturedIndex());
                if (!known.contains(name)) {
                    known.append(name);
                    variables.append(qMakePair(name, QString()));
                }
            }
            if (ref.hasMatch()) {
                auto name = ref.captured(use.lastCapturedIndex());
                if (!known.contains(name)) known.append(name);
            }
        }
    }
}

// open file and switch CWD to path of file
void LammpsGui::open_file(const QString &fileName)
{
    // do nothing, if no file name provided
    if (fileName.isEmpty()) return;

    if (lammps.is_running()) {
        stop_run();
        runner->wait();
        delete runner;
        runner = nullptr;
    }
    // close windows
    delete chartwindow;
    delete logwindow;
    delete slideshow;
    delete imagewindow;
    delete varwindow;
    chartwindow = nullptr;
    logwindow   = nullptr;
    slideshow   = nullptr;
    imagewindow = nullptr;
    varwindow   = nullptr;
    silence_stdout();
    lammps.close();
    restore_stdout();

    purge_inspect_list();
    textEdit->setStyleSheet("");
    if (textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + current_file + " has changes");
        msg.setInformativeText("Do you want to save the file before opening a new file?");
        msg.setIcon(QMessageBox::Question);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        auto *button = msg.button(QMessageBox::Yes);
        button->setIcon(QIcon(":/icons/dialog-ok.png"));
        button = msg.button(QMessageBox::No);
        button->setIcon(QIcon(":/icons/dialog-no.png"));
        button = msg.button(QMessageBox::Cancel);
        button->setIcon(QIcon(":/icons/dialog-cancel.png"));

        msg.setFont(font());
        int rv = msg.exec();
        switch (rv) {
            case QMessageBox::Yes:
                save();
                break;
            case QMessageBox::Cancel:
                return;
                break;
            case QMessageBox::No: // fallthrough
            default:
                // do nothing
                break;
        }
    }
    textEdit->setHighlight(CodeEditor::NO_HIGHLIGHT, false);

    QFileInfo path(fileName);
    current_file = path.fileName();
    current_dir  = path.absolutePath();
    QFile file(path.absoluteFilePath());

    update_recents(path.absoluteFilePath());

    QDir::setCurrent(current_dir);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        warning(this, "LAMMPS-GUI Warning", "Cannot open file " + path.absoluteFilePath() + ":",
                file.errorString() + "\n\nWill create new file on saving editor buffer.");
        textEdit->document()->clear();
        textEdit->document()->setPlainText(citeme);
        textEdit->document()->setModified(false);
        textEdit->setStyleSheet(bannerstyle);
    } else {
        QTextStream in(&file);
        QString text = in.readAll();
        textEdit->document()->clear();
        textEdit->document()->setPlainText(text);
        textEdit->moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
        file.close();
    }
    setWindowTitle(QString("LAMMPS-GUI - Editor - " + current_file));
    run_counter = 0;
    textEdit->document()->setModified(false);
    textEdit->setGroupList();
    textEdit->setVarNameList();
    textEdit->setComputeIDList();
    textEdit->setFixIDList();
    textEdit->setFileList();
    dirstatus->setText(QString(" Directory: ") + current_dir);
    status->setText("Ready.");
    cpuuse->hide();

    update_variables();
    silence_stdout();
    lammps.close();
    restore_stdout();
}

// open file in read-only mode for viewing in separate window
void LammpsGui::view_file(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QFile::Text)) {
        warning(this, "LAMMPS-GUI Warning", "Cannot open file " + fileName + ":",
                file.errorString());
    } else {
        file.close();
        auto *viewer = new FileViewer(fileName);
        viewer->show();
    }
}

void LammpsGui::purge_inspect_list()
{
    for (auto *item : inspectList) {
        if (item->info) {
            if (!item->info->isVisible()) {
                delete item->info;
                item->info = nullptr;
            }
        }
        if (item->data) {
            if (!item->data->isVisible()) {
                delete item->data;
                item->data = nullptr;
            }
        }
        if (item->image) {
            if (!item->image->isVisible()) {
                delete item->image;
                item->image = nullptr;
            }
        }
        if (!item->image && !item->data && !item->info) inspectList.removeOne(item);
    }
}

// read restart file into LAMMPS instance and launch image viewer
void LammpsGui::inspect_file(const QString &fileName)
{
    QFile file(fileName);
    auto shortName = QFileInfo(fileName).fileName();

    purge_inspect_list();
    auto *ilist  = new InspectData;
    ilist->info  = nullptr;
    ilist->data  = nullptr;
    ilist->image = nullptr;
    inspectList.append(ilist);

    if (file.size() > 262144000L) {
        QMessageBox msg;
        msg.setWindowTitle("  Warning:  Large Restart File  ");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("<center>The restart file ") + shortName + " is large</center>");
        QString details = "Inspecting the restart file %1 with LAMMPS-GUI may need an additional "
                          "%2 GB of free RAM (or more) to proceed";
        msg.setDetailedText(details.arg(shortName).arg(file.size() / 134217728.0));
        msg.setInformativeText("Do you want to continue?");
        msg.setIcon(QMessageBox::Question);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msg.setDefaultButton(QMessageBox::No);
        msg.setEscapeButton(QMessageBox::No);
        msg.setFont(font());

        auto *button = msg.button(QMessageBox::Yes);
        button->setIcon(QIcon(":/icons/dialog-ok.png"));
        button = msg.button(QMessageBox::No);
        button->setIcon(QIcon(":/icons/dialog-no.png"));

        int rv = msg.exec();
        switch (rv) {
            case QMessageBox::No:
                return;
                break;
            case QMessageBox::Yes: // fallthrough
            default:
                // do nothing
                break;
        }
    }

    if (!file.open(QIODevice::ReadOnly)) {
        warning(this, "LAMMPS-GUI Warning", "Cannot open file " + fileName + ":",
                file.errorString());
        return;
    }

    char magic[16] = "               ";
    QDataStream in(&file);
    in.readRawData(magic, 16);
    file.close();
    if (strcmp(magic, LAMMPS_MAGIC) != 0) {
        warning(this, "LAMMPS-GUI Warning", "File " + fileName + " is not a LAMMPS restart file.");
        return;
    }

    // LAMMPS is not re-entrant, so we can only query LAMMPS when it is not running a simulation
    if (!lammps.is_running()) {
        start_lammps();
        silence_stdout();
        lammps.command("clear");
        clear_variables();
        lammps.command(QString("read_restart %1").arg(fileName));
        restore_stdout();
        capturer->BeginCapture();
        lammps.command("info system group compute fix");
        capturer->EndCapture();
        auto info    = capturer->GetCapture();
        auto infolog = QString("%1.info.log").arg(fileName);
        QFile dumpinfo(infolog);
        if (dumpinfo.open(QIODevice::WriteOnly)) {
            auto infodata = QString("%1.tmp.data").arg(fileName);
            dumpinfo.write(info.c_str(), info.size());
            dumpinfo.close();
            auto *infoviewer =
                new FileViewer(infolog, QString("LAMMPS-GUI: restart info for %1").arg(shortName));
            infoviewer->show();
            ilist->info = infoviewer;
            dumpinfo.remove();
            silence_stdout();
            lammps.command(QString("write_data %1 pair ij noinit").arg(infodata));
            restore_stdout();
            auto *dataviewer =
                new FileViewer(infodata, QString("LAMMPS-GUI: data file for %1").arg(shortName));
            dataviewer->show();
            ilist->data = dataviewer;
            QFile(infodata).remove();
            auto *inspect_image = new ImageViewer(fileName, &lammps);
            inspect_image->setFont(font());
            inspect_image->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
            inspect_image->show();
            ilist->image = inspect_image;
        }
    }
}

// write file and update CWD to its folder

void LammpsGui::write_file(const QString &fileName)
{
    QFileInfo path(fileName);
    current_file = path.fileName();
    current_dir  = path.absolutePath();
    QFile file(path.absoluteFilePath());

    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        warning(this, "LAMMPS-GUI Warning", "Cannot save to file " + fileName + ":",
                file.errorString());
        return;
    }
    setWindowTitle(QString("LAMMPS-GUI - Editor - " + current_file));
    QDir::setCurrent(current_dir);

    update_recents(path.absoluteFilePath());

    QTextStream out(&file);
    QString text = textEdit->toPlainText();
    out << text;
    if (text.back().toLatin1() != '\n') out << "\n"; // add final newline if missing
    file.close();
    dirstatus->setText(QString(" Directory: ") + current_dir);
    // update list of files for completion since we may have changed the working directory
    textEdit->setFileList();
    textEdit->document()->setModified(false);
}

void LammpsGui::save()
{
    purge_inspect_list();
    QString fileName = current_file;
    // If we don't have a filename from before, get one.
    if (fileName.isEmpty()) fileName = QFileDialog::getSaveFileName(this, "Save");

    write_file(fileName);
}

void LammpsGui::save_as()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save as");
    write_file(fileName);
}

void LammpsGui::quit()
{
    removeEventFilter(this);

    if (lammps.is_running()) {
        stop_run();
        runner->wait();
        delete runner;
        runner = nullptr;
    }

    // close LAMMPS instance
    silence_stdout();
    lammps.close();
    restore_stdout();
    lammpsstatus->hide();
    lammps.finalize();

    autoSave();
    if (textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + current_file + " has changes");
        msg.setInformativeText("Do you want to save the file before exiting?");
        msg.setIcon(QMessageBox::Question);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

        auto *button = msg.button(QMessageBox::Yes);
        button->setIcon(QIcon(":/icons/dialog-ok.png"));
        button = msg.button(QMessageBox::No);
        button->setIcon(QIcon(":/icons/dialog-no.png"));
        button = msg.button(QMessageBox::Cancel);
        button->setIcon(QIcon(":/icons/dialog-cancel.png"));

        msg.setFont(font());
        int rv = msg.exec();
        switch (rv) {
            case QMessageBox::Yes:
                save();
                break;
            case QMessageBox::Cancel:
                return;
                break;
            case QMessageBox::No: // fallthrough
            default:
                // do nothing
                break;
        }
    }

    // store some global settings
    QSettings settings;
    if (!isMaximized()) {
        settings.setValue("mainx", width());
        settings.setValue("mainy", height());
    }
    settings.sync();

#if QT_CONFIG(clipboard)
    auto *clip = QGuiApplication::clipboard();
    if (clip) clip->clear();
#endif

    // quit application
    QCoreApplication::quit();
}

void LammpsGui::copy()
{
#if QT_CONFIG(clipboard)
    textEdit->copy();
#endif
}

void LammpsGui::cut()
{
#if QT_CONFIG(clipboard)
    textEdit->cut();
#endif
}

void LammpsGui::paste()
{
#if QT_CONFIG(clipboard)
    textEdit->paste();
#endif
}

void LammpsGui::undo()
{
    textEdit->undo();
}

void LammpsGui::redo()
{
    textEdit->redo();
}

void LammpsGui::stop_run()
{
    lammps.force_timeout();
}

void LammpsGui::logupdate()
{
    double t_elapsed, t_remain, t_total;
    int completed = 1000;

    // estimate completion percentage
    if (lammps.is_running()) {
        t_elapsed = lammps.get_thermo("cpu");
        t_remain  = lammps.get_thermo("cpuremain");
        t_total   = t_elapsed + t_remain + 1.0e-10;
        completed = t_elapsed / t_total * 1000.0;
        // update cpu usage
        int percent_cpu = (int)lammps.get_thermo("cpuuse");
        // clear any pending error messages from polling those thermo keywords
        lammps.get_last_error_message(nullptr, 0);

        cpuuse->setText(QString("%1%CPU").arg(percent_cpu, 4));
        if (percent_cpu < 25.0 * nthreads) {
            cpuuse->setStyleSheet("QLabel {background-color: black; color: white;}");
        } else if (percent_cpu < 50.0 * nthreads) {
            cpuuse->setStyleSheet("QLabel {background-color: darkblue; color: white;}");
        } else if (percent_cpu > 100.0 * nthreads + 50.0) {
            cpuuse->setStyleSheet("QLabel {background-color: firebrick; color: white;}");
        } else if (percent_cpu < 100.0 * nthreads - 50.0) {
            cpuuse->setStyleSheet("QLabel {background-color: firebrick; color: white;}");
        } else if (percent_cpu > 100.0 * nthreads + 20.0) {
            cpuuse->setStyleSheet("QLabel {background-color: gold; color: black;}");
        } else if (percent_cpu < 100.0 * nthreads - 20.0) {
            cpuuse->setStyleSheet("QLabel {background-color: gold; color: black;}");
        } else {
            cpuuse->setStyleSheet("QLabel {background-color: forestgreen; color: white;}");
        }

        int nline = -1;
        void *ptr = lammps.last_thermo("line", 0);
        if (ptr) {
            nline = *((int *)ptr);
            textEdit->setHighlight(nline, false);
        }

        if (varwindow) {
            int nvar = lammps.id_count("variable");
            char buffer[DEFAULT_BUFLEN];
            QString varinfo("\n");
            for (int i = 0; i < nvar; ++i) {
                memset(buffer, 0, DEFAULT_BUFLEN);
                if (lammps.variable_info(i, buffer, DEFAULT_BUFLEN)) varinfo += buffer;
            }
            if (nvar == 0) varinfo += "  (none)  ";

            varwindow->setText(varinfo);
            varwindow->adjustSize();
        }
    }

    progress->setValue(completed);
    if (logwindow) {
        const auto text = capturer->GetChunk();
        if (!text.empty()) {
            logwindow->moveCursor(QTextCursor::End);
            logwindow->insertPlainText(text.c_str());
            logwindow->moveCursor(QTextCursor::End);
            logwindow->textCursor().deleteChar();
        }
    }

    // get timestep
    int step  = 0;
    void *ptr = lammps.last_thermo("step", 0);
    if (ptr) {
        if (lammps.extract_setting("bigint") == 4)
            step = *(int *)ptr;
        else
            step = (int)*(int64_t *)ptr;
    }

    // extract cached thermo data when LAMMPS is executing a minimize or run command
    if (chartwindow && lammps.is_running()) {
        // thermo data is not yet valid during setup
        void *ptr = lammps.last_thermo("setup", 0);
        if (ptr && *(int *)ptr) return;

        lammps.last_thermo("lock", 0);
        ptr = lammps.last_thermo("num", 0);
        if (ptr) {
            int ncols = *(int *)ptr;

            // check if the column assignment has changed
            // if yes, delete charts and start over
            if (chartwindow->num_charts() > 0) {
                int count     = 0;
                bool do_reset = false;
                if (step < chartwindow->get_step()) do_reset = true;
                for (int i = 0, idx = 0; i < ncols; ++i) {
                    QString label = (const char *)lammps.last_thermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    if (!chartwindow->has_title(label, idx)) {
                        do_reset = true;
                    } else {
                        ++count;
                    }
                    ++idx;
                }
                if (chartwindow->num_charts() != count) do_reset = true;
                if (do_reset) chartwindow->reset_charts();
            }

            if (chartwindow->num_charts() == 0) {
                for (int i = 0; i < ncols; ++i) {
                    QString label = (const char *)lammps.last_thermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    chartwindow->add_chart(label, i);
                }
            }

            for (int i = 0; i < ncols; ++i) {
                int datatype = -1;
                double data  = 0.0;
                void *ptr    = lammps.last_thermo("type", i);
                if (ptr) datatype = *(int *)ptr;
                ptr = lammps.last_thermo("data", i);
                if (ptr) {
                    if (datatype == 0) // int
                        data = *(int *)ptr;
                    else if (datatype == 2) // double
                        data = *(double *)ptr;
                    else if (datatype == 4) // bigint
                        data = (double)*(int64_t *)ptr;
                }
                chartwindow->add_data(step, data, i);
            }
        }
        lammps.last_thermo("unlock", 0);
    }

    // update list of available image file names

    QString imagefile = (const char *)lammps.last_thermo("imagename", 0);
    if (!imagefile.isEmpty()) {
        if (!slideshow) {
            slideshow = new SlideShow(current_file);
            if (QSettings().value("viewslide", true).toBool())
                slideshow->show();
            else
                slideshow->hide();
        } else {
            slideshow->setWindowTitle(QString("LAMMPS-GUI - Slide Show - %1 - Run %2")
                                          .arg(current_file)
                                          .arg(run_counter));
            if (QSettings().value("viewslide", true).toBool()) slideshow->show();
        }
        slideshow->add_image(imagefile);
    }
}

void LammpsGui::modified()
{
    const QString modflag(" - *modified*");
    auto title = windowTitle().remove(modflag);
    if (textEdit->document()->isModified()) {
        textEdit->setStyleSheet("");
        setWindowTitle(title + modflag);
    } else
        setWindowTitle(title);
}

void LammpsGui::run_done()
{
    if (logupdater) {
        logupdater->stop();
        delete logupdater;
        logupdater = nullptr;
    }
    progress->setValue(1000);
    textEdit->setHighlight(CodeEditor::NO_HIGHLIGHT, false);

    capturer->EndCapture();

    if (logwindow) {
        auto log = capturer->GetCapture();
        logwindow->insertPlainText(log.c_str());
        logwindow->moveCursor(QTextCursor::End);
    }

    // check stdout capture buffer utilization and print warning message if large

    double bufferuse = capturer->get_bufferuse();
    if (bufferuse > 0.333) {
        int thermo_val     = lammps.extract_setting("thermo_every");
        int thermo_suggest = 5 * (int)round(bufferuse * thermo_val);
        int update_val     = QSettings().value("updfreq", 100).toInt();
        int update_suggest = std::max(1, update_val / 5);

        QString mesg1("<p align=\"justified\">The I/O buffer for capturing the LAMMPS screen "
                      "output was used by up to %1%.</p>"
                      "<p align=\"justified\"><b>This can slow down the simulation.</b></p>");
        QString mesg2("<p align=\"justified\">Please consider reducing the amount of output "
                      "to the screen, for example by increasing the thermo interval in the "
                      "input from %1 to %2, or reducing the data update interval in the "
                      "preferences from %3 to %4, or something similar.</p>");

        critical(this, "LAMMPS-GUI Warning: High I/O Buffer Usage",
                 mesg1.arg((int)(100.0 * bufferuse)),
                 mesg2.arg(thermo_val).arg(thermo_suggest).arg(update_val).arg(update_suggest));
    }

    if (chartwindow) {
        void *ptr = lammps.last_thermo("step", 0);
        if (ptr) {
            int step = 0;
            if (lammps.extract_setting("bigint") == 4)
                step = *(int *)ptr;
            else
                step = (int)*(int64_t *)ptr;
            int ncols = *(int *)lammps.last_thermo("num", 0);
            for (int i = 0; i < ncols; ++i) {
                if (chartwindow->num_charts() == 0) {
                    QString label = (const char *)lammps.last_thermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    chartwindow->add_chart(label, i);
                }
                int datatype = *(int *)lammps.last_thermo("type", i);
                double data  = 0.0;
                if (datatype == 0) // int
                    data = *(int *)lammps.last_thermo("data", i);
                else if (datatype == 2) // double
                    data = *(double *)lammps.last_thermo("data", i);
                else if (datatype == 4) // bigint
                    data = (double)*(int64_t *)lammps.last_thermo("data", i);
                chartwindow->add_data(step, data, i);
            }
        }
        chartwindow->reset_zoom();
    }

    bool success = true;
    bool valid   = true;
    char errorbuf[DEFAULT_BUFLEN];

    if (lammps.has_error()) {
        lammps.get_last_error_message(errorbuf, DEFAULT_BUFLEN);
        // ignore "Invalid LAMMPS handle", but report other errors
        if (!strstr(errorbuf, "Invalid LAMMPS handle")) {
            success = false;
        } else {
            valid = false;
        }
    }

    int nline = CodeEditor::NO_HIGHLIGHT;
    if (valid) {
        void *ptr = lammps.last_thermo("line", 0);
        if (ptr) nline = *((int *)ptr);
    }

    if (success) {
        status->setText("Ready.");
        cpuuse->setText("   0%CPU");
    } else {
        status->setText("Failed.");
        textEdit->setHighlight(nline, true);
        critical(this, "LAMMPS-GUI Error", "<p>Error running LAMMPS:</p>",
                 QString("<p><pre>%1</pre></p>").arg(errorbuf));
    }
    textEdit->setCursor(nline);
    textEdit->setFileList();
    progress->hide();
    cpuuse->hide();
    dirstatus->show();
}

void LammpsGui::restart_lammps()
{
    if (lammps.is_running()) {
        warning(this, "LAMMPS-GUI Warning", "Must stop current run before relaunching LAMMPS");
        return;
    }
    silence_stdout();
    lammps.close();
    restore_stdout();
};

void LammpsGui::do_run(bool use_buffer)
{
    if (lammps.is_running()) {
        warning(this, "LAMMPS-GUI Warning", "Must stop current run before starting a new run");
        return;
    }

    purge_inspect_list();
    autoSave();
    if (!use_buffer && textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + current_file + " has changes");
        msg.setInformativeText("Do you want to save the buffer before running LAMMPS?");
        msg.setIcon(QMessageBox::Question);
        msg.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);

        auto *button = msg.button(QMessageBox::Yes);
        button->setIcon(QIcon(":/icons/dialog-ok.png"));
        button = msg.button(QMessageBox::No);
        button->setIcon(QIcon(":/icons/dialog-no.png"));
        button = msg.button(QMessageBox::Cancel);
        button->setIcon(QIcon(":/icons/dialog-cancel.png"));

        msg.setFont(font());
        int rv = msg.exec();
        switch (rv) {
            case QMessageBox::Yes:
                save();
                break;
            case QMessageBox::Cancel: // falthrough
            default:
                return;
                break;
        }
    }

    QSettings settings;
    progress->setValue(0);
    dirstatus->hide();
    progress->show();
    cpuuse->show();

    int numthreads = nthreads;
    int accel      = settings.value("accelerator", AcceleratorTab::OpenMP).toInt();
    if ((accel != AcceleratorTab::OpenMP) && (accel != AcceleratorTab::Intel) &&
        (accel != AcceleratorTab::Kokkos) && (accel != AcceleratorTab::Gpu))
        numthreads = 1;
    if (numthreads > 1)
        status->setText(QString("Running LAMMPS with %1 thread(s)...").arg(numthreads));
    else
        status->setText(QString("Running LAMMPS ..."));
    status->repaint();
    start_lammps();
    if (!lammps.is_open()) return;
    capturer->BeginCapture();

    runner     = new LammpsRunner(this);
    is_running = true;
    ++run_counter;

    // must delete all variables since clear does not delete them
    clear_variables();

    // define "gui_run" variable set to run_counter value
    lammps.command(std::string("variable gui_run index " + std::to_string(run_counter)));
    if (use_buffer) {
        // always add final newline since the text edit widget does not do it
        char *input = mystrdup(textEdit->toPlainText() + "\n");
        runner->setup_run(&lammps, input, nullptr);
    } else {
        char *fname = mystrdup(current_file);
        runner->setup_run(&lammps, nullptr, fname);
    }

    // apply https proxy setting: prefer environment variable or fall back to preferences value
    auto https_proxy = QString::fromLocal8Bit(qgetenv("https_proxy"));
    if (https_proxy.isEmpty()) https_proxy = settings.value("https_proxy", "").toString();
    if (!https_proxy.isEmpty()) lammps.command(QString("shell putenv https_proxy=") + https_proxy);

    connect(runner, &LammpsRunner::resultReady, this, &LammpsGui::run_done);
    connect(runner, &LammpsRunner::finished, runner, &QObject::deleteLater);
    runner->start();

    // if configured, delete old log window before opening new one
    if (settings.value("logreplace", true).toBool()) delete logwindow;
    logwindow = new LogWindow(current_file);
    logwindow->setReadOnly(true);
    logwindow->setCenterOnScroll(true);
    logwindow->moveCursor(QTextCursor::End);
    logwindow->setWindowTitle(
        QString("LAMMPS-GUI - Output - %1 - Run %2").arg(current_file).arg(run_counter));
    logwindow->setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    logwindow->setLineWrapMode(LogWindow::NoWrap);
    logwindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), logwindow);
    connect(shortcut, &QShortcut::activated, logwindow, &LogWindow::close);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), logwindow);
    connect(shortcut, &QShortcut::activated, this, &LammpsGui::stop_run);
    if (settings.value("viewlog", true).toBool())
        logwindow->show();
    else
        logwindow->hide();

    // if configured, delete old log window before opening new one
    if (settings.value("chartreplace", true).toBool()) delete chartwindow;
    chartwindow = new ChartWindow(current_file);
    chartwindow->setWindowTitle(
        QString("LAMMPS-GUI - Charts - %2 - Run %3").arg(current_file).arg(run_counter));
    chartwindow->setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    chartwindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    const auto *unitptr = (const char *)lammps.extract_global("units");
    if (unitptr) chartwindow->set_units(QString("%1").arg(unitptr));
    auto normflag = lammps.extract_setting("thermo_norm");
    chartwindow->set_norm(normflag != 0);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), chartwindow);
    connect(shortcut, &QShortcut::activated, chartwindow, &ChartWindow::close);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), chartwindow);
    connect(shortcut, &QShortcut::activated, this, &LammpsGui::stop_run);
    if (settings.value("viewchart", true).toBool())
        chartwindow->show();
    else
        chartwindow->hide();

    if (slideshow) {
        slideshow->setWindowTitle(QString("LAMMPS-GUI - Slide Show - " + current_file));
        slideshow->clear();
        slideshow->hide();
    }

    logupdater = new QTimer(this);
    connect(logupdater, &QTimer::timeout, this, &LammpsGui::logupdate);
    logupdater->start(settings.value("updfreq", "10").toInt());
}

void LammpsGui::render_image()
{
    // LAMMPS is not re-entrant, so we can only query LAMMPS when it is not running
    if (!lammps.is_running()) {
        start_lammps();
        if (!lammps.extract_setting("box_exist")) {
            // there is no current system defined yet.
            // so we select the input from the start to the first run or minimize command
            // add a run 0 and thus create the state of the initial system without running.
            // this will allow us to create a snapshot image.
            auto saved = textEdit->textCursor();
            if (textEdit->find(
                    QRegularExpression(QStringLiteral(R"(^\s*(run|minimize)\s+)")))) {
                auto cursor = textEdit->textCursor();
                cursor.movePosition(QTextCursor::PreviousBlock);
                cursor.movePosition(QTextCursor::EndOfLine);
                cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
                auto selection = cursor.selectedText().replace(QChar(0x2029), '\n');
                selection += "\nrun 0 pre yes post no";
                textEdit->setTextCursor(saved);
                silence_stdout();
                lammps.command("clear");
                clear_variables();
                lammps.commands_string(selection);
                restore_stdout();

                if (lammps.has_error()) {
                    char errormesg[DEFAULT_BUFLEN];
                    lammps.get_last_error_message(errormesg, DEFAULT_BUFLEN);
                    // ignore "Invalid LAMMPS handle", but report other errors
                    if (!strstr(errormesg, "Invalid LAMMPS handle")) {
                        warning(this, "Image Viewer File Creation Error",
                                "LAMMPS failed to create the image:",
                                QString("<br><code>%1</code>").arg(errormesg));
                        return;
                    }
                }
            }
            // still no system box. bail out with a suitable message
            if (!lammps.extract_setting("box_exist")) {
                warning(this, "ImageViewer File Creation Error",
                        "Cannot create snapshot image from an input not creating a system box");
                return;
            }
            textEdit->setTextCursor(saved);
        }
        // if configured, delete old image window before opening new one
        if (QSettings().value("imagereplace", true).toBool()) delete imagewindow;
        imagewindow = new ImageViewer(current_file, &lammps);
        imagewindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    } else {
        warning(this, "ImageViewer File Creation Error",
                "Cannot create snapshot image while LAMMPS is running");
        return;
    }
    imagewindow->show();
}

void LammpsGui::view_slides()
{
    if (!slideshow) {
        slideshow = new SlideShow(current_file);
        slideshow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    }
    if (slideshow->isVisible())
        slideshow->hide();
    else
        slideshow->show();
}

void LammpsGui::view_chart()
{
    QSettings settings;
    if (chartwindow) {
        if (chartwindow->isVisible()) {
            chartwindow->hide();
            settings.setValue("viewchart", false);
        } else {
            chartwindow->show();
            settings.setValue("viewchart", true);
        }
    }
}

void LammpsGui::view_log()
{
    QSettings settings;
    if (logwindow) {
        if (logwindow->isVisible()) {
            logwindow->hide();
            settings.setValue("viewlog", false);
        } else {
            logwindow->show();
            settings.setValue("viewlog", true);
        }
    }
}

void LammpsGui::view_image()
{
    if (imagewindow) {
        if (imagewindow->isVisible()) {
            imagewindow->hide();
        } else {
            imagewindow->show();
        }
    }
}

void LammpsGui::view_variables()
{
    if (varwindow) {
        if (varwindow->isVisible()) {
            varwindow->hide();
        } else {
            varwindow->show();
        }
    }
}

void LammpsGui::setDocver()
{
    QString git_branch = (const char *)lammps.extract_global("git_branch");
    if ((git_branch == "stable") || (git_branch == "maintenance")) {
        docver = "/stable/";
    } else if (git_branch == "release") {
        docver = "/";
    } else {
        docver = "/latest/";
    }
}

void LammpsGui::autoSave()
{
    // no need to auto-save, if the document has no name or is not modified.
    QString fileName = current_file;
    if (fileName.isEmpty()) return;
    if (!textEdit->document()->isModified()) return;

    // check preference
    bool autosave = false;
    QSettings settings;
    settings.beginGroup("reformat");
    autosave = settings.value("autosave", false).toBool();
    settings.endGroup();

    if (autosave) write_file(fileName);
}

void LammpsGui::setFont(const QFont &newfont)
{
    QMainWindow::setFont(newfont);
    if (textEdit) {
        textEdit->setFont(newfont);
        menubar->setFont(newfont);
        menuFile->setFont(newfont);
        menuEdit->setFont(newfont);
        menuRun->setFont(newfont);
        menuTutorial->setFont(newfont);
        menuAbout->setFont(newfont);
        menuView->setFont(newfont);
    }
}

void LammpsGui::about()
{
    std::string version = "<b>This is LAMMPS-GUI version " LAMMPS_GUI_VERSION;
    version += " using Qt version " QT_VERSION_STR;
    if (is_light_theme())
        version += " using light theme";
    else
        version += " using dark theme";
    version += "</b><br><br>\n";
    if (lammps.has_plugin()) {
        version += "LAMMPS library loaded as plugin";
        if (!plugin_path.isEmpty()) {
            version += " from file ";
            version += plugin_path.toStdString();
        }
    } else {
        version += "LAMMPS library linked to executable";
    }

    QString to_clipboard(version.c_str());
    to_clipboard += "\n\n";

    std::string info    = "LAMMPS is currently running. LAMMPS config info not available.\n";
    std::string details = "";

    // LAMMPS is not re-entrant, so we can only query LAMMPS when it is not running
    if (!lammps.is_running()) {
        start_lammps();
        capturer->BeginCapture();
        lammps.command("info config styles");
        capturer->EndCapture();
        info       = capturer->GetCapture();
        auto start = info.find("LAMMPS version:");
        auto mid   = info.find("Styles information:", start);
        auto end   = info.find("Info-Info-Info", start);

        // protect from a failed or incomplete capture
        if ((start != std::string::npos) && (mid != std::string::npos) &&
            (end != std::string::npos)) {
            details = std::string(info, mid, end - mid);
            info    = std::string(info, start, mid - start);

            // condense newlines and trailing whitespace in detailed styles info string
            auto loc = details.find("\n\n\n\n");
            while (loc != std::string::npos) {
                details.replace(loc, 4, "\n\n");
                loc = details.find("\n\n\n\n");
            }
            loc = details.find("\r\n\r\n\r\n\r\n");
            while (loc != std::string::npos) {
                details.replace(loc, 8, "\r\n\r\n");
                loc = details.find("\r\n\r\n\r\n\r\n");
            }
            loc = details.find("les:\n\n");
            while (loc != std::string::npos) {
                details.replace(loc, 6, "les:\n");
                loc = details.find("les:\n\n");
            }
            loc = details.find("les:\r\n\r\n");
            while (loc != std::string::npos) {
                details.replace(loc, 8, "les:\r\n");
                loc = details.find("les:\r\n\r\n");
            }
            loc = details.find(" \n");
            while (loc != std::string::npos) {
                details.replace(loc, 2, "\n");
                loc = details.find(" \n");
            }
            loc = details.find(" \r\n");
            while (loc != std::string::npos) {
                details.replace(loc, 3, "\r\n");
                loc = details.find(" \r\n");
            }
        }
    }

    info += citeme.toStdString();
    to_clipboard += info.c_str();
    to_clipboard += details.c_str();

#if QT_CONFIG(clipboard)
    if (auto *clip = QGuiApplication::clipboard()) clip->setText(to_clipboard);
#endif

    auto fsize = QFontMetrics(QApplication::font()).size(Qt::TextSingleLine, citeme);
    AboutDialog dialog(QString::fromStdString(version).trimmed(),
                       QString::fromStdString(info).trimmed(),
                       QString::fromStdString(details).trimmed(), fsize.width(), this);
    dialog.exec();
}

void LammpsGui::help()
{
    QMessageBox msg(this);
    msg.setWindowTitle("LAMMPS-GUI Quick Help");
    msg.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    msg.setText("<div>This is LAMMPS-GUI version " LAMMPS_GUI_VERSION "</div>");
    msg.setInformativeText(
        "<p>LAMMPS-GUI is a graphical text editor that is customized for "
        "editing LAMMPS input files and linked to the LAMMPS "
        "library and thus can run LAMMPS directly using the contents of the "
        "text buffer as input. It can retrieve and display information from "
        "LAMMPS while it is running and  display visualizations created "
        "with the dump image command.</p>"
        "<p>The main window of the LAMMPS-GUI is a text editor window with "
        "LAMMPS specific syntax highlighting. When typing <b>Ctrl-Enter</b> "
        "or clicking on 'Run LAMMMPS' in the 'Run' menu, LAMMPS will be run "
        "with the contents of editor buffer as input. The output of the LAMMPS "
        "run is captured and displayed in an Output window. The thermodynamic data "
        "is displayed in a chart window. Both are updated regularly during the "
        "run, as is a progress bar in the main window. The running simulation "
        "can be stopped cleanly by typing <b>Ctrl-/</b> or by clicking on "
        "'Stop LAMMPS' in the 'Run' menu. While LAMMPS is not running, "
        "an image of the simulated system can be created and shown in an image "
        "viewer window by typing <b>Ctrl-i</b> or by clicking on 'View Image' "
        "in the 'Run' menu. Multiple image settings can be changed through the "
        "buttons in the menu bar and the image will be re-renderd.  In case "
        "an input file contains a dump image command, LAMMPS-GUI will load "
        "the images as they are created and display them in a slide show. </p>"
        "<p>When opening a file, the editor will determine the directory "
        "where the input file resides and switch its current working directory "
        "to that same folder and thus enabling the run to read other files in "
        "that folder, e.g. a data file. The GUI will show its current working "
        "directory in the status bar. In addition to using the menu, the "
        "editor window can also receive files as the first command line "
        "argument or via drag-n-drop from a graphical file manager or a "
        "desktop environment.</p>"
        "<p>Almost all commands are accessible via keyboard shortcuts. Which "
        "those shortcuts are, is typically shown next to their entries in the "
        "menus. "
        "In addition, the documentation for the command in the current line "
        "can be viewed by typing <b>Ctrl-?</b> or by choosing the respective "
        "entry in the context menu, available by right-clicking the mouse. "
        "Log, chart, slide show, and image windows can be closed with "
        "<b>Ctrl-W</b> and the application terminated with <b>Ctrl-Q</b>.</p>"
        "<p>The 'About LAMMPS-GUI' dialog will show the LAMMPS version and the "
        "features included into the LAMMPS library linked to the LAMMPS-GUI. "
        "A number of settings can be adjusted in the 'Preferences' dialog (in "
        "the 'Edit' menu or from <b>Ctrl-P</b>) which includes selecting "
        "accelerator packages and number of OpenMP threads. Due to its nature "
        "as a graphical application, it is <b>not</b> possible to use the "
        "LAMMPS-GUI in parallel with MPI.</p>");
    msg.setIconPixmap(QPixmap(":/icons/lammps-gui-icon-128x128.png").scaled(64, 64));
    msg.setStandardButtons(QMessageBox::Close);
    auto *button = msg.button(QMessageBox::Close);
    button->setIcon(QIcon(":/icons/window-close.png"));
    msg.setFont(font());
    msg.exec();
}

void LammpsGui::manual()
{
    if (docver.isEmpty()) setDocver();
    QDesktopServices::openUrl(QUrl(QString("https://docs.lammps.org%1").arg(docver)));
}

void LammpsGui::tutorial_web()
{
    QDesktopServices::openUrl(QUrl("https://lammpstutorials.github.io/"));
}

QWizardPage *LammpsGui::tutorial_intro(const int ntutorial, const QString &infotext)
{
    auto *page = new QWizardPage;
    page->setTitle(QString("Getting Started With Tutorial %1").arg(ntutorial));
    page->setPixmap(QWizard::WatermarkPixmap,
                    QPixmap(QString(":/icons/tutorial%1-logo.png").arg(ntutorial)));

    // TBD: TODO: update URL to published tutorial DOI
    auto *label = new QLabel(
        QString("<p>This dialog will help you to select and populate a folder with materials "
                "required to work through tutorial ") +
        QString::number(ntutorial) +
        QString(" from the LAMMPS tutorials article by Simon Gravelle, Cecilia Alvares, "
                "Jake Gissinger, and Axel Kohlmeyer.</p>"
                "<p>The materials for this tutorial are downloaded from:<br><b><a "
                "href=\"https://github.com/lammpstutorials/lammpstutorials-article\">https://"
                "github.com/lammpstutorials/lammpstutorials-article</a></b></p>") +
        infotext);
    label->setWordWrap(true);

    auto *layout = new QVBoxLayout;
    layout->addWidget(label);
    page->setLayout(layout);
    return page;
}

QWizardPage *LammpsGui::tutorial_directory(const int ntutorial)
{
    QSettings settings;
    settings.beginGroup("tutorial");
    auto *page = new QWizardPage;
    page->setTitle(QString("Select Directory for Tutorial %1").arg(ntutorial));
    page->setPixmap(QWizard::WatermarkPixmap,
                    QPixmap(QString(":/icons/tutorial%1-logo.png").arg(ntutorial)));

    auto *label = new QLabel(
        QString("<p>Select a directory to store the files for tutorial %1.  The directory will be "
                "created if necessary and LAMMPS-GUI will download the files required for the "
                "tutorial.  If selected, an existing directory may be cleared from old "
                "files.</p>\n<p>Available files of the tutorial solution may be downloaded to a "
                "sub-folder called \"solution\", if requested.</p>\n")
            .arg(ntutorial));
    label->setWordWrap(true);

    auto *layout = new QVBoxLayout;
    layout->addWidget(label);

    auto *dirlayout = new QHBoxLayout;
    auto *directory = new QLineEdit;

    // if we are already in a tutorial folder, stay there or pick folder in same parent dir
    bool haveDir = false;
    for (int i = 1; i < 99; ++i) {
        if (current_dir.endsWith(QString("tutorial%1").arg(i))) {
            if (i > 9) { // We assume there are no more than 99 tutorials
                current_dir.chop(2);
            } else {
                current_dir.chop(1);
            }
            current_dir.append(QString::number(ntutorial));
            haveDir = true;
        }
    }

    // if current dir is home, or application folder, switch to desktop path
    if ((current_dir == QDir::homePath()) || current_dir.contains("AppData") ||
        current_dir.contains("system32") || current_dir.contains("Program Files")) {
        current_dir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    if (!haveDir) current_dir.append(QString("/tutorial%1").arg(ntutorial));
    directory->setText(current_dir);

    auto *dirbutton = new QPushButton("&Choose");
    dirlayout->addWidget(directory);
    dirlayout->addWidget(dirbutton);
    directory->setObjectName("t_directory");
    connect(dirbutton, &QPushButton::released, this, &LammpsGui::get_directory);
    layout->addLayout(dirlayout);

    auto *purgeval = new QCheckBox("&Remove existing files from directory");
    auto *solval   = new QCheckBox("&Download solutions");

    purgeval->setChecked(false);
    purgeval->setObjectName("t_dirpurge");
    layout->addWidget(purgeval, Qt::AlignVCenter | Qt::AlignLeft);

    solval->setChecked(settings.value("solution", false).toBool());
    solval->setObjectName("t_getsolution");
    layout->addWidget(solval, Qt::AlignVCenter | Qt::AlignLeft);

    // we have tutorials 1 to 8 currently available online

    QCheckBox *webval = nullptr;
    if ((ntutorial > 0) && (ntutorial < 9)) {
        webval = new QCheckBox("&Open tutorial webpage in web browser");
        webval->setChecked(settings.value("webpage", true).toBool());
        webval->setObjectName("t_webopen");
        layout->addWidget(webval, Qt::AlignVCenter | Qt::AlignLeft);
    }

    auto *label2 = new QLabel(
        QString("<hr width=\"33%\">\n<p align=\"center\">Click on "
                "the \"Finish\" button to complete the setup and start the download.</p>"));
    label2->setWordWrap(false);

    layout->addWidget(label2);
    settings.endGroup();

    page->setLayout(layout);
    return page;
}

void LammpsGui::start_tutorial1()
{
    delete wizard;
    wizard = new TutorialWizard(1);
    const auto infotext =
        QString("<p>In tutorial 1 you will learn about LAMMPS input files, their syntax and "
                "structure, how to create and set up models and their interactions, how to run a "
                "minimization and a molecular dynamics trajectory, how to plot thermodynamic data "
                "and how to create visualizations of your system</p><hr width=\"33%\"\\>\n<p "
                "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(1, infotext));
    wizard->addPage(tutorial_directory(1));
    wizard->setWindowTitle("Tutorial 1 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial2()
{
    delete wizard;
    wizard = new TutorialWizard(2);
    const auto infotext =
        QString("<p>In tutorial 2 you will learn about setting up a simulation for a molecular "
                "system with bonds.  The target is to simulate a carbon nanotube with a "
                "conventional molecular force field under growing strain and observe the response "
                "to it.  Since bonds are represented by a harmonic potential, they cannot break.  "
                "This is then compared to simulating the same system with a reactive force field "
                "(AIREBO) where bonds may be broken and formed.</p><hr width=\"33%\"\\>\n<p "
                "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(2, infotext));
    wizard->addPage(tutorial_directory(2));
    wizard->setWindowTitle("Tutorial 2 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial3()
{
    delete wizard;
    wizard              = new TutorialWizard(3);
    const auto infotext = QString(
        "<p>In tutorial 3 you will learn setting up a multi-component, a polymer molecule embedded "
        "in liquid water.  The model employs a long-range Coulomb solver and a stretching force is "
        "applied to the polymer. This is used to demonstrate how to use the type label facility in "
        "LAMMPS to make components more generic.</p><hr width=\"33%\"\\>\n<p "
        "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(3, infotext));
    wizard->addPage(tutorial_directory(3));
    wizard->setWindowTitle("Tutorial 3 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial4()
{
    delete wizard;
    wizard = new TutorialWizard(4);
    const auto infotext =
        QString("<p>In tutorial 4 an electrolyte is simulated while confined between two walls and "
                "thus illustrating the specifics of simulating systems with fluid-solid "
                "interfaces.  The water model is more complex than in tutorial 3 and also a "
                "non-equilibrium MD simulation is performed by imposing shearing forces on the "
                "electrolyte through moving the walls.</p><hr width=\"33%\"\\>\n<p "
                "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(4, infotext));
    wizard->addPage(tutorial_directory(4));
    wizard->setWindowTitle("Tutorial 4 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial5()
{
    delete wizard;
    wizard = new TutorialWizard(5);
    const auto infotext =
        QString("<p>Tutorial 5 demonstrates the use of the ReaxFF reactive force field which "
                "includes a dynamic bond topology based on determining the bond order.  ReaxFF "
                "includes charge equilibration (QEq) and thus the atoms can change their partial "
                "charges according to the local environment.</p><hr width=\"33%\"\\>\n<p "
                "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(5, infotext));
    wizard->addPage(tutorial_directory(5));
    wizard->setWindowTitle("Tutorial 5 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial6()
{
    delete wizard;
    wizard              = new TutorialWizard(6);
    const auto infotext = QString(
        "<p>In tutorial 6 an MD simulation is combined with Monte Carlo (MC) steps to implement "
        "a Grand Canonical ensemble.  This represents an open system where atoms or "
        "molecules may be exchanged with a reservoir.</p><hr width=\"33%\"\\>\n<p "
        "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(6, infotext));
    wizard->addPage(tutorial_directory(6));
    wizard->setWindowTitle("Tutorial 6 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial7()
{
    delete wizard;
    wizard = new TutorialWizard(7);
    const auto infotext =
        QString("<p>In tutorial 7 you will determine the height of a free energy barrier through "
                "using umbrella sampling.  This is one of many advanced methods using specific "
                "reaction coordinates or so-called collective variables to map out relevant parts "
                "of free energy landscapes, where unbiased MD or MC simulation may take too "
                "long.</p><hr width=\"33%\"\\>\n<p align=\"center\">Click on the \"Next\" button "
                "to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(7, infotext));
    wizard->addPage(tutorial_directory(7));
    wizard->setWindowTitle("Tutorial 7 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::start_tutorial8()
{
    delete wizard;
    wizard = new TutorialWizard(8);
    const auto infotext =
        QString("<p>In tutorial 8 a CNT embedded in a Nylon-6,6 polymer melt is simulated.  The "
                "REACTER protocol is used to model the polymerization of Nylon without having to "
                "employ far more computationally demanding models like ReaxFF.  Also, the "
                "formation of water molecules is tracked over time.</p><hr width=\"33%\"\\>\n<p "
                "align=\"center\">Click on the \"Next\" button to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorial_intro(8, infotext));
    wizard->addPage(tutorial_directory(8));
    wizard->setWindowTitle("Tutorial 8 Setup Wizard");
    wizard->setWizardStyle(QWizard::ModernStyle);
    wizard->show();
}

void LammpsGui::howto()
{
    QDesktopServices::openUrl(QUrl("https://lammps-gui.lammps.org/"));
}

void LammpsGui::defaults()
{
    QSettings settings;
    settings.clear();
    settings.sync();
}

void LammpsGui::edit_variables()
{
    QList<QPair<QString, QString>> newvars = variables;
    SetVariables vars(newvars);
    vars.setFont(font());
    if (vars.exec() == QDialog::Accepted) {
        variables = newvars;
        if (lammps.is_running()) {
            stop_run();
            runner->wait();
            delete runner;
        }
        silence_stdout();
        lammps.close();
        restore_stdout();
        lammpsstatus->hide();
    }
}

void LammpsGui::findandreplace()
{
    FindAndReplace find(textEdit, this);
    find.setFont(font());
    find.setObjectName("find");
    find.exec();
}

void LammpsGui::preferences()
{
    // default settings are committed to QSettings during initialization of LAMMPS-GUI
    QSettings settings;
    int oldthreads   = settings.value("nthreads", 1).toInt();
    int oldaccel     = settings.value("accelerator", AcceleratorTab::None).toInt();
    bool oldecho     = settings.value("echo", false).toBool();
    bool oldcite     = settings.value("cite", false).toBool();
    int oldiprec     = settings.value("intelprec", AcceleratorTab::Mixed).toInt();
    bool oldgpuneigh = settings.value("gpuneigh", true).toBool();
    bool oldgpupair  = settings.value("gpupaironly", false).toBool();

    Preferences prefs(&lammps);
    prefs.setFont(font());
    prefs.setObjectName("preferences");
    if (prefs.exec() == QDialog::Accepted) {
        // must delete LAMMPS instance after preferences have changed that require
        // using different command line flags when creating the LAMMPS instance like
        // suffixes or package commands
        int newthreads = settings.value("nthreads", nthreads).toInt();
        int newaccel   = settings.value("accelerator", AcceleratorTab::None).toInt();
        int newiprec   = settings.value("intelprec", AcceleratorTab::Mixed).toInt();
        if ((oldaccel != newaccel) || (oldthreads != newthreads) || (oldiprec != newiprec) ||
            (oldecho != settings.value("echo", false).toBool()) ||
            (oldcite != settings.value("cite", false).toBool()) ||
            (oldgpuneigh != settings.value("gpuneigh", true).toBool()) ||
            (oldgpupair != settings.value("gpupaironly", false).toBool())) {
            if (lammps.is_running()) {
                stop_run();
                runner->wait();
                delete runner;
            }
            silence_stdout();
            lammps.close();
            restore_stdout();
            lammpsstatus->hide();
            // reset nthreads if accelerator does not support threads
            if ((newaccel == AcceleratorTab::Opt) || (newaccel == AcceleratorTab::None))
                nthreads = 1;
            else
                nthreads = newthreads;

            qputenv("OMP_NUM_THREADS", QByteArray::number(nthreads));
        }
        if (imagewindow) imagewindow->createImage();
        settings.beginGroup("reformat");
        textEdit->setReformatOnReturn(settings.value("return", false).toBool());
        textEdit->setAutoComplete(settings.value("automatic", true).toBool());
        settings.endGroup();
    }
}

void LammpsGui::start_lammps()
{
    // temporary extend lammps_args with additional arguments
    int initial_narg = lammps_args.size();
    QSettings settings;
    int accel = settings.value("accelerator", AcceleratorTab::None).toInt();
    // if non-threaded accelerator selected reset threads
    if ((accel == AcceleratorTab::None) || (accel == AcceleratorTab::Opt)) {
        nthreads = 1;
    }
    qputenv("OMP_NUM_THREADS", QByteArray::number(nthreads));

    if (accel == AcceleratorTab::Opt) {
        lammps_args.push_back(mystrdup("-suffix"));
        lammps_args.push_back(mystrdup("opt"));
    } else if (accel == AcceleratorTab::OpenMP) {
        lammps_args.push_back(mystrdup("-suffix"));
        lammps_args.push_back(mystrdup("omp"));
        lammps_args.push_back(mystrdup("-pk"));
        lammps_args.push_back(mystrdup("omp"));
        lammps_args.push_back(mystrdup(std::to_string(nthreads)));
    } else if (accel == AcceleratorTab::Intel) {
        lammps_args.push_back(mystrdup("-suffix"));
        if (lammps.config_has_package("OPENMP")) {
            lammps_args.push_back(mystrdup("hybrid"));
            lammps_args.push_back(mystrdup("intel"));
            lammps_args.push_back(mystrdup("omp"));
            lammps_args.push_back(mystrdup("-pk"));
            lammps_args.push_back(mystrdup("omp"));
            lammps_args.push_back(mystrdup(std::to_string(nthreads)));
        } else {
            lammps_args.push_back(mystrdup("intel"));
        }
        lammps_args.push_back(mystrdup("-pk"));
        lammps_args.push_back(mystrdup("intel"));
        lammps_args.push_back(mystrdup("0"));
        lammps_args.push_back(mystrdup("omp"));
        lammps_args.push_back(mystrdup(std::to_string(nthreads)));
        lammps_args.push_back(mystrdup("mode"));
        int iprec = settings.value("intelprec", AcceleratorTab::Mixed).toInt();
        if (iprec == AcceleratorTab::Double)
            lammps_args.push_back(mystrdup("double"));
        else if (iprec == AcceleratorTab::Mixed)
            lammps_args.push_back(mystrdup("mixed"));
        else if (iprec == AcceleratorTab::Single)
            lammps_args.push_back(mystrdup("single"));
        else // use mixed precision for invalid value so there is no syntax error crash
            lammps_args.push_back(mystrdup("mixed"));
    } else if (accel == AcceleratorTab::Gpu) {
        lammps_args.push_back(mystrdup("-suffix"));
        if ((nthreads > 1) && lammps.config_has_package("OPENMP")) {
            lammps_args.push_back(mystrdup("hybrid"));
            lammps_args.push_back(mystrdup("gpu"));
            lammps_args.push_back(mystrdup("omp"));
            lammps_args.push_back(mystrdup("-pk"));
            lammps_args.push_back(mystrdup("omp"));
            lammps_args.push_back(mystrdup(std::to_string(nthreads)));
        } else {
            lammps_args.push_back(mystrdup("gpu"));
        }
        lammps_args.push_back(mystrdup("-pk"));
        lammps_args.push_back(mystrdup("gpu"));
        lammps_args.push_back(mystrdup("1")); // can use only one GPU without MPI
        lammps_args.push_back(mystrdup("omp"));
        lammps_args.push_back(mystrdup(std::to_string(nthreads)));
        lammps_args.push_back(mystrdup("neigh"));
        if (settings.value("gpuneigh", true).toBool())
            lammps_args.push_back(mystrdup("yes"));
        else
            lammps_args.push_back(mystrdup("no"));
        lammps_args.push_back(mystrdup("pair/only"));
        if (settings.value("gpupaironly", false).toBool())
            lammps_args.push_back(mystrdup("on"));
        else
            lammps_args.push_back(mystrdup("off"));
    } else if (accel == AcceleratorTab::Kokkos) {
        lammps_args.push_back(mystrdup("-kokkos"));
        lammps_args.push_back(mystrdup("on"));
        lammps_args.push_back(mystrdup("t"));
        lammps_args.push_back(mystrdup(std::to_string(nthreads)));
        lammps_args.push_back(mystrdup("-suffix"));
        lammps_args.push_back(mystrdup("kk"));
    }
    if (settings.value("echo", false).toBool()) {
        lammps_args.push_back(mystrdup("-echo"));
        lammps_args.push_back(mystrdup("screen"));
    }
    if (settings.value("cite", false).toBool()) {
        lammps_args.push_back(mystrdup("-cite"));
        lammps_args.push_back(mystrdup("screen"));
    }

    // add variables, if defined
    for (auto &var : variables) {
        QString name  = var.first;
        QString value = var.second;
        if (!name.isEmpty() && !value.isEmpty()) {
            lammps_args.push_back(mystrdup("-var"));
            lammps_args.push_back(mystrdup(name));
            for (const auto &v : value.split(' '))
                lammps_args.push_back(mystrdup(v));
        }
    }

    char **args = lammps_args.data();
    int narg    = lammps_args.size();
    lammps.open(narg, args);
    lammpsstatus->show();

    // Must have a LAMMPS version that was released after the 22 July 2025 stable version
    if (lammps.version() < 20250722) {
        critical(this, "LAMMPS-GUI Error", "Incompatible LAMMPS Version:",
                 "LAMMPS-GUI version " LAMMPS_GUI_VERSION " requires\n"
                 "a LAMMPS version of at least 22 July 2025 update 2");
        exit(1);
    }

    // delete additional arguments again (3 were there initially)
    while ((int)lammps_args.size() > initial_narg) {
        delete[] lammps_args.back();
        lammps_args.pop_back();
    }

    if (lammps.has_error()) {
        char errorbuf[DEFAULT_BUFLEN];
        lammps.get_last_error_message(errorbuf, DEFAULT_BUFLEN);

        critical(this, "LAMMPS-GUI Error", "Error launching LAMMPS:", errorbuf);
    }
}

bool LammpsGui::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        autoSave();
        quit();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

// LAMMPS geturl command template with current location of the input and solution files on the web
static const QString geturl =
    "geturl https://raw.githubusercontent.com/lammpstutorials/"
    "lammpstutorials-article/refs/heads/main/files/tutorial%1/%2 output %2 verify no";

void LammpsGui::setup_tutorial(int tutno, const QString &dir, bool purgedir, bool getsolution,
                               bool openwebpage)
{
    char errorbuf[DEFAULT_BUFLEN];

    if (!lammps.config_has_curl_support()) {
        critical(this, "LAMMPS-GUI Error", "Failed to download tutorial files",
                 "<p align=\"center\">LAMMPS must be compiled with libcurl to support downloading "
                 "files</p>");
        return;
    }

    QDir directory(dir);
    directory.cd(dir);

    // open web page of the corresponding online tutorial
    if (openwebpage) {
        QString weburl = "https://lammpstutorials.github.io/sphinx/build/html/tutorial%1/%2.html";
        switch (tutno) {
            case 1:
                weburl = weburl.arg(tutno).arg("lennard-jones-fluid");
                break;
            case 2:
                weburl = weburl.arg(tutno).arg("breaking-a-carbon-nanotube");
                break;
            case 3:
                weburl = weburl.arg(tutno).arg("polymer-in-water");
                break;
            case 4:
                weburl = weburl.arg(tutno).arg("nanosheared-electrolyte");
                break;
            case 5:
                weburl = weburl.arg(tutno).arg("reactive-silicon-dioxide");
                break;
            case 6:
                weburl = weburl.arg(tutno).arg("water-adsorption-in-silica");
                break;
            case 7:
                weburl = weburl.arg(tutno).arg("free-energy-calculation");
                break;
            case 8:
                weburl = weburl.arg(tutno).arg("reactive-molecular-dynamics");
                break;
            default:
                weburl = "https://lammpstutorials.github.io/";
        }
        QDesktopServices::openUrl(QUrl(weburl));
    }

    if (purgedir) purge_directory(dir);
    if (getsolution) directory.mkpath("solution");

    start_lammps();
    lammps.command("clear");
    lammps.command(QString("shell cd '%1'").arg(dir));

    // apply https proxy setting: prefer environment variable or fall back to preferences value
    auto https_proxy = QString::fromLocal8Bit(qgetenv("https_proxy"));
    if (https_proxy.isEmpty()) https_proxy = QSettings().value("https_proxy", "").toString();
    if (!https_proxy.isEmpty()) lammps.command(QString("shell putenv https_proxy=") + https_proxy);

    // download and process manifest for selected tutorial
    // must check for error after download, e.g. when there is no network.
    lammps.command(geturl.arg(tutno).arg(".manifest"));
    if (lammps.has_error()) {
        lammps.get_last_error_message(errorbuf, DEFAULT_BUFLEN);
        critical(this, "LAMMPS-GUI Error", "Tutorial files download error:", QString(errorbuf));
        return;
    }

    QFile manifest(".manifest");
    QString line, first;
    struct DownloadItem {
        DownloadItem(int _n, const QString &_f) : ntutorial(_n), fname(_f) {}

        int ntutorial;
        QString fname;
    };

    QList<DownloadItem> downloads;
    if (manifest.open(QIODevice::ReadOnly)) {
        while (!manifest.atEnd()) {
            line = (const char *)manifest.readLine();
            line = line.trimmed();

            // skip empty and comment lines
            if (line.isEmpty() || line.startsWith('#')) continue;

            // file in subfolder
            if (line.contains('/')) {
                if (getsolution && line.startsWith("solution")) {
                    downloads.append(DownloadItem(tutno, line));
                }
            } else {
                // first file is the initial template
                if (first.isEmpty()) first = line;
                downloads.append(DownloadItem(tutno, line));
            }
        }
        manifest.close();
        manifest.remove();
    }

    int i   = 0;
    int num = downloads.size();
    if (!num) num = 1;

    progress->setValue(0);
    progress->show();
    dirstatus->hide();

    for (const auto &item : downloads) {
        ++i;
        status->setText(QString("Downloading file %1 of %2").arg(i).arg(num));
        progress->setValue((int)((double)i / ((double)num) * 1000.0));
        status->repaint();
        lammps.command(geturl.arg(item.ntutorial).arg(item.fname));

        // download failed. abort, restore status line, and launch error dialog
        if (lammps.has_error()) {
            status->setText("Error.");
            progress->hide();
            dirstatus->show();
            status->repaint();
            lammps.get_last_error_message(errorbuf, DEFAULT_BUFLEN);
            critical(this, "LAMMPS-GUI Error", "Tutorial files download error:", QString(errorbuf));
            return;
        }

        // check if download is a placeholder for a symbolic link and make a copy instead.
        QFile dlfile(item.fname);
        QFileInfo dlpath(item.fname);
        if (dlfile.open(QIODevice::ReadOnly)) {
            line = (const char *)dlfile.readLine();
            line = line.trimmed();
            dlfile.close();

#if defined(_WIN32)
            if (line == QString("../") + dlpath.fileName())
                // must replace "/" path separator with "\" on Windows
                lammps.command(QString("shell copy /y %1 %2")
                                   .arg(dlpath.fileName())
                                   .arg(QString(item.fname).replace('/', '\\')));
#else
            if (line == QString("../") + dlpath.fileName())
                lammps.command(QString("shell cp -f %1 %2").arg(dlpath.fileName()).arg(item.fname));
#endif
        }
    }
    progress->setValue(1000);
    status->setText("Ready.");
    progress->hide();
    dirstatus->show();
    status->repaint();
    if (!first.isEmpty()) open_file(first);
}

TutorialWizard::TutorialWizard(int ntutorial, QWidget *parent) :
    QWizard(parent), _ntutorial(ntutorial)
{
    setWindowIcon(QIcon(":/icons/tutorial-logo.png"));
}

// actions to perform when the wizard for tutorial 1 is complete
// and the user has clicked on "Finish"

void TutorialWizard::accept()
{
    // get pointers to the widgets with the information we need
    auto *dirname    = findChild<QLineEdit *>("t_directory");
    auto *dirpurge   = findChild<QCheckBox *>("t_dirpurge");
    auto *getsol     = findChild<QCheckBox *>("t_getsolution");
    auto *webopen    = findChild<QCheckBox *>("t_webopen");
    bool purgedir    = false;
    bool getsolution = false;
    bool openwebpage = false;
    QString curdir;

    if (webopen) openwebpage = webopen->isChecked();

    // create and populate directory.
    if (dirname) {
        QDir directory;
        curdir = dirname->text().trimmed();
        if (!directory.mkpath(curdir)) {
            warning(this, "LAMMPS-GUI Warning",
                    QString("Cannot create tutorial %1 working directory '%2'.")
                        .arg(_ntutorial)
                        .arg(curdir),
                    "Going back to directory selection.");
            back();
            return;
        }

        purgedir    = dirpurge && dirpurge->isChecked();
        getsolution = getsol && getsol->isChecked();
    }
    QDialog::accept();

    // get hold of LAMMPS-GUI main widget
    if (dirname) {
        auto *main = dynamic_cast<LammpsGui *>(get_main_widget());
        if (main) main->setup_tutorial(_ntutorial, curdir, purgedir, getsolution, openwebpage);
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
