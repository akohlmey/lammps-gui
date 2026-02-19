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

#include "imageviewer.h"

#include "helpers.h"
#include "lammpsgui.h"
#include "lammpswrapper.h"
#include "qaddon.h"
#include "stdcapture.h"

#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QDoubleValidator>
#include <QEvent>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontMetrics>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QImage>
#include <QImageReader>
#include <QIntValidator>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPalette>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QSizePolicy>
#include <QSpinBox>
#include <QString>
#include <QStringList>
#include <QTemporaryFile>
#include <QVBoxLayout>
#include <QVariant>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

// clang-format off
/* periodic table of elements for translation of ordinal to atom type */
namespace {
    const char * const pte_label[] = {
    "X",  "H",  "He", "Li", "Be", "B",  "C",  "N",  "O",  "F",  "Ne",
    "Na", "Mg", "Al", "Si", "P" , "S",  "Cl", "Ar", "K",  "Ca", "Sc",
    "Ti", "V",  "Cr", "Mn", "Fe", "Co", "Ni", "Cu", "Zn", "Ga", "Ge",
    "As", "Se", "Br", "Kr", "Rb", "Sr", "Y",  "Zr", "Nb", "Mo", "Tc",
    "Ru", "Rh", "Pd", "Ag", "Cd", "In", "Sn", "Sb", "Te", "I",  "Xe",
    "Cs", "Ba", "La", "Ce", "Pr", "Nd", "Pm", "Sm", "Eu", "Gd", "Tb",
    "Dy", "Ho", "Er", "Tm", "Yb", "Lu", "Hf", "Ta", "W",  "Re", "Os",
    "Ir", "Pt", "Au", "Hg", "Tl", "Pb", "Bi", "Po", "At", "Rn", "Fr",
    "Ra", "Ac", "Th", "Pa", "U",  "Np", "Pu", "Am", "Cm", "Bk", "Cf",
    "Es", "Fm", "Md", "No", "Lr", "Rf", "Db", "Sg", "Bh", "Hs", "Mt",
    "Ds", "Rg"
};
constexpr int nr_pte_entries = sizeof(pte_label) / sizeof(char *);

/* corresponding table of masses. */
constexpr double pte_mass[] = {
    /* X  */ 0.00000, 1.00794, 4.00260, 6.941, 9.012182, 10.811,
    /* C  */ 12.0107, 14.0067, 15.9994, 18.9984032, 20.1797,
    /* Na */ 22.989770, 24.3050, 26.981538, 28.0855, 30.973761,
    /* S  */ 32.065, 35.453, 39.948, 39.0983, 40.078, 44.955910,
    /* Ti */ 47.867, 50.9415, 51.9961, 54.938049, 55.845, 58.9332,
    /* Ni */ 58.6934, 63.546, 65.409, 69.723, 72.64, 74.92160,
    /* Se */ 78.96, 79.904, 83.798, 85.4678, 87.62, 88.90585,
    /* Zr */ 91.224, 92.90638, 95.94, 98.0, 101.07, 102.90550,
    /* Pd */ 106.42, 107.8682, 112.411, 114.818, 118.710, 121.760,
    /* Te */ 127.60, 126.90447, 131.293, 132.90545, 137.327,
    /* La */ 138.9055, 140.116, 140.90765, 144.24, 145.0, 150.36,
    /* Eu */ 151.964, 157.25, 158.92534, 162.500, 164.93032,
    /* Er */ 167.259, 168.93421, 173.04, 174.967, 178.49, 180.9479,
    /* W  */ 183.84, 186.207, 190.23, 192.217, 195.078, 196.96655,
    /* Hg */ 200.59, 204.3833, 207.2, 208.98038, 209.0, 210.0, 222.0,
    /* Fr */ 223.0, 226.0, 227.0, 232.0381, 231.03588, 238.02891,
    /* Np */ 237.0, 244.0, 243.0, 247.0, 247.0, 251.0, 252.0, 257.0,
    /* Md */ 258.0, 259.0, 262.0, 261.0, 262.0, 266.0, 264.0, 269.0,
    /* Mt */ 268.0, 271.0, 272.0
};

/*
 * corresponding table of VDW radii.
 * van der Waals radii are taken from A. Bondi,
 * J. Phys. Chem., 68, 441 - 452, 1964,
 * except the value for H, which is taken from R.S. Rowland & R. Taylor,
 * J.Phys.Chem., 100, 7384 - 7391, 1996. Radii that are not available in
 * either of these publications have RvdW = 2.00 \AA
 * The radii for Ions (Na, K, Cl, Ca, Mg, and Cs are based on the CHARMM27
 * Rmin/2 parameters for (SOD, POT, CLA, CAL, MG, CES) by default.
 */
constexpr double pte_vdw_radius[] = {
    /* X  */ 1.5, 1.2, 1.4, 1.82, 2.0, 2.0,
    /* C  */ 1.7, 1.55, 1.52, 1.47, 1.54,
    /* Na */ 1.36, 1.18, 2.0, 2.1, 1.8,
    /* S  */ 1.8, 2.27, 1.88, 1.76, 1.37, 2.0,
    /* Ti */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Ni */ 1.63, 1.4, 1.39, 1.07, 2.0, 1.85,
    /* Se */ 1.9, 1.85, 2.02, 2.0, 2.0, 2.0,
    /* Zr */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Pd */ 1.63, 1.72, 1.58, 1.93, 2.17, 2.0,
    /* Te */ 2.06, 1.98, 2.16, 2.1, 2.0,
    /* La */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Eu */ 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Er */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* W  */ 2.0, 2.0, 2.0, 2.0, 1.72, 1.66,
    /* Hg */ 1.55, 1.96, 2.02, 2.0, 2.0, 2.0, 2.0,
    /* Fr */ 2.0, 2.0, 2.0, 2.0, 2.0, 1.86,
    /* Np */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Md */ 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0, 2.0,
    /* Mt */ 2.0, 2.0, 2.0
};

// clang-format on

int get_pte_from_mass(double mass)
{
    int idx = 0;
    for (int i = 0; i < nr_pte_entries; ++i)
        if (fabs(mass - pte_mass[i]) < 0.65) idx = i;
    if ((mass > 0.0) && (mass < 2.2)) idx = 1;
    // discriminate between Cobalt and Nickel. The loop will detect Nickel
    if ((mass < 61.24) && (mass > 58.8133)) idx = 27;
    return idx;
}

QStringList defaultcolors = {"white", "gray",  "magenta", "cyan",   "yellow",
                             "blue",  "green", "red",     "orange", "brown"};

// constants
const QString blank(" ");
constexpr double VDW_ON           = 1.6;
constexpr double VDW_OFF          = 0.5;
constexpr double VDW_CUT          = 1.0;
constexpr double SHINY_ON         = 0.6;
constexpr double SHINY_OFF        = 0.2;
constexpr double SHINY_CUT        = 0.4;
constexpr int DEFAULT_BUFLEN      = 1024;
constexpr int DEFAULT_NPOINTS     = 100000;
constexpr double DEFAULT_DIAMETER = 0.2;
constexpr double DEFAULT_OPACITY  = 0.5;
constexpr int EXTRA_WIDTH         = 150;
constexpr int EXTRA_HEIGHT        = 105;
constexpr int TITLE_MARGIN        = 10;

enum { FRAME, FILLED, TRANSPARENT, POINTS };
enum { TYPE, ELEMENT, CONSTANT };

} // namespace

/**
 * @brief Store settings for displaying graphics from a fix or compute in a LAMMPS snapshot image
 */
class ImageInfo {
public:
    ImageInfo() = delete;
    /** Custom constructor */
    ImageInfo(bool _enabled, const QString &_style, int _colorstyle, const std::string &_color,
              double _opacity, double _flag1, double _flag2) :
        enabled(_enabled), style(_style), colorstyle(_colorstyle), color(_color), opacity(_opacity),
        flag1(_flag1), flag2(_flag2)
    {
    }

    bool enabled;      ///< display graphics if true
    QString style;     ///< name of style
    int colorstyle;    ///< color style for graphics: TYPE, ELEMENT, CONSTANT
    std::string color; ///< custom color of graphics objects for style == CONSTANT
    double opacity;    ///< opacity of graphics objects for style == CONSTANT
    double flag1;      ///< Flag #1 for graphics
    double flag2;      ///< Flag #2 for graphics
};

/**
 * @brief Store settings for displaying a region in a LAMMPS snapshot image
 */
class RegionInfo {
public:
    RegionInfo() = delete;
    /** Custom constructor */
    RegionInfo(bool _enabled, int _style, const std::string &_color, double _diameter,
               double _opacity, int _npoints) :
        enabled(_enabled), style(_style), color(_color), diameter(_diameter), opacity(_opacity),
        npoints(_npoints)
    {
    }

    bool enabled;      ///< display region if true
    int style;         ///< style of region object: FRAME, FILLED, TRANSPARENT, or POINTS
    std::string color; ///< color of region display
    double diameter;   ///< diameter value for POINTS and FRAME
    double opacity;    ///< opacity for TRANSPARENT
    int npoints;       ///< number of points to be used for POINTS style region display
};

ImageViewer::ImageViewer(const QString &fileName, LammpsWrapper *_lammps, QWidget *parent) :
    QDialog(parent), menuBar(new QMenuBar), imageLabel(new QLabel), scrollArea(new QScrollArea),
    buttonBox(nullptr), atomSize(1.0), saveAsAct(nullptr), copyAct(nullptr), cmdAct(nullptr),
    zoomInAct(nullptr), zoomOutAct(nullptr), normalSizeAct(nullptr), lammps(_lammps), group("all"),
    molecule("none"), filename(fileName), useelements(false), usediameter(false), usesigma(false)
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    imageLabel->setScaledContents(true);
    imageLabel->minimumSizeHint();

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setVisible(false);

    auto *imageLayout    = new QHBoxLayout;
    auto *settingsLayout = new QVBoxLayout;
    auto *mainLayout     = new QVBoxLayout;

    QFile image_styles(":/image_style.table");
    if (image_styles.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!image_styles.atEnd()) {
            auto line  = QString(image_styles.readLine());
            auto words = line.trimmed().replace('\t', ' ').split(' ');
            if (words.size() == 2) {
                if (words.at(0) == "compute") {
                    image_computes << words.at(1);
                } else if (words.at(0) == "fix") {
                    image_fixes << words.at(1);
                } else {
                    fprintf(stderr, "unhandled image style: %s", line.toStdString().c_str());
                }
            } else {
                fprintf(stderr, "unhandled image style: %s", line.toStdString().c_str());
            }
        }
        image_styles.close();
    }

    QSettings settings;
    settings.beginGroup("snapshot");
    xsize          = settings.value("xsize", "600").toInt();
    ysize          = settings.value("ysize", "600").toInt();
    zoom           = settings.value("zoom", 1.0).toDouble();
    hrot           = settings.value("hrot", 60).toInt();
    vrot           = settings.value("vrot", 30).toInt();
    shinyfactor    = settings.value("shinystyle", true).toBool() ? SHINY_ON : SHINY_OFF;
    vdwfactor      = settings.value("vdwstyle", false).toBool() ? VDW_ON : VDW_OFF;
    autobond       = settings.value("autobond", false).toBool();
    bondcutoff     = settings.value("bondcutoff", 1.6).toDouble();
    showbox        = settings.value("box", true).toBool();
    showsubbox     = false;
    boxdiam        = settings.value("boxdiam", 0.025).toDouble();
    subboxdiam     = boxdiam;
    boxcolor       = settings.value("boxcolor", "yellow").toString();
    showaxes       = settings.value("axes", false).toBool();
    usessao        = settings.value("ssao", false).toBool();
    antialias      = settings.value("antialias", false).toBool();
    axeslen        = settings.value("axeslen", 0.5).toDouble();
    axesdiam       = settings.value("axesdiam", 0.05).toDouble();
    axestrans      = 1.0;
    axesloc        = "yes"; // = "lowerleft"
    boxtrans       = 1.0;
    backcolor      = settings.value("backcolor", "black").toString();
    backcolor2     = settings.value("backcolor2", "white").toString();
    ssaoval        = 0.6;
    showbodies     = true;
    bodydiam       = 0.2;
    bodyflag       = 3;
    showellipsoids = true;
    showlines      = true;
    linediam       = 0.2;
    showtris       = true;
    tridiam        = 0.2;
    triflag        = 3;
    xcenter = ycenter = zcenter = 0.5;

    if (lammps->extract_setting("dimension") == 2) zcenter = 0.0;
    settings.endGroup();

    auto pix   = QPixmap(":/icons/emblem-photos.png");
    auto bsize = QFontMetrics(QApplication::font()).size(Qt::TextSingleLine, "Height:  200xxxx");

    auto *renderstatus = new QLabel(QString());
    renderstatus->setPixmap(pix.scaled(22, 22, Qt::KeepAspectRatio));
    renderstatus->setEnabled(false);
    renderstatus->setToolTip("Render status");
    renderstatus->setObjectName("renderstatus");
    auto *asize = new QLineEdit(QString::number(atomSize));
    auto *valid = new QDoubleValidator(1.0e-10, 1.0e10, 10, asize);
    asize->setValidator(valid);
    asize->setObjectName("atomSize");
    asize->setToolTip("Set Atom size");
    asize->setEnabled(false);
    asize->hide();

    auto *xval = new QSpinBox;
    xval->setRange(100, 10000);
    xval->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    xval->setValue(xsize);
    xval->setObjectName("xsize");
    xval->setToolTip("Set rendered image width");
    xval->setMinimumSize(bsize);
    auto *yval = new QSpinBox;
    yval->setRange(100, 10000);
    yval->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    yval->setValue(ysize);
    yval->setObjectName("ysize");
    yval->setToolTip("Set rendered image height");
    yval->setMinimumSize(bsize);

    connect(asize, &QLineEdit::editingFinished, this, &ImageViewer::set_atom_size);
    connect(xval, &QAbstractSpinBox::editingFinished, this, &ImageViewer::edit_size);
    connect(yval, &QAbstractSpinBox::editingFinished, this, &ImageViewer::edit_size);

    // workaround for incorrect highlight bug on macOS
    auto *dummy1 = new QPushButton(QIcon(), "");
    dummy1->hide();
    auto *dummy2 = new QPushButton(QIcon(), "");
    dummy2->hide();

    auto *dossao = new QPushButton(QIcon(":/icons/hd-img.png"), "");
    dossao->setCheckable(true);
    dossao->setToolTip("Toggle SSAO rendering");
    dossao->setObjectName("ssao");
    auto *doanti = new QPushButton(QIcon(":/icons/antialias.png"), "");
    doanti->setCheckable(true);
    doanti->setToolTip("Toggle anti-aliasing");
    doanti->setObjectName("antialias");
    auto *doshiny = new QPushButton(QIcon(":/icons/image-shiny.png"), "");
    doshiny->setCheckable(true);
    doshiny->setToolTip("Toggle shininess");
    doshiny->setObjectName("shiny");
    auto *dovdw = new QPushButton(QIcon(":/icons/vdw-style.png"), "");
    dovdw->setCheckable(true);
    dovdw->setToolTip("Toggle VDW style representation");
    dovdw->setObjectName("vdw");
    auto *dobond = new QPushButton(QIcon(":/icons/autobonds.png"), "");
    dobond->setCheckable(true);
    dobond->setToolTip("Toggle dynamic bond representation");
    dobond->setObjectName("autobond");
    dobond->setEnabled(false);
    auto *bondcut = new QLineEdit(QString::number(bondcutoff));
    bondcut->setMaxLength(5);
    bondcut->setObjectName("bondcut");
    bondcut->setToolTip("Set dynamic bond cutoff");
    QFontMetrics metrics(bondcut->fontMetrics());
    bondcut->setFixedSize(metrics.averageCharWidth() * 6, metrics.height() + 4);
    bondcut->setEnabled(false);
    auto *dobox = new QPushButton(QIcon(":/icons/system-box.png"), "");
    dobox->setCheckable(true);
    dobox->setToolTip("Toggle displaying box");
    dobox->setObjectName("box");
    auto *doaxes = new QPushButton(QIcon(":/icons/axes-img.png"), "");
    doaxes->setCheckable(true);
    doaxes->setToolTip("Toggle displaying axes");
    doaxes->setObjectName("axes");
    auto *zoomin = new QPushButton(QIcon(":/icons/gtk-zoom-in.png"), "");
    zoomin->setToolTip("Zoom in by 10 percent");
    auto *zoomout = new QPushButton(QIcon(":/icons/gtk-zoom-out.png"), "");
    zoomout->setToolTip("Zoom out by 10 percent");
    auto *rotleft = new QPushButton(QIcon(":/icons/object-rotate-left.png"), "");
    rotleft->setToolTip("Rotate left by 15 degrees");
    auto *rotright = new QPushButton(QIcon(":/icons/object-rotate-right.png"), "");
    rotright->setToolTip("Rotate right by 15 degrees");
    auto *rotup = new QPushButton(QIcon(":/icons/gtk-go-up.png"), "");
    rotup->setToolTip("Rotate up by 15 degrees");
    auto *rotdown = new QPushButton(QIcon(":/icons/gtk-go-down.png"), "");
    rotdown->setToolTip("Rotate down by 15 degrees");
    auto *recenter = new QPushButton(QIcon(":/icons/move-recenter.png"), "");
    recenter->setToolTip("Recenter on group");
    auto *reset = new QPushButton(QIcon(":/icons/gtk-zoom-fit.png"), "");
    reset->setToolTip("Reset view to defaults");

    auto *setviz = new QPushButton("&Settings");
    setviz->setToolTip("Open dialog for general graphics settings");
    setviz->setObjectName("settings");
    auto *atomviz = new QPushButton("&Atoms");
    atomviz->setToolTip("Open dialog for Atom and Bond settings");
    atomviz->setObjectName("atoms");

    auto *fixviz = new QPushButton("&Computes\nand Fixes");
    fixviz->setToolTip("Open dialog for visualizing graphics from computes and fixes");
    fixviz->setObjectName("image_styles");
    fixviz->setEnabled(false);
    auto *regviz = new QPushButton("&Regions");
    regviz->setToolTip("Open dialog for visualizing regions");
    regviz->setObjectName("regions");
    regviz->setEnabled(false);

    constexpr int BUFLEN = 256;
    char gname[BUFLEN];
    auto *combo = new QComboBox;
    combo->setToolTip("Select group to display");
    combo->setObjectName("group");
    int ngroup = lammps->id_count("group");
    for (int i = 0; i < ngroup; ++i) {
        memset(gname, 0, BUFLEN);
        lammps->id_name("group", i, gname, BUFLEN);
        combo->addItem(gname);
    }

    auto *molbox = new QComboBox;
    molbox->setToolTip("Select molecule to display");
    molbox->setObjectName("molecule");
    molbox->addItem("none");
    int nmols = lammps->id_count("molecule");
    for (int i = 0; i < nmols; ++i) {
        memset(gname, 0, BUFLEN);
        lammps->id_name("molecule", i, gname, BUFLEN);
        molbox->addItem(gname);
    }

    auto *menuLayout   = new QHBoxLayout;
    auto *buttonLayout = new QHBoxLayout;
    auto *topLayout    = new QVBoxLayout;
    topLayout->addLayout(menuLayout);
    topLayout->addLayout(buttonLayout);

    menuLayout->addWidget(menuBar);
    menuLayout->insertStretch(1, 10);
    menuLayout->addWidget(renderstatus);
    menuLayout->addWidget(new QLabel(" Atom Size: "));
    menuLayout->addWidget(asize);
    // hide item initially
    menuLayout->itemAt(3)->widget()->setObjectName("AtomLabel");
    menuLayout->itemAt(3)->widget()->hide();
    menuLayout->addWidget(new QLabel(" <u>W</u>idth: "));
    menuLayout->addWidget(xval);
    menuLayout->addWidget(new QLabel(" <u>H</u>eight: "));
    menuLayout->addWidget(yval);
    menuLayout->insertStretch(-1, 50);
    buttonLayout->addWidget(dummy2);
    buttonLayout->addWidget(dossao);
    buttonLayout->addWidget(doanti);
    buttonLayout->addWidget(doshiny);
    buttonLayout->addWidget(dovdw);
    buttonLayout->addWidget(dobond);
    buttonLayout->addWidget(bondcut);
    buttonLayout->addWidget(dobox);
    buttonLayout->addWidget(doaxes);
    buttonLayout->addWidget(zoomin);
    buttonLayout->addWidget(zoomout);
    buttonLayout->addWidget(rotleft);
    buttonLayout->addWidget(rotright);
    buttonLayout->addWidget(rotup);
    buttonLayout->addWidget(rotdown);
    buttonLayout->addWidget(recenter);
    buttonLayout->addWidget(reset);
    buttonLayout->insertStretch(-1, 1);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(new QLabel("<u>G</u>roup:"));
    settingsLayout->addWidget(combo);
    settingsLayout->addWidget(new QLabel("<u>M</u>olecule:"));
    settingsLayout->addWidget(molbox);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(setviz);
    settingsLayout->addWidget(atomviz);
    settingsLayout->addWidget(fixviz);
    settingsLayout->addWidget(regviz);
    settingsLayout->addWidget(new QHline);
    settingsLayout->insertStretch(-1, 10);

    connect(dossao, &QPushButton::released, this, &ImageViewer::toggle_ssao);
    connect(doanti, &QPushButton::released, this, &ImageViewer::toggle_anti);
    connect(doshiny, &QPushButton::released, this, &ImageViewer::toggle_shiny);
    connect(dovdw, &QPushButton::released, this, &ImageViewer::toggle_vdw);
    connect(dobond, &QPushButton::released, this, &ImageViewer::toggle_bond);
    connect(bondcut, &QLineEdit::editingFinished, this, &ImageViewer::set_bondcut);
    connect(dobox, &QPushButton::released, this, &ImageViewer::toggle_box);
    connect(doaxes, &QPushButton::released, this, &ImageViewer::toggle_axes);
    connect(zoomin, &QPushButton::released, this, &ImageViewer::do_zoom_in);
    connect(zoomout, &QPushButton::released, this, &ImageViewer::do_zoom_out);
    connect(rotleft, &QPushButton::released, this, &ImageViewer::do_rot_left);
    connect(rotright, &QPushButton::released, this, &ImageViewer::do_rot_right);
    connect(rotup, &QPushButton::released, this, &ImageViewer::do_rot_up);
    connect(rotdown, &QPushButton::released, this, &ImageViewer::do_rot_down);
    connect(recenter, &QPushButton::released, this, &ImageViewer::do_recenter);
    connect(reset, &QPushButton::released, this, &ImageViewer::reset_view);
    connect(setviz, &QPushButton::released, this, &ImageViewer::global_settings);
    connect(atomviz, &QPushButton::released, this, &ImageViewer::atom_settings);
    connect(fixviz, &QPushButton::released, this, &ImageViewer::fix_settings);
    connect(regviz, &QPushButton::released, this, &ImageViewer::region_settings);
    connect(combo, SIGNAL(currentIndexChanged(int)), this, SLOT(change_group(int)));
    connect(molbox, SIGNAL(currentIndexChanged(int)), this, SLOT(change_molecule(int)));

    mainLayout->addLayout(topLayout);
    imageLayout->addWidget(scrollArea, 10);
    imageLayout->addLayout(settingsLayout, 0);
    mainLayout->addLayout(imageLayout);
    setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setWindowTitle(QString("LAMMPS-GUI - Image Viewer - ") + QFileInfo(fileName).fileName());
    createActions();

    reset_view();
    // layout has not yet been established, so we need to fix up some pushbutton
    // properties directly since lookup in reset_view() will have failed
    dobox->setChecked(showbox);
    doshiny->setChecked(shinyfactor > SHINY_CUT);
    dovdw->setChecked(vdwfactor > VDW_CUT);
    dovdw->setEnabled(useelements || usediameter || usesigma);
    dobond->setChecked(autobond);
    dobond->setEnabled(has_autobonds());
    doaxes->setChecked(showaxes);
    dossao->setChecked(usessao);
    doanti->setChecked(antialias);

    scrollArea->setVisible(true);
    updateActions();
    setLayout(mainLayout);
    adjustWindowSize();
    update_fixes();
    update_regions();
    menuBar->setFocus();

    // make Alt-G, Alt-H, Alt-M, and Alt-W hotkeys work for comboboxes and spinboxes
    xval->installEventFilter(this);
    yval->installEventFilter(this);
    combo->installEventFilter(this);
    for (auto &obj : combo->children())
        obj->installEventFilter(this);
    molbox->installEventFilter(this);
    for (auto &obj : molbox->children())
        obj->installEventFilter(this);
    installEventFilter(this);

    // set window flags for window manager
    auto flags = windowFlags();
    flags &= ~Qt::Dialog;
    flags |= Qt::CustomizeWindowHint;
    flags |= Qt::WindowMinimizeButtonHint;
    setWindowFlags(flags);
}

void ImageViewer::reset_view()
{
    QSettings settings;
    settings.beginGroup("snapshot");
    xsize       = settings.value("xsize", "600").toInt();
    ysize       = settings.value("ysize", "600").toInt();
    zoom        = settings.value("zoom", 1.0).toDouble();
    hrot        = settings.value("hrot", 60).toInt();
    vrot        = settings.value("vrot", 30).toInt();
    shinyfactor = settings.value("shinystyle", true).toBool() ? SHINY_ON : SHINY_OFF;
    vdwfactor   = settings.value("vdwstyle", false).toBool() ? VDW_ON : VDW_OFF;
    autobond    = settings.value("autobond", false).toBool();
    bondcutoff  = settings.value("bondcutoff", 1.6).toDouble();
    showbox     = settings.value("box", true).toBool();
    showaxes    = settings.value("axes", false).toBool();
    usessao     = settings.value("ssao", false).toBool();
    antialias   = settings.value("antialias", false).toBool();
    boxdiam     = settings.value("boxdiam", 0.025).toDouble();
    axeslen     = settings.value("axeslen", 0.5).toDouble();
    axesdiam    = settings.value("axesdiam", 0.05).toDouble();
    xcenter = ycenter = zcenter = 0.5;
    if (lammps->extract_setting("dimension") == 2) zcenter = 0.0;
    settings.endGroup();

    // reset state of checkable push buttons and combo box (if accessible)

    auto *field = findChild<QSpinBox *>("xsize");
    if (field) field->setValue(xsize);
    field = findChild<QSpinBox *>("ysize");
    if (field) field->setValue(ysize);

    auto *button = findChild<QPushButton *>("ssao");
    if (button) button->setChecked(usessao);
    button = findChild<QPushButton *>("antialias");
    if (button) button->setChecked(antialias);
    button = findChild<QPushButton *>("shiny");
    if (button) button->setChecked(shinyfactor > SHINY_CUT);
    button = findChild<QPushButton *>("vdw");
    if (button) button->setChecked(vdwfactor > VDW_CUT);
    button = findChild<QPushButton *>("autobond");
    if (button) {
        button->setEnabled(has_autobonds());
        button->setChecked(autobond && has_autobonds());
    }
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) {
        cutoff->setEnabled(autobond && has_autobonds());
        cutoff->setText(QString::number(bondcutoff));
    }
    button = findChild<QPushButton *>("box");
    if (button) button->setChecked(showbox);
    button = findChild<QPushButton *>("axes");
    if (button) button->setChecked(showaxes);
    auto *cb = findChild<QComboBox *>("combo");
    if (cb) cb->setCurrentText("all");
    createImage();
}

void ImageViewer::set_atom_size()
{
    auto *field = qobject_cast<QLineEdit *>(sender());
    atomSize    = field->text().toDouble();
    createImage();
}

void ImageViewer::edit_size()
{
    auto *field = qobject_cast<QSpinBox *>(sender());
    if (field->objectName() == "xsize") {
        xsize = field->value();
    } else if (field->objectName() == "ysize") {
        ysize = field->value();
    }
    createImage();
}

void ImageViewer::toggle_ssao()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    usessao      = !usessao;
    button->setChecked(usessao);
    createImage();
}

void ImageViewer::toggle_anti()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    antialias    = !antialias;
    button->setChecked(antialias);
    createImage();
}

void ImageViewer::toggle_shiny()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (shinyfactor > SHINY_CUT)
        shinyfactor = SHINY_OFF;
    else
        shinyfactor = SHINY_ON;
    button->setChecked(shinyfactor > SHINY_CUT);
    createImage();
}

void ImageViewer::toggle_vdw()
{
    auto *button = qobject_cast<QPushButton *>(sender());

    if (button->isChecked())
        vdwfactor = VDW_ON;
    else
        vdwfactor = VDW_OFF;

    // when enabling VDW rendering, we must turn off autobond
    bool do_vdw = vdwfactor > VDW_CUT;
    if (do_vdw) {
        autobond   = false;
        auto *bond = findChild<QPushButton *>("autobond");
        if (bond) bond->setChecked(false);
        auto *cutoff = findChild<QLineEdit *>("bondcut");
        if (cutoff) cutoff->setEnabled(false);
    }

    button->setChecked(do_vdw);
    createImage();
}

void ImageViewer::toggle_bond()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (button) autobond = button->isChecked();
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) cutoff->setEnabled(autobond);
    set_bondcut();

    // when enabling autobond, we must turn off VDW
    if (autobond) {
        vdwfactor = VDW_OFF;
        auto *vdw = findChild<QPushButton *>("vdw");
        if (vdw) vdw->setChecked(false);
    }

    button->setChecked(autobond);
    createImage();
}

void ImageViewer::set_bondcut()
{
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) {
        auto *dptr            = (double *)lammps->extract_global("neigh_cutmax");
        double max_bondcutoff = (dptr) ? *dptr : 0.0;
        double new_bondcutoff = cutoff->text().toDouble();

        if ((max_bondcutoff > 0.1) && (new_bondcutoff > max_bondcutoff))
            new_bondcutoff = max_bondcutoff;
        if (new_bondcutoff > 0.1) bondcutoff = new_bondcutoff;

        cutoff->setText(QString::number(bondcutoff));
    }
    createImage();
}

void ImageViewer::toggle_box()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    showbox      = !showbox;
    button->setChecked(showbox);
    createImage();
}

void ImageViewer::toggle_axes()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    showaxes     = !showaxes;
    button->setChecked(showaxes);
    createImage();
}

void ImageViewer::do_zoom_in()
{
    zoom = zoom * 1.1;
    zoom = std::min(zoom, 10.0);
    createImage();
}

void ImageViewer::do_zoom_out()
{
    zoom = zoom / 1.1;
    zoom = std::max(zoom, 0.25);
    createImage();
}

void ImageViewer::do_rot_left()
{
    vrot -= 10;
    if (vrot < -180) vrot += 360;
    createImage();
}

void ImageViewer::do_rot_right()
{
    vrot += 10;
    if (vrot > 180) vrot -= 360;
    createImage();
}

void ImageViewer::do_rot_down()
{
    hrot -= 10;
    if (hrot < 0) hrot += 360;
    createImage();
}

void ImageViewer::do_rot_up()
{
    hrot += 10;
    if (hrot > 360) hrot -= 360;
    createImage();
}

void ImageViewer::do_recenter()
{
    QString commands = QString("variable LAMMPSGUI_CX delete\n"
                               "variable LAMMPSGUI_CY delete\n"
                               "variable LAMMPSGUI_CZ delete\n"
                               "variable LAMMPSGUI_CX equal (xcm(%1,x)-xlo)/lx\n"
                               "variable LAMMPSGUI_CY equal (xcm(%1,y)-ylo)/ly\n"
                               "variable LAMMPSGUI_CZ equal (xcm(%1,z)-zlo)/lz\n")
                           .arg(group);
    lammps->commands_string(commands);
    xcenter = lammps->extract_variable("LAMMPSGUI_CX");
    ycenter = lammps->extract_variable("LAMMPSGUI_CY");
    zcenter = lammps->extract_variable("LAMMPSGUI_CZ");
    if (lammps->extract_setting("dimension") == 2) zcenter = 0.0;
    lammps->commands_string("variable LAMMPSGUI_CX delete\n"
                            "variable LAMMPSGUI_CY delete\n"
                            "variable LAMMPSGUI_CZ delete\n");
    createImage();
}

void ImageViewer::cmd_to_clipboard()
{
    auto words = split_line(last_dump_cmd.toStdString());
    int modidx = 0;
    int maxidx = words.size();
    for (int i = 0; i < maxidx; ++i) {
        if (words[i] == "modify") {
            modidx = i;
            break;
        }
    }

    std::string dumpcmd = "dump viz ";
    dumpcmd += words[1];

    if (lammps->config_has_png_support()) {
        dumpcmd += " image 100 myimage-*.png";
    } else if (lammps->config_has_jpeg_support()) {
        dumpcmd += " image 100 myimage-*.jpg";
    } else {
        dumpcmd += " image 100 myimage-*.ppm";
    }

    for (int i = 4; i < modidx; ++i)
        if (words[i] != "noinit") dumpcmd += " " + words[i];
    dumpcmd += '\n';

    dumpcmd += "dump_modify viz pad 9";
    for (int i = modidx + 1; i < maxidx; ++i)
        dumpcmd += " " + words[i];
    dumpcmd += '\n';
#if QT_CONFIG(clipboard)
    QGuiApplication::clipboard()->setText(dumpcmd.c_str(), QClipboard::Clipboard);
    if (QGuiApplication::clipboard()->supportsSelection())
        QGuiApplication::clipboard()->setText(dumpcmd.c_str(), QClipboard::Selection);
#else
    fprintf(stderr, "# customized dump image command:\n%s", dumpcmd.c_str());
#endif
}

void ImageViewer::global_settings()
{
    QDialog setview;
    setview.setWindowTitle(QString("LAMMPS-GUI - Global image settings"));
    setview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setview.setMinimumSize(100, 100);
    setview.setContentsMargins(5, 5, 5, 5);
    setview.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    auto *title = new QLabel("Global image settings:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *colorcompleter = new QColorCompleter;
    auto *colorvalidator = new QColorValidator;
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 5);
    QFontMetrics metrics(setview.fontMetrics());

    auto *layout          = new QGridLayout;
    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 7;
    layout->addWidget(title, idx++, n, 1, MAXCOLS, Qt::AlignCenter);
    layout->addWidget(new QHline, idx++, n, 1, MAXCOLS);
    for (int i = 0; i < MAXCOLS; ++i)
        layout->setColumnStretch(i, 2.0);
    layout->setColumnStretch(MAXCOLS - 1, 1.0);

    auto *axesbutton = new QCheckBox("Axes ", this);
    axesbutton->setCheckState(showaxes ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(axesbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Location:"), idx, n++, 1, 1);
    auto *llbutton = new QRadioButton("Lower Left", this);
    llbutton->setChecked(axesloc == "yes");
    layout->addWidget(llbutton, idx, n++, 1, 1);
    auto *lrbutton = new QRadioButton("Lower Right", this);
    lrbutton->setChecked(axesloc == "lowerright");
    layout->addWidget(lrbutton, idx, n++, 1, 1);
    auto *ulbutton = new QRadioButton("Upper Left", this);
    ulbutton->setChecked(axesloc == "upperleft");
    layout->addWidget(ulbutton, idx, n++, 1, 1);
    auto *urbutton = new QRadioButton("Upper Right", this);
    urbutton->setChecked(axesloc == "upperright");
    layout->addWidget(urbutton, idx, n++, 1, 1);
    auto *cbutton = new QRadioButton("Center", this);
    cbutton->setChecked(axesloc == "center");
    layout->addWidget(cbutton, idx++, n++, 1, 1);

    n = 1;

    layout->addWidget(new QLabel("Length:"), idx, n++, 1, 1);
    auto *alval = new QLineEdit(QString::number(axeslen));
    alval->setValidator(new QDoubleValidator(0.000001, 10.0, 100, this));
    layout->addWidget(alval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *adval = new QLineEdit(QString::number(axesdiam));
    adval->setValidator(new QDoubleValidator(0.000001, 1.0, 100, this));
    layout->addWidget(adval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Tansparency:"), idx, n++, 1, 1);
    auto *atval = new QLineEdit(QString::number(axestrans));
    atval->setValidator(transvalidator);
    layout->addWidget(atval, idx++, n++, 1, 1);
    // disable and uncheck unsupported fields for older LAMMPS versions
    if (lammps->version() < 20260211) {
        llbutton->setChecked(true);
        axesloc = "yes";
        lrbutton->setEnabled(false);
        lrbutton->setChecked(false);
        ulbutton->setEnabled(false);
        ulbutton->setChecked(false);
        urbutton->setEnabled(false);
        urbutton->setChecked(false);
        cbutton->setEnabled(false);
        cbutton->setChecked(false);
        atval->setEnabled(false);
        atval->setText("1.0");
    }

    n = 0;

    auto *boxbutton = new QCheckBox("Box ", this);
    boxbutton->setCheckState(showbox ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(boxbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Color:"), idx, n++, 1, 1);
    auto *bcolor = new QLineEdit(boxcolor);
    bcolor->setCompleter(colorcompleter);
    bcolor->setValidator(colorvalidator);
    layout->addWidget(bcolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *bdiam = new QLineEdit(QString::number(boxdiam));
    bdiam->setValidator(new QDoubleValidator(0.000001, 1.0, 100, this));
    layout->addWidget(bdiam, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Tansparency:"), idx, n++, 1, 1);
    auto *btrans = new QLineEdit(QString::number(boxtrans));
    btrans->setValidator(transvalidator);
    layout->addWidget(btrans, idx++, n++, 1, 1);
    if (lammps->version() < 20260211) {
        btrans->setEnabled(false);
    }

    n = 0;

    auto *subboxbutton = new QCheckBox("Subbox ", this);
    subboxbutton->setCheckState(showsubbox ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(subboxbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *subdiam = new QLineEdit(QString::number(subboxdiam));
    subdiam->setValidator(new QDoubleValidator(0.000001, 1.0, 100, this));
    layout->addWidget(subdiam, idx++, n++, 1, 1);

    n = 0;

    layout->addWidget(new QLabel("Background:"), idx, n++, 1, 1);
    layout->addWidget(new QLabel("Bottomcolor:"), idx, n++, 1, 1);
    auto *bgcolor = new QLineEdit(backcolor);
    bgcolor->setCompleter(colorcompleter);
    bgcolor->setValidator(colorvalidator);
    layout->addWidget(bgcolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Topcolor:"), idx, n++, 1, 1);
    auto *b2color = new QLineEdit(backcolor2);
    b2color->setCompleter(colorcompleter);
    b2color->setValidator(colorvalidator);
    layout->addWidget(b2color, idx++, n++, 1, 1);
    if (lammps->version() < 20260211) {
        b2color->setEnabled(false);
        b2color->setText(backcolor);
    }

    n = 0;
    layout->addWidget(new QLabel("Quality:"), idx, n++, 1, 1);
    auto *fsaa = new QCheckBox("FSAA ", this);
    fsaa->setCheckState(antialias ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(fsaa, idx, n++, 1, 1);
    auto *ssao = new QCheckBox("SSAO ", this);
    ssao->setCheckState(usessao ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(ssao, idx, n++, 1, 1);
    layout->addWidget(new QLabel("SSAO strength:"), idx, n++, 1, 1);
    auto *aoval = new QLineEdit(QString::number(ssaoval));
    aoval->setValidator(new QDoubleValidator(0.0, 1.0, 100, this));
    layout->addWidget(aoval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Shiny:"), idx, n++, 1, 1);
    auto *shiny = new QLineEdit(QString::number(shinyfactor));
    shiny->setValidator(new QDoubleValidator(0.0, 1.0, 100, this));
    layout->addWidget(shiny, idx++, n++, 1, 1);

    n = 0;
    layout->addWidget(new QLabel("Center:"), idx, n++, 1, 1);
    layout->addWidget(new QLabel("X-direction:"), idx, n++, 1, 1);
    auto *xval = new QLineEdit(QString::number(xcenter));
    xval->setValidator(new QDoubleValidator(0.0, 1.0, 100, this));
    layout->addWidget(xval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Y-direction:"), idx, n++, 1, 1);
    auto *yval = new QLineEdit(QString::number(ycenter));
    yval->setValidator(new QDoubleValidator(0.0, 1.0, 100, this));
    layout->addWidget(yval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Z-direction:"), idx, n++, 1, 1);
    auto *zval = new QLineEdit(QString::number(zcenter));
    zval->setValidator(new QDoubleValidator(0.0, 1.0, 100, this));
    layout->addWidget(zval, idx++, n++, 1, 1);

    n = 0;
    layout->addWidget(new QHline, idx++, n, 1, MAXCOLS);
    auto *cancel = new QPushButton("&Cancel");
    auto *apply  = new QPushButton("&Apply");
    cancel->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    layout->addWidget(cancel, idx, 0, 1, MAXCOLS / 2, Qt::AlignHCenter);
    layout->addWidget(apply, idx, MAXCOLS / 2, 1, MAXCOLS / 2, Qt::AlignHCenter);
    connect(cancel, &QPushButton::released, &setview, &QDialog::reject);
    connect(apply, &QPushButton::released, &setview, &QDialog::accept);
    setview.setLayout(layout);

    int rv = setview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve and apply data
    showaxes = axesbutton->isChecked();
    if (llbutton->isChecked()) {
        axesloc = "yes";
    } else if (lrbutton->isChecked()) {
        axesloc = "lowerright";
    } else if (ulbutton->isChecked()) {
        axesloc = "upperleft";
    } else if (urbutton->isChecked()) {
        axesloc = "upperright";
    } else if (cbutton->isChecked()) {
        axesloc = "center";
    }
    auto *button = findChild<QPushButton *>("axes");
    if (button) button->setChecked(showaxes);

    if (alval->hasAcceptableInput()) axeslen = alval->text().toDouble();
    if (adval->hasAcceptableInput()) axesdiam = adval->text().toDouble();
    if (atval->hasAcceptableInput()) axestrans = atval->text().toDouble();

    showbox = boxbutton->isChecked();
    button  = findChild<QPushButton *>("box");
    if (button) button->setChecked(showbox);
    if (bdiam->hasAcceptableInput()) boxdiam = bdiam->text().toDouble();
    if (btrans->hasAcceptableInput()) boxtrans = btrans->text().toDouble();
    if (bcolor->hasAcceptableInput()) boxcolor = bcolor->text();
    showsubbox = subboxbutton->isChecked();
    if (subdiam->hasAcceptableInput()) subboxdiam = subdiam->text().toDouble();
    if (bgcolor->hasAcceptableInput()) backcolor = bgcolor->text();
    if (b2color->hasAcceptableInput()) backcolor2 = b2color->text();

    antialias = fsaa->isChecked();
    button    = findChild<QPushButton *>("antialias");
    if (button) button->setChecked(antialias);
    usessao = ssao->isChecked();
    button  = findChild<QPushButton *>("ssao");
    if (button) button->setChecked(usessao);
    if (aoval->hasAcceptableInput()) ssaoval = aoval->text().toDouble();
    if (shiny->hasAcceptableInput()) shinyfactor = shiny->text().toDouble();

    if (xval->hasAcceptableInput()) xcenter = xval->text().toDouble();
    if (yval->hasAcceptableInput()) ycenter = yval->text().toDouble();
    if (zval->hasAcceptableInput()) zcenter = zval->text().toDouble();

    // update image with new settings
    createImage();
}

void ImageViewer::atom_settings()
{
    QDialog setview;
    setview.setWindowTitle(QString("LAMMPS-GUI - Atom and Bond settings for images"));
    setview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setview.setMinimumSize(100, 100);
    setview.setContentsMargins(5, 5, 5, 5);
    setview.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    auto *title = new QLabel("Atom and Bond settings for images:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);
    title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    auto *colorcompleter = new QColorCompleter;
    auto *colorvalidator = new QColorValidator;
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 5);
    QFontMetrics metrics(setview.fontMetrics());

    auto *layout          = new QGridLayout;
    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 7;
    layout->addWidget(title, idx++, n, 1, MAXCOLS, Qt::AlignCenter);
    layout->addWidget(new QHline, idx++, n, 1, MAXCOLS);
    for (int i = 0; i < MAXCOLS; ++i)
        layout->setColumnStretch(i, 2.0);

    n = 0;

    auto *bodybutton = new QCheckBox("Bodies ", this);
    bodybutton->setCheckState(showbodies ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(bodybutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *bdiam = new QLineEdit(QString::number(bodydiam));
    bdiam->setValidator(new QDoubleValidator(0.1, 10.0, 100, this));
    layout->addWidget(bdiam, idx, n++, 1, 1);
    auto *bstyle = new QLabel("Style:");
    bstyle->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(bstyle, idx, n++, 1, 1);
    auto *bgroup   = new QButtonGroup(this);
    auto *bcbutton = new QRadioButton("Cylinders", this);
    bcbutton->setChecked(bodyflag == 2);
    bgroup->addButton(bcbutton);
    layout->addWidget(bcbutton, idx, n++, 1, 1);
    auto *btbutton = new QRadioButton("Triangles", this);
    btbutton->setChecked(bodyflag == 1);
    bgroup->addButton(btbutton);
    layout->addWidget(btbutton, idx, n++, 1, 1);
    auto *bbbutton = new QRadioButton("Both", this);
    bbbutton->setChecked(bodyflag == 3);
    bgroup->addButton(bbbutton);
    layout->addWidget(bbbutton, idx++, n++, 1, 1);
    if (lammps->extract_setting("body_flag") != 1) {
        bodybutton->setEnabled(false);
        bdiam->setEnabled(false);
        bcbutton->setEnabled(false);
        btbutton->setEnabled(false);
        bbbutton->setEnabled(false);
    }

    n = 0;

    auto *linebutton = new QCheckBox("Lines ", this);
    linebutton->setCheckState(showlines ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(linebutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *ldiam = new QLineEdit(QString::number(linediam));
    ldiam->setValidator(new QDoubleValidator(0.1, 10.0, 100, this));
    layout->addWidget(ldiam, idx++, n++, 1, 1);
    if (lammps->extract_setting("line_flag") != 1) {
        linebutton->setEnabled(false);
        ldiam->setEnabled(false);
    }

    n = 0;

    auto *tributton = new QCheckBox("Triangles ", this);
    tributton->setCheckState(showtris ? Qt::Checked : Qt::Unchecked);
    layout->addWidget(tributton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter:"), idx, n++, 1, 1);
    auto *tdiam = new QLineEdit(QString::number(tridiam));
    tdiam->setValidator(new QDoubleValidator(0.1, 10.0, 100, this));
    layout->addWidget(tdiam, idx, n++, 1, 1);
    auto *tstyle = new QLabel("Style:");
    tstyle->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(tstyle, idx, n++, 1, 1);
    auto *tgroup   = new QButtonGroup(this);
    auto *tcbutton = new QRadioButton("Cylinders", this);
    tcbutton->setChecked(triflag == 1);
    tgroup->addButton(tcbutton);
    layout->addWidget(tcbutton, idx, n++, 1, 1);
    auto *ttbutton = new QRadioButton("Triangles", this);
    ttbutton->setChecked(triflag == 2);
    tgroup->addButton(ttbutton);
    layout->addWidget(ttbutton, idx, n++, 1, 1);
    auto *tbbutton = new QRadioButton("Both", this);
    tbbutton->setChecked(triflag == 3);
    tgroup->addButton(tbbutton);
    layout->addWidget(tbbutton, idx++, n++, 1, 1);
    if (lammps->extract_setting("tri_flag") != 1) {
        tributton->setEnabled(false);
        tdiam->setEnabled(false);
        tcbutton->setEnabled(false);
        ttbutton->setEnabled(false);
        tbbutton->setEnabled(false);
    }

    n = 0;
    layout->addWidget(new QHline, idx++, n, 1, MAXCOLS);
    auto *cancel = new QPushButton("&Cancel");
    auto *apply  = new QPushButton("&Apply");
    cancel->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    layout->addWidget(cancel, idx, 0, 1, MAXCOLS / 2, Qt::AlignHCenter);
    layout->addWidget(apply, idx, MAXCOLS / 2, 1, MAXCOLS / 2, Qt::AlignHCenter);
    connect(cancel, &QPushButton::released, &setview, &QDialog::reject);
    connect(apply, &QPushButton::released, &setview, &QDialog::accept);
    setview.setLayout(layout);

    int rv = setview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve and apply data
    showbodies = bodybutton->isChecked();
    if (bdiam->hasAcceptableInput()) bodydiam = bdiam->text().toDouble();
    if (bcbutton->isChecked()) {
        bodyflag = 2;
    } else if (btbutton->isChecked()) {
        bodyflag = 1;
    } else if (bbbutton->isChecked()) {
        bodyflag = 3;
    }

    showlines = linebutton->isChecked();
    if (ldiam->hasAcceptableInput()) linediam = ldiam->text().toDouble();

    showtris = tributton->isChecked();
    if (tdiam->hasAcceptableInput()) tridiam = tdiam->text().toDouble();
    if (tcbutton->isChecked()) {
        triflag = 1;
    } else if (ttbutton->isChecked()) {
        triflag = 2;
    } else if (tbbutton->isChecked()) {
        triflag = 3;
    }

    // update image with new settings
    createImage();
}

void ImageViewer::fix_settings()
{
    update_fixes();
    if ((computes.size() + fixes.size()) == 0) return;
    QDialog fixview;
    fixview.setWindowTitle(QString("LAMMPS-GUI - Visualize Compute and Fix Graphics Objects"));
    fixview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    fixview.setMinimumSize(100, 100);
    fixview.setContentsMargins(5, 5, 5, 5);
    fixview.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    auto *title = new QLabel("Visualize Compute and Fix Graphics Objects:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    auto *colorcompleter = new QColorCompleter;
    auto *colorvalidator = new QColorValidator;
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 5);
    QFontMetrics metrics(fixview.fontMetrics());

    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 8;
    auto *layout          = new QGridLayout;
    layout->addWidget(title, idx++, n, 1, MAXCOLS, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    for (int i = 0; i < MAXCOLS; ++i)
        layout->setColumnStretch(i, 2.0);

    int computes_offset = idx + 2;
    if (computes.size() > 0) {
        n = 0;
        layout->addWidget(new QLabel("Compute ID:"), idx, n++, 1, 1, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Compute style:"), idx, n++, 1, 1, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Show:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Color Style:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Color:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Opacity:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Flag #1:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Flag #2:"), idx++, n++, Qt::AlignHCenter);
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

        for (const auto &comp : computes) {
            n = 0;

            auto *label = new QLabel(comp.first.c_str());
            layout->addWidget(label, idx, n++);
            layout->addWidget(new QLabel(comp.second->style), idx, n++);
            auto *check = new QCheckBox("");
            check->setCheckState(comp.second->enabled ? Qt::Checked : Qt::Unchecked);
            layout->addWidget(check, idx, n++, Qt::AlignHCenter);
            auto *cstyle = new QComboBox;
            cstyle->setEditable(false);
            cstyle->addItem("type");
            cstyle->addItem("element");
            cstyle->addItem("const");
            cstyle->setCurrentIndex(comp.second->colorstyle);
            layout->addWidget(cstyle, idx, n++);
            auto *color = new QLineEdit(comp.second->color.c_str());
            color->setCompleter(colorcompleter);
            color->setValidator(colorvalidator);
            color->setFixedSize(metrics.averageCharWidth() * 12, metrics.height() + 4);
            color->setText(comp.second->color.c_str());
            layout->addWidget(color, idx, n++);
            auto *trans = new QLineEdit(QString::number(comp.second->opacity));
            trans->setValidator(transvalidator);
            trans->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            trans->setText(QString::number(comp.second->opacity));
            layout->addWidget(trans, idx, n++);
            auto *flag1 = new QLineEdit(QString::number(comp.second->flag1));
            flag1->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            flag1->setText(QString::number(comp.second->flag1));
            layout->addWidget(flag1, idx, n++);
            auto *flag2 = new QLineEdit(QString::number(comp.second->flag2));
            flag2->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            flag2->setText(QString::number(comp.second->flag2));
            layout->addWidget(flag2, idx, n++);
            ++idx;
        }
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    }

    int fixes_offset = idx + 2;
    if (fixes.size() > 0) {
        n = 0;

        layout->addWidget(new QLabel("Fix ID:"), idx, n++, 1, 1, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Fix style:"), idx, n++, 1, 1, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Show:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Color Style:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Color:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Opacity:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Flag #1:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Flag #2:"), idx++, n++, Qt::AlignHCenter);
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

        for (const auto &fix : fixes) {
            n = 0;

            auto *label = new QLabel(fix.first.c_str());
            layout->addWidget(label, idx, n++);
            layout->addWidget(new QLabel(fix.second->style), idx, n++);
            auto *check = new QCheckBox("");
            check->setCheckState(fix.second->enabled ? Qt::Checked : Qt::Unchecked);
            layout->addWidget(check, idx, n++, Qt::AlignHCenter);
            auto *cstyle = new QComboBox;
            cstyle->setEditable(false);
            cstyle->addItem("type");
            cstyle->addItem("element");
            cstyle->addItem("const");
            cstyle->setCurrentIndex(fix.second->colorstyle);
            layout->addWidget(cstyle, idx, n++);
            auto *color = new QLineEdit(fix.second->color.c_str());
            color->setCompleter(colorcompleter);
            color->setValidator(colorvalidator);
            color->setFixedSize(metrics.averageCharWidth() * 12, metrics.height() + 4);
            color->setText(fix.second->color.c_str());
            layout->addWidget(color, idx, n++);
            auto *trans = new QLineEdit(QString::number(fix.second->opacity));
            trans->setValidator(transvalidator);
            trans->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            trans->setText(QString::number(fix.second->opacity));
            layout->addWidget(trans, idx, n++);
            auto *flag1 = new QLineEdit(QString::number(fix.second->flag1));
            flag1->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            flag1->setText(QString::number(fix.second->flag1));
            layout->addWidget(flag1, idx, n++);
            auto *flag2 = new QLineEdit(QString::number(fix.second->flag2));
            flag2->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
            flag2->setText(QString::number(fix.second->flag2));
            layout->addWidget(flag2, idx, n++);
            ++idx;
        }
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    }

    auto *cancel = new QPushButton("&Cancel");
    auto *apply  = new QPushButton("&Apply");
    cancel->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    layout->addWidget(cancel, idx, 0, 1, 4, Qt::AlignHCenter);
    layout->addWidget(apply, idx, 4, 1, 4, Qt::AlignHCenter);
    connect(cancel, &QPushButton::released, &fixview, &QDialog::reject);
    connect(apply, &QPushButton::released, &fixview, &QDialog::accept);
    fixview.setLayout(layout);

    int rv = fixview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve compute data from dialog and store in map
    for (int idx = computes_offset; idx < computes_offset + computes.size(); ++idx) {
        n          = 0;
        auto *item = layout->itemAtPosition(idx, n++);
        if (!item) continue;
        auto *label = qobject_cast<QLabel *>(item->widget());

        auto id = label->text().toStdString();
        // compute ID is not registered with a widget; skip rest to avoid segfault
        if (computes.count(id) == 0) continue;

        ++n; // nothing to do with label for style name
        item                     = layout->itemAtPosition(idx, n++);
        auto *box                = qobject_cast<QCheckBox *>(item->widget());
        computes[id]->enabled    = (box->checkState() == Qt::Checked);
        item                     = layout->itemAtPosition(idx, n++);
        auto *combo              = qobject_cast<QComboBox *>(item->widget());
        computes[id]->colorstyle = combo->currentIndex();
        item                     = layout->itemAtPosition(idx, n++);
        auto *line               = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) computes[id]->color = line->text().toStdString();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) computes[id]->opacity = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) computes[id]->flag1 = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) computes[id]->flag2 = line->text().toDouble();
    }

    // retrieve fix data from dialog and store in map
    for (int idx = fixes_offset; idx < fixes_offset + fixes.size(); ++idx) {
        n          = 0;
        auto *item = layout->itemAtPosition(idx, n++);
        if (!item) continue;
        auto *label = qobject_cast<QLabel *>(item->widget());
        auto id     = label->text().toStdString();
        // fix ID is not registered with a widget; skip rest to avoid segfault
        if (fixes.count(id) == 0) continue;
        ++n; // skip over label for style name

        item                  = layout->itemAtPosition(idx, n++);
        auto *box             = qobject_cast<QCheckBox *>(item->widget());
        fixes[id]->enabled    = (box->checkState() == Qt::Checked);
        item                  = layout->itemAtPosition(idx, n++);
        auto *combo           = qobject_cast<QComboBox *>(item->widget());
        fixes[id]->colorstyle = combo->currentIndex();
        item                  = layout->itemAtPosition(idx, n++);
        auto *line            = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) fixes[id]->color = line->text().toStdString();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) fixes[id]->opacity = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) fixes[id]->flag1 = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) fixes[id]->flag2 = line->text().toDouble();
    }
    createImage();
}

void ImageViewer::region_settings()
{
    update_regions();
    if (regions.size() == 0) return;
    QDialog regionview;
    regionview.setWindowTitle(QString("LAMMPS-GUI - Visualize Regions"));
    regionview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    regionview.setMinimumSize(100, 100);
    regionview.setContentsMargins(5, 5, 5, 5);
    regionview.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    int idx     = 0;
    int n       = 0;
    auto *title = new QLabel("Visualize Regions:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    constexpr int MAXCOLS = 7;
    auto *layout          = new QGridLayout;
    layout->addWidget(title, idx++, n, 1, MAXCOLS, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, n, 1, MAXCOLS);

    layout->addWidget(new QLabel("Region ID:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Show:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Style:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Color:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Size:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("# Points:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Opacity:"), idx++, n++, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *colorcompleter = new QColorCompleter;
    auto *colorvalidator = new QColorValidator;
    auto *framevalidator = new QDoubleValidator(1.0e-10, 1.0e10, 10);
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 5);
    auto *pointvalidator = new QIntValidator(100, 1000000);
    QFontMetrics metrics(regionview.fontMetrics());

    for (const auto &reg : regions) {
        n = 0;
        layout->addWidget(new QLabel(reg.first.c_str()), idx, n++);
        layout->setObjectName(QString(reg.first.c_str()));

        auto *check = new QCheckBox("");
        check->setCheckState(reg.second->enabled ? Qt::Checked : Qt::Unchecked);
        layout->addWidget(check, idx, n++, Qt::AlignHCenter);
        auto *style = new QComboBox;
        style->setEditable(false);
        style->addItem("frame");
        style->addItem("filled");
        style->addItem("transparent");
        style->addItem("points");
        style->setCurrentIndex(reg.second->style);
        layout->addWidget(style, idx, n++);
        auto *color = new QLineEdit(reg.second->color.c_str());
        color->setCompleter(colorcompleter);
        color->setValidator(colorvalidator);
        color->setFixedSize(metrics.averageCharWidth() * 12, metrics.height() + 4);
        color->setText(reg.second->color.c_str());
        layout->addWidget(color, idx, n++);
        auto *frame = new QLineEdit(QString::number(reg.second->diameter));
        frame->setValidator(framevalidator);
        frame->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        frame->setText(QString::number(reg.second->diameter));
        layout->addWidget(frame, idx, n++);
        auto *points = new QLineEdit(QString::number(reg.second->npoints));
        points->setValidator(pointvalidator);
        points->setFixedSize(metrics.averageCharWidth() * 10, metrics.height() + 4);
        points->setText(QString::number(reg.second->npoints));
        layout->addWidget(points, idx, n++);
        auto *trans = new QLineEdit(QString::number(reg.second->opacity));
        trans->setValidator(transvalidator);
        trans->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        trans->setText(QString::number(reg.second->opacity));
        layout->addWidget(trans, idx, n++);
        ++idx;
    }
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *cancel = new QPushButton("&Cancel");
    auto *apply  = new QPushButton("&Apply");
    cancel->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    layout->addWidget(cancel, idx, 0, 1, 3, Qt::AlignHCenter);
    layout->addWidget(apply, idx, 3, 1, 3, Qt::AlignHCenter);
    connect(cancel, &QPushButton::released, &regionview, &QDialog::reject);
    connect(apply, &QPushButton::released, &regionview, &QDialog::accept);
    regionview.setLayout(layout);

    int rv = regionview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve data from dialog and store in map
    for (int idx = 4; idx < (int)regions.size() + 4; ++idx) {
        n                    = 0;
        auto *item           = layout->itemAtPosition(idx, n++);
        auto *label          = qobject_cast<QLabel *>(item->widget());
        auto id              = label->text().toStdString();
        item                 = layout->itemAtPosition(idx, n++);
        auto *box            = qobject_cast<QCheckBox *>(item->widget());
        regions[id]->enabled = (box->checkState() == Qt::Checked);
        item                 = layout->itemAtPosition(idx, n++);
        auto *combo          = qobject_cast<QComboBox *>(item->widget());
        regions[id]->style   = combo->currentIndex();
        item                 = layout->itemAtPosition(idx, n++);
        auto *line           = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) regions[id]->color = line->text().toStdString();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) regions[id]->diameter = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) regions[id]->npoints = line->text().toInt();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) regions[id]->opacity = line->text().toDouble();
    }
    createImage();
}

void ImageViewer::change_group(int)
{
    auto *box = findChild<QComboBox *>("group");
    group     = box ? box->currentText() : "all";

    // reset molecule to "none" when changing group
    box = findChild<QComboBox *>("molecule");
    if (box && (box->currentIndex() > 0)) {
        box->setCurrentIndex(0); // triggers call to createImage()
    } else {
        createImage();
    }
}

void ImageViewer::change_molecule(int)
{
    auto *box = findChild<QComboBox *>("molecule");
    molecule  = box ? box->currentText() : "none";

    box = findChild<QComboBox *>("group");
    if (molecule == "none") {
        box->setEnabled(true);
    } else {
        box->setEnabled(false);
    }

    createImage();
}

// intercept events
bool ImageViewer::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *kev = static_cast<QKeyEvent *>(event);
        if ((kev->key() == Qt::Key_G) && (kev->modifiers() == Qt::AltModifier)) {
            auto *box = findChild<QComboBox *>("molecule");
            if (box) box->hidePopup();

            box = findChild<QComboBox *>("group");
            if (box) {
                box->setFocus();
                box->showPopup();
                return true;
            }
        } else if ((kev->key() == Qt::Key_M) && (kev->modifiers() == Qt::AltModifier)) {
            auto *box = findChild<QComboBox *>("group");
            if (box) box->hidePopup();

            box = findChild<QComboBox *>("molecule");
            if (box) {
                box->setFocus();
                box->showPopup();
                return true;
            }
        } else if ((kev->key() == Qt::Key_W) && (kev->modifiers() == Qt::AltModifier)) {
            auto *combo = findChild<QComboBox *>("molecule");
            if (combo) combo->hidePopup();
            combo = findChild<QComboBox *>("group");
            if (combo) combo->hidePopup();

            auto *box = findChild<QSpinBox *>("xsize");
            if (box) {
                box->setFocus();
                box->selectAll();
                return true;
            }
        } else if ((kev->key() == Qt::Key_H) && (kev->modifiers() == Qt::AltModifier)) {
            auto *combo = findChild<QComboBox *>("molecule");
            if (combo) combo->hidePopup();
            combo = findChild<QComboBox *>("group");
            if (combo) combo->hidePopup();

            auto *box = findChild<QSpinBox *>("ysize");
            if (box) {
                box->setFocus();
                box->selectAll();
                return true;
            }
        } else if (kev->modifiers() == Qt::AltModifier) {
            auto *combo = findChild<QComboBox *>("molecule");
            if (combo) combo->hidePopup();
            combo = findChild<QComboBox *>("group");
            if (combo) combo->hidePopup();
            setFocus();
        }
    }
    return QDialog::eventFilter(watched, event);
}

// This function creates a visualization of the current system using the
// "dump image" command and reads and displays the renderd image.
// To visualize molecules we create new atoms with create_atoms and
// put them into a new, temporary group and then visualize that group.
// After rendering the image, the atoms and group are deleted.
// to update bond data, we also need to issue a "run 0" command.

void ImageViewer::createImage()
{
    auto *renderstatus = findChild<QLabel *>("renderstatus");
    if (renderstatus) renderstatus->setEnabled(true);
    repaint();

    QString oldgroup = group;

    if (molecule != "none") {

        // get center of box
        double *boxlo, *boxhi, xmid, ymid, zmid;
        boxlo = (double *)lammps->extract_global("boxlo");
        boxhi = (double *)lammps->extract_global("boxhi");
        if (boxlo && boxhi) {
            xmid = 0.5 * (boxhi[0] + boxlo[0]);
            ymid = 0.5 * (boxhi[1] + boxlo[1]);
            zmid = 0.5 * (boxhi[2] + boxlo[2]);
        } else {
            xmid = ymid = zmid = 0.0;
        }

        QString molcreate = "create_atoms 0 single %1 %2 %3 mol %4 312944 group %5 units box";
        group             = "imgviewer_tmp_mol";
        lammps->command(molcreate.arg(xmid).arg(ymid).arg(zmid).arg(molecule).arg(group));
        lammps->command(QString("neigh_modify exclude group all %1").arg(group));
        lammps->command("run 0 post no");
    }

    QSettings settings;
    // attempt to clean up if a previous write_dump command failed
    lammps->command("if $(is_defined(dump,WRITE_DUMP)) then 'undump WRITE_DUMP'");
    QString dumpcmd = QString("write_dump ") + group + " image ";
    QDir dumpdir(QDir::tempPath());
    QFile dumpfile(dumpdir.absoluteFilePath(filename + ".ppm"));
    dumpcmd += "'" + dumpfile.fileName() + "'";

    settings.beginGroup("snapshot");
    int hhrot = (hrot > 180) ? 360 - hrot : hrot;

    // determine elements from masses and set their covalent radii
    int ntypes       = lammps->extract_setting("ntypes");
    int nbondtypes   = lammps->extract_setting("nbondtypes");
    auto *masses     = (double *)lammps->extract_atom("mass");
    QString units    = (const char *)lammps->extract_global("units");
    QString elements = "element ";
    QString adiams;
    useelements = false;
    if (masses && ((units == "real") || (units == "metal"))) {
        useelements = true;
        for (int i = 1; i <= ntypes; ++i) {
            int idx = get_pte_from_mass(masses[i]);
            if (idx == 0) useelements = false;
            elements += QString(pte_label[idx]) + blank;
            adiams += QString("adiam %1 %2 ").arg(i).arg(vdwfactor * pte_vdw_radius[idx]);
        }
    }
    usediameter = lammps->extract_setting("radius_flag") != 0;
    // use Lennard-Jones sigma for radius, if available
    usesigma               = false;
    const char *pair_style = (const char *)lammps->extract_global("pair_style");
    if (!useelements && !usediameter && pair_style && (strncmp(pair_style, "lj/", 3) == 0)) {
        auto **sigma = (double **)lammps->extract_pair("sigma");
        if (sigma) {
            usesigma = true;
            for (int i = 1; i <= ntypes; ++i) {
                if (sigma[i][i] > 0.0)
                    adiams += QString("adiam %1 %2 ").arg(i).arg(vdwfactor * sigma[i][i]);
            }
        }
    }
    // adjust pushbutton state and clear adiams string to disable VDW display, if needed
    if (useelements || usediameter || usesigma) {
        auto *button = findChild<QPushButton *>("vdw");
        if (button) button->setEnabled(true);
        auto *edit = findChild<QLineEdit *>("atomSize");
        if (edit) {
            edit->setEnabled(false);
            edit->hide();
        }
        auto *label = findChild<QLabel *>("AtomLabel");
        if (label) {
            label->setEnabled(false);
            label->hide();
        }

    } else {
        adiams.clear();
        auto *button = findChild<QPushButton *>("vdw");
        if (button) button->setEnabled(false);

        auto *label = findChild<QLabel *>("AtomLabel");
        if (label) {
            label->setEnabled(true);
            label->show();
        }
        auto *edit = findChild<QLineEdit *>("atomSize");
        if (edit) {
            if (!edit->isEnabled()) {
                edit->setEnabled(true);
                edit->show();
                // initialize with lattice spacing
                const auto *xlattice = (const double *)lammps->extract_global("xlattice");
                if (xlattice) atomSize = *xlattice;
                edit->setText(QString::number(atomSize));
            }
            atomSize = edit->text().toDouble();
        }
        if (atomSize != 1.0) {
            for (int i = 1; i <= ntypes; ++i)
                adiams += QString("adiam %1 %2 ").arg(i).arg(atomSize);
        }
    }

    // color
    if (useelements)
        dumpcmd += blank + "element";
    else
        dumpcmd += blank + settings.value("color", "type").toString();

    bool do_vdw = vdwfactor > VDW_CUT;
    // diameter
    if (usediameter && do_vdw)
        dumpcmd += blank + "diameter";
    else
        dumpcmd += blank + settings.value("diameter", "type").toString();

    if (showbodies && (lammps->extract_setting("body_flag") == 1))
        dumpcmd += QString(" body type %1 %2").arg(bodydiam).arg(bodyflag);
    else if (showlines && (lammps->extract_setting("line_flag") == 1))
        dumpcmd += QString(" line type %1").arg(linediam);
    else if (showtris && (lammps->extract_setting("tri_flag") == 1))
        dumpcmd += QString(" tri type %1 %2").arg(triflag).arg(tridiam);
    else if ((lammps->extract_setting("ellipsoid_flag") == 1) && (lammps->version() > 20260210)) {
        // available since 11 February 2026 release
        dumpcmd += QString(" ellipsoid type 1 3 0.2");
    }
    dumpcmd += QString(" size %1 %2").arg(xsize).arg(ysize);
    dumpcmd += QString(" zoom %1").arg(zoom);
    dumpcmd += QString(" shiny %1 ").arg(shinyfactor);
    dumpcmd += QString(" fsaa %1").arg(antialias ? "yes" : "no");
    if (nbondtypes > 0) {
        if (do_vdw)
            dumpcmd += " bond none none ";
        else
            dumpcmd += " bond atom 0.5 ";
    }
    if (lammps->extract_setting("dimension") == 3) {
        dumpcmd += QString(" view %1 %2").arg(hhrot).arg(vrot);
    }
    if (usessao) dumpcmd += QString(" ssao yes 453983 %1").arg(ssaoval);
    if (showbox)
        dumpcmd += QString(" box yes %1").arg(boxdiam);
    else
        dumpcmd += " box no 0.0";
    if (showsubbox)
        dumpcmd += QString(" subbox yes %1").arg(subboxdiam);
    else
        dumpcmd += " subbox no 0.0";

    if (showaxes)
        dumpcmd += QString(" axes %1 %2 %3").arg(axesloc).arg(axeslen).arg(axesdiam);
    else
        dumpcmd += " axes no 0.0 0.0";

    if (autobond && pair_style && (strcmp(pair_style, "none") != 0))
        dumpcmd += blank + "autobond" + blank + QString::number(bondcutoff) + " 0.5";

    if (regions.size() > 0) {
        for (const auto &reg : regions) {
            if (reg.second->enabled) {
                QString id(reg.first.c_str());
                QString color(reg.second->color.c_str());
                switch (reg.second->style) {
                    case FRAME:
                        dumpcmd += " region " + id + blank + color;
                        dumpcmd += " frame " + QString::number(reg.second->diameter);
                        break;
                    case FILLED:
                        dumpcmd += " region " + id + blank + color + " filled";
                        break;
                    case TRANSPARENT:
                        dumpcmd += " region " + id + blank + color;
                        dumpcmd += " transparent " + QString::number(reg.second->opacity);
                        break;
                    case POINTS:
                    default:
                        dumpcmd += " region " + id + blank + color;
                        dumpcmd += " points " + QString::number(reg.second->npoints);
                        dumpcmd += blank + QString::number(reg.second->diameter);
                        break;
                }
                dumpcmd += blank;
            }
        }
    }

    bool dofixes = false;
    if (computes.size() > 0) {
        for (const auto &comp : computes) {
            if (comp.second->enabled) {
                dofixes = true;
                QString id(comp.first.c_str());
                switch (comp.second->colorstyle) {
                    case TYPE:
                        dumpcmd += " compute " + id + blank + "type ";
                        break;
                    case ELEMENT:
                        dumpcmd += " compute " + id + blank + "element ";
                        break;
                    case CONSTANT: // FALLTHROUGH
                    default:
                        dumpcmd += " compute " + id + blank + "const ";
                        break;
                }
                dumpcmd += QString::number(comp.second->flag1) + blank +
                           QString::number(comp.second->flag2) + blank;
            }
        }
    }
    if (fixes.size() > 0) {
        for (const auto &fix : fixes) {
            if (fix.second->enabled) {
                dofixes = true;
                QString id(fix.first.c_str());
                switch (fix.second->colorstyle) {
                    case TYPE:
                        dumpcmd += " fix " + id + blank + "type ";
                        break;
                    case ELEMENT:
                        dumpcmd += " fix " + id + blank + "element ";
                        break;
                    case CONSTANT: // FALLTHROUGH
                    default:
                        dumpcmd += " fix " + id + blank + "const ";
                        break;
                }
                dumpcmd += QString::number(fix.second->flag1) + blank +
                           QString::number(fix.second->flag2) + blank;
            }
        }
    }

    dumpcmd += QString(" center s %1 %2 %3").arg(xcenter).arg(ycenter).arg(zcenter);
    if (!dofixes) dumpcmd += " noinit";
    dumpcmd += " modify boxcolor " + boxcolor;
    dumpcmd += " backcolor " + backcolor;
    if (lammps->version() > 20260210) {
        dumpcmd += " backcolor2 " + backcolor2;
        dumpcmd += QString(" axestrans %1").arg(axestrans);
        dumpcmd += QString(" boxtrans %1").arg(boxtrans);
    }

    if (useelements) dumpcmd += blank + elements + blank + adiams + blank;
    if (usesigma) dumpcmd += blank + adiams + blank;
    if (!useelements && !usesigma && (atomSize != 1.0)) dumpcmd += blank + adiams + blank;
    settings.endGroup();

    if (computes.size() > 0) {
        for (const auto &comp : computes) {
            if (comp.second->enabled) {
                QString id(comp.first.c_str());
                QString color(comp.second->color.c_str());
                dumpcmd += " ccolor " + id + blank + color;
                dumpcmd += " ctrans " + id + blank + QString::number(comp.second->opacity);
                dumpcmd += blank;
            }
        }
    }
    if (fixes.size() > 0) {
        for (const auto &fix : fixes) {
            if (fix.second->enabled) {
                QString id(fix.first.c_str());
                QString color(fix.second->color.c_str());
                dumpcmd += " fcolor " + id + blank + color;
                dumpcmd += " ftrans " + id + blank + QString::number(fix.second->opacity);
                dumpcmd += blank;
            }
        }
    }

    last_dump_cmd = dumpcmd;
    lammps->command(dumpcmd);

    QImageReader reader(dumpfile.fileName());
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    dumpfile.remove();

    // read of new image failed. nothing left to do.
    if (newImage.isNull()) return;

    // show show image
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->adjustSize();
    adjustWindowSize();
    if (renderstatus) renderstatus->setEnabled(false);
    repaint();

    if (molecule != "none") {
        lammps->command("neigh_modify exclude none");
        lammps->command(QString("delete_atoms group %1 compress no").arg(group));
        lammps->command(QString("group %1 delete").arg(group));
        group = oldgroup;
    }
}

void ImageViewer::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Save Image File As", QString(),
        "Image Files (*.png *.jpg *.jpeg *.gif *.bmp *.tga *.ppm *.tiff *.pgm *.xpm *.xbm)");
    saveFile(fileName);
}

void ImageViewer::copy() {}

void ImageViewer::quit()
{
    auto *main = dynamic_cast<LammpsGui *>(get_main_widget());
    if (main) main->quit();
}

void ImageViewer::saveFile(const QString &fileName)
{
    if (fileName.isEmpty()) return;

    // try direct save and if it fails write to PNG and then convert with ImageMagick if available
    if (!image.save(fileName)) {
        if (has_exe("magick") || has_exe("convert")) {
            QTemporaryFile tmpfile(QDir::tempPath() + "/LAMMPS_GUI.XXXXXX.png");
            // open and close to generate temporary file name
            (void)tmpfile.open();
            (void)tmpfile.close();
            if (!image.save(tmpfile.fileName())) {
                QMessageBox::warning(this, "Image Viewer Error",
                                     "Could not save image to file " + fileName);
                return;
            }

            QString cmd = "magick";
            QStringList args{tmpfile.fileName(), fileName};
            if (!has_exe("magick")) cmd = "convert";
            auto *convert = new QProcess(this);
            convert->start(cmd, args);
            bool finished = convert->waitForFinished(-1);
            if (!finished || convert->exitStatus() != QProcess::NormalExit ||
                convert->exitCode() != 0) {
                QString errorOutput = QString::fromLocal8Bit(convert->readAllStandardError());
                QString message     = "ImageMagick failed to convert image to file " + fileName;
                if (!errorOutput.trimmed().isEmpty()) {
                    message += "\n\n" + errorOutput.trimmed();
                }
                QMessageBox::warning(this, "Image Viewer Error", message);
            }
            delete convert;
        } else {
            QMessageBox::warning(this, "Image Viewer Error",
                                 "Could not save image to file " + fileName);
        }
    }
}

void ImageViewer::createActions()
{
    QMenu *fileMenu = menuBar->addMenu("&File");

    saveAsAct = fileMenu->addAction("&Save As...", this, &ImageViewer::saveAs);
    saveAsAct->setIcon(QIcon(":/icons/document-save-as.png"));
    saveAsAct->setEnabled(false);
    saveAsAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    fileMenu->addSeparator();
    copyAct = fileMenu->addAction("&Copy Image", this, &ImageViewer::copy);
    copyAct->setIcon(QIcon(":/icons/edit-copy.png"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);
    cmdAct = fileMenu->addAction("Copy &dump image command", this, &ImageViewer::cmd_to_clipboard);
    cmdAct->setIcon(QIcon(":/icons/file-clipboard.png"));
    cmdAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    fileMenu->addSeparator();
    QAction *exitAct = fileMenu->addAction("&Close", this, &QWidget::close);
    exitAct->setIcon(QIcon(":/icons/window-close.png"));
    exitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_W));
    QAction *quitAct = fileMenu->addAction("&Quit", this, &ImageViewer::quit);
    quitAct->setIcon(QIcon(":/icons/application-exit.png"));
    quitAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q));
}

void ImageViewer::updateActions()
{
    saveAsAct->setEnabled(!image.isNull());
    copyAct->setEnabled(!image.isNull());
}

void ImageViewer::adjustWindowSize()
{
    if (image.isNull()) return;

    int desiredWidth  = image.width() + EXTRA_WIDTH;
    int desiredHeight = image.height() + EXTRA_HEIGHT;

    auto *screen = QGuiApplication::primaryScreen();
    if (screen) {
        auto screenSize = screen->availableSize();
        desiredWidth    = std::min(desiredWidth, screenSize.width() * 2 / 3);
        desiredHeight   = std::min(desiredHeight, screenSize.height() * 4 / 5);
    }
    resize(desiredWidth, desiredHeight);
}

void ImageViewer::update_fixes()
{
    if (!lammps) return;
    // we can query for fixes before 10 December 2025, but there is no support
    // for fix graphics until after that version. So the check is needed for this date.
    if (lammps->version() < 20251210) return;

    // remove any fixes that no longer exist. to avoid inconsistencies while looping
    // over the fixes, we first collect the list of missing ids and then apply it.
    std::unordered_set<std::string> oldkeys;
    for (const auto &istyle : computes) {
        if (!lammps->has_id("compute", istyle.first.c_str())) oldkeys.insert(istyle.first);
    }
    for (const auto &id : oldkeys) {
        delete computes[id];
        computes.erase(id);
    }
    oldkeys.clear();
    for (const auto &istyle : fixes) {
        if (!lammps->has_id("fix", istyle.first.c_str())) oldkeys.insert(istyle.first);
    }
    for (const auto &id : oldkeys) {
        delete fixes[id];
        fixes.erase(id);
    }

    // map compute and fix styles to their ids by parsing info command output.
    StdCapture capturer;
    capturer.BeginCapture();
    lammps->command("info computes fixes");
    capturer.EndCapture();
    QString styleinfo(capturer.GetCapture().c_str());
    QRegularExpression infoline(
        QStringLiteral("^(Compute|Fix)\\[.*\\]: *([^,]+), *style = ([^,]+).*"));
    QRegularExpression newline(QStringLiteral("[\r\n]+"));
    int i = 0;
    for (const auto &line : styleinfo.split(newline, Qt::SkipEmptyParts)) {
        auto match = infoline.match(line);
        if (match.hasMatch()) {
            auto id    = match.captured(2).toStdString();
            auto style = match.captured(3);
            if (match.captured(1) == "Compute") {
                if (image_computes.contains(style)) {
                    if (computes.count(id) == 0) {
                        const auto &color = defaultcolors[i % defaultcolors.size()].toStdString();
                        computes[id]      = new ImageInfo(false, style, TYPE, color, 1.0, 0.0, 0.0);
                        ++i;
                    } else {
                        computes[id]->style = style;
                    }
                }
            } else if (match.captured(1) == "Fix") {
                if (image_fixes.contains(style)) {
                    if (fixes.count(id) == 0) {
                        const auto &color = defaultcolors[i % defaultcolors.size()].toStdString();
                        fixes[id]         = new ImageInfo(false, style, TYPE, color, 1.0, 0.0, 0.0);
                        ++i;
                    } else {
                        fixes[id]->style = style;
                    }
                }
            }
        }
    }

    auto *button = findChild<QPushButton *>("image_styles");
    if (button) button->setEnabled((computes.size() + fixes.size()) > 0);
}

void ImageViewer::update_regions()
{
    if (!lammps) return;
    if (lammps->version() < 20250910) return;

    // remove any regions that no longer exist. to avoid inconsistencies while looping
    // over the regions, we first collect the list of missing ids and then apply it.
    std::unordered_set<std::string> oldkeys;
    for (const auto &reg : regions) {
        if (!lammps->has_id("region", reg.first.c_str())) oldkeys.insert(reg.first);
    }
    for (const auto &id : oldkeys) {
        delete regions[id];
        regions.erase(id);
    }

    // add any new regions
    char buffer[DEFAULT_BUFLEN];
    int nregions = lammps->id_count("region");
    for (int i = 0; i < nregions; ++i) {
        if (lammps->id_name("region", i, buffer, DEFAULT_BUFLEN)) {
            std::string id = buffer;
            if (regions.count(id) == 0) {
                const auto &color = defaultcolors[i % defaultcolors.size()].toStdString();
                auto *reginfo     = new RegionInfo(false, FRAME, color, DEFAULT_DIAMETER,
                                                   DEFAULT_OPACITY, DEFAULT_NPOINTS);
                regions[id]       = reginfo;
            }
        }
    }

    auto *button = findChild<QPushButton *>("regions");
    if (button) button->setEnabled(regions.size() > 0);
}

bool ImageViewer::has_autobonds()
{
    if (!lammps) return false;
    if (lammps->version() < 20250910) return false;
    const auto *pair_style = (const char *)lammps->extract_global("pair_style");
    if (!pair_style) return false;
    return strcmp(pair_style, "none") != 0;
}

// Local Variables:
// c-basic-offset: 4
// End:
