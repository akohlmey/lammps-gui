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

#include "plotdata.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QScrollArea>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

PlotDataDialog::PlotDataDialog(const PlotData &data, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Select Columns to Plot");
    const int ncol = data.columnCount();
    const int nrow = data.rowCount();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(QString("%1 rows, %2 columns").arg(nrow).arg(ncol)));
    layout->addWidget(
        new QLabel("The first unselected column will be used as the x-axis."));

    // y-column selection checkboxes (default: every column but the first)
    auto *ybox = new QGroupBox("Columns to plot (uncheck the x-axis column)");
    auto *yl   = new QVBoxLayout(ybox);
    for (int c = 0; c < ncol; ++c) {
        auto *cb = new QCheckBox(data.columnName(c));
        cb->setChecked(c != 0);
        ychecks.append(cb);
        yl->addWidget(cb);
    }
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
            headers << data.columnName(c);
        table->setHorizontalHeaderLabels(headers);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        for (int c = 0; c < ncol; ++c) {
            const std::vector<double> &col = data.column(c);
            for (int r = 0; r < previewRows; ++r)
                table->setItem(r, c, new QTableWidgetItem(QString::number(col[r])));
        }
        table->resizeColumnsToContents();
        layout->addWidget(new QLabel("Preview:"));
        layout->addWidget(table);
    }

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
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

// Local Variables:
// c-basic-offset: 4
// End:
