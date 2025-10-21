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

/**
 * @brief Preferences/Settings dialog for LAMMPS-GUI
 * 
 * This dialog provides a tabbed interface for configuring various aspects
 * of LAMMPS-GUI including:
 * - General settings (LAMMPS library path, plugins, etc.)
 * - Accelerator package settings
 * - Image viewer defaults
 * - Editor appearance and behavior
 * - Chart viewer settings
 * 
 * Settings are persisted using QSettings and loaded on startup.
 */
class Preferences : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param lammps Pointer to LammpsWrapper for querying LAMMPS configuration
     * @param parent Parent widget
     */
    explicit Preferences(LammpsWrapper *lammps, QWidget *parent = nullptr);
    
    /**
     * @brief Destructor - saves settings on close
     */
    ~Preferences() override;

    Preferences()                               = delete;
    Preferences(const Preferences &)            = delete;
    Preferences(Preferences &&)                 = delete;
    Preferences &operator=(const Preferences &) = delete;
    Preferences &operator=(Preferences &&)      = delete;

private slots:
    /**
     * @brief Handle dialog acceptance - saves all settings
     */
    void accept() override;

public:
    /**
     * @brief Set flag indicating application needs restart
     * @param val true if restart needed, false otherwise
     * 
     * Some settings require restarting the application to take effect.
     */
    void set_relaunch(bool val) { need_relaunch = val; }

private:
    QTabWidget *tabWidget;       ///< Tab widget for preference categories
    QDialogButtonBox *buttonBox; ///< Dialog buttons (OK, Cancel)
    QSettings *settings;         ///< Qt settings storage
    LammpsWrapper *lammps;       ///< LAMMPS interface for configuration queries
    bool need_relaunch;          ///< Flag indicating restart is needed
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
