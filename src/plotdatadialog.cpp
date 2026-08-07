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

#include "plotdatadialog.h"

#include "constants.h"
#include "customfunc.h"
#include "helpers.h"

#include <QAbstractItemView>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSpinBox>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <cctype>
#include <exception>
#include <map>
#include <string>

// Sanitize a column name to a valid LeptonMini variable identifier.
// Replaces anything that is not alphanumeric or '_' with '_', and
// prepends '_' if the name starts with a digit.
static std::string sanitizeVarName(const QString &name)
{
    std::string result;
    for (QChar c : name) {
        if (c.isLetterOrNumber() || c == '_')
            result += c.toLatin1();
        else
            result += '_';
    }
    if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0])))
        result = "_" + result;
    return result.empty() ? std::string("_col") : std::move(result);
}

// The kinds offered in the format combo, in the order they appear there.
static const AveFileKind ave_kinds[] = {AveFileKind::AveTimeVector, AveFileKind::AveHisto,
                                        AveFileKind::AveCorrelate, AveFileKind::AveCorrelateLong,
                                        AveFileKind::Unknown};

PlotDataDialog::PlotDataDialog(const PlotData &data, QWidget *parent) :
    QDialog(parent), workingData(data), colsLayout(nullptr), xgroup(nullptr), sizeLabel(nullptr),
    preview(nullptr), deriveNameEdit(nullptr), deriveExprEdit(nullptr), kindCombo(nullptr),
    avgRadio(nullptr), firstSpin(nullptr), lastSpin(nullptr), errCombo(nullptr),
    singleRadio(nullptr), blockSpin(nullptr), rangeLabel(nullptr), blockLabel(nullptr),
    statusLabel(nullptr)
{
    workingErrors.resize(workingData.columnCount());
    // the historical default: first column on the x axis, all others plotted
    for (int c = 1; c < workingData.columnCount(); ++c)
        defaultYColumns << c;
    buildUi();
}

PlotDataDialog::PlotDataDialog(const PlotBlockData &blocks, QWidget *parent) :
    QDialog(parent), blockData(blocks), colsLayout(nullptr), xgroup(nullptr), sizeLabel(nullptr),
    preview(nullptr), deriveNameEdit(nullptr), deriveExprEdit(nullptr), kindCombo(nullptr),
    avgRadio(nullptr), firstSpin(nullptr), lastSpin(nullptr), errCombo(nullptr),
    singleRadio(nullptr), blockSpin(nullptr), rangeLabel(nullptr), blockLabel(nullptr),
    statusLabel(nullptr)
{
    const AveImportDefaults def = aveImportDefaults(blockData);
    defaultXColumn              = def.xColumn;
    defaultYColumns             = def.yColumns;
    // buildUi() ends by applying the reduction the detected format calls for,
    // so the common case is already on screen when the dialog opens
    buildUi();
}

/* -------------------------------------------------------------------- */

void PlotDataDialog::buildUi()
{
    setWindowTitle("Select Columns to Plot");
    auto *layout = new QVBoxLayout(this);

    if (!blockData.isEmpty()) layout->addWidget(buildBlockGroup());

    sizeLabel = new QLabel;
    layout->addWidget(sizeLabel);

    // per-column role selection: exclusive x radio buttons, y checkboxes, name
    // editors.  The rows themselves are built by rebuildColumnRows(), which also
    // replaces them when a changed block reduction changes the table.
    auto *ybox       = new QGroupBox("Columns to plot");
    auto *yboxLayout = new QVBoxLayout(ybox);
    // the grid sits in a widget of its own with a stretch below it, so that a
    // handful of columns stay at the top instead of spreading over the height
    auto *colsWidget = new QWidget;
    colsLayout       = new QGridLayout(colsWidget);
    colsLayout->setContentsMargins(0, 0, 0, 0);
    yboxLayout->addWidget(colsWidget);
    yboxLayout->addStretch(1);
    colsLayout->addWidget(new QLabel("X"), 0, 0, Qt::AlignHCenter);
    colsLayout->addWidget(new QLabel("Y"), 0, 1, Qt::AlignHCenter);
    colsLayout->addWidget(new QLabel("Column name"), 0, 2);
    colsLayout->setColumnStretch(2, 1);
    xgroup = new QButtonGroup(this);

    // the x-axis column cannot be plotted on the y-axis at the same time;
    // a column that stops being the x-axis becomes a y-axis column again
    connect(xgroup, &QButtonGroup::idToggled, this, [this](int id, bool checked) {
        if ((id < 0) || (id >= ychecks.size())) return;
        ychecks[id]->setChecked(!checked);
        ychecks[id]->setDisabled(checked);
    });

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(ybox);
    // the column list is what the dialog is for, so give it room to show
    // several rows before the preview below claims the rest
    scroll->setMinimumHeight(Cfg::PLOTDIALOG_COLUMN_LIST_HEIGHT);
    layout->addWidget(scroll, 1);

    // small preview of the first rows
    layout->addWidget(new QLabel("Preview:"));
    preview = new QTableWidget(0, 0);
    preview->verticalHeader()->setVisible(false);
    preview->setEditTriggers(QAbstractItemView::NoEditTriggers);
    preview->setSelectionMode(QAbstractItemView::NoSelection);
    layout->addWidget(preview);

    // derived column computation section
    auto *deriveBox    = new QGroupBox("Compute derived column");
    auto *deriveLayout = new QVBoxLayout(deriveBox);
    auto *namRow       = new QHBoxLayout;
    auto *exprRow      = new QHBoxLayout;
    deriveNameEdit     = new QLineEdit;
    deriveNameEdit->setPlaceholderText("new column name");
    deriveExprEdit = new QLineEdit;
    deriveExprEdit->setPlaceholderText(
        "expression using column names, e.g.  nfcc/ntot  or  pe/area*16021.766");
    deriveExprEdit->setMinimumWidth(280);
    auto *addBtn = new QPushButton("Add column");
    namRow->addWidget(new QLabel("Name:"));
    namRow->addWidget(deriveNameEdit, 1);
    exprRow->addWidget(new QLabel("Expr:"));
    exprRow->addWidget(deriveExprEdit, 1);
    exprRow->addWidget(addBtn);
    deriveLayout->addLayout(namRow);
    deriveLayout->addLayout(exprRow);
    deriveLayout->addWidget(
        new QLabel("Column names are variables (use colname_first for the first-row value)."));
    layout->addWidget(deriveBox);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    styleDialogButtons(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(addBtn, &QPushButton::clicked, this, &PlotDataDialog::computeColumn);
    layout->addWidget(buttons);

    if (blockData.isEmpty()) {
        rebuildColumnRows();
        refreshPreview();
    } else {
        // fills in the block group's labels and builds the column rows from the
        // reduced table, so the two can never start out disagreeing
        applyReduction();
    }
}

/* -------------------------------------------------------------------- */

QGroupBox *PlotDataDialog::buildBlockGroup()
{
    const int nblock = blockData.blockCount();

    auto *box = new QGroupBox("Data blocks");
    auto *lay = new QVBoxLayout(box);

    // detected format, which the user may correct.  It only selects the
    // defaults below; the data was already parsed and is never reinterpreted.
    auto *topRow = new QHBoxLayout;
    topRow->addWidget(new QLabel("Format:"));
    kindCombo = new QComboBox;
    for (AveFileKind k : ave_kinds)
        kindCombo->addItem(aveFileKindName(k), static_cast<int>(k));
    for (int i = 0; i < kindCombo->count(); ++i)
        if (kindCombo->itemData(i).toInt() == static_cast<int>(blockData.kind))
            kindCombo->setCurrentIndex(i);
    kindCombo->setToolTip("The format detected from the file. Correcting it changes which\n"
                          "reduction and columns are preselected, nothing about the data.");
    topRow->addWidget(kindCombo);
    if (!blockData.fixId.isEmpty())
        topRow->addWidget(new QLabel(QString("written by fix <b>%1</b>").arg(blockData.fixId)));
    topRow->addStretch(1);
    lay->addLayout(topRow);

    // what is actually in the file
    int minrows = -1, maxrows = 0;
    for (const auto &b : blockData.blocks) {
        const int n = b.rows.rowCount();
        minrows     = (minrows < 0) ? n : qMin(minrows, n);
        maxrows     = qMax(maxrows, n);
    }
    const QString rows = (minrows == maxrows) ? QString("%1 rows each").arg(maxrows)
                                              : QString("%1 to %2 rows").arg(minrows).arg(maxrows);
    lay->addWidget(new QLabel(QString("%1 block(s), steps %2 to %3, %4")
                                  .arg(nblock)
                                  .arg(blockData.blocks.front().step)
                                  .arg(blockData.blocks.back().step)
                                  .arg(rows)));

    // averaging a range of blocks
    auto *avgRow = new QHBoxLayout;
    avgRadio     = new QRadioButton("Average blocks");
    avgRadio->setToolTip("Average the selected blocks row by row.  Successive averaging\n"
                         "windows are not strictly independent, so the standard error of\n"
                         "the mean is a lower bound on the true uncertainty.");
    firstSpin = new QSpinBox;
    firstSpin->setRange(1, nblock);
    firstSpin->setValue(1);
    firstSpin->setToolTip("First block of the averaged range; raise it to drop equilibration.");
    lastSpin = new QSpinBox;
    lastSpin->setRange(1, nblock);
    lastSpin->setValue(nblock);
    errCombo = new QComboBox;
    for (auto t : {BlockErrorType::StdDev, BlockErrorType::StdError, BlockErrorType::None})
        errCombo->addItem(blockErrorTypeName(t), static_cast<int>(t));
    rangeLabel = new QLabel;
    avgRow->addWidget(avgRadio);
    avgRow->addWidget(new QLabel("from"));
    avgRow->addWidget(firstSpin);
    avgRow->addWidget(new QLabel("to"));
    avgRow->addWidget(lastSpin);
    avgRow->addWidget(rangeLabel, 1);
    avgRow->addWidget(new QLabel("error bars:"));
    avgRow->addWidget(errCombo);
    lay->addLayout(avgRow);

    // showing a single block
    auto *oneRow = new QHBoxLayout;
    singleRadio  = new QRadioButton("Single block");
    singleRadio->setToolTip("Show one block as it stands.  This is what a correlator that\n"
                            "accumulates over the whole run needs: there the last block is\n"
                            "the result and averaging the blocks would be wrong.");
    blockSpin = new QSpinBox;
    blockSpin->setRange(1, nblock);
    blockSpin->setValue(nblock);
    blockLabel = new QLabel;
    oneRow->addWidget(singleRadio);
    oneRow->addWidget(blockSpin);
    oneRow->addWidget(blockLabel, 1);
    lay->addLayout(oneRow);

    statusLabel = new QLabel;
    lay->addWidget(statusLabel);

    // the reduction the detected format calls for is already applied
    const bool avg = aveImportDefaults(blockData).averageBlocks;
    avgRadio->setChecked(avg);
    singleRadio->setChecked(!avg);

    connect(avgRadio, &QRadioButton::toggled, this, &PlotDataDialog::applyReduction);
    connect(firstSpin, &QSpinBox::valueChanged, this, &PlotDataDialog::applyReduction);
    connect(lastSpin, &QSpinBox::valueChanged, this, &PlotDataDialog::applyReduction);
    connect(blockSpin, &QSpinBox::valueChanged, this, &PlotDataDialog::applyReduction);
    connect(errCombo, &QComboBox::currentIndexChanged, this, &PlotDataDialog::applyReduction);
    // a corrected format only moves the preselected columns and reduction
    connect(kindCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        blockData.kind              = static_cast<AveFileKind>(kindCombo->itemData(index).toInt());
        const AveImportDefaults def = aveImportDefaults(blockData);
        defaultXColumn              = def.xColumn;
        defaultYColumns             = def.yColumns;
        resetColumnRoles            = true; // the new defaults replace the old roles
        avgRadio->setChecked(def.averageBlocks);
        singleRadio->setChecked(!def.averageBlocks);
        applyReduction();
    });

    return box;
}

/* -------------------------------------------------------------------- */

BlockErrorType PlotDataDialog::errorType() const
{
    if (!errCombo) return BlockErrorType::None;
    return static_cast<BlockErrorType>(errCombo->currentData().toInt());
}

void PlotDataDialog::updateBlockLabels()
{
    const bool avg = avgRadio && avgRadio->isChecked();
    firstSpin->setEnabled(avg);
    lastSpin->setEnabled(avg);
    errCombo->setEnabled(avg);
    blockSpin->setEnabled(!avg);

    const int lo = qMin(firstSpin->value(), lastSpin->value()) - 1;
    const int hi = qMax(firstSpin->value(), lastSpin->value()) - 1;
    rangeLabel->setText(
        QString("(steps %1 to %2)").arg(blockData.blocks[lo].step).arg(blockData.blocks[hi].step));
    blockLabel->setText(QString("(step %1)").arg(blockData.blocks[blockSpin->value() - 1].step));
}

void PlotDataDialog::applyReduction()
{
    if (blockData.isEmpty() || !avgRadio) return;
    updateBlockLabels();

    if (avgRadio->isChecked()) {
        const BlockAverage avg =
            averageBlocks(blockData, firstSpin->value() - 1, lastSpin->value() - 1, errorType());
        workingData   = avg.data;
        workingErrors = avg.errors;
        QString text  = QString("averaged %1 of %2 block(s)")
                           .arg(avg.usedBlocks)
                           .arg(avg.usedBlocks + avg.skippedBlocks);
        if (avg.skippedBlocks > 0)
            text += QString("; %1 dropped for having a different number of rows")
                        .arg(avg.skippedBlocks);
        if (avg.errors.empty() && (errorType() != BlockErrorType::None))
            text += "; error bars need at least two blocks";
        statusLabel->setText(text);
    } else {
        const int index = blockSpin->value() - 1;
        workingData     = singleBlock(blockData, index);
        workingErrors.clear();
        statusLabel->setText(QString("block %1 of %2, step %3, %4 row(s)")
                                 .arg(index + 1)
                                 .arg(blockData.blockCount())
                                 .arg(blockData.blocks[index].step)
                                 .arg(workingData.rowCount()));
    }
    workingErrors.resize(workingData.columnCount());

    // derived columns were computed from the previous reduction, so they are
    // re-evaluated against the new table rather than silently dropped
    const auto derived = derivedColumns;
    derivedColumns.clear();
    for (const auto &d : derived) {
        std::vector<double> values;
        if (evaluateColumn(d.second, values).isEmpty()) {
            workingData.addColumn(d.first, std::move(values));
            workingErrors.emplace_back();
            derivedColumns.append(d);
        }
    }

    rebuildColumnRows();
    refreshPreview();
}

/* -------------------------------------------------------------------- */

void PlotDataDialog::appendColumnRow(const QString &name, bool checked)
{
    const int row = ychecks.size();
    auto *xb      = new QRadioButton;
    xgroup->addButton(xb, row);
    auto *cb = new QCheckBox;
    cb->setChecked(checked);
    ychecks.append(cb);
    auto *nameEdit = new QLineEdit(name);
    nameEdit->setPlaceholderText("column name");
    nameEdit->setMinimumWidth(120);
    ynames.append(nameEdit);
    colsLayout->addWidget(xb, row + 1, 0, Qt::AlignHCenter);
    colsLayout->addWidget(cb, row + 1, 1, Qt::AlignHCenter);
    colsLayout->addWidget(nameEdit, row + 1, 2);
}

void PlotDataDialog::rebuildColumnRows()
{
    // a changed reduction does not change the columns, so keep the roles and
    // the names the user has already assigned instead of resetting them
    QStringList prevNames;
    QList<bool> prevY;
    int prevX = -1;
    if (!ychecks.isEmpty() && !resetColumnRoles) {
        prevNames = columnNames();
        for (auto *cb : ychecks)
            prevY.append(cb->isChecked());
        prevX = xgroup->checkedId();
    }
    resetColumnRoles = false;

    const auto buttons = xgroup->buttons();
    for (auto *b : buttons)
        delete b; // also removes it from the button group
    qDeleteAll(ychecks);
    ychecks.clear();
    qDeleteAll(ynames);
    ynames.clear();

    const int ncol = workingData.columnCount();
    const int nrow = workingData.rowCount();
    sizeLabel->setText(QString("%1 rows, %2 columns").arg(nrow).arg(ncol));

    for (int c = 0; c < ncol; ++c) {
        const bool y = (c < prevY.size()) ? prevY[c] : defaultYColumns.contains(c);
        appendColumnRow(c < prevNames.size() ? prevNames[c] : workingData.columnName(c), y);
    }
    if (ncol > 0) {
        const int x = qBound(0, (prevX >= 0) ? prevX : defaultXColumn, ncol - 1);
        if (auto *b = xgroup->button(x)) b->setChecked(true);
        // setChecked() is a no-op when the button is already checked, so make
        // sure the x column is never also a y column
        ychecks[x]->setChecked(false);
        ychecks[x]->setDisabled(true);
    }
}

void PlotDataDialog::refreshPreview()
{
    if (!preview) return;
    const int ncol = workingData.columnCount();
    const int nrow = qMin(workingData.rowCount(), Cfg::PLOTDIALOG_PREVIEW_ROWS);
    preview->setRowCount(nrow);
    preview->setColumnCount(ncol);
    preview->setVisible(nrow > 0);
    if (nrow == 0) return;

    QStringList headers;
    for (int c = 0; c < ncol; ++c)
        headers << workingData.columnName(c);
    preview->setHorizontalHeaderLabels(headers);
    for (int c = 0; c < ncol; ++c) {
        const std::vector<double> &col = workingData.column(c);
        for (int r = 0; r < nrow; ++r)
            preview->setItem(r, c, new QTableWidgetItem(QString::number(col[r])));
    }
    preview->resizeColumnsToContents();
    // a table widget neither asks for the height of the few rows it shows nor
    // insists on it, so pin it to exactly that and let the column list above
    // have the rest
    int height = preview->horizontalHeader()->height() + 2 * preview->frameWidth() +
                 preview->horizontalScrollBar()->sizeHint().height();
    for (int r = 0; r < nrow; ++r)
        height += preview->rowHeight(r);
    preview->setFixedHeight(height);
}

/* -------------------------------------------------------------------- */

QString PlotDataDialog::evaluateColumn(const QString &expr, std::vector<double> &values) const
{
    const int ncol = workingData.columnCount();
    const int nrow = workingData.rowCount();

    // map "<sanitized column name>_first" to the column's first-row value for LeptonMini
    std::map<std::string, double> constants;
    for (int c = 0; c < ncol; ++c) {
        const std::string var = sanitizeVarName(workingData.columnName(c));
        if (nrow > 0) constants[var + "_first"] = workingData.column(c).front();
    }

    CompiledExpression program(expr);
    if (!program.isValid()) return program.error();

    values.clear();
    values.reserve(static_cast<std::size_t>(nrow));
    try {
        std::map<std::string, double> vars = std::move(constants);
        vars["row"]                        = 0.0;
        for (int r = 0; r < nrow; ++r) {
            for (int c = 0; c < ncol; ++c)
                vars[sanitizeVarName(workingData.columnName(c))] = workingData.column(c)[r];
            vars["row"] = static_cast<double>(r);
            values.push_back(program.evaluate(vars));
        }
    } catch (const std::exception &e) {
        return QString::fromUtf8(e.what());
    }
    return {};
}

void PlotDataDialog::computeColumn()
{
    const QString colName = deriveNameEdit->text().trimmed();
    const QString expr    = deriveExprEdit->text().trimmed();
    if (colName.isEmpty() || expr.isEmpty()) {
        warning(this, "Compute Column", "Enter both a column name and an expression.");
        return;
    }

    std::vector<double> values;
    const QString error = evaluateColumn(expr, values);
    if (!error.isEmpty()) {
        warning(this, "Compute Column",
                QString("Could not evaluate the expression:\n%1").arg(error));
        return;
    }

    workingData.addColumn(colName, std::move(values));
    workingErrors.emplace_back(); // a derived column has no error bars
    derivedColumns.append({colName, expr});
    appendColumnRow(colName, true);
    deriveNameEdit->clear();
    deriveExprEdit->clear();
    refreshPreview();
    adjustSize();
}

/* -------------------------------------------------------------------- */

int PlotDataDialog::xColumn() const
{
    const int id = xgroup->checkedId();
    return (id >= 0) ? id : 0;
}

QList<int> PlotDataDialog::yColumns() const
{
    QList<int> result;
    for (int i = 0; i < ychecks.size(); ++i)
        if (ychecks[i]->isChecked()) result.append(i);
    return result;
}

QStringList PlotDataDialog::columnNames() const
{
    QStringList names;
    for (auto *e : ynames)
        names << (e->text().trimmed().isEmpty() ? e->placeholderText() : e->text().trimmed());
    return names;
}

PlotData PlotDataDialog::buildData() const
{
    PlotData result = workingData;
    result.renameColumns(columnNames());
    return result;
}

PlotErrors PlotDataDialog::buildErrors() const
{
    PlotErrors result = workingErrors;
    result.resize(workingData.columnCount());
    return result;
}

// Local Variables:
// c-basic-offset: 4
// End:
