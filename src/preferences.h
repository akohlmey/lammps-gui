// -*- c++ -*- /////////////////////////////////////////////////////////////////////////
// LAMMPS-GUI - A Graphical Tool to Learn and Explore the LAMMPS MD Simulation Software
//
// Copyright (c) 2023, 2024, 2025  Axel Kohlmeyer
//
// Documentation: https://lammps-gui.lammps.org/
// Contact: akohlmey@gmail.com
//
// This software is distributed under the GNU General Public License version 2 or later.
////////////////////////////////////////////////////////////////////////////////////////

#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

class QDialogButtonBox;
class QFont;
class QSettings;
class QTabWidget;
class LammpsWrapper;

class Preferences : public QDialog {
    Q_OBJECT

public:
    explicit Preferences(LammpsWrapper *lammps, QWidget *parent = nullptr);
    ~Preferences() override;

    Preferences()                               = delete;
    Preferences(const Preferences &)            = delete;
    Preferences(Preferences &&)                 = delete;
    Preferences &operator=(const Preferences &) = delete;
    Preferences &operator=(Preferences &&)      = delete;

private slots:
    void accept() override;

public:
    void set_relaunch(bool val) { need_relaunch = val; }

private:
    QTabWidget *tabWidget;
    QDialogButtonBox *buttonBox;
    QSettings *settings;
    LammpsWrapper *lammps;
    bool need_relaunch;
};

// individual tabs

class GeneralTab : public QWidget {
    Q_OBJECT

public:
    explicit GeneralTab(QSettings *settings, LammpsWrapper *lammps, QWidget *parent = nullptr);

private slots:
    void pluginpath();
    void newallfont();
    void newtextfont();

private:
    void updatefonts(const QFont &all, const QFont &text);
    QSettings *settings;
    LammpsWrapper *lammps;
};

class AcceleratorTab : public QWidget {
    Q_OBJECT

public:
    explicit AcceleratorTab(QSettings *settings, LammpsWrapper *lammps, QWidget *parent = nullptr);
    enum { None, Opt, OpenMP, Intel, Kokkos, Gpu };
    enum { Double, Mixed, Single };

private slots:
    void update_accel();

private:
    QSettings *settings;
    LammpsWrapper *lammps;
};

class SnapshotTab : public QWidget {
    Q_OBJECT

public:
    explicit SnapshotTab(QSettings *settings, QWidget *parent = nullptr);

private slots:
    void choose_vdw();
    void choose_bond();

private:
    QSettings *settings;
};

class EditorTab : public QWidget {
    Q_OBJECT

public:
    explicit EditorTab(QSettings *settings, QWidget *parent = nullptr);

private:
    QSettings *settings;
};

class ChartsTab : public QWidget {
    Q_OBJECT

public:
    explicit ChartsTab(QSettings *settings, QWidget *parent = nullptr);

private:
    QSettings *settings;
};

#endif

// Local Variables:
// c-basic-offset: 4
// End:
