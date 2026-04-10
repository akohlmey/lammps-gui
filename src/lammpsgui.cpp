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
#include "tutorialwizard.h"
#include "urldownloader.h"

#include <QAction>
#include <QApplication>
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
    setObjectName("LammpsGui");
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
        recentActions[i] = new QAction(QIcon(":/icons/document-open-recent.png"),
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

    actionSearchAndReplace = new QAction(QIcon(":/icons/search.png"), "&Find and Replace...", this);
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

    actionRunFile = new QAction(QIcon(":/icons/run-file.png"), "Run LAMMPS from &File", this);
    actionRunFile->setShortcut(QKeySequence("Ctrl+Shift+Return"));

    actionStopLAMMPS = new QAction(QIcon(":/icons/process-stop.png"), "&Stop LAMMPS", this);
    actionStopLAMMPS->setShortcut(QKeySequence("Ctrl+/"));

    actionRestartLAMMPS =
        new QAction(QIcon(":/icons/system-restart.png"), "Relaunch &LAMMPS Instance", this);

    actionSetVariables =
        new QAction(QIcon(":/icons/preferences-desktop-personal.png"), "Set &Variables...", this);
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

    actionViewImageWindow = new QAction(QIcon(":/icons/emblem-photos.png"), "&Image Window", this);
    actionViewImageWindow->setShortcut(QKeySequence("Ctrl+Shift+I"));

    actionViewSlideShow =
        new QAction(QIcon(":/icons/image-x-generic.png"), "&Slide Show Window", this);
    actionViewSlideShow->setShortcut(QKeySequence("Ctrl+L"));

    actionViewVariableWindow =
        new QAction(QIcon(":/icons/preferences-desktop-personal.png"), "&Variables Window", this);
    actionViewVariableWindow->setShortcut(QKeySequence("Ctrl+Shift+W"));

    // Tutorial actions
    for (int i = 0; i < 8; ++i) {
        tutorialActions[i] = new QAction(QIcon(":/icons/tutorial-logo.png"),
                                         QString("Start LAMMPS Tutorial &%1").arg(i + 1), this);
    }

    // About actions
    actionAboutLAMMPSGUI = new QAction(QIcon(":/icons/help-about.png"), "&About LAMMPS-GUI", this);
    actionAboutLAMMPSGUI->setShortcut(QKeySequence("Ctrl+Shift+A"));

    actionHelp = new QAction(QIcon(":/icons/help-faq.png"), "Quick &Help", this);
    actionHelp->setShortcut(QKeySequence("Ctrl+Shift+H"));

    actionHowto = new QAction(QIcon(":/icons/system-help.png"), "LAMMPS-&GUI Documentation", this);
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
    QMainWindow(parent), highlighter(nullptr), capturer(nullptr), status(nullptr), cpuuse(nullptr),
    logwindow(nullptr), imagewindow(nullptr), chartwindow(nullptr), slideshow(nullptr),
    logupdater(nullptr), dirstatus(nullptr), progress(nullptr), prefdialog(nullptr),
    lammpsstatus(nullptr), varwindow(nullptr), wizard(nullptr), runner(nullptr), isRunning(false),
    runCounter(0), nthreads(1), mainx(width), mainy(height)
{
    docver = "";
    setupUi();
    textEdit->document()->setPlainText(citeme);
    textEdit->document()->setModified(false);
    textEdit->setStyleSheet(bannerstyle);
    highlighter = new Highlighter(textEdit->document());
    capturer    = new StdCapture;
    currentFile.clear();
    currentDir = QDir(".").absolutePath();
    // use $HOME if we get dropped to "/" like on macOS or the installation folder or
    // system folder like on Windows
    if ((currentDir == "/") || (currentDir.contains("AppData")) ||
        (currentDir.contains("system32")))
        currentDir = QDir::homePath();
    QDir::setCurrent(currentDir);

    inspectList.clear();
    setAutoFillBackground(true);

    // restore and initialize settings
    QSettings settings;

#if defined(LAMMPS_GUI_USE_PLUGIN)
    // first try to load from existing setting
    pluginPath = settings.value("plugin_path", "").toString();
    if (!pluginPath.isEmpty()) {
        // make canonical and try loading; reset to empty string if loading failed
        pluginPath = QFileInfo(pluginPath).canonicalFilePath();
        if (!lammps.loadLib(pluginPath)) {
            pluginPath.clear();
            // could not load successfully -> remove any existing setting.
            settings.remove("plugin_path");
        }
    }

    if (pluginPath.isEmpty()) {
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
            if (lammps.loadLib(libpath)) {
                pluginPath = libpath;
                settings.setValue("plugin_path", pluginPath);
                settings.sync();
                break;
            }
        }

        // No suitable plugin found automatically.  Show a dialog with three choices:
        // 1) Exit LAMMPS-GUI
        // 2) Browse the filesystem for a suitable shared library file
        // 3) Download a pre-compiled shared library from the LAMMPS webserver
        while (pluginPath.isEmpty()) {
            // remove key so we won't get stuck in a loop reading a bad file
            settings.remove("plugin_path");

            QMessageBox msgBox(this);
            msgBox.setWindowTitle("LAMMPS-GUI - No LAMMPS Library");
            msgBox.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
            msgBox.setIcon(QMessageBox::Warning);
            msgBox.setText("No suitable LAMMPS shared library found.");
            msgBox.setInformativeText(
                "<p align=\"justify\">Either the shared library path has been reset, the "
                "configured or default library file was not found, or the selected library failed "
                "to load.</p><p align=\"justify\">You may either download a pre-compiled LAMMPS "
                "shared library file for your platform from the LAMMPS webserver, browse the "
                "filesystem for a suitable LAMMPS library file, or exit LAMMPS-GUI now.</p>");

            auto *exitBtn     = msgBox.addButton("Exit", QMessageBox::DestructiveRole);
            auto *browseBtn   = msgBox.addButton("Browse Filesystem...", QMessageBox::AcceptRole);
            auto *downloadBtn = msgBox.addButton("Download Library...", QMessageBox::ResetRole);
            exitBtn->setIcon(QIcon(":/icons/application-exit.png"));
            browseBtn->setIcon(QIcon(":/icons/document-open.png"));
            downloadBtn->setIcon(QIcon(":/icons/download-file.png"));
            msgBox.setDefaultButton(downloadBtn);
            msgBox.setEscapeButton(exitBtn);
            msgBox.exec();

            if (msgBox.clickedButton() == exitBtn) {
                exit(1);

            } else if (msgBox.clickedButton() == browseBtn) {
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
                    const char *path = mystrdup(QCoreApplication::applicationFilePath());
                    const char *arg0 = mystrdup(QCoreApplication::arguments().at(0));
                    execl(path, arg0, (char *)nullptr);
                    critical(this, "LAMMPS-GUI Error", "Relaunching LAMMPS-GUI failed.",
                             "LAMMPS-GUI must be restarted to correctly load the selected "
                             "LAMMPS shared library. Click on 'Close' to exit.");
                    exit(1);
                }
                // user cancelled file dialog -> loop back to show the dialog again

            } else if (msgBox.clickedButton() == downloadBtn) {
                // determine platform-specific library file name and config directory
#if defined(Q_OS_MACOS)
                const QString libName = "liblammps.0.dylib";
#elif defined(Q_OS_WIN32)
                const QString libName = "liblammps.dll";
#else
                const QString libName = "liblammps.so.0";
#endif
                // store in the same config directory where QSettings stores preferences
                QString configDir =
                    QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
                if (configDir.isEmpty() || !QDir().mkpath(configDir)) {
                    critical(this, "LAMMPS-GUI Error", "Cannot determine configuration directory.",
                             "Unable to create a directory for storing the "
                             "downloaded LAMMPS shared library.");
                    continue;
                }
                QString libPath = configDir + QDir::separator() + libName;

                QString downloadUrl =
                    QString("https://download.lammps.org/lammps-gui/%1").arg(libName);

                URLDownloader downloader(this);
                if (downloader.download(downloadUrl, libPath, true)) {
                    // try loading the downloaded library
                    if (lammps.loadLib(libPath)) {
                        pluginPath = libPath;
                        settings.setValue("plugin_path", pluginPath);
                        settings.sync();
                    } else {
                        QFile::remove(libPath);
                        critical(this, "LAMMPS-GUI Error",
                                 "Downloaded LAMMPS library could not be loaded.",
                                 "<p align=\"justify\">The downloaded shared library file "
                                 "is not compatible with this system.</p>");
                    }
                } else {
                    critical(this, "LAMMPS-GUI Error", "Failed to download LAMMPS shared library.",
                             downloader.errorString());
                }
            }
        }
    }
#endif

    // default accelerator package is OPENMP, but we switch the configured accelerator to
    // "none" if the selected package is not available to have an option that always works
    int accel = settings.value("accelerator", AcceleratorTab::OpenMP).toInt();
    switch (accel) {
        case AcceleratorTab::Opt:
            if (!lammps.configHasPackage("OPT")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::OpenMP:
            if (!lammps.configHasPackage("OPENMP")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Intel:
            if (!lammps.configHasPackage("INTEL")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Gpu:
            if (!lammps.configHasPackage("GPU")) accel = AcceleratorTab::None;
            break;
        case AcceleratorTab::Kokkos:
            if (!lammps.configHasPackage("KOKKOS")) accel = AcceleratorTab::None;
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
    lammpsArgs.clear();
    lammpsArgs.push_back(mystrdup("LAMMPS-GUI"));
    lammpsArgs.push_back(mystrdup("-log"));
    lammpsArgs.push_back(mystrdup("none"));

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

    updateRecents();

    // check if we have OVITO and VMD installed and deactivate actions if not
    actionViewInOVITO->setEnabled(hasExe("ovito"));
    actionViewInOVITO->setData("ovito");
    actionViewInVMD->setEnabled(hasExe("vmd"));
    actionViewInVMD->setData("vmd");

    connect(actionNew, &QAction::triggered, this, &LammpsGui::newDocument);
    connect(actionOpen, &QAction::triggered, this, &LammpsGui::open);
    connect(actionSave, &QAction::triggered, this, &LammpsGui::save);
    connect(actionSaveAs, &QAction::triggered, this, &LammpsGui::saveAs);
    connect(actionView, &QAction::triggered, this, &LammpsGui::view);
    connect(actionInspect, &QAction::triggered, this, &LammpsGui::inspect);
    connect(actionQuit, &QAction::triggered, this, &LammpsGui::quit);
    connect(actionCopy, &QAction::triggered, this, &LammpsGui::copy);
    connect(actionCut, &QAction::triggered, this, &LammpsGui::cut);
    connect(actionPaste, &QAction::triggered, this, &LammpsGui::paste);
    connect(actionUndo, &QAction::triggered, this, &LammpsGui::undo);
    connect(actionRedo, &QAction::triggered, this, &LammpsGui::redo);
    connect(actionSearchAndReplace, &QAction::triggered, this, &LammpsGui::findAndReplace);
    connect(actionRunBuffer, &QAction::triggered, this, &LammpsGui::runBuffer);
    connect(actionRunFile, &QAction::triggered, this, &LammpsGui::runFile);
    connect(actionStopLAMMPS, &QAction::triggered, this, &LammpsGui::stopRun);
    connect(actionRestartLAMMPS, &QAction::triggered, this, &LammpsGui::restartLammps);
    connect(actionSetVariables, &QAction::triggered, this, &LammpsGui::editVariables);
    connect(actionImage, &QAction::triggered, this, &LammpsGui::renderImage);
    connect(actionLAMMPSTutorial, &QAction::triggered, this, &LammpsGui::tutorialWeb);
    for (int i = 0; i < 8; ++i)
        connect(tutorialActions[i], &QAction::triggered, this, [this, i]() {
            startTutorial(i + 1);
        });
    connect(actionAboutLAMMPSGUI, &QAction::triggered, this, &LammpsGui::about);
    connect(actionHelp, &QAction::triggered, this, &LammpsGui::help);
    connect(actionHowto, &QAction::triggered, this, &LammpsGui::howto);
    connect(actionLAMMPSManual, &QAction::triggered, this, &LammpsGui::manual);
    connect(actionPreferences, &QAction::triggered, this, &LammpsGui::preferences);
    connect(actionDefaults, &QAction::triggered, this, &LammpsGui::defaults);
    connect(actionViewInOVITO, &QAction::triggered, this, &LammpsGui::startExe);
    connect(actionViewInVMD, &QAction::triggered, this, &LammpsGui::startExe);
    connect(actionViewLogWindow, &QAction::triggered, this, &LammpsGui::viewLog);
    connect(actionViewGraphWindow, &QAction::triggered, this, &LammpsGui::viewChart);
    connect(actionViewImageWindow, &QAction::triggered, this, &LammpsGui::viewImage);
    connect(actionViewSlideShow, &QAction::triggered, this, &LammpsGui::viewSlides);
    connect(actionViewVariableWindow, &QAction::triggered, this, &LammpsGui::viewVariables);
    connect(recentActions[0], &QAction::triggered, this, &LammpsGui::openRecent);
    connect(recentActions[1], &QAction::triggered, this, &LammpsGui::openRecent);
    connect(recentActions[2], &QAction::triggered, this, &LammpsGui::openRecent);
    connect(recentActions[3], &QAction::triggered, this, &LammpsGui::openRecent);
    connect(recentActions[4], &QAction::triggered, this, &LammpsGui::openRecent);

    connect(textEdit->document(), &QTextDocument::modificationChanged, this, &LammpsGui::modified);

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
    connect(lammpsrun, &QPushButton::released, this, &LammpsGui::runBuffer);
    connect(lammpsstop, &QPushButton::released, this, &LammpsGui::stopRun);
    connect(lammpsimage, &QPushButton::released, this, &LammpsGui::renderImage);

    cpuuse = new QLabel("   0%CPU");
    cpuuse->setFixedWidth(90);
    statusbar->addWidget(cpuuse);
    cpuuse->hide();
    status = new QLabel("Ready.");
    status->setFixedWidth(300);
    statusbar->addWidget(status);
    dirstatus = new QLabel(QString(" Directory: ") + currentDir);
    dirstatus->setMinimumWidth(MINIMUM_WIDTH);
    statusbar->addWidget(dirstatus);
    progress = new QProgressBar();
    progress->setRange(0, 1000);
    progress->setMinimumWidth(MINIMUM_WIDTH);
    progress->hide();
    dirstatus->show();
    statusbar->addWidget(progress);

    if ((filename.size() > 0) && !filename.endsWith("lammps-gui.exe")) {
        openFile(filename);
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
    startLammps();
    QStringList style_list;
    char buf[DEFAULT_BUFLEN];
    QFile internal_commands(":/lammps_internal_commands.txt");
    if (internal_commands.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!internal_commands.atEnd()) {
            style_list << QString(internal_commands.readLine()).trimmed();
        }
    }
    internal_commands.close();
    int ncmds = lammps.styleCount("command");
    for (int i = 0; i < ncmds; ++i) {
        if (lammps.styleName("command", i, buf, DEFAULT_BUFLEN)) {
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
    ncmds = lammps.styleCount(#keyword);                                                       \
    for (int i = 0; i < ncmds; ++i) {                                                          \
        if (lammps.styleName(#keyword, i, buf, DEFAULT_BUFLEN)) {                              \
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

void LammpsGui::newDocument()
{
    currentFile.clear();
    textEdit->document()->setPlainText(citeme);
    textEdit->document()->setModified(false);
    textEdit->setStyleSheet(bannerstyle);

    if (lammps.isRunning()) {
        stopRun();
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

    silenceStdout();
    lammps.close();
    restoreStdout();
    lammpsstatus->hide();
    setWindowTitle("LAMMPS-GUI - Editor - *unknown*");
    runCounter = 0;
}

void LammpsGui::open()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    openFile(fileName);
}

void LammpsGui::view()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the file");
    viewFile(fileName);
}

void LammpsGui::inspect()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Open the restart file");
    inspectFile(fileName);
}

void LammpsGui::openRecent()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act) openFile(act->data().toString());
}

void LammpsGui::getDirectory()
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

void LammpsGui::startExe()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (act) {
        auto exe = act->data().toString();
        QStringList args;
        if (lammps.extractSetting("box_exist")) {
            QString datacmd = "write_data '";
            QDir datadir(QDir::tempPath());
            QFile datafile(datadir.absoluteFilePath(currentFile + ".data"));
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
                    silenceStdout();
                    lammps.command(datacmd);
                    restoreStdout();
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
                silenceStdout();
                lammps.command(datacmd);
                restoreStdout();
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

void LammpsGui::updateRecents(const QString &filename)
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

    for (int i = 0; i < 5; ++i) {
        recentActions[i]->setVisible(false);
        if (i < recent.size() && !recent[i].isEmpty()) {
            QFileInfo fi(recent[i]);
            recentActions[i]->setText(QString("&%1. ").arg(i + 1) + fi.fileName());
            recentActions[i]->setData(recent[i]);
            recentActions[i]->setVisible(true);
        }
    }
}

// delete all current variables in the LAMMPS instance
void LammpsGui::clearVariables()
{
    int nvar = lammps.idCount("variable");
    char buffer[DEFAULT_BUFLEN];

    // delete from back so they are not re-indexed
    for (int i = nvar - 1; i >= 0; --i) {
        memset(buffer, 0, DEFAULT_BUFLEN);
        if (lammps.idName("variable", i, buffer, DEFAULT_BUFLEN))
            lammps.command(QString("variable %1 delete").arg(buffer));
    }
}

void LammpsGui::updateVariables()
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
void LammpsGui::openFile(const QString &fileName)
{
    // do nothing, if no file name provided
    if (fileName.isEmpty()) return;

    if (lammps.isRunning()) {
        stopRun();
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
    silenceStdout();
    lammps.close();
    restoreStdout();

    purgeInspectList();
    textEdit->setStyleSheet("");
    if (textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + currentFile + " has changes");
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
    currentFile = path.fileName();
    currentDir  = path.absolutePath();
    QFile file(path.absoluteFilePath());

    updateRecents(path.absoluteFilePath());

    QDir::setCurrent(currentDir);
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
    setWindowTitle(QString("LAMMPS-GUI - Editor - " + currentFile));
    runCounter = 0;
    textEdit->document()->setModified(false);
    textEdit->setGroupList();
    textEdit->setVarNameList();
    textEdit->setComputeIDList();
    textEdit->setFixIDList();
    textEdit->setFileList();
    dirstatus->setText(QString(" Directory: ") + currentDir);
    status->setText("Ready.");
    cpuuse->hide();

    updateVariables();
    silenceStdout();
    lammps.close();
    restoreStdout();
}

// open file in read-only mode for viewing in separate window
void LammpsGui::viewFile(const QString &fileName)
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

void LammpsGui::purgeInspectList()
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
void LammpsGui::inspectFile(const QString &fileName)
{
    QFile file(fileName);
    auto shortName = QFileInfo(fileName).fileName();

    purgeInspectList();
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
    if (!lammps.isRunning()) {
        startLammps();
        silenceStdout();
        lammps.command("clear");
        clearVariables();
        lammps.command(QString("read_restart %1").arg(fileName));
        restoreStdout();
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
            silenceStdout();
            lammps.command(QString("write_data %1 pair ij noinit").arg(infodata));
            restoreStdout();
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

void LammpsGui::writeFile(const QString &fileName)
{
    QFileInfo path(fileName);
    currentFile = path.fileName();
    currentDir  = path.absolutePath();
    QFile file(path.absoluteFilePath());

    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        warning(this, "LAMMPS-GUI Warning", "Cannot save to file " + fileName + ":",
                file.errorString());
        return;
    }
    setWindowTitle(QString("LAMMPS-GUI - Editor - " + currentFile));
    QDir::setCurrent(currentDir);

    updateRecents(path.absoluteFilePath());

    QTextStream out(&file);
    QString text = textEdit->toPlainText();
    out << text;
    if (text.back().toLatin1() != '\n') out << "\n"; // add final newline if missing
    file.close();
    dirstatus->setText(QString(" Directory: ") + currentDir);
    // update list of files for completion since we may have changed the working directory
    textEdit->setFileList();
    textEdit->document()->setModified(false);
}

void LammpsGui::save()
{
    purgeInspectList();
    QString fileName = currentFile;
    // If we don't have a filename from before, get one.
    if (fileName.isEmpty()) fileName = QFileDialog::getSaveFileName(this, "Save");

    writeFile(fileName);
}

void LammpsGui::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Save as");
    writeFile(fileName);
}

void LammpsGui::quit()
{
    removeEventFilter(this);

    if (lammps.isRunning()) {
        stopRun();
        runner->wait();
        delete runner;
        runner = nullptr;
    }

    // close LAMMPS instance
    silenceStdout();
    lammps.close();
    restoreStdout();
    lammpsstatus->hide();
    lammps.finalize();

    autoSave();
    if (textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + currentFile + " has changes");
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

void LammpsGui::stopRun()
{
    lammps.forceTimeout();
}

void LammpsGui::logUpdate()
{
    double t_elapsed, t_remain, t_total;
    int completed = 1000;

    // estimate completion percentage
    if (lammps.isRunning()) {
        t_elapsed = lammps.getThermo("cpu");
        t_remain  = lammps.getThermo("cpuremain");
        t_total   = t_elapsed + t_remain + 1.0e-10;
        completed = t_elapsed / t_total * 1000.0;
        // update cpu usage
        int percent_cpu = (int)lammps.getThermo("cpuuse");
        // clear any pending error messages from polling those thermo keywords
        lammps.getLastErrorMessage(nullptr, 0);

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
        void *ptr = lammps.lastThermo("line", 0);
        if (ptr) {
            nline = *((int *)ptr);
            textEdit->setHighlight(nline, false);
        }

        if (varwindow) {
            int nvar = lammps.idCount("variable");
            char buffer[DEFAULT_BUFLEN];
            QString varinfo("\n");
            for (int i = 0; i < nvar; ++i) {
                memset(buffer, 0, DEFAULT_BUFLEN);
                if (lammps.variableInfo(i, buffer, DEFAULT_BUFLEN)) varinfo += buffer;
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
    void *ptr = lammps.lastThermo("step", 0);
    if (ptr) {
        if (lammps.extractSetting("bigint") == 4)
            step = *(int *)ptr;
        else
            step = (int)*(int64_t *)ptr;
    }

    // extract cached thermo data when LAMMPS is executing a minimize or run command
    if (chartwindow && lammps.isRunning()) {
        // thermo data is not yet valid during setup
        void *ptr = lammps.lastThermo("setup", 0);
        if (ptr && *(int *)ptr) return;

        lammps.lastThermo("lock", 0);
        ptr = lammps.lastThermo("num", 0);
        if (ptr) {
            int ncols = *(int *)ptr;

            // check if the column assignment has changed
            // if yes, delete charts and start over
            if (chartwindow->numCharts() > 0) {
                int count     = 0;
                bool do_reset = false;
                if (step < chartwindow->getStep()) do_reset = true;
                for (int i = 0, idx = 0; i < ncols; ++i) {
                    QString label = (const char *)lammps.lastThermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    if (!chartwindow->hasTitle(label, idx)) {
                        do_reset = true;
                    } else {
                        ++count;
                    }
                    ++idx;
                }
                if (chartwindow->numCharts() != count) do_reset = true;
                if (do_reset) chartwindow->resetCharts();
            }

            if (chartwindow->numCharts() == 0) {
                for (int i = 0; i < ncols; ++i) {
                    QString label = (const char *)lammps.lastThermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    chartwindow->addChart(label, i);
                }
            }

            for (int i = 0; i < ncols; ++i) {
                int datatype = -1;
                double data  = 0.0;
                void *ptr    = lammps.lastThermo("type", i);
                if (ptr) datatype = *(int *)ptr;
                ptr = lammps.lastThermo("data", i);
                if (ptr) {
                    if (datatype == 0) // int
                        data = *(int *)ptr;
                    else if (datatype == 2) // double
                        data = *(double *)ptr;
                    else if (datatype == 4) // bigint
                        data = (double)*(int64_t *)ptr;
                }
                chartwindow->addData(step, data, i);
            }
        }
        lammps.lastThermo("unlock", 0);
    }

    // update list of available image file names

    QString imagefile = (const char *)lammps.lastThermo("imagename", 0);
    if (!imagefile.isEmpty()) {
        if (!slideshow) {
            slideshow = new SlideShow(currentFile);
            if (QSettings().value("viewslide", true).toBool())
                slideshow->show();
            else
                slideshow->hide();
        } else {
            slideshow->setWindowTitle(
                QString("LAMMPS-GUI - Slide Show - %1 - Run %2").arg(currentFile).arg(runCounter));
            if (QSettings().value("viewslide", true).toBool()) slideshow->show();
        }
        slideshow->addImage(imagefile);
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

void LammpsGui::runDone()
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
        int thermo_val     = lammps.extractSetting("thermo_every");
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
        void *ptr = lammps.lastThermo("step", 0);
        if (ptr) {
            int step = 0;
            if (lammps.extractSetting("bigint") == 4)
                step = *(int *)ptr;
            else
                step = (int)*(int64_t *)ptr;
            int ncols = *(int *)lammps.lastThermo("num", 0);
            for (int i = 0; i < ncols; ++i) {
                if (chartwindow->numCharts() == 0) {
                    QString label = (const char *)lammps.lastThermo("keyword", i);
                    // no need to store the timestep column
                    if (label == "Step") continue;
                    chartwindow->addChart(label, i);
                }
                int datatype = *(int *)lammps.lastThermo("type", i);
                double data  = 0.0;
                if (datatype == 0) // int
                    data = *(int *)lammps.lastThermo("data", i);
                else if (datatype == 2) // double
                    data = *(double *)lammps.lastThermo("data", i);
                else if (datatype == 4) // bigint
                    data = (double)*(int64_t *)lammps.lastThermo("data", i);
                chartwindow->addData(step, data, i);
            }
        }
        chartwindow->resetZoom();
    }

    bool success = true;
    bool valid   = true;
    char errorbuf[DEFAULT_BUFLEN];

    if (lammps.hasError()) {
        lammps.getLastErrorMessage(errorbuf, DEFAULT_BUFLEN);
        // ignore "Invalid LAMMPS handle", but report other errors
        if (!strstr(errorbuf, "Invalid LAMMPS handle")) {
            success = false;
        } else {
            valid = false;
        }
    }

    int nline = CodeEditor::NO_HIGHLIGHT;
    if (valid) {
        void *ptr = lammps.lastThermo("line", 0);
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

void LammpsGui::restartLammps()
{
    if (lammps.isRunning()) {
        warning(this, "LAMMPS-GUI Warning", "Must stop current run before relaunching LAMMPS");
        return;
    }
    silenceStdout();
    lammps.close();
    restoreStdout();
};

void LammpsGui::doRun(bool use_buffer)
{
    if (lammps.isRunning()) {
        warning(this, "LAMMPS-GUI Warning", "Must stop current run before starting a new run");
        return;
    }

    purgeInspectList();
    autoSave();
    if (!use_buffer && textEdit->document()->isModified()) {
        QMessageBox msg;
        msg.setWindowTitle("Unsaved Changes");
        msg.setWindowIcon(windowIcon());
        msg.setText(QString("The buffer ") + currentFile + " has changes");
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
    startLammps();
    if (!lammps.is_open()) return;
    capturer->BeginCapture();

    runner    = new LammpsRunner(this);
    isRunning = true;
    ++runCounter;

    // must delete all variables since clear does not delete them
    clearVariables();

    // define "gui_run" variable set to runCounter value
    lammps.command(std::string("variable gui_run index " + std::to_string(runCounter)));
    if (use_buffer) {
        // always add final newline since the text edit widget does not do it
        char *input = mystrdup(textEdit->toPlainText() + "\n");
        runner->setupRun(&lammps, input, nullptr);
    } else {
        char *fname = mystrdup(currentFile);
        runner->setupRun(&lammps, nullptr, fname);
    }

    // apply https proxy setting: prefer environment variable or fall back to preferences value
    auto https_proxy = QString::fromLocal8Bit(qgetenv("https_proxy"));
    if (https_proxy.isEmpty()) https_proxy = settings.value("https_proxy", "").toString();
    if (!https_proxy.isEmpty()) lammps.command(QString("shell putenv https_proxy=") + https_proxy);

    connect(runner, &LammpsRunner::resultReady, this, &LammpsGui::runDone);
    connect(runner, &LammpsRunner::finished, runner, &QObject::deleteLater);
    runner->start();

    // if configured, delete old log window before opening new one
    if (settings.value("logreplace", true).toBool()) delete logwindow;
    logwindow = new LogWindow(currentFile);
    logwindow->setReadOnly(true);
    logwindow->setCenterOnScroll(true);
    logwindow->moveCursor(QTextCursor::End);
    logwindow->setWindowTitle(
        QString("LAMMPS-GUI - Output - %1 - Run %2").arg(currentFile).arg(runCounter));
    logwindow->setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    logwindow->setLineWrapMode(LogWindow::NoWrap);
    logwindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), logwindow);
    connect(shortcut, &QShortcut::activated, logwindow, &LogWindow::close);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), logwindow);
    connect(shortcut, &QShortcut::activated, this, &LammpsGui::stopRun);
    if (settings.value("viewlog", true).toBool())
        logwindow->show();
    else
        logwindow->hide();

    // if configured, delete old log window before opening new one
    if (settings.value("chartreplace", true).toBool()) delete chartwindow;
    chartwindow = new ChartWindow(currentFile);
    chartwindow->setWindowTitle(
        QString("LAMMPS-GUI - Charts - %2 - Run %3").arg(currentFile).arg(runCounter));
    chartwindow->setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    chartwindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    const auto *unitptr = (const char *)lammps.extractGlobal("units");
    if (unitptr) chartwindow->setUnits(QString("%1").arg(unitptr));
    auto normflag = lammps.extractSetting("thermo_norm");
    chartwindow->setNorm(normflag != 0);

    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), chartwindow);
    connect(shortcut, &QShortcut::activated, chartwindow, &ChartWindow::close);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), chartwindow);
    connect(shortcut, &QShortcut::activated, this, &LammpsGui::stopRun);
    if (settings.value("viewchart", true).toBool())
        chartwindow->show();
    else
        chartwindow->hide();

    if (slideshow) {
        slideshow->setWindowTitle(QString("LAMMPS-GUI - Slide Show - " + currentFile));
        slideshow->clear();
        slideshow->hide();
    }

    logupdater = new QTimer(this);
    connect(logupdater, &QTimer::timeout, this, &LammpsGui::logUpdate);
    logupdater->start(settings.value("updfreq", "10").toInt());
}

void LammpsGui::renderImage()
{
    // LAMMPS is not re-entrant, so we can only query LAMMPS when it is not running
    if (!lammps.isRunning()) {
        startLammps();
        if (!lammps.extractSetting("box_exist")) {
            // there is no current system defined yet.
            // so we select the input from the start to the first run or minimize command
            // add a run 0 and thus create the state of the initial system without running.
            // this will allow us to create a snapshot image.
            auto saved = textEdit->textCursor();
            if (textEdit->find(QRegularExpression(QStringLiteral(R"(^\s*(run|minimize)\s+)")))) {
                auto cursor = textEdit->textCursor();
                cursor.movePosition(QTextCursor::PreviousBlock);
                cursor.movePosition(QTextCursor::EndOfLine);
                cursor.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
                auto selection = cursor.selectedText().replace(QChar(0x2029), '\n');
                selection += "\nrun 0 pre yes post no";
                textEdit->setTextCursor(saved);
                silenceStdout();
                lammps.command("clear");
                clearVariables();
                lammps.commandsString(selection);
                restoreStdout();

                if (lammps.hasError()) {
                    char errormesg[DEFAULT_BUFLEN];
                    lammps.getLastErrorMessage(errormesg, DEFAULT_BUFLEN);
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
            if (!lammps.extractSetting("box_exist")) {
                warning(this, "ImageViewer File Creation Error",
                        "Cannot create snapshot image from an input not creating a system box");
                return;
            }
            textEdit->setTextCursor(saved);
        }
        // if configured, delete old image window before opening new one
        if (QSettings().value("imagereplace", true).toBool()) delete imagewindow;
        imagewindow = new ImageViewer(currentFile, &lammps);
        imagewindow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    } else {
        warning(this, "ImageViewer File Creation Error",
                "Cannot create snapshot image while LAMMPS is running");
        return;
    }
    imagewindow->show();
}

void LammpsGui::viewSlides()
{
    if (!slideshow) {
        slideshow = new SlideShow(currentFile);
        slideshow->setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    }
    if (slideshow->isVisible())
        slideshow->hide();
    else
        slideshow->show();
}

void LammpsGui::viewChart()
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

void LammpsGui::viewLog()
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

void LammpsGui::viewImage()
{
    if (imagewindow) {
        if (imagewindow->isVisible()) {
            imagewindow->hide();
        } else {
            imagewindow->show();
        }
    }
}

void LammpsGui::viewVariables()
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
    QString git_branch = (const char *)lammps.extractGlobal("git_branch");
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
    QString fileName = currentFile;
    if (fileName.isEmpty()) return;
    if (!textEdit->document()->isModified()) return;

    // check preference
    bool autosave = false;
    QSettings settings;
    settings.beginGroup("reformat");
    autosave = settings.value("autosave", false).toBool();
    settings.endGroup();

    if (autosave) writeFile(fileName);
}

void LammpsGui::setFont(const QFont &newFont)
{
    QMainWindow::setFont(newFont);
    if (textEdit) {
        textEdit->setFont(newFont);
        menubar->setFont(newFont);
        menuFile->setFont(newFont);
        menuEdit->setFont(newFont);
        menuRun->setFont(newFont);
        menuTutorial->setFont(newFont);
        menuAbout->setFont(newFont);
        menuView->setFont(newFont);
    }
}

void LammpsGui::about()
{
    std::string version = "<b>This is LAMMPS-GUI version " LAMMPS_GUI_VERSION;
    version += " using Qt version " QT_VERSION_STR;
#ifdef LAMMPS_GUI_USE_QTGRAPHS
    version += " with QtGraphs";
#else
    version += " with QtCharts";
#endif
    if (isLightTheme())
        version += " using light theme";
    else
        version += " using dark theme";
    version += "</b><br><br>\n";
    if (lammps.hasPlugin()) {
        version += "LAMMPS library loaded as plugin";
        if (!pluginPath.isEmpty()) {
            version += " from file ";
            version += pluginPath.toStdString();
        }
    } else {
        version += "LAMMPS library linked to executable";
    }

    QString to_clipboard(version.c_str());
    to_clipboard += "\n\n";

    std::string info    = "LAMMPS is currently running. LAMMPS config info not available.\n";
    std::string details = "";

    // LAMMPS is not re-entrant, so we can only query LAMMPS when it is not running
    if (!lammps.isRunning()) {
        startLammps();
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

void LammpsGui::tutorialWeb()
{
    QDesktopServices::openUrl(QUrl("https://lammpstutorials.github.io/"));
}

QWizardPage *LammpsGui::tutorialIntro(const int ntutorial, const QString &infotext)
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

QWizardPage *LammpsGui::tutorialDirectory(const int ntutorial)
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
        if (currentDir.endsWith(QString("tutorial%1").arg(i))) {
            if (i > 9) { // We assume there are no more than 99 tutorials
                currentDir.chop(2);
            } else {
                currentDir.chop(1);
            }
            currentDir.append(QString::number(ntutorial));
            haveDir = true;
        }
    }

    // if current dir is home, or application folder, switch to desktop path
    if ((currentDir == QDir::homePath()) || currentDir.contains("AppData") ||
        currentDir.contains("system32") || currentDir.contains("Program Files")) {
        currentDir = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    }
    if (!haveDir) currentDir.append(QString("/tutorial%1").arg(ntutorial));
    directory->setText(currentDir);

    auto *dirbutton = new QPushButton("&Choose");
    dirlayout->addWidget(directory);
    dirlayout->addWidget(dirbutton);
    directory->setObjectName("t_directory");
    connect(dirbutton, &QPushButton::released, this, &LammpsGui::getDirectory);
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

void LammpsGui::startTutorial(int tutno)
{
    static const char *descriptions[] = {
        "<p>In tutorial 1 you will learn about LAMMPS input files, their syntax and "
        "structure, how to create and set up models and their interactions, how to run a "
        "minimization and a molecular dynamics trajectory, how to plot thermodynamic data "
        "and how to create visualizations of your system</p>",
        "<p>In tutorial 2 you will learn about setting up a simulation for a molecular "
        "system with bonds.  The target is to simulate a carbon nanotube with a "
        "conventional molecular force field under growing strain and observe the response "
        "to it.  Since bonds are represented by a harmonic potential, they cannot break.  "
        "This is then compared to simulating the same system with a reactive force field "
        "(AIREBO) where bonds may be broken and formed.</p>",
        "<p>In tutorial 3 you will learn setting up a multi-component, a polymer molecule embedded "
        "in liquid water.  The model employs a long-range Coulomb solver and a stretching force is "
        "applied to the polymer. This is used to demonstrate how to use the type label facility in "
        "LAMMPS to make components more generic.</p>",
        "<p>In tutorial 4 an electrolyte is simulated while confined between two walls and "
        "thus illustrating the specifics of simulating systems with fluid-solid "
        "interfaces.  The water model is more complex than in tutorial 3 and also a "
        "non-equilibrium MD simulation is performed by imposing shearing forces on the "
        "electrolyte through moving the walls.</p>",
        "<p>Tutorial 5 demonstrates the use of the ReaxFF reactive force field which "
        "includes a dynamic bond topology based on determining the bond order.  ReaxFF "
        "includes charge equilibration (QEq) and thus the atoms can change their partial "
        "charges according to the local environment.</p>",
        "<p>In tutorial 6 an MD simulation is combined with Monte Carlo (MC) steps to implement "
        "a Grand Canonical ensemble.  This represents an open system where atoms or "
        "molecules may be exchanged with a reservoir.</p>",
        "<p>In tutorial 7 you will determine the height of a free energy barrier through "
        "using umbrella sampling.  This is one of many advanced methods using specific "
        "reaction coordinates or so-called collective variables to map out relevant parts "
        "of free energy landscapes, where unbiased MD or MC simulation may take too "
        "long.</p>",
        "<p>In tutorial 8 a CNT embedded in a Nylon-6,6 polymer melt is simulated.  The "
        "REACTER protocol is used to model the polymerization of Nylon without having to "
        "employ far more computationally demanding models like ReaxFF.  Also, the "
        "formation of water molecules is tracked over time.</p>",
    };

    if (tutno < 1 || tutno > 8) return;

    delete wizard;
    wizard = new TutorialWizard(tutno);
    const auto infotext =
        QString(descriptions[tutno - 1]) +
        QString("<hr width=\"33%\"\\>\n<p align=\"center\">Click on the \"Next\" button "
                "to select a folder.</p>");
    wizard->setFont(font());
    wizard->addPage(tutorialIntro(tutno, infotext));
    wizard->addPage(tutorialDirectory(tutno));
    wizard->setWindowTitle(QString("Tutorial %1 Setup Wizard").arg(tutno));
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

void LammpsGui::editVariables()
{
    QList<QPair<QString, QString>> newvars = variables;
    SetVariables vars(newvars);
    vars.setFont(font());
    if (vars.exec() == QDialog::Accepted) {
        variables = newvars;
        if (lammps.isRunning()) {
            stopRun();
            runner->wait();
            delete runner;
        }
        silenceStdout();
        lammps.close();
        restoreStdout();
        lammpsstatus->hide();
    }
}

void LammpsGui::findAndReplace()
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
            if (lammps.isRunning()) {
                stopRun();
                runner->wait();
                delete runner;
            }
            silenceStdout();
            lammps.close();
            restoreStdout();
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

void LammpsGui::startLammps()
{
    // temporary extend lammpsArgs with additional arguments
    int initial_narg = lammpsArgs.size();
    QSettings settings;
    int accel = settings.value("accelerator", AcceleratorTab::None).toInt();
    // if non-threaded accelerator selected reset threads
    if ((accel == AcceleratorTab::None) || (accel == AcceleratorTab::Opt)) {
        nthreads = 1;
    }
    qputenv("OMP_NUM_THREADS", QByteArray::number(nthreads));

    if (accel == AcceleratorTab::Opt) {
        lammpsArgs.push_back(mystrdup("-suffix"));
        lammpsArgs.push_back(mystrdup("opt"));
    } else if (accel == AcceleratorTab::OpenMP) {
        lammpsArgs.push_back(mystrdup("-suffix"));
        lammpsArgs.push_back(mystrdup("omp"));
        lammpsArgs.push_back(mystrdup("-pk"));
        lammpsArgs.push_back(mystrdup("omp"));
        lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
    } else if (accel == AcceleratorTab::Intel) {
        lammpsArgs.push_back(mystrdup("-suffix"));
        if (lammps.configHasPackage("OPENMP")) {
            lammpsArgs.push_back(mystrdup("hybrid"));
            lammpsArgs.push_back(mystrdup("intel"));
            lammpsArgs.push_back(mystrdup("omp"));
            lammpsArgs.push_back(mystrdup("-pk"));
            lammpsArgs.push_back(mystrdup("omp"));
            lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
        } else {
            lammpsArgs.push_back(mystrdup("intel"));
        }
        lammpsArgs.push_back(mystrdup("-pk"));
        lammpsArgs.push_back(mystrdup("intel"));
        lammpsArgs.push_back(mystrdup("0"));
        lammpsArgs.push_back(mystrdup("omp"));
        lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
        lammpsArgs.push_back(mystrdup("mode"));
        int iprec = settings.value("intelprec", AcceleratorTab::Mixed).toInt();
        if (iprec == AcceleratorTab::Double)
            lammpsArgs.push_back(mystrdup("double"));
        else if (iprec == AcceleratorTab::Mixed)
            lammpsArgs.push_back(mystrdup("mixed"));
        else if (iprec == AcceleratorTab::Single)
            lammpsArgs.push_back(mystrdup("single"));
        else // use mixed precision for invalid value so there is no syntax error crash
            lammpsArgs.push_back(mystrdup("mixed"));
    } else if (accel == AcceleratorTab::Gpu) {
        lammpsArgs.push_back(mystrdup("-suffix"));
        if ((nthreads > 1) && lammps.configHasPackage("OPENMP")) {
            lammpsArgs.push_back(mystrdup("hybrid"));
            lammpsArgs.push_back(mystrdup("gpu"));
            lammpsArgs.push_back(mystrdup("omp"));
            lammpsArgs.push_back(mystrdup("-pk"));
            lammpsArgs.push_back(mystrdup("omp"));
            lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
        } else {
            lammpsArgs.push_back(mystrdup("gpu"));
        }
        lammpsArgs.push_back(mystrdup("-pk"));
        lammpsArgs.push_back(mystrdup("gpu"));
        lammpsArgs.push_back(mystrdup("1")); // can use only one GPU without MPI
        lammpsArgs.push_back(mystrdup("omp"));
        lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
        lammpsArgs.push_back(mystrdup("neigh"));
        if (settings.value("gpuneigh", true).toBool())
            lammpsArgs.push_back(mystrdup("yes"));
        else
            lammpsArgs.push_back(mystrdup("no"));
        lammpsArgs.push_back(mystrdup("pair/only"));
        if (settings.value("gpupaironly", false).toBool())
            lammpsArgs.push_back(mystrdup("on"));
        else
            lammpsArgs.push_back(mystrdup("off"));
    } else if (accel == AcceleratorTab::Kokkos) {
        lammpsArgs.push_back(mystrdup("-kokkos"));
        lammpsArgs.push_back(mystrdup("on"));
        lammpsArgs.push_back(mystrdup("t"));
        lammpsArgs.push_back(mystrdup(std::to_string(nthreads)));
        lammpsArgs.push_back(mystrdup("-suffix"));
        lammpsArgs.push_back(mystrdup("kk"));
    }
    if (settings.value("echo", false).toBool()) {
        lammpsArgs.push_back(mystrdup("-echo"));
        lammpsArgs.push_back(mystrdup("screen"));
    }
    if (settings.value("cite", false).toBool()) {
        lammpsArgs.push_back(mystrdup("-cite"));
        lammpsArgs.push_back(mystrdup("screen"));
    }

    // add variables, if defined
    for (auto &var : variables) {
        QString name  = var.first;
        QString value = var.second;
        if (!name.isEmpty() && !value.isEmpty()) {
            lammpsArgs.push_back(mystrdup("-var"));
            lammpsArgs.push_back(mystrdup(name));
            for (const auto &v : value.split(' '))
                lammpsArgs.push_back(mystrdup(v));
        }
    }

    char **args = lammpsArgs.data();
    int narg    = lammpsArgs.size();
    lammps.open(narg, args);
    lammpsstatus->show();

    // Must have at least LAMMPS version 30 March 2026
    if (lammps.version() < 20260330) {
        critical(this, "LAMMPS-GUI Error", "Incompatible LAMMPS Version:",
                 "LAMMPS-GUI version " LAMMPS_GUI_VERSION " requires\n"
                 "a LAMMPS version of at least 30 March 2026");
        exit(1);
    }

    // delete additional arguments again (3 were there initially)
    while ((int)lammpsArgs.size() > initial_narg) {
        delete[] lammpsArgs.back();
        lammpsArgs.pop_back();
    }

    if (lammps.hasError()) {
        char errorbuf[DEFAULT_BUFLEN];
        lammps.getLastErrorMessage(errorbuf, DEFAULT_BUFLEN);

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

// Base URL for tutorial input and solution files on the web
static const QString tutorialBaseUrl =
    "https://raw.githubusercontent.com/lammpstutorials/"
    "lammpstutorials-article/refs/heads/main/files/tutorial%1/%2";

void LammpsGui::setupTutorial(int tutno, const QString &dir, bool purgedir, bool getsolution,
                              bool openwebpage)
{
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

    if (purgedir) purgeDirectory(dir);
    if (getsolution) directory.mkpath("solution");

    URLDownloader downloader(this);

    // download and process manifest for selected tutorial
    // must check for error after download, e.g. when there is no network.
    QString manifestPath = dir + QDir::separator() + ".manifest";
    if (!downloader.download(tutorialBaseUrl.arg(tutno).arg(".manifest"), manifestPath)) {
        critical(this, "LAMMPS-GUI Error",
                 "Tutorial files download error:", downloader.errorString());
        return;
    }

    QFile manifest(manifestPath);
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

        QString localPath = dir + QDir::separator() + item.fname;
        if (!downloader.download(tutorialBaseUrl.arg(item.ntutorial).arg(item.fname), localPath)) {
            // download failed. abort, restore status line, and launch error dialog
            status->setText("Error.");
            progress->hide();
            dirstatus->show();
            status->repaint();
            critical(this, "LAMMPS-GUI Error",
                     "Tutorial files download error:", downloader.errorString());
            return;
        }

        // check if download is a placeholder for a symbolic link and make a copy instead.
        QFile dlfile(localPath);
        QFileInfo dlpath(localPath);
        if (dlfile.open(QIODevice::ReadOnly)) {
            line = (const char *)dlfile.readLine();
            line = line.trimmed();
            dlfile.close();

            if (line == QString("../") + dlpath.fileName()) {
                // the file is a symbolic link placeholder: copy the referenced file instead
                QString srcFile = dir + QDir::separator() + dlpath.fileName();
                QFile::remove(localPath);
                QFile::copy(srcFile, localPath);
            }
        }
    }
    progress->setValue(1000);
    status->setText("Ready.");
    progress->hide();
    dirstatus->show();
    status->repaint();
    if (!first.isEmpty()) openFile(first);
}

// Local Variables:
// c-basic-offset: 4
// End:
