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

#include "helpers.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QMessageBox>
#include <QPalette>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QTemporaryFile>
#include <QWidget>

#include <cstdio>
#include <fcntl.h>

// define consistent function aliases to avoid complications from pre-processing
#ifdef _WIN32
#include <io.h>
const auto &mydup    = _dup;
const auto &mydup2   = _dup2;
const auto &myfileno = _fileno;
const auto &myclose  = _close;
const auto &myopen   = _open;
#else
#include <unistd.h>
const auto &mydup    = dup;
const auto &mydup2   = dup2;
const auto &myfileno = fileno;
const auto &myclose  = close;
const auto &myopen   = open;
#endif

namespace {
const QStringList months({"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
                          "Nov", "Dec"});

#ifdef _WIN32
constexpr char NULL_DEVICE[] = "NUL:";
constexpr char TTY_DEVICE[]  = "COM1:";
#else
constexpr char NULL_DEVICE[] = "/dev/null";
constexpr char TTY_DEVICE[]  = "/dev/tty";
#endif

int saved_stdout_fd    = -1;
int silenced_counter   = 0;
bool stdout_silenced   = false;
bool capture_is_active = false;
} // namespace

// will be allocated and initialized in main() to avoid segfault on macOS
QFont *GUI_MONOFONT = nullptr;
QFont *GUI_ALLFONT  = nullptr;

// duplicate string, STL version
char *mystrdup(const std::string &text)
{
    auto *tmp = new char[text.size() + 1];
    memcpy(tmp, text.c_str(), text.size() + 1);
    return tmp;
}

// duplicate string, pointer version
char *mystrdup(const char *text)
{
    if (text == nullptr) return mystrdup("");
    return mystrdup(std::string(text));
}

// duplicate string, Qt version
char *mystrdup(const QString &text)
{
    return mystrdup(text.toStdString());
}

// compare two date strings return -1 if first is older than second, 0 if same, or 1 if
// otherwise

int dateCompare(const QString &one, const QString &two)
{
    if (one == two) return 0;

    // split string into words and check each of them
    auto onelist = one.split(" ", Qt::SkipEmptyParts);
    auto twolist = two.split(" ", Qt::SkipEmptyParts);
    if (onelist.size() != 3) return -1;
    if (twolist.size() != 3) return 1;

    if (onelist[2].toInt() < twolist[2].toInt()) {
        return -1;
    } else if (onelist[2].toInt() > twolist[2].toInt()) {
        return 1;
    }

    onelist[1].truncate(3);
    twolist[1].truncate(3);
    if (months.indexOf(onelist[1]) < months.indexOf(twolist[1])) {
        return -1;
    } else if (months.indexOf(onelist[1]) > months.indexOf(twolist[1])) {
        return 1;
    }

    if (onelist[0].toInt() < twolist[0].toInt()) {
        return -1;
    } else if (onelist[0].toInt() > twolist[0].toInt()) {
        return 1;
    }
    return 0;
}

// Convert string into words on whitespace while handling single and double
// quotes. Adapted from LAMMPS_NS::utils::split_words() to preserve quotes.

std::vector<std::string> splitLine(const std::string &text)
{
    std::vector<std::string> list;
    const char *buf = text.c_str();
    std::size_t beg = 0;
    std::size_t len = 0;
    std::size_t add = 0;

    char c = *buf;
    while (c) { // leading whitespace
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\f') {
            c = *++buf;
            ++beg;
            continue;
        };
        len = 0;

    // handle escaped/quoted text.
    quoted:

        if (c == '\'') { // handle single quote
            add = 0;
            len = 1;
            c   = *++buf;
            while (((c != '\'') && (c != '\0')) || ((c == '\\') && (buf[1] == '\''))) {
                if ((c == '\\') && (buf[1] == '\'')) {
                    ++buf;
                    ++len;
                }
                c = *++buf;
                ++len;
            }
            ++len;
            c = *++buf;

            // handle triple double quotation marks
        } else if ((c == '"') && (buf[1] == '"') && (buf[2] == '"') && (buf[3] != '"')) {
            len = 3;
            add = 1;
            buf += 3;
            c = *buf;

        } else if (c == '"') { // handle double quote
            add = 0;
            len = 1;
            c   = *++buf;
            while (((c != '"') && (c != '\0')) || ((c == '\\') && (buf[1] == '"'))) {
                if ((c == '\\') && (buf[1] == '"')) {
                    ++buf;
                    ++len;
                }
                c = *++buf;
                ++len;
            }
            ++len;
            c = *++buf;
        }

        while (true) { // unquoted
            if ((c == '\'') || (c == '"')) goto quoted;
            // skip escaped quote
            if ((c == '\\') && ((buf[1] == '\'') || (buf[1] == '"'))) {
                ++buf;
                ++len;
                c = *++buf;
                ++len;
            }
            if ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n') || (c == '\f') ||
                (c == '\0')) {
                if (beg < text.size()) list.push_back(text.substr(beg, len));
                beg += len + add;
                break;
            }
            c = *++buf;
            ++len;
        }
    }
    return list;
}

// get pointer to LAMMPS-GUI main widget

QWidget *getMainWidget()
{
    for (QWidget *widget : QApplication::topLevelWidgets())
        if (widget->objectName() == "LammpsGui") return widget;
    return nullptr;
}

// customized critical error dialog

void critical(QWidget *parent, const QString &title, const QString &text1, const QString &text2)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text1);
    if (!text2.isEmpty()) mb.setInformativeText(QString("<p>%1</p>").arg(text2));
    mb.setIcon(QMessageBox::Critical);
    mb.setStandardButtons(QMessageBox::Close);
    mb.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    // customize button icon
    auto *button = mb.button(QMessageBox::Close);
    button->setIcon(QIcon(":/icons/window-close.png"));
    mb.exec();
}

// customized warning dialog

void warning(QWidget *parent, const QString &title, const QString &text1, const QString &text2)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text1);
    if (!text2.isEmpty()) mb.setInformativeText(QString("<p>%1</p>").arg(text2));
    mb.setIcon(QMessageBox::Warning);
    mb.setStandardButtons(QMessageBox::Close);
    mb.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    // customize button icon
    auto *button = mb.button(QMessageBox::Close);
    button->setIcon(QIcon(":/icons/window-close.png"));
    mb.exec();
}

// save image directly and if that fails, save to PNG and convert with ImageMagick
void exportImage(QWidget *parent, QImage *image, const QString &title)
{
    if (!image) return;
    QString fileName = QFileDialog::getSaveFileName(
        parent, "Export Current Image to Image File", ".",
        "Image Files (*.png *.jpg *.jpeg *.gif *.bmp *.tga *.ppm *.tiff *.webp *.pgm *.xpm *.xbm)");
    if (fileName.isEmpty()) return;

    // try direct save and if it fails write to PNG and then convert with ImageMagick if available
    if (!image->save(fileName)) {
        if (hasExe("magick") || hasExe("convert")) {
            QTemporaryFile tmpfile(QDir::tempPath() + "/LAMMPS_GUI.XXXXXX.png");
            // open and close to generate temporary file name
            (void)tmpfile.open();
            (void)tmpfile.close();
            if (!image->save(tmpfile.fileName())) {
                warning(parent, title + " Error", "Could not save image to file " + fileName);
                return;
            }

            QString cmd = "magick";
            QStringList args{tmpfile.fileName(), fileName};
            if (!hasExe("magick")) cmd = "convert";
            auto *convert = new QProcess(parent);
            convert->start(cmd, args);
            if (!convert->waitForFinished(-1)) {
                const QString errmesg = convert->errorString();
                delete convert;
                QFile::remove(fileName);
                warning(parent, title + " Error",
                        "ImageMagick conversion failed while saving to file " + fileName + ":",
                        errmesg);
                return;
            }
            if (convert->exitStatus() != QProcess::NormalExit || convert->exitCode() != 0) {
                const QString stderrText = QString::fromLocal8Bit(convert->readAllStandardError());
                delete convert;
                QFile::remove(fileName);
                warning(parent, title + " Error",
                        "ImageMagick conversion failed while saving to file " + fileName + ":",
                        stderrText.trimmed().isEmpty() ? "Details:\n" + stderrText.trimmed() : "");

                return;
            }
            delete convert;
            if (!QFile::exists(fileName)) {
                warning(parent, title + " Error",
                        "ImageMagick reported success, but the output file " + fileName +
                            " was not created.");
                return;
            }
        } else {
            warning(parent, title + " Error", "Could not save image to file " + fileName);
        }
    }
}

// find if executable is in path
// https://stackoverflow.com/a/51041497

bool hasExe(const QString &exe)
{
    QProcess findProcess;
    QStringList arguments;
    arguments << exe;
#if defined(_WIN32)
    findProcess.start("where", arguments);
#else
    findProcess.start("which", arguments);
#endif
    findProcess.setReadChannel(QProcess::ProcessChannel::StandardOutput);

    if (!findProcess.waitForFinished()) return false; // Not found or which does not work

    QString retStr(findProcess.readAll());

    // truncate multi-line output to first line
    auto idx = retStr.indexOf(QRegularExpression("[\n\r]"), 0);
    if (idx > 0) retStr.truncate(idx);

    QFile file(retStr.trimmed());
    QFileInfo check_file(file);
    return (check_file.exists() && check_file.isFile());
}

// recursively remove all contents from a directory

void purgeDirectory(const QString &dir)
{
    QDir directory(dir);

    directory.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    const auto &entries = directory.entryList();
    for (const auto &entry : entries) {
        if (!directory.remove(entry)) {
            if (directory.cd(entry)) {
                directory.removeRecursively();
                directory.cdUp();
            }
        }
    }
}

// compare black level of foreground and background color
bool isLightTheme()
{
    QPalette p;
    int fg = p.brush(QPalette::Active, QPalette::WindowText).color().black();
    int bg = p.brush(QPalette::Active, QPalette::Window).color().black();

    return (fg > bg);
}

// standardized "Unsaved Changes" confirmation dialog
int showUnsavedChangesDialog(QWidget *parent, const QString &filename, const QString &question)
{
    QMessageBox msg(parent);
    msg.setWindowTitle("Unsaved Changes");
    msg.setWindowIcon(parent ? parent->windowIcon() : QIcon());
    msg.setText(QString("The buffer ") + filename + " has changes");
    msg.setInformativeText(question);
    msg.setIcon(QMessageBox::Question);
    msg.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    auto *button = msg.button(QMessageBox::Yes);
    button->setIcon(QIcon(":/icons/dialog-ok.png"));
    button = msg.button(QMessageBox::No);
    button->setIcon(QIcon(":/icons/dialog-no.png"));
    button = msg.button(QMessageBox::Cancel);
    button->setIcon(QIcon(":/icons/dialog-cancel.png"));

    if (parent) msg.setFont(parent->font());
    return msg.exec();
}

// silence stdout by redirecting to the null device

void silenceStdout()
{
    if (capture_is_active || stdout_silenced) return;

    ++silenced_counter;
    fflush(stdout);
    saved_stdout_fd = mydup(myfileno(stdout));
    if (saved_stdout_fd == -1) return;

    int devnull = myopen(NULL_DEVICE, O_WRONLY, 0);
    if (devnull == -1) {
        myclose(saved_stdout_fd);
        saved_stdout_fd = -1;
        return;
    }
    mydup2(devnull, myfileno(stdout));
    myclose(devnull);
    stdout_silenced = true;
}

// restore stdout after silencing

void restoreStdout()
{
    if (silenced_counter > 0) --silenced_counter;
    if (!stdout_silenced || (saved_stdout_fd == -1) || (silenced_counter > 0)) return;

    fflush(stdout);
    mydup2(saved_stdout_fd, myfileno(stdout));
    myclose(saved_stdout_fd);
    saved_stdout_fd = -1;
    stdout_silenced = false;
}

// check if stdout is currently silenced

bool isStdoutSilenced()
{
    return stdout_silenced;
}

// notify silence/restore system about StdCapture state changes

void notifyCaptureState(bool active)
{
    capture_is_active = active;
}

// Local Variables:
// c-basic-offset: 4
// End:
