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
#include "constants.h"

#include <QAbstractButton>
#include <QBrush>
#include <QColor>
#include <QCoreApplication>
#include <QDataStream>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QStyle>
#include <QTemporaryFile>
#include <QWidget>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <fcntl.h>

// define consistent function aliases to avoid complications from pre-processing
#ifdef _WIN32
#include <io.h>
#include <process.h>

const auto &mydup    = _dup;
const auto &mydup2   = _dup2;
const auto &myfileno = _fileno;
const auto &myclose  = _close;
const auto &myopen   = _open;
const auto &myexecl  = _execl;
#else
#include <unistd.h>
const auto &mydup    = dup;
const auto &mydup2   = dup2;
const auto &myfileno = fileno;
const auto &myclose  = close;
const auto &myopen   = open;
const auto &myexecl  = execl;
#endif

namespace {
const QStringList months({"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
                          "Nov", "Dec"});

#ifdef _WIN32
constexpr char NULL_DEVICE[] = "NUL:";
#else
constexpr char NULL_DEVICE[] = "/dev/null";
#endif

int saved_stdout_fd    = -1;
int silenced_counter   = 0;
bool stdout_silenced   = false;
bool capture_is_active = false;
} // namespace

// will be allocated and initialized in main() to avoid segfault on macOS
std::unique_ptr<QFont> GUI_MONOFONT;
std::unique_ptr<QFont> GUI_ALLFONT;

// re-exec the current process in place; returns only if the re-exec failed
void relaunchApplication()
{
    const auto path = QCoreApplication::applicationFilePath().toStdString();
    const auto arg0 = QCoreApplication::arguments().at(0).toStdString();
    myexecl(path.c_str(), arg0.c_str(), static_cast<char *>(nullptr));
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
// Operates directly on the QString's UTF-16 data so callers never need a
// QString <-> std::string round trip.

QStringList splitLine(const QString &text)
{
    QStringList list;
    const ushort *buf = text.utf16();
    qsizetype beg     = 0;
    qsizetype len     = 0;
    qsizetype add     = 0;

    ushort c = *buf;
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
            while ((c != '\'') && (c != '\0')) {
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
            while ((c != '"') && (c != '\0')) {
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
                if (beg < text.size()) list << text.mid(beg, len);
                beg += len + add;
                break;
            }
            c = *++buf;
            ++len;
        }
    }
    return list;
}

namespace {

// Use one of our own SVG icons as the large QMessageBox icon instead
// of the standard icon. Also set button and window icon consistently.

void setDialogIcons(QMessageBox &mb, const QString &iconPath)
{
    const int extent = mb.style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, &mb);
    mb.setIconPixmap(QIcon(iconPath).pixmap(QSize(extent, extent), mb.devicePixelRatioF()));
    mb.setWindowIcon(QIcon(Cfg::MAIN_ICON));
    mb.setStandardButtons(QMessageBox::Ok);
    auto *button = mb.button(QMessageBox::Ok);
    button->setIcon(QIcon(":/icons/dialog-ok.svg"));
}
} // namespace

// customized information dialog

void information(QWidget *parent, const QString &title, const QString &text1, const QString &text2)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text1);
    if (!text2.isEmpty()) mb.setInformativeText(QString("<p>%1</p>").arg(text2));
    setDialogIcons(mb, ":/icons/help-tutorial.svg");
    mb.exec();
}

// customized critical error dialog

void critical(QWidget *parent, const QString &title, const QString &text1, const QString &text2)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text1);
    if (!text2.isEmpty()) mb.setInformativeText(QString("<p>%1</p>").arg(text2));
    setDialogIcons(mb, ":/icons/process-stop.svg");
    mb.exec();
}

// customized warning dialog

void warning(QWidget *parent, const QString &title, const QString &text1, const QString &text2)
{
    QMessageBox mb(parent);
    mb.setWindowTitle(title);
    mb.setText(text1);
    if (!text2.isEmpty()) mb.setInformativeText(QString("<p>%1</p>").arg(text2));
    setDialogIcons(mb, ":/icons/warning.svg");
    mb.exec();
}

// platform specific shared library name

QString getLammpsLibName()
{
#if defined(LAMMPS_GUI_USE_PLUGIN)
#if defined(Q_OS_MACOS)
    return QStringLiteral("liblammps.0.dylib");
#elif defined(Q_OS_WIN32)
    return QStringLiteral("liblammps.dll");
#else
    return QStringLiteral("liblammps.so.0");
#endif
#else
    return QStringLiteral("");
#endif
}

// platform specific shared library download URL

QString getLammpsDownloadUrl()
{
    const QString libName = getLammpsLibName();
    if (libName.isEmpty()) return libName;
    return QStringLiteral("https://download.lammps.org/lammps-gui/") + libName;
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
                        stderrText.trimmed().isEmpty() ? "" : "Details:\n" + stderrText.trimmed());

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

bool hasExe(const QString &exe)
{
    return !QStandardPaths::findExecutable(exe).isEmpty();
}

bool looksLikeBinaryFile(const QString &filename)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QByteArray chunk = f.read(8192);
    return chunk.contains('\0');
}

bool isImageFile(const QString &filename)
{
    // known image extensions, including formats Qt cannot read natively but
    // that ImageMagick can convert for display (tga, eps, sgi, ...)
    static const QStringList imageExtensions = {"png", "jpg", "jpeg", "bmp", "ppm", "pgm", "pbm",
                                                "gif", "tif", "tiff", "tga", "eps", "sgi", "webp",
                                                "xpm", "ico", "svg",  "jp2", "heic"};
    const QString suffix                     = QFileInfo(filename).suffix().toLower();
    if (imageExtensions.contains(suffix)) return true;

    // otherwise let Qt sniff the contents, but only for a file that exists
    if (!QFileInfo::exists(filename)) return false;
    return !QImageReader::imageFormat(filename).isEmpty();
}

bool isMovieFile(const QString &filename)
{
    // container formats that FFmpeg can decode into a sequence of images
    static const QStringList movieExtensions = {"mp4", "m4v",  "mkv", "webm", "avi", "mov",
                                                "mpg", "mpeg", "ogv", "wmv",  "flv"};
    const QString suffix                     = QFileInfo(filename).suffix().toLower();
    if (movieExtensions.contains(suffix)) return true;

    // a GIF is only a movie when it has more than a single frame
    if ((suffix == "gif") && QFileInfo::exists(filename))
        return QImageReader(filename).imageCount() > 1;
    return false;
}

bool isRestartFile(const QString &filename)
{
    // LAMMPS binary restart files start with this magic string
    static const char magic[] = "LammpS RestartT";
    char buffer[16]           = "               ";
    QFile file(filename);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in.readRawData(buffer, 16);
        file.close();
    }
    return strcmp(buffer, magic) == 0;
}

// recursively remove all contents from a directory

void purgeDirectory(const QString &dir)
{
    QDir directory(dir);
    for (const auto &entry : directory.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        if (entry.isDir()) {
            QDir(entry.absoluteFilePath()).removeRecursively();
        } else {
            QFile::remove(entry.absoluteFilePath());
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
    QMessageBox mb(parent);
    mb.setWindowTitle("Unsaved Changes");
    mb.setWindowIcon(parent ? parent->windowIcon() : QIcon());
    mb.setText(QString("The buffer ") + filename + " has changes");
    mb.setInformativeText(question);
    const int extent = mb.style()->pixelMetric(QStyle::PM_MessageBoxIconSize, nullptr, &mb);
    mb.setIconPixmap(
        QIcon(":/icons/system-help.svg").pixmap(QSize(extent, extent), mb.devicePixelRatioF()));
    mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);

    auto *button = mb.button(QMessageBox::Yes);
    button->setIcon(QIcon(":/icons/dialog-ok.svg"));
    button = mb.button(QMessageBox::No);
    button->setIcon(QIcon(":/icons/dialog-no.svg"));
    button = mb.button(QMessageBox::Cancel);
    button->setIcon(QIcon(":/icons/dialog-cancel.svg"));

    if (parent) mb.setFont(parent->font());
    return mb.exec();
}

// apply our bundled SVG icons to a dialog button box's standard buttons (see helpers.h)

void styleDialogButtons(QDialogButtonBox *box)
{
    if (!box) return;

    const struct {
        QDialogButtonBox::StandardButton id;
        const char *icon;
    } iconmap[] = {
        {QDialogButtonBox::Ok, ":/icons/dialog-ok.svg"},
        {QDialogButtonBox::Yes, ":/icons/dialog-ok.svg"},
        {QDialogButtonBox::No, ":/icons/dialog-no.svg"},
        {QDialogButtonBox::Cancel, ":/icons/dialog-cancel.svg"},
        {QDialogButtonBox::Close, ":/icons/window-close.svg"},
    };
    for (const auto &entry : iconmap) {
        if (auto *button = box->button(entry.id)) button->setIcon(QIcon(entry.icon));
    }
}

// silence stdout by redirecting to the null device

void silenceStdout()
{
    if (capture_is_active) return;

    // count nested silence requests even when stdout is already redirected, so
    // restoreStdout() only restores when the outermost request is released
    ++silenced_counter;
    if (stdout_silenced) return;

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

// RAII guard collecting Qt log messages instead of printing them (see helpers.h)

QtMessageSilencer *QtMessageSilencer::active = nullptr;

// Only the outermost guard swaps the message handler and thus remembers the
// real one. A nested guard that installed collect() again would make previous
// point at collect() itself, and forwarding to it would never terminate.
QtMessageSilencer::QtMessageSilencer() : outer(active), previous(nullptr)
{
    if (!active) previous = qInstallMessageHandler(collect);
    active = this;
}

QtMessageSilencer::~QtMessageSilencer()
{
    active = outer;
    if (!active) qInstallMessageHandler(previous);
}

QString QtMessageSilencer::messages() const
{
    return collected.join('\n');
}

void QtMessageSilencer::collect(QtMsgType type, const QMessageLogContext &context,
                                const QString &message)
{
    auto *guard = active;
    if (!guard) return;

    // an error that aborts the program, or one the application may act on, must
    // reach whoever was handling messages before the outermost guard took over
    if ((type == QtFatalMsg) || (type == QtCriticalMsg)) {
        const QtMessageSilencer *root = guard;
        while (root->outer)
            root = root->outer;
        if (root->previous)
            root->previous(type, context, message);
        else
            fprintf(stderr, "%s\n", qUtf8Printable(message));
        return;
    }
    guard->collected << message;
}

// desaturate and flatten an image, keeping its alpha channel (see helpers.h)

QImage grayscaleImage(const QImage &src)
{
    QImage img = src.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < img.height(); ++y) {
        auto *line = reinterpret_cast<QRgb *>(img.scanLine(y));
        for (int x = 0; x < img.width(); ++x) {
            // Dropping the color alone leaves an icon that still has all of its
            // contrast and thus reads as active. Pull the gray levels towards a
            // common midpoint as well, so the result looks unmistakably faded.
            const double gray =
                Cfg::GRAYSCALE_MIDPOINT +
                (qGray(line[x]) - Cfg::GRAYSCALE_MIDPOINT) * Cfg::GRAYSCALE_CONTRAST;
            const int v = std::clamp(qRound(gray), 0, 255);
            line[x]     = qRgba(v, v, v, qAlpha(line[x]));
        }
    }
    return img;
}

QPixmap grayscalePixmap(const QPixmap &src)
{
    return QPixmap::fromImage(grayscaleImage(src.toImage()));
}

// shared tool-button sizing policy (see helpers.h)

QSize toolButtonSize(const QAbstractButton *sample)
{
    const int side = sample->minimumSizeHint().height() + Cfg::TOOLBAR_BUTTON_MARGIN;
    return {side, side};
}

void styleToolButtons(const QSize &size, std::initializer_list<QAbstractButton *> buttons)
{
    const QSize iconsize(Cfg::TOOLBAR_ICON_SIZE, Cfg::TOOLBAR_ICON_SIZE);
    for (auto *button : buttons) {
        button->setMinimumSize(size);
        button->setMaximumSize(size);
        button->setIconSize(iconsize);
    }
}

// shared window-manager hint policy for output windows (see helpers.h)

void applyWindowFlags(QWidget *window)
{
    if (!window) return;
    auto flags = window->windowFlags();
    flags &= ~Qt::Dialog;
    flags |= Qt::CustomizeWindowHint;
    flags &= ~Qt::WindowMinimizeButtonHint;
#if defined(Q_OS_MACOS)
    // keep the maximize button on macOS: removing it disables window resizing
    flags |= Qt::WindowMaximizeButtonHint;
#else
    flags &= ~Qt::WindowMaximizeButtonHint;
#endif
    window->setWindowFlags(flags);
}

// Local Variables:
// c-basic-offset: 4
// End:
