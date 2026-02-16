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

#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QComboBox>
#include <QDialog>
#include <QImage>
#include <QString>
#include <map>

class QAction;
class QMenuBar;
class QDialogButtonBox;
class QLabel;
class QObject;
class QScrollArea;
class QStatusBar;
class LammpsWrapper;
class QComboBox;
class FixInfo;
class RegionInfo;

/**
 * @brief Dialog for viewing and manipulating LAMMPS snapshot images
 *
 * This class provides an image viewer dialog for displaying LAMMPS snapshots
 * created by the `dump image` command. It allows interactive manipulation of
 * visualization parameters such as zoom, rotation, atom size, coloring, and
 * rendering options. Changes can be applied to regenerate the image using the
 * LAMMPS library interface.
 */
class ImageViewer : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param fileName Path to the image file to display
     * @param _lammps Pointer to LammpsWrapper for regenerating images
     * @param parent Parent widget
     */
    explicit ImageViewer(const QString &fileName, LammpsWrapper *_lammps,
                         QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~ImageViewer() override = default;

    ImageViewer()                               = delete;
    ImageViewer(const ImageViewer &)            = delete;
    ImageViewer(ImageViewer &&)                 = delete;
    ImageViewer &operator=(const ImageViewer &) = delete;
    ImageViewer &operator=(ImageViewer &&)      = delete;

private slots:
    void saveAs(); ///< Save image to file
    void copy();   ///< Copy image to clipboard
    void quit();   ///< Close dialog

    void set_atom_size();      ///< Set atom display size
    void edit_size();          ///< Edit image dimensions
    void reset_view();         ///< Reset view to defaults
    void toggle_ssao();        ///< Toggle screen-space ambient occlusion
    void toggle_anti();        ///< Toggle antialiasing
    void toggle_shiny();       ///< Toggle shiny/specular rendering
    void toggle_vdw();         ///< Toggle Van der Waals radii
    void toggle_bond();        ///< Toggle bond display
    void set_bondcut();        ///< Set bond cutoff distance
    void toggle_box();         ///< Toggle simulation box display
    void toggle_axes();        ///< Toggle coordinate axes display
    void do_zoom_in();         ///< Zoom in view
    void do_zoom_out();        ///< Zoom out view
    void do_rot_left();        ///< Rotate view left
    void do_rot_right();       ///< Rotate view right
    void do_rot_up();          ///< Rotate view up
    void do_rot_down();        ///< Rotate view down
    void do_recenter();        ///< Recenter view
    void cmd_to_clipboard();   ///< Copy dump command to clipboard
    void global_settings();    ///< Configure global dump image settings
    void atom_settings();      ///< Configure atom and bond settings
    void fix_settings();       ///< Configure fix graphics display
    void region_settings();    ///< Configure region display
    void change_group(int);    ///< Change atom group selection
    void change_molecule(int); ///< Change molecule selection

public:
    /**
     * @brief Generate image using current settings
     *
     * Constructs and executes a LAMMPS dump image command with current
     * visualization parameters and updates the displayed image.
     */
    void createImage();

private:
    void createActions();                   ///< Setup menu actions
    void updateActions();                   ///< Update action states
    void saveFile(const QString &fileName); ///< Save image file
    void adjustWindowSize();                ///< Auto-resize window to fit image
    void update_fixes();                    ///< Update fix graphics information
    void update_regions();                  ///< Update region information
    bool has_autobonds();                   ///< Check if autobonds are enabled

private:
    QImage image;                ///< Currently displayed image
    QMenuBar *menuBar;           ///< Menu bar
    QLabel *imageLabel;          ///< Label displaying the image
    QScrollArea *scrollArea;     ///< Scrollable area for image
    QDialogButtonBox *buttonBox; ///< Dialog buttons
    double atomSize;             ///< Atom display size

    QAction *saveAsAct;     ///< Save As action
    QAction *copyAct;       ///< Copy action
    QAction *cmdAct;        ///< Copy command action
    QAction *zoomInAct;     ///< Zoom in action
    QAction *zoomOutAct;    ///< Zoom out action
    QAction *normalSizeAct; ///< Normal size action

    LammpsWrapper *lammps;                       ///< LAMMPS interface for image generation
    QString group;                               ///< Current atom group
    QString molecule;                            ///< Current molecule selection
    QString filename;                            ///< Image filename
    QString last_dump_cmd;                       ///< Last executed dump command
    int xsize, ysize;                            ///< Image dimensions in pixels
    int hrot, vrot;                              ///< Horizontal and vertical rotation angles
    double bodydiam;                             ///< bflag1 setting (diameter)
    int bodyflag;                                ///< bflag2 setting (cylinder, triangle or both)
    double linediam;                             ///< linewidth setting (diameter)
    double tridiam;                              ///< lflag1 setting (diameter)
    int triflag;                                 ///< lflag2 setting (cylinder, triangle or both)
    double zoom;                                 ///< Zoom level
    double vdwfactor;                            ///< Van der Waals radius scaling factor
    double shinyfactor;                          ///< Shininess/specular factor
    double bondcutoff;                           ///< Bond cutoff distance
    double boxdiam;                              ///< Simulation box diameter
    double subboxdiam;                           ///< Simulation subbox diameter
    double boxtrans;                             ///< Transparency for box and subbox
    double axeslen;                              ///< Axes length
    double axesdiam;                             ///< Axes diameter
    double axestrans;                            ///< Axes transparency
    double ssaoval;                              ///< SSAO strength
    QString axesloc;                             ///< Axes location
    QString boxcolor;                            ///< Color for box and subbox
    QString backcolor;                           ///< (lower) background color
    QString backcolor2;                          ///< (upper) background color
    double xcenter, ycenter, zcenter;            ///< View center coordinates
    bool showbox;                                ///< Show simulation box flag
    bool showsubbox;                             ///< Show subdomain boxes flag
    bool showaxes;                               ///< Show coordinate axes flag
    bool antialias;                              ///< Antialiasing enabled flag
    bool usessao;                                ///< SSAO enabled flag
    bool showbodies;                             ///< Show bodies if atom style supports it
    bool showellipsoids;                         ///< Show ellipsoids if atom style supports it
    bool showlines;                              ///< Show lines if atom style supports it
    bool showtris;                               ///< Show tris if atom style supports it
    bool useelements;                            ///< Use element properties flag
    bool usediameter;                            ///< Use diameter attribute flag
    bool usesigma;                               ///< Use sigma attribute flag
    bool autobond;                               ///< Auto-detect bonds flag
    std::map<std::string, FixInfo *> fixes;      ///< Fix graphics definitions
    std::map<std::string, RegionInfo *> regions; ///< Region definitions
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
