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

#include "tutorialwizard.h"

#include "helpers.h"
#include "lammpsgui.h"

#include <QCheckBox>
#include <QDir>
#include <QIcon>
#include <QLineEdit>

TutorialWizard::TutorialWizard(int ntutorial, QWidget *parent) :
    QWizard(parent), _ntutorial(ntutorial)
{
    setWindowIcon(QIcon(":/icons/tutorial-logo.png"));
}

// actions to perform when the wizard for tutorial 1 is complete
// and the user has clicked on "Finish"

void TutorialWizard::accept()
{
    // get pointers to the widgets with the information we need
    auto *dirname    = findChild<QLineEdit *>("t_directory");
    auto *dirpurge   = findChild<QCheckBox *>("t_dirpurge");
    auto *getsol     = findChild<QCheckBox *>("t_getsolution");
    auto *webopen    = findChild<QCheckBox *>("t_webopen");
    bool purgedir    = false;
    bool getsolution = false;
    bool openwebpage = false;
    QString curdir;

    if (webopen) openwebpage = webopen->isChecked();

    // create and populate directory.
    if (dirname) {
        QDir directory;
        curdir = dirname->text().trimmed();
        if (!directory.mkpath(curdir)) {
            warning(this, "LAMMPS-GUI Warning",
                    QString("Cannot create tutorial %1 working directory '%2'.")
                        .arg(_ntutorial)
                        .arg(curdir),
                    "Going back to directory selection.");
            back();
            return;
        }

        purgedir    = dirpurge && dirpurge->isChecked();
        getsolution = getsol && getsol->isChecked();
    }
    QDialog::accept();

    // get hold of LAMMPS-GUI main widget
    if (dirname) {
        auto *main = dynamic_cast<LammpsGui *>(getMainWidget());
        if (main) main->setupTutorial(_ntutorial, curdir, purgedir, getsolution, openwebpage);
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
