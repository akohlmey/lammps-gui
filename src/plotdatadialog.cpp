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

#include "customfunc.h"
#include "helpers.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
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

PlotDataDialog::PlotDataDialog(const PlotData &data, QWidget *parent) :
    QDialog(parent), workingData(data), colsLayout(nullptr), deriveNameEdit(nullptr),
    deriveExprEdit(nullptr)
{
    setWindowTitle("Select Columns to Plot");
    const int ncol = workingData.columnCount();
    const int nrow = workingData.rowCount();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("%1 rows, %2 columns").arg(nrow).arg(ncol)));
    layout->addWidget(new QLabel("The first unselected column will be used as the x-axis."));

    // y-column selection checkboxes + name editors (default: every column but the first)
    auto *ybox = new QGroupBox("Columns to plot (uncheck the x-axis column)");
    colsLayout = new QVBoxLayout(ybox);
    for (int c = 0; c < ncol; ++c)
        appendColumnRow(workingData.columnName(c), c != 0);

    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setWidget(ybox);
    layout->addWidget(scroll, 1);

    // small preview of the first rows
    const int previewRows = qMin(nrow, 8);
    if (previewRows > 0) {
        auto *table = new QTableWidget(previewRows, ncol);
        QStringList headers;
        for (int c = 0; c < ncol; ++c)
            headers << workingData.columnName(c);
        table->setHorizontalHeaderLabels(headers);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        for (int c = 0; c < ncol; ++c) {
            const std::vector<double> &col = workingData.column(c);
            for (int r = 0; r < previewRows; ++r)
                table->setItem(r, c, new QTableWidgetItem(QString::number(col[r])));
        }
        table->resizeColumnsToContents();
        layout->addWidget(new QLabel("Preview:"));
        layout->addWidget(table);
    }

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
}

void PlotDataDialog::appendColumnRow(const QString &name, bool checked)
{
    auto *cb = new QCheckBox;
    cb->setChecked(checked);
    ychecks.append(cb);
    auto *nameEdit = new QLineEdit(name);
    nameEdit->setPlaceholderText("column name");
    nameEdit->setMinimumWidth(120);
    ynames.append(nameEdit);
    auto *row = new QHBoxLayout;
    row->addWidget(cb);
    row->addWidget(nameEdit, 1);
    colsLayout->addLayout(row);
}

void PlotDataDialog::computeColumn()
{
    const QString colName = deriveNameEdit->text().trimmed();
    const QString expr    = deriveExprEdit->text().trimmed();
    if (colName.isEmpty() || expr.isEmpty()) {
        warning(this, "Compute Column", "Enter both a column name and an expression.");
        return;
    }

    const int ncol = workingData.columnCount();
    const int nrow = workingData.rowCount();

    // map "<sanitized column name>_first" to the column's first-row value for LeptonMini
    std::map<std::string, double> constants;
    for (int c = 0; c < ncol; ++c) {
        const std::string var = sanitizeVarName(workingData.columnName(c));
        if (nrow > 0) constants[var + "_first"] = workingData.column(c).front();
    }

    CompiledExpression program(expr);
    if (!program.isValid()) {
        warning(this, "Compute Column",
                QString("Could not evaluate the expression:\n%1").arg(program.error()));
        return;
    }

    std::vector<double> result;
    result.reserve(static_cast<std::size_t>(nrow));

    try {
        std::map<std::string, double> vars = std::move(constants);
        vars["row"]                        = 0.0;
        for (int r = 0; r < nrow; ++r) {
            for (int c = 0; c < ncol; ++c)
                vars[sanitizeVarName(workingData.columnName(c))] = workingData.column(c)[r];
            vars["row"] = static_cast<double>(r);
            result.push_back(program.evaluate(vars));
        }
    } catch (const std::exception &e) {
        warning(this, "Compute Column",
                QString("Could not evaluate the expression:\n%1").arg(e.what()));
        return;
    }

    workingData.addColumn(colName, std::move(result));
    appendColumnRow(colName, true);
    deriveNameEdit->clear();
    deriveExprEdit->clear();
    adjustSize();
}

int PlotDataDialog::xColumn() const
{
    for (int i = 0; i < ychecks.size(); ++i)
        if (!ychecks[i]->isChecked()) return i;
    return 0;
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

// Local Variables:
// c-basic-offset: 4
// End:
