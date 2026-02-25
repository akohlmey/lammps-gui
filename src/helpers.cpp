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

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QProcess>
#include <QRegularExpression>
#include <QStringList>
#include <QWidget>

#include <cstdio>

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define fileno _fileno
#else
#include <unistd.h>
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
} // namespace

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

int date_compare(const QString &one, const QString &two)
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

std::vector<std::string> split_line(const std::string &text)
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

QWidget *get_main_widget()
{
    for (QWidget *widget : QApplication::topLevelWidgets())
        if (widget->objectName() == "LammpsGui") return widget;
    return nullptr;
}

// find if executable is in path
// https://stackoverflow.com/a/51041497

bool has_exe(const QString &exe)
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

void purge_directory(const QString &dir)
{
    QDir directory(dir);

    directory.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
    const auto &entries = directory.entryList();
    for (const auto &entry : entries) {
        if (!directory.remove(entry)) {
            directory.cd(entry);
            directory.removeRecursively();
            directory.cdUp();
        }
    }
}

int silence_stdout()
{
    int oldfd = fileno(stdout);
    int newfd = dup(oldfd);
    fflush(stdout);
    freopen(NULL_DEVICE, "ab", stdout);
    return newfd;
}

void restore_stdout(int fd)
{
    fflush(stdout);
    dup2(fd, fileno(stdout));
}

// compare black level of foreground and background color
bool is_light_theme()
{
    QPalette p;
    int fg = p.brush(QPalette::Active, QPalette::WindowText).color().black();
    int bg = p.brush(QPalette::Active, QPalette::Window).color().black();

    return (fg > bg);
}

// Local Variables:
// c-basic-offset: 4
// End:
