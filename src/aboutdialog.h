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

#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QDialog>
#include <QScrollArea>

/**
 * @brief Custom About dialog for LAMMPS-GUI
 *
 * AboutDialog displays version information, LAMMPS configuration details,
 * and available styles in scrollable text areas. The dialog automatically
 * scrolls down when the content exceeds the visible area, pauses at the
 * bottom, and then loops back to the top.
 *
 * When details content is available, the dialog allocates 2/3 of the
 * scroll area space to the info text and 1/3 to the details text.
 * The details text uses the fixed-width font from QSettings "textfont".
 */
class AboutDialog : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param version Version information text displayed at the top
     * @param info LAMMPS configuration info displayed in a scroll area
     * @param details Style information displayed in a scroll area with fixed-width font
     * @param parent Parent widget
     */
    AboutDialog(const QString &version, const QString &info, const QString &details,
                QWidget *parent = nullptr);

    ~AboutDialog() override = default;

    AboutDialog()                                 = delete;
    AboutDialog(const AboutDialog &)              = delete;
    AboutDialog(AboutDialog &&)                   = delete;
    AboutDialog &operator=(const AboutDialog &)   = delete;
    AboutDialog &operator=(AboutDialog &&)        = delete;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void setupAutoScroll(QScrollArea *area);

    QScrollArea *infoScrollArea;    ///< Scroll area for LAMMPS configuration info
    QScrollArea *detailsScrollArea; ///< Scroll area for styles information (may be null)
};

#endif
// Local Variables:
// c-basic-offset: 4
// End:
