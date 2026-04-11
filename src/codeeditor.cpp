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

#include "codeeditor.h"
#include "fileviewer.h"
#include "helpers.h"
#include "lammpsgui.h"
#include "lammpswrapper.h"
#include "linenumberarea.h"

#include <QAbstractItemView>
#include <QAction>
#include <QCompleter>
#include <QDesktopServices>
#include <QDir>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QKeySequence>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QRect>
#include <QRegularExpression>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QStringListModel>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocumentFragment>
#include <QUrl>
#include <QVariant>
#include <QWidget>

#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

CodeEditor::CodeEditor(QWidget *parent) :
    QPlainTextEdit(parent), currentComp(nullptr), commandComp(new QCompleter(this)),
    fixComp(new QCompleter(this)), computeComp(new QCompleter(this)),
    dumpComp(new QCompleter(this)), atomComp(new QCompleter(this)), pairComp(new QCompleter(this)),
    bondComp(new QCompleter(this)), angleComp(new QCompleter(this)),
    dihedralComp(new QCompleter(this)), improperComp(new QCompleter(this)),
    kspaceComp(new QCompleter(this)), regionComp(new QCompleter(this)),
    integrateComp(new QCompleter(this)), minimizeComp(new QCompleter(this)),
    variableComp(new QCompleter(this)), unitsComp(new QCompleter(this)),
    groupComp(new QCompleter(this)), varnameComp(new QCompleter(this)),
    fixidComp(new QCompleter(this)), compidComp(new QCompleter(this)),
    fileComp(new QCompleter(this)), extraComp(new QCompleter(this)), highlight(NO_HIGHLIGHT),
    reformatOnReturn(false), automaticCompletion(true), docver("")
{
    helpAction = new QShortcut(QKeySequence::fromString("Ctrl+?"), parent);
    connect(helpAction, &QShortcut::activated, this, &CodeEditor::getHelp);

    // set up completer class (without a model currently)
#define COMPLETER_SETUP(completer)                                                   \
    completer->setCompletionMode(QCompleter::UnfilteredPopupCompletion);             \
    completer->setModelSorting(QCompleter::CaseInsensitivelySortedModel);            \
    completer->setWidget(this);                                                      \
    completer->setMaxVisibleItems(16);                                               \
    completer->setWrapAround(false);                                                 \
    connect(completer, QOverload<const QString &>::of(&QCompleter::activated), this, \
            &CodeEditor::insertCompletedCommand)

    COMPLETER_SETUP(commandComp);
    COMPLETER_SETUP(fixComp);
    COMPLETER_SETUP(computeComp);
    COMPLETER_SETUP(dumpComp);
    COMPLETER_SETUP(atomComp);
    COMPLETER_SETUP(pairComp);
    COMPLETER_SETUP(bondComp);
    COMPLETER_SETUP(angleComp);
    COMPLETER_SETUP(dihedralComp);
    COMPLETER_SETUP(improperComp);
    COMPLETER_SETUP(kspaceComp);
    COMPLETER_SETUP(regionComp);
    COMPLETER_SETUP(integrateComp);
    COMPLETER_SETUP(minimizeComp);
    COMPLETER_SETUP(variableComp);
    COMPLETER_SETUP(unitsComp);
    COMPLETER_SETUP(groupComp);
    COMPLETER_SETUP(varnameComp);
    COMPLETER_SETUP(fixidComp);
    COMPLETER_SETUP(compidComp);
    COMPLETER_SETUP(fileComp);
    COMPLETER_SETUP(extraComp);
#undef COMPLETER_SETUP

    // initialize help system
    QFile help_index(":/help_index.table");
    if (help_index.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!help_index.atEnd()) {
            auto line  = QString(help_index.readLine());
            auto words = line.trimmed().replace('\t', ' ').split(' ');
            if (words.size() > 2) {
                if (words.at(1) == "pair_style") {
                    pairMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "bond_style") {
                    bondMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "angle_style") {
                    angleMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "dihedral_style") {
                    dihedralMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "improper_style") {
                    improperMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "fix") {
                    fixMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "compute") {
                    computeMap[words.at(2)] = words.at(0);
                } else if (words.at(1) == "kspace_style") {
                    cmdMap["kspace_style"] = "kspace_style.html";
                }
                // ignoring: dump, fix_modify ATC
            } else if (words.size() == 2) {
                cmdMap[words.at(1)] = words.at(0);
            } else {
                fprintf(stderr, "unhandled help item: %s", line.toStdString().c_str());
            }
        }
        help_index.close();
    }

    setBackgroundRole(QPalette::Light);
    lineNumberArea = new LineNumberArea(this);
    lineNumberArea->setBackgroundRole(QPalette::Dark);
    lineNumberArea->setAutoFillBackground(true);
    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    updateLineNumberAreaWidth(0);
    setCursorWidth(2);
}

CodeEditor::~CodeEditor()
{
    delete helpAction;
    delete lineNumberArea;

    delete commandComp;
    delete fixComp;
    delete computeComp;
    delete atomComp;
    delete pairComp;
    delete bondComp;
    delete angleComp;
    delete dihedralComp;
    delete improperComp;
    delete kspaceComp;
    delete regionComp;
    delete integrateComp;
    delete minimizeComp;
    delete variableComp;
    delete unitsComp;
    delete groupComp;
    delete varnameComp;
    delete fixidComp;
    delete compidComp;
    delete fileComp;
    delete extraComp;
}

int CodeEditor::lineNumberAreaWidth()
{
    int digits = 1;
    int max    = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }

    int space = 3 + (fontMetrics().horizontalAdvance(QLatin1Char('9')) * (digits + 2));
    return space;
}

void CodeEditor::setFont(const QFont &newfont)
{
    lineNumberArea->setFont(newfont);
    document()->setDefaultFont(newfont);
}

void CodeEditor::setCursor(int block)
{
    // move cursor to given position
    auto cursor = textCursor();
    int moves   = block - cursor.blockNumber();
    if (moves < 0)
        cursor.movePosition(QTextCursor::Up, QTextCursor::MoveAnchor, -moves);
    else
        cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, moves);
    setTextCursor(cursor);
}

void CodeEditor::setHighlight(int block, bool error)
{
    if (error)
        highlight = -block;
    else
        highlight = block;

    // also reset the cursor
    setCursor(block);

    // update graphics
    repaint();
}

// reformat line

QString CodeEditor::reformatLine(const QString &line)
{
    auto words = splitLine(line.toStdString());
    QString newtext;
    QSettings settings;
    settings.beginGroup("reformat");
    int cmdsize  = settings.value("command", "16").toInt();
    int typesize = settings.value("type", "4").toInt();
    int idsize   = settings.value("id", "4").toInt();
    int namesize = settings.value("name", "8").toInt();
    settings.endGroup();

    bool rebuildGroupComp     = false;
    bool rebuildVarNameComp   = false;
    bool rebuildComputeIDComp = false;
    bool rebuildFixIDComp     = false;

    if (!words.empty()) {
        // commented line. do nothing
        if (words[0][0] == '#') return line;

        // start with LAMMPS command plus padding if another word follows
        newtext = words[0].c_str();
        if (words.size() > 1) {
            for (int i = words[0].size() + 1; i < cmdsize; ++i)
                newtext += ' ';
            // new/updated group command -> update completer
            if (words[0] == "group") rebuildGroupComp = true;
            // new/updated variable command -> update completer
            if (words[0] == "variable") rebuildVarNameComp = true;
            // new/updated compute command -> update completer
            if (words[0] == "compute") rebuildComputeIDComp = true;
            // new/updated fix command -> update completer
            if (words[0] == "fix") rebuildFixIDComp = true;
        }

        // append remaining words with just a single blank added.
        for (std::size_t i = 1; i < words.size(); ++i) {
            newtext += ' ';
            newtext += words[i].c_str();

            // special cases

            if (i < 3) {
                // additional space for types or type ranges
                if (words[0] == "pair_coeff")
                    for (int j = words[i].size(); j < typesize; ++j)
                        newtext += ' ';

                // pad 4 for IDs and 8 for groups
                if ((words[0] == "fix") || (words[0] == "compute") || (words[0] == "dump")) {
                    if (i == 1) {
                        for (int j = words[i].size(); j < idsize; ++j)
                            newtext += ' ';
                    } else if (i == 2) {
                        for (int j = words[i].size(); j < namesize; ++j)
                            newtext += ' ';
                    }
                }
            }

            if (i < 2) {
                if ((words[0] == "bond_coeff") || (words[0] == "angle_coeff") ||
                    (words[0] == "dihedral_coeff") || (words[0] == "improper_coeff") ||
                    (words[0] == "mass"))
                    for (int j = words[i].size(); j < typesize; ++j)
                        newtext += ' ';
            }
        }
    }
    if (rebuildGroupComp) setGroupList();
    if (rebuildVarNameComp) setVarNameList();
    if (rebuildComputeIDComp) setComputeIDList();
    if (rebuildFixIDComp) setFixIDList();
    return newtext;
}

#define COMPLETER_INIT_FUNC(keyword, Type)                                   \
    void CodeEditor::set##Type##List(const QStringList &words)               \
    {                                                                        \
        keyword##Comp->setModel(new QStringListModel(words, keyword##Comp)); \
    }

COMPLETER_INIT_FUNC(command, Command)
COMPLETER_INIT_FUNC(fix, Fix)
COMPLETER_INIT_FUNC(compute, Compute)
COMPLETER_INIT_FUNC(dump, Dump)
COMPLETER_INIT_FUNC(atom, Atom)
COMPLETER_INIT_FUNC(pair, Pair)
COMPLETER_INIT_FUNC(bond, Bond)
COMPLETER_INIT_FUNC(angle, Angle)
COMPLETER_INIT_FUNC(dihedral, Dihedral)
COMPLETER_INIT_FUNC(improper, Improper)
COMPLETER_INIT_FUNC(kspace, Kspace)
COMPLETER_INIT_FUNC(region, Region)
COMPLETER_INIT_FUNC(integrate, Integrate)
COMPLETER_INIT_FUNC(minimize, Minimize)
COMPLETER_INIT_FUNC(variable, Variable)
COMPLETER_INIT_FUNC(units, Units)
COMPLETER_INIT_FUNC(extra, Extra)

#undef COMPLETER_INIT_FUNC

// build completer for groups by parsing through edit buffer

void CodeEditor::setGroupList()
{
    QStringList groups;
    QRegularExpression groupcmd(QStringLiteral(R"(^\s*group\s+(\S+)(\s+|$))"));

    auto saved = textCursor();
    // reposition cursor to beginning of text and search for group commands
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
    while (find(groupcmd)) {
        auto words = splitLine(textCursor().block().text().replace('\t', ' ').toStdString());
        if ((words.size() > 1) && !groups.contains(words[1].c_str())) groups << words[1].c_str();
    }
    groups.sort();
    groups.prepend(QStringLiteral("all"));

    setTextCursor(saved);
    groupComp->setModel(new QStringListModel(groups, groupComp));
}

void CodeEditor::setVarNameList()
{
    QStringList vars;

    // variable "gui_run" is always defined by LAMMPS-GUI
    vars << QString("${gui_run}");
    vars << QString("v_gui_run");

    LammpsWrapper *lammps = &qobject_cast<LammpsGui *>(parent())->lammps;
    int nvar              = lammps->idCount("variable");
    constexpr int BUFLEN  = 256;
    char buffer[BUFLEN];
    for (int i = 0; i < nvar; ++i) {
        memset(buffer, 0, BUFLEN);
        if (lammps->variableInfo(i, buffer, BUFLEN)) {
            if (strlen(buffer) == 1) vars << QString("$%1").arg(buffer);
            vars << QString("${%1}").arg(buffer);
            vars << QString("v_%1").arg(buffer);
        }
    }

    QRegularExpression varcmd(QStringLiteral(R"(^\s*variable\s+(\S+)(\s+|$))"));
    auto saved = textCursor();
    // reposition cursor to beginning of text and search for group commands
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
    while (find(varcmd)) {
        auto words = splitLine(textCursor().block().text().replace('\t', ' ').toStdString());
        if ((words.size() > 1)) {
            QString w = QString("$%1").arg(words[1].c_str());
            if ((words[1].size() == 1) && !vars.contains(w)) vars << w;
            w = QString("${%1}").arg(words[1].c_str());
            if (!vars.contains(w)) vars << w;
            w = QString("v_%1").arg(words[1].c_str());
            if (!vars.contains(w)) vars << w;
        }
    }
    vars.sort();

    setTextCursor(saved);
    varnameComp->setModel(new QStringListModel(vars, varnameComp));
}

void CodeEditor::setComputeIDList()
{
    QStringList compid;
    QRegularExpression compcmd(QStringLiteral(R"(^\s*compute\s+(\S+)\s+)"));

    auto saved = textCursor();
    // reposition cursor to beginning of text and search for group commands
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
    while (find(compcmd)) {
        auto words = splitLine(textCursor().block().text().replace('\t', ' ').toStdString());
        if ((words.size() > 1)) {
            QString w = QString("c_%1").arg(words[1].c_str());
            if (!compid.contains(w)) compid << w;
            w = QString("C_%1").arg(words[1].c_str());
            if (!compid.contains(w)) compid << w;
        }
    }
    compid.sort();

    setTextCursor(saved);
    compidComp->setModel(new QStringListModel(compid, compidComp));
}

void CodeEditor::setFixIDList()
{
    QStringList fixid;
    QRegularExpression fixcmd(QStringLiteral(R"(^\s*fix\s+(\S+)\s+)"));

    auto saved = textCursor();
    // reposition cursor to beginning of text and search for group commands
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::Start);
    setTextCursor(cursor);
    while (find(fixcmd)) {
        auto words = splitLine(textCursor().block().text().replace('\t', ' ').toStdString());
        if ((words.size() > 1)) {
            QString w = QString("f_%1").arg(words[1].c_str());
            if (!fixid.contains(w)) fixid << w;
            w = QString("F_%1").arg(words[1].c_str());
            if (!fixid.contains(w)) fixid << w;
        }
    }
    fixid.sort();

    setTextCursor(saved);
    fixidComp->setModel(new QStringListModel(fixid, fixidComp));
}

void CodeEditor::setFileList()
{
    QStringList files;
    QDir dir(".");
    for (const auto &file : dir.entryInfoList(QDir::Files))
        files << file.fileName();
    files.sort();
    fileComp->setModel(new QStringListModel(files, fileComp));
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    const auto key = event->key();

    if (currentComp && currentComp->popup()->isVisible()) {
        // The following keys are forwarded by the completer to the widget
        switch (key) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                event->ignore();
                return; // let the completer do default behavior
            default:
                break;
        }
    }

    // reformat current line and consume key event
    if (key == Qt::Key_Tab) {
        reformatCurrentLine();
        return;
    }

    // run command completion and consume key event
    if (key == Qt::Key_Backtab) {
        runCompletion();
        return;
    }

    // automatically reformat when hitting the return or enter key
    QSettings settings;
    reformatOnReturn = settings.value("return", false).toBool();
    if (reformatOnReturn && ((key == Qt::Key_Return) || (key == Qt::Key_Enter))) {
        reformatCurrentLine();
    }

    // process key event in parent class
    QPlainTextEdit::keyPressEvent(event);

    // if enabled, try pop up completion automatically after 2 characters
    automaticCompletion = settings.value("automatic", true).toBool();
    if (automaticCompletion) {
        auto cursor = textCursor();
        auto line   = cursor.block().text();
        if (line.isEmpty()) return;

        // QTextCursor::WordUnderCursor is unusable here since it recognizes '/' as word boundary.
        // Work around it by manually searching for the location of the beginning of the word.
        int begin = qMin(cursor.positionInBlock(), line.length() - 1);

        while (begin >= 0) {
            if (line[begin].isSpace()) break;
            --begin;
        }
        if (((cursor.positionInBlock() - begin) > 2) ||
            ((line.length() > begin + 1) && (line[begin + 1] == '$')))
            runCompletion();
        if (currentComp && currentComp->popup()->isVisible() &&
            ((cursor.positionInBlock() - begin) < 2)) {
            currentComp->popup()->hide();
        }
    }
}

void CodeEditor::updateLineNumberAreaWidth(int /* newBlockCount */)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        lineNumberArea->scroll(0, dy);
    else
        lineNumberArea->update(0, rect.y(), lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect())) updateLineNumberAreaWidth(0);
}

void CodeEditor::dragEnterEvent(QDragEnterEvent *event)
{
    event->acceptProposedAction();
}

void CodeEditor::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
    cut();
    QPlainTextEdit::dragLeaveEvent(event);
}

bool CodeEditor::canInsertFromMimeData(const QMimeData *source) const
{
    return source->hasUrls() || source->hasText();
}

void CodeEditor::dropEvent(QDropEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->accept();
        auto file = event->mimeData()->urls()[0].toLocalFile();
        auto *gui = dynamic_cast<LammpsGui *>(parent());
        if (gui) {
            moveCursor(QTextCursor::Start, QTextCursor::MoveAnchor);
            gui->openFile(file);
        }
        // properly handle drop event in base class, but set editor
        // buffer readonly to prevent undesired changes
        setReadOnly(true);
        QPlainTextEdit::dropEvent(event);
        setReadOnly(false);
    } else if (event->mimeData()->hasText()) {
        event->accept();
        // cut selected text to clipboard before we reposition
        // the cursor and re-insert the text with drag-n-drop
        cut();
        cursorForPosition(event->position().toPoint()).insertText(event->mimeData()->text());
        // properly handle drop event in base class, but set editor
        // buffer readonly to prevent undesired changes
        setReadOnly(true);
        QPlainTextEdit::dropEvent(event);
        setReadOnly(false);
    } else
        event->ignore();
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);

    QRect cr = contentsRect();
    lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(lineNumberArea);
    QTextBlock block = firstVisibleBlock();
    int blockNumber  = block.blockNumber();

    int top    = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());
    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1) + " ";
            if ((highlight == NO_HIGHLIGHT) || (blockNumber != std::abs(highlight))) {
                painter.setPen(palette().color(QPalette::WindowText));
            } else {
                number = QString(">") + QString::number(blockNumber + 1) + "<";
                if (highlight < 0)
                    painter.fillRect(0, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::darkRed);
                else
                    painter.fillRect(0, top, lineNumberArea->width(), fontMetrics().height(),
                                     Qt::darkGreen);

                painter.setPen(Qt::white);
            }
            painter.drawText(0, top, lineNumberArea->width(), fontMetrics().height(),
                             Qt::AlignRight, number);
        }

        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    // reposition the cursor here, but only if there is no active selection
    if (!textCursor().hasSelection()) setTextCursor(cursorForPosition(event->pos()));

    QString page, help;
    findHelp(page, help);

    auto *menu = createStandardContextMenu();
    menu->addSeparator();
    if (textCursor().hasSelection()) {
        auto *action1 = menu->addAction("Comment out selection");
        action1->setIcon(QIcon(":/icons/expand-text.png"));
        connect(action1, &QAction::triggered, this, &CodeEditor::commentSelection);
        auto *action2 = menu->addAction("Uncomment selection");
        action2->setIcon(QIcon(":/icons/expand-text.png"));
        connect(action2, &QAction::triggered, this, &CodeEditor::uncommentSelection);
    } else {
        auto *action1 = menu->addAction("Comment out line");
        action1->setIcon(QIcon(":/icons/expand-text.png"));
        connect(action1, &QAction::triggered, this, &CodeEditor::commentLine);
        auto *action2 = menu->addAction("Uncomment line");
        action2->setIcon(QIcon(":/icons/expand-text.png"));
        connect(action2, &QAction::triggered, this, &CodeEditor::uncommentLine);
    }
    menu->addSeparator();
    LammpsWrapper *lammps = &qobject_cast<LammpsGui *>(parent())->lammps;
    if (lammps->isRunning()) {
        auto *action1 = menu->addAction("Stop LAMMPS");
        action1->setIcon(QIcon(":/icons/process-stop.png"));
        connect(action1, &QAction::triggered, qobject_cast<LammpsGui *>(parent()),
                &LammpsGui::stopRun);
    } else {
        auto *action1 = menu->addAction("Run LAMMPS from Editor Buffer");
        action1->setIcon(QIcon(":/icons/system-run.png"));
        connect(action1, &QAction::triggered, qobject_cast<LammpsGui *>(parent()),
                &LammpsGui::runBuffer);
        auto *action2 = menu->addAction("Run LAMMPS from File");
        action2->setIcon(QIcon(":/icons/run-file.png"));
        connect(action2, &QAction::triggered, qobject_cast<LammpsGui *>(parent()),
                &LammpsGui::runFile);
    }
    menu->addSeparator();

    // print augmented context menu if an entry was found
    if (!help.isEmpty()) {
        auto *action = menu->addAction(QString("Display available completions for '%1'").arg(help));
        action->setIcon(QIcon(":/icons/expand-text.png"));
        connect(action, &QAction::triggered, this, &CodeEditor::runCompletion);
        menu->addSeparator();
    }

    if (!page.isEmpty()) {
        auto *action = menu->addAction(QString("Reformat '%1' command").arg(help));
        action->setIcon(QIcon(":/icons/format-indent-less-3.png"));
        connect(action, &QAction::triggered, this, &CodeEditor::reformatCurrentLine);

        menu->addSeparator();
        action = menu->addAction(QString("View Documentation for '%1'").arg(help));
        action->setIcon(QIcon(":/icons/system-help.png"));
        action->setData(page);
        connect(action, &QAction::triggered, this, &CodeEditor::openHelp);
        // if we link to help with specific styles (fix, compute, pair, bond, ...)
        // also link to the docs for the primary command
        auto words = help.split(' ');
        if (words.size() > 1) {
            help = words.at(0);
            page = words.at(0);
            page += ".html";
            auto *action2 = menu->addAction(QString("View Documentation for '%1'").arg(help));
            action2->setIcon(QIcon(":/icons/system-help.png"));
            action2->setData(page);
            connect(action2, &QAction::triggered, this, &CodeEditor::openHelp);
        }
    }

    // check if word under cursor is file
    {
        auto cursor = textCursor();
        auto line   = cursor.block().text();
        if (!line.isEmpty()) {
            // QTextCursor::WordUnderCursor is unusable here since it recognizes '/' as word
            // boundary. Work around it by manually searching for the location of the beginning of
            // the word.
            int begin = qMin(cursor.positionInBlock(), line.length() - 1);

            while (begin >= 0) {
                if (line[begin].isSpace()) break;
                --begin;
            }
            int end = begin + 1;
            while (end < line.length()) {
                if (line[end].isSpace()) break;
                ++end;
            }

            QString word = line.mid(begin, end - begin).trimmed();
            QFileInfo fi(word);
            if (fi.exists() && fi.isFile()) {
                // check if file is a LAMMPS restart
                char magic[16] = "               ";
                QFile file(word);
                if (file.open(QIODevice::ReadOnly)) {
                    QDataStream in(&file);
                    in.readRawData(magic, 16);
                    file.close();
                }
                if (strcmp(magic, LAMMPS_MAGIC) == 0) {
                    auto *action = menu->addAction(QString("Inspect restart file '%1'").arg(word));
                    action->setIcon(QIcon(":/icons/document-open.png"));
                    action->setData(word);
                    connect(action, &QAction::triggered, this, &CodeEditor::inspectFile);
                } else {
                    auto *action = menu->addAction(QString("View file '%1'").arg(word));
                    action->setIcon(QIcon(":/icons/document-open.png"));
                    action->setData(word);
                    connect(action, &QAction::triggered, this, &CodeEditor::viewFile);
                }
            }
        }
    }

    auto *action = menu->addAction(QString("LAMMPS Commands Overview"));
    action->setIcon(QIcon(":/icons/help-browser.png"));
    action->setData(QString("/Commands_all.html"));
    connect(action, &QAction::triggered, this, &CodeEditor::openHelp);

    action = menu->addAction(QString("LAMMPS Manual"));
    action->setIcon(QIcon(":/icons/help-browser.png"));
    action->setData(QString());
    connect(action, &QAction::triggered, this, &CodeEditor::openHelp);

    action = menu->addAction(QString("LAMMPS Tutorial"));
    action->setIcon(QIcon(":/icons/help-tutorial.png"));
    action->setData(QString("https://lammpstutorials.github.io/"));
    connect(action, &QAction::triggered, this, &CodeEditor::openUrl);

    menu->exec(event->globalPos());
    delete menu;
}

void CodeEditor::reformatCurrentLine()
{
    auto cursor  = textCursor();
    auto text    = cursor.block().text();
    auto newtext = reformatLine(text);

    // perform edit but only if text has changed
    if (QString::compare(text, newtext)) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor, 1);
        cursor.insertText(newtext);
        cursor.endEditBlock();
    }
}

void CodeEditor::commentLine()
{
    auto cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.insertText("#");
}

void CodeEditor::commentSelection()
{
    auto cursor = textCursor();
    auto text   = cursor.selection().toPlainText();
    auto lines  = text.split('\n');
    QString newtext;
    for (const auto &line : lines) {
        newtext.append('#');
        newtext.append(line);
        newtext.append('\n');
    }
    if (newtext.isEmpty()) newtext = "#\n";
    cursor.insertText(newtext);
    setTextCursor(cursor);
}

void CodeEditor::uncommentSelection()
{
    auto cursor = textCursor();
    auto text   = cursor.selection().toPlainText();
    auto lines  = text.split('\n');
    QString newtext;
    for (const auto &line : lines) {
        QString newline;
        bool start = true;
        for (auto letter : line) {
            if (start && (letter == '#')) {
                start = false;
                continue;
            }
            if (start && !letter.isSpace()) start = false;
            newline.append(letter);
        }
        newtext.append(newline);
        newtext.append('\n');
    }
    cursor.insertText(newtext);
    setTextCursor(cursor);
}

void CodeEditor::uncommentLine()
{
    auto cursor = textCursor();
    auto text   = cursor.block().text();
    QString newtext;
    bool start = true;
    for (auto letter : text) {
        if (start && (letter == '#')) {
            start = false;
            continue;
        }
        if (start && !letter.isSpace()) start = false;
        newtext.append(letter);
    }

    // perform edit but only if text has changed
    if (QString::compare(text, newtext)) {
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::StartOfLine);
        cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor, 1);
        cursor.insertText(newtext);
        cursor.endEditBlock();
    }
}

void CodeEditor::runCompletion()
{
    QAbstractItemView *popup = nullptr;
    if (currentComp) popup = currentComp->popup();

    auto cursor = textCursor();
    auto line   = cursor.block().text().trimmed();
    // no completion possible on empty lines
    if (line.isEmpty()) return;
    auto words = splitLine(line.toStdString());

    // QTextCursor::WordUnderCursor is unusable here since it recognizes '/' as word boundary.
    // Work around it by manually searching for the beginning and end position of the word
    // under the cursor and then using that substring.
    int begin = qMin(cursor.positionInBlock(), line.length() - 1);
    line      = cursor.block().text();
    while (begin >= 0) {
        if (line[begin].isSpace()) break;
        --begin;
    }
    int end = ++begin;
    while (end < line.length()) {
        if (line[end].isSpace()) break;
        ++end;
    }
    const auto selected = line.mid(begin, end - begin);


    // if on first word, try to complete command
    if ((!words.empty()) && (words[0] == selected.toStdString())) {
        // no completion on comment lines
        if (words[0][0] == '#') return;

        currentComp = commandComp;
        currentComp->setCompletionPrefix(words[0].c_str());
        if (popup && (popup != currentComp->popup())) popup->hide();
        popup = currentComp->popup();
        // if the command is already a complete command, remove existing popup
        if (words[0] == currentComp->currentCompletion().toStdString()) {
            if (popup->isVisible()) {
                popup->hide();
                currentComp = nullptr;
            }
            return;
        }
        QRect cr = cursorRect();
        cr.setWidth(popup->sizeHintForColumn(0) + popup->verticalScrollBar()->sizeHint().width());
        popup->setAlternatingRowColors(true);
        currentComp->complete(cr);

        // completions for second word
    } else if ((words.size() > 1) && (words[1] == selected.toStdString())) {
        // no completion on comment lines
        if (words[0][0] == '#') return;

        currentComp = nullptr;
        if (words[0] == "pair_style")
            currentComp = pairComp;
        else if (words[0] == "bond_style")
            currentComp = bondComp;
        else if (words[0] == "angle_style")
            currentComp = angleComp;
        else if (words[0] == "dihedral_style")
            currentComp = dihedralComp;
        else if (words[0] == "improper_style")
            currentComp = improperComp;
        else if (words[0] == "kspace_style")
            currentComp = kspaceComp;
        else if (words[0] == "atom_style")
            currentComp = atomComp;
        else if (words[0] == "run_style")
            currentComp = integrateComp;
        else if (words[0] == "minimize_style")
            currentComp = minimizeComp;
        else if (words[0] == "units")
            currentComp = unitsComp;
        else if ((words[0] == "change_box") || (words[0] == "displace_atoms") ||
                 (words[0] == "velocity") || (words[0] == "write_dump"))
            currentComp = groupComp;
        else if ((words[0] == "fitpod") || (words[0] == "include") || (words[0] == "ndx2group") ||
                 (words[0] == "read_data") || (words[0] == "read_dump") ||
                 (words[0] == "read_restart") || (words[0] == "rerun")) {
            if (selected.contains('/')) {
                if (popup && popup->isVisible()) popup->hide();
            } else
                currentComp = fileComp;
        } else if (selected.startsWith("v_"))
            currentComp = varnameComp;
        else if (selected.startsWith("c_") || selected.startsWith("C_"))
            currentComp = compidComp;
        else if (selected.startsWith("f_") || selected.startsWith("F_"))
            currentComp = fixidComp;

        if (currentComp) {
            currentComp->setCompletionPrefix(words[1].c_str());
            if (popup && (popup != currentComp->popup())) popup->hide();
            popup = currentComp->popup();
            // if the command is already a complete command, remove existing popup
            if (words[1] == currentComp->currentCompletion().toStdString()) {
                if (popup->isVisible()) popup->hide();
                return;
            }
            QRect cr = cursorRect();
            cr.setWidth(popup->sizeHintForColumn(0) +
                        popup->verticalScrollBar()->sizeHint().width());
            popup->setAlternatingRowColors(true);
            currentComp->complete(cr);
        }
        // completions for third word
    } else if ((words.size() > 2) && (words[2] == selected.toStdString())) {
        // no completion on comment lines
        if (words[0][0] == '#') return;

        currentComp = nullptr;
        if (words[0] == "region")
            currentComp = regionComp;
        else if (words[0] == "variable")
            currentComp = variableComp;
        else if ((words[0] == "fix") || (words[0] == "compute") || (words[0] == "dump"))
            currentComp = groupComp;
        else if (selected.startsWith("v_"))
            currentComp = varnameComp;
        else if (selected.startsWith("c_") || selected.startsWith("C_"))
            currentComp = compidComp;
        else if (selected.startsWith("f_") || selected.startsWith("F_"))
            currentComp = fixidComp;
        else if ((words[0] == "read_data") && selected.startsWith("ex"))
            currentComp = extraComp;
        else if ((words[0] == "fitpod") || (words[0] == "molecule")) {
            if (selected.contains('/')) {
                if (popup && popup->isVisible()) popup->hide();
            } else
                currentComp = fileComp;
        }
        if (currentComp) {
            currentComp->setCompletionPrefix(words[2].c_str());
            if (popup && (popup != currentComp->popup())) popup->hide();
            popup = currentComp->popup();
            // if the command is already a complete command, remove existing popup
            if (words[2] == currentComp->currentCompletion().toStdString()) {
                if (popup->isVisible()) popup->hide();
                return;
            }
            QRect cr = cursorRect();
            cr.setWidth(popup->sizeHintForColumn(0) +
                        popup->verticalScrollBar()->sizeHint().width());
            popup->setAlternatingRowColors(true);
            currentComp->complete(cr);
        }
        // completions for fourth word
    } else if ((words.size() > 3) && (words[3] == selected.toStdString())) {
        // no completion on comment lines
        if (words[0][0] == '#') return;

        currentComp = nullptr;
        if (words[0] == "fix")
            currentComp = fixComp;
        else if (words[0] == "compute")
            currentComp = computeComp;
        else if (words[0] == "dump")
            currentComp = dumpComp;
        else if ((words[0] == "pair_coeff") && (words[1] == "*") && (words[2] == "*")) {
            if (selected.contains('/')) {
                if (popup && popup->isVisible()) popup->hide();
            } else
                currentComp = fileComp;
        } else if (selected.startsWith("v_"))
            currentComp = varnameComp;
        else if (selected.startsWith("c_") || selected.startsWith("C_"))
            currentComp = compidComp;
        else if (selected.startsWith("f_") || selected.startsWith("F_"))
            currentComp = fixidComp;
        else if ((words[0] == "read_data") && selected.startsWith("ex"))
            currentComp = extraComp;

        if (currentComp) {
            currentComp->setCompletionPrefix(words[3].c_str());
            if (popup && (popup != currentComp->popup())) popup->hide();
            popup = currentComp->popup();
            // if the command is already a complete command, remove existing popup
            if (words[3] == currentComp->currentCompletion().toStdString()) {
                if (popup->isVisible()) popup->hide();
                return;
            }
            QRect cr = cursorRect();
            cr.setWidth(popup->sizeHintForColumn(0) +
                        popup->verticalScrollBar()->sizeHint().width());
            popup->setAlternatingRowColors(true);
            currentComp->complete(cr);
        }
        // reference located anywhere further right in the line
    } else if (words.size() > 4) {
        currentComp = nullptr;
        if (selected.startsWith("v_"))
            currentComp = varnameComp;
        else if (selected.startsWith("c_") || selected.startsWith("C_"))
            currentComp = compidComp;
        else if (selected.startsWith("f_") || selected.startsWith("F_"))
            currentComp = fixidComp;
        else if ((words[0] == "read_data") && selected.startsWith("ex"))
            currentComp = extraComp;

        if (currentComp) {
            currentComp->setCompletionPrefix(selected);
            if (popup && (popup != currentComp->popup())) popup->hide();
            popup = currentComp->popup();
            // if the command is already a complete command, remove existing popup
            if (selected == currentComp->currentCompletion()) {
                if (popup->isVisible()) popup->hide();
                return;
            }
            QRect cr = cursorRect();
            cr.setWidth(popup->sizeHintForColumn(0) +
                        popup->verticalScrollBar()->sizeHint().width());
            popup->setAlternatingRowColors(true);
            currentComp->complete(cr);
        }
    }
}

void CodeEditor::insertCompletedCommand(const QString &completion)
{
    auto *completer = qobject_cast<QCompleter *>(sender());
    if (completer->widget() != this) return;

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
    cursor.insertText(completion);
    setTextCursor(cursor);
}

void CodeEditor::setDocver()
{
    LammpsWrapper *lammps = &qobject_cast<LammpsGui *>(parent())->lammps;
    docver                = "/";
    if (lammps) {
        QString git_branch = (const char *)lammps->extractGlobal("git_branch");
        if ((git_branch == "stable") || (git_branch == "maintenance")) {
            docver = "/stable/";
        } else if (git_branch == "release") {
            docver = "/";
        } else {
            docver = "/latest/";
        }
    }
}

void CodeEditor::getHelp()
{
    QString page, help;
    findHelp(page, help);
    if (docver.isEmpty()) setDocver();
    if (!page.isEmpty())
        QDesktopServices::openUrl(
            QUrl(QString("https://docs.lammps.org%1%2").arg(docver).arg(page)));
}

void CodeEditor::findHelp(QString &page, QString &help)
{
    // process line of text where the cursor is
    auto text = textCursor().block().text().replace('\t', ' ').trimmed();
    auto style =
        QRegularExpression(R"(^(pair|bond|angle|dihedral|improper)_style\s+(\S+))").match(text);
    help.clear();
    page.clear();
    if (style.hasMatch()) {
        if (style.captured(1) == "pair") {
            page = pairMap.value(style.captured(2), QString());
            help = QString("pair_style %1").arg(style.captured(2));
        } else if (style.captured(1) == "bond") {
            page = bondMap.value(style.captured(2), QString());
            help = QString("bond_style %1").arg(style.captured(2));
        } else if (style.captured(1) == "angle") {
            page = angleMap.value(style.captured(2), QString());
            help = QString("angle_style %1").arg(style.captured(2));
        } else if (style.captured(1) == "dihedral") {
            page = dihedralMap.value(style.captured(2), QString());
            help = QString("dihedral_style %1").arg(style.captured(2));
        } else if (style.captured(1) == "improper") {
            page = improperMap.value(style.captured(2), QString());
            help = QString("improper_style %1").arg(style.captured(2));
        }
    }

    style = QRegularExpression(R"(^(fix|compute)\s+\w+\s+\w+\s+(\S+))").match(text);
    if (style.hasMatch()) {
        help = QString("%1 %2").arg(style.captured(1), style.captured(2));
        if (style.captured(1) == "fix") {
            page = fixMap.value(style.captured(2), QString());
        } else if (style.captured(1) == "compute") {
            page = computeMap.value(style.captured(2), QString());
        }
    }

    // could not find a matching "style", now try the plain command
    if (page.isEmpty() && !text.isEmpty()) {
        auto cmd = text.split(' ').at(0);
        help     = cmd;
        page     = cmdMap.value(cmd, QString());
    }
}

void CodeEditor::openHelp()
{
    auto *act = qobject_cast<QAction *>(sender());
    if (docver.isEmpty()) setDocver();
    QDesktopServices::openUrl(
        QUrl(QString("https://docs.lammps.org%1%2").arg(docver).arg(act->data().toString())));
}

void CodeEditor::openUrl()
{
    auto *act = qobject_cast<QAction *>(sender());
    QDesktopServices::openUrl(QUrl(act->data().toString()));
}

// forward requests to view or inspect files to the corresponding LammpsGui methods

void CodeEditor::viewFile()
{
    auto *act     = qobject_cast<QAction *>(sender());
    auto *guimain = qobject_cast<LammpsGui *>(parent());
    guimain->viewFile(act->data().toString());
}

void CodeEditor::inspectFile()
{
    auto *act     = qobject_cast<QAction *>(sender());
    auto *guimain = qobject_cast<LammpsGui *>(parent());
    guimain->inspectFile(act->data().toString());
}

// Local Variables:
// c-basic-offset: 4
// End:
