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

#include "logwindow.h"

#include "flagwarnings.h"
#include "helpers.h"
#include "lammpsgui.h"

#include <QAction>
#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QFontInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QKeySequence>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QShortcut>
#include <QSpacerItem>
#include <QString>
#include <QTextStream>

namespace {
constexpr auto YAML_REGEX = R"(^(keywords:.*$|data:$|---$|\.\.\.$|  - \[.*\]$))";
constexpr auto URL_REGEX  = "^.*(https://docs.lammps.org/err[0-9]+).*$";
} // namespace

LogWindow::LogWindow(const QString &_filename, LammpsGui *_lammpsgui, QWidget *parent) :
    QPlainTextEdit(parent), filename(_filename), lammpsgui(_lammpsgui), warnings(nullptr)
{
    QSettings settings;
    resize(settings.value("logx", 500).toInt(), settings.value("logy", 320).toInt());

    QFont mono_font;
    QFontInfo mono_info(*GUI_MONOFONT);
    mono_font.setFamily(settings.value("monofamily", mono_info.family()).toString());
    mono_font.setPointSize(settings.value("monosize", mono_info.pointSize()).toInt());
    mono_font.setStyleHint(GUI_MONOFONT->styleHint());
    mono_font.setFixedPitch(true);
    document()->setDefaultFont(mono_font);

    summary = new QLabel("0 Warnings / Errors  -  0 Lines");
    summary->setMargin(1);

    auto *frame = new QFrame;
    frame->setAutoFillBackground(true);
    frame->setFrameStyle(QFrame::Box | QFrame::Plain);
    frame->setLineWidth(2);

    auto *button = new QPushButton(QIcon(":/icons/warning.png"), "");
    button->setToolTip("Jump to next warning");
    connect(button, &QPushButton::released, this, &LogWindow::nextWarning);

    auto *spacer = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    auto *panel  = new QHBoxLayout(frame);
    auto *grid   = new QGridLayout(this);

    panel->addWidget(summary);
    panel->addWidget(button);
    panel->setStretchFactor(summary, 10);
    panel->setStretchFactor(button, 1);

    grid->addItem(spacer, 0, 0, 1, 3);
    grid->addWidget(frame, 1, 1, 1, 1);
    grid->setColumnStretch(0, 5);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(2, 5);

    warnings = new FlagWarnings(summary, document());

    auto *action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_S), this);
    connect(action, &QShortcut::activated, this, &LogWindow::saveAs);
    action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y), this);
    connect(action, &QShortcut::activated, this, &LogWindow::extractYaml);
    action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    connect(action, &QShortcut::activated, this, &LogWindow::quit);
    action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_N), this);
    connect(action, &QShortcut::activated, this, &LogWindow::nextWarning);
    action = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    connect(action, &QShortcut::activated, this, &LogWindow::stopRun);

    installEventFilter(this);
    // set window flags for window manager
    auto flags = windowFlags();
    flags &= ~Qt::Dialog;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowMinimizeButtonHint;
    // must add maximize button for macOS to allow resizing, but remove on other platforms
#if defined(Q_OS_MACOS)
    flags |= Qt::WindowMaximizeButtonHint;
#else
    flags &= ~Qt::WindowMaximizeButtonHint;
#endif
    setWindowFlags(flags);
}

LogWindow::~LogWindow()
{
    delete warnings;
    delete summary;
}

void LogWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings;
    if (!isMaximized()) {
        settings.setValue("logx", width());
        settings.setValue("logy", height());
    }
    QPlainTextEdit::closeEvent(event);
}

void LogWindow::quit()
{
    if (lammpsgui) lammpsgui->quit();
}

void LogWindow::stopRun()
{
    if (lammpsgui) lammpsgui->stopRun();
}

void LogWindow::nextWarning()
{
    auto regex = QRegularExpression(QStringLiteral("^(ERROR|WARNING).*$"));

    if (warnings->getNWarnings() > 0) {
        // wrap around search
        if (!find(regex)) {
            moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
            find(regex);
        }
        // move cursor to unselect
        moveCursor(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
    }
}

void LogWindow::saveAs()
{
    QString defaultname = filename + ".log";
    if (filename.isEmpty()) defaultname = "lammps.log";
    QString logFileName = QFileDialog::getSaveFileName(this, "Save Log to File", defaultname,
                                                       "Log files (*.log *.out *.txt)");
    if (logFileName.isEmpty()) return;

    QFileInfo path(logFileName);
    QFile file(path.absoluteFilePath());

    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        warning(this, "LogWindow Warning", "Cannot save to file " + logFileName + ": ",
                file.errorString());
        return;
    }

    QTextStream out(&file);
    QString text = toPlainText();
    out << text;
    if (text.back().toLatin1() != '\n') out << "\n"; // add final newline if missing
    file.close();
}

bool LogWindow::checkYaml()
{
    QRegularExpression is_yaml(YAML_REGEX);
    QStringList lines = toPlainText().split('\n');
    for (const auto &line : lines)
        if (is_yaml.match(line).hasMatch()) return true;
    return false;
}

void LogWindow::extractYaml()
{
    // ignore if no YAML format lines in buffer
    if (!checkYaml()) return;

    QString defaultname = filename + ".yaml";
    if (filename.isEmpty()) defaultname = "lammps.yaml";
    QString yamlFileName = QFileDialog::getSaveFileName(this, "Save YAML data to File", defaultname,
                                                        "YAML files (*.yaml *.yml)");
    // cannot save without filename
    if (yamlFileName.isEmpty()) return;

    QFileInfo path(yamlFileName);
    QFile file(path.absoluteFilePath());
    if (!file.open(QIODevice::WriteOnly | QFile::Text)) {
        warning(this, "LogWindow Warning", "Cannot save to file " + yamlFileName + ":",
                file.errorString());
        return;
    }

    QRegularExpression is_yaml(YAML_REGEX);
    QTextStream out(&file);
    QStringList lines = toPlainText().split('\n');
    for (const auto &line : lines) {
        if (is_yaml.match(line).hasMatch()) out << line << '\n';
    }
    file.close();
}

void LogWindow::openErrorUrl()
{
    if (!errorurl.isEmpty()) QDesktopServices::openUrl(QUrl(errorurl));
}

void LogWindow::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // select the entire word (non-space text) under the cursor
        // we need to do it in this complicated way, since QTextCursor does not recognize
        // special characters as part of a word.
        auto cursor = textCursor();
        auto line   = cursor.block().text();
        int begin   = qMin(cursor.positionInBlock(), line.length() - 1);

        while (begin >= 0) {
            if (line[begin].isSpace()) break;
            --begin;
        }

        int end = begin + 1;
        while (end < line.length()) {
            if (line[end].isSpace()) break;
            ++end;
        }
        cursor.setPosition(cursor.position() - cursor.positionInBlock() + begin + 1);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, end - begin - 1);

        auto text = cursor.selectedText();
        auto url  = QRegularExpression(URL_REGEX).match(text);
        if (url.hasMatch()) {
            errorurl = url.captured(1);
            if (!errorurl.isEmpty()) {
                QDesktopServices::openUrl(QUrl(errorurl));
                return;
            }
        }
    }
    // forward event to parent class for all unhandled cases
    QPlainTextEdit::mouseDoubleClickEvent(event);
}

void LogWindow::contextMenuEvent(QContextMenuEvent *event)
{
    // reposition the cursor here, but only if there is no active selection
    if (!textCursor().hasSelection()) setTextCursor(cursorForPosition(event->pos()));

    // show augmented context menu
    auto *menu = createStandardContextMenu();
    menu->addSeparator();
    auto *action = menu->addAction(QString("Save Log to File ..."));
    action->setIcon(QIcon(":/icons/document-save-as.png"));
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    connect(action, &QAction::triggered, this, &LogWindow::saveAs);
    // only show export-to-yaml entry if there is YAML format content.
    if (checkYaml()) {
        action = menu->addAction(QString("&Export YAML Data to File ..."));
        action->setIcon(QIcon(":/icons/yaml-file-icon.png"));
        action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Y));
        connect(action, &QAction::triggered, this, &LogWindow::extractYaml);
    }

    // process line of text where the cursor is
    auto text = textCursor().block().text().replace('\t', ' ').trimmed();
    auto url  = QRegularExpression(URL_REGEX).match(text);
    if (url.hasMatch()) {
        errorurl = url.captured(1);
        action   = menu->addAction("Open &URL in Web Browser", this, &LogWindow::openErrorUrl);
        action->setIcon(QIcon(":/icons/help-browser.png"));
    }
    action = menu->addAction("&Jump to next warning or error", this, &LogWindow::nextWarning);
    action->setIcon(QIcon(":/icons/warning.png"));
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_N));
    menu->addSeparator();
    action = menu->addAction("&Close Window", this, &QWidget::close);
    action->setIcon(QIcon(":/icons/window-close.png"));
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    action = menu->addAction("&Quit LAMMPS-GUI", this, &LogWindow::quit);
    action->setIcon(QIcon(":/icons/application-exit.png"));
    action->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
    menu->exec(event->globalPos());
    delete menu;
}

// event filter to handle "Ambiguous shortcut override" issues
bool LogWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        auto *keyEvent = dynamic_cast<QKeyEvent *>(event);
        if (!keyEvent) return QAbstractScrollArea::eventFilter(watched, event);
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == '/') {
            stopRun();
            event->accept();
            return true;
        }
        if (keyEvent->modifiers().testFlag(Qt::ControlModifier) && keyEvent->key() == 'W') {
            close();
            event->accept();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Local Variables:
// c-basic-offset: 4
// End:
