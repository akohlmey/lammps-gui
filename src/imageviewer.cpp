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
#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QDoubleValidator>
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
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QLinearGradient>
#include <QMenu>
#include <QMenuBar>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QProcess>
#include <QPushButton>
#include <QRadioButton>
#include <QRect>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
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

// helper functions:

// 1) find element in periodic table from their mass
int get_pte_from_mass(double mass)
{
    if (mass <= 0.0) return 0;
    int idx = 0;
    for (int i = 0; i < nr_pte_entries; ++i)
        if (fabs(mass - pte_mass[i]) < 0.65) idx = i;
    if ((mass > 0.0) && (mass < 2.2)) idx = 1;
    // discriminate between Cobalt and Nickel. The loop will detect Nickel
    if ((mass < 61.24) && (mass > 58.8133)) idx = 27;
    return idx;
}

constexpr int ICON_SIZE = 48;

// 2) create a color gradient icon
QIcon gradient_icon(const QList<QColor> &colors)
{
    if (colors.isEmpty()) return QIcon();

    // define pixmap and horizontal gradient
    QPixmap pixmap(ICON_SIZE, ICON_SIZE);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    QLinearGradient gradient(0, 0, ICON_SIZE, 0);

    // distribute colors across gradient
    for (int i = 0; i < colors.size(); ++i) {
        qreal pos = static_cast<qreal>(i) / qMax(1, colors.size() - 1);
        gradient.setColorAt(pos, colors[i]);
    }

    painter.fillRect(pixmap.rect(), gradient);
    painter.end();

    return QIcon(pixmap);
}

// 3) create a color sequence icon
QIcon sequence_icon(const QList<QColor> &colors)
{
    // if no colors or too many colors return empty icon
    if (colors.isEmpty() || (colors.size() * 2 > ICON_SIZE)) return QIcon();

    // define pixmap
    QPixmap pixmap(ICON_SIZE, ICON_SIZE);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    // distribute colors across icon in evenly sized chunks
    const int chunk = ICON_SIZE / colors.size();
    for (int i = 0; i < colors.size(); ++i)
        painter.fillRect(QRect(i * chunk, 0, chunk, ICON_SIZE), colors[i]);

    painter.end();

    return QIcon(pixmap);
}

// 4) create a single color icon
QPixmap color_icon(const QColor &color)
{
    // define pixmap and fill with color
    QPixmap pixmap(ICON_SIZE / 2, ICON_SIZE / 2);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.fillRect(pixmap.rect(), color);
    painter.end();
    return pixmap;
}

// read JSON color and light data from file
QJsonObject loadJsonColors(QWidget *parent)
{
    QJsonObject obj;
    QString fileName = QFileDialog::getOpenFileName(parent, "Load Colors from JSON", "",
                                                    "JSON files (*.json);;All files (*)");
    if (fileName.isEmpty()) return obj;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        warning(parent, "Load Colors", "Could not open file '" + fileName + "' for reading.");
        return obj;
    }

    QJsonParseError err;
    auto doc = QJsonDocument::fromJson(file.readAll(), &err);
    if (doc.isNull() || !doc.isObject()) {
        warning(parent, "Load Colors",
                "Invalid JSON colors file '" + fileName + "': " + err.errorString());
        return obj;
    }
    obj      = doc.object();
    auto app = obj.value("application").toString();
    auto key = obj.value("format").toString();
    auto rev = obj.value("revision").toInt();
    if ((app != "LAMMPS") || (key != "colors")) {
        warning(parent, "Load Colors",
                "JSON colors file '" + fileName + "' is not a LAMMPS colors file.");
        return obj;
    }
    if (rev != 1) {
        warning(parent, "Load Colors",
                QString("JSON colors file '%1' has incompatible revision %2 instead of 1")
                    .arg(fileName)
                    .arg(rev));
        return obj;
    }

    auto arr = obj.value("colors").toArray();
    if (arr.isEmpty()) {
        warning(parent, "Load Colors",
                "JSON colors file '" + fileName + "' contains no colors entry");
        return obj;
    }

    arr = obj.value("lights").toArray();
    if (arr.isEmpty()) {
        warning(parent, "Load Colors",
                "JSON colors file '" + fileName + "' contains no lights entry");
        return obj;
    }
    return obj;
}

// save JSON color and light data to file
void saveJsonColors(QWidget *parent, const QJsonArray &colors, const QJsonObject &lights)
{
    QJsonObject root;
    root["application"] = QStringLiteral("LAMMPS");
    root["format"]      = QStringLiteral("colors");
    root["revision"]    = 1;
    root["schema"]      = QStringLiteral("https://download.lammps.org/json/color-schema.json");
    root["colors"]      = colors;
    root["lights"]      = lights;

    QString fileName = QFileDialog::getSaveFileName(parent, "Save Colors to JSON", "",
                                                    "JSON files (*.json);;All files (*)");
    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        warning(parent, "Save Colors", "Could not open file '" + fileName + "' for writing.");
        return;
    }
    file.write(QJsonDocument(root).toJson());
}

QStringList defaultcolors = {"red",       "green",    "blue",       "yellow",   "cyan",
                             "magenta",   "orange",   "chartreuse", "brown",    "darkred",
                             "darkgreen", "darkblue", "darkyellow", "darkcyan", "darkmagenta",
                             "silver",    "gray"};

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
constexpr int TITLE_MARGIN        = 10;
constexpr int CONTENT_MARGIN      = 5;
constexpr int LAYOUT_SPACING      = 6;
constexpr int MINIMUM_WIDTH       = 400;
constexpr int MINIMUM_HEIGHT      = 300;
constexpr int EXTRA_WIDTH         = 150;
constexpr int EXTRA_HEIGHT        = 100;
constexpr int RESET_ALL_COLORS    = 10;

enum { FRAME, FILLED, TRANSPARENT, POINTS };
enum { TYPE, ELEMENT, CONSTANT };

// needs to be kept in sync with the dump image tri flag values
enum { NONE, TRIANGLES, CYLINDERS, BOTH };

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

ImageViewer::ImageViewer(const QString &fileName, LammpsWrapper *_lammps, LammpsGui *_lammpsgui,
                         QWidget *parent) :
    QDialog(parent), menuBar(new QMenuBar), imageLabel(new QLabel), scrollArea(new QScrollArea),
    atomSize(1.0), bondSize(0.4), saveAsAct(nullptr), copyAct(nullptr), cmdAct(nullptr),
    lammps(_lammps), lammpsgui(_lammpsgui), group("all"), molecule("none"), filename(fileName),
    useelements(false), usediameter(false), usesigma(false), shutdown(false)
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    imageLabel->setScaledContents(false);

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    scrollArea->setVisible(false);

    auto *imageLayout    = new QHBoxLayout;
    auto *settingsLayout = new QVBoxLayout;
    auto *mainLayout     = new QVBoxLayout;

    QFile image_styles(":/image_style.table");
    if (image_styles.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!image_styles.atEnd()) {
            auto line  = QString(image_styles.readLine());
            auto words = line.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (words.size() == 2) {
                if (words.at(0) == "compute") {
                    image_computes << words.at(1);
                } else if (words.at(0) == "fix") {
                    image_fixes << words.at(1);
                } else {
                    fprintf(stderr, "unhandled image style: %s\n", line.toStdString().c_str());
                }
            } else {
                fprintf(stderr, "unhandled image style: %s\n", line.toStdString().c_str());
            }
        }
        image_styles.close();
    }

    // store help URL info for computes and fixes with dump image support
    QFile help_index(":/help_index.table");
    if (help_index.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!help_index.atEnd()) {
            auto line  = QString(help_index.readLine());
            auto words = line.trimmed().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
            if (words.size() == 3) {
                if (words.at(1) == "fix") {
                    if (image_fixes.contains(words.at(2))) fix_map[words.at(2)] = words.at(0);
                } else if (words.at(1) == "compute") {
                    if (image_computes.contains(words.at(2)))
                        compute_map[words.at(2)] = words.at(0);
                }
            }
        }
        help_index.close();
    }

    readImageSettings();
    // initialize atomSize with lattice spacing
    const auto *xlattice = (const double *)lammps->extractGlobal("xlattice");
    if (xlattice) atomSize = *xlattice;

    auto pix   = QPixmap(":/icons/emblem-photos.png");
    auto fsize = QFontMetrics(QApplication::font()).size(Qt::TextSingleLine, "Height: 200");
#if defined(Q_OS_WIN32)
    fsize = fsize * 3 / 2;
#endif

    auto *renderstatus = new QLabel(QString());
    renderstatus->setPixmap(pix.scaled(22, 22, Qt::KeepAspectRatio));
    renderstatus->setEnabled(false);
    renderstatus->setToolTip("Render status");
    renderstatus->setObjectName("renderstatus");
    auto *asize = new QLineEdit(QString::number(2.0 * atomSize, 'f', 2));
    auto *valid = new QDoubleValidator(1.0e-10, 1.0e10, 3, this);
    asize->setValidator(valid);
    asize->setObjectName("atomSize");
    asize->setToolTip("Set Atom size");
    asize->setMinimumWidth(fsize.width() / 4);
    asize->setMaximumWidth(fsize.width() / 2);
    asize->setEnabled(false);
    asize->hide();
    auto *bsize = new QLineEdit(QString::number(bondSize, 'f', 2));
    bsize->setValidator(valid);
    bsize->setObjectName("bondSize");
    bsize->setToolTip("Set Bond size");
    bsize->setMinimumWidth(fsize.width() / 4);
    bsize->setMaximumWidth(fsize.width() / 2);
    bsize->setEnabled(false);
    bsize->hide();

    auto *xval = new QSpinBox;
    xval->setRange(100, 10000);
    xval->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    xval->setValue(xsize);
    xval->setObjectName("xsize");
    xval->setToolTip("Set rendered image width");
    xval->setMinimumSize(fsize);
    auto *yval = new QSpinBox;
    yval->setRange(100, 10000);
    yval->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    yval->setValue(ysize);
    yval->setObjectName("ysize");
    yval->setToolTip("Set rendered image height");
    yval->setMinimumSize(fsize);

    connect(asize, &QLineEdit::editingFinished, this, &ImageViewer::setAtomSize);
    connect(bsize, &QLineEdit::editingFinished, this, &ImageViewer::setBondSize);
    connect(xval, &QAbstractSpinBox::editingFinished, this, &ImageViewer::editSize);
    connect(yval, &QAbstractSpinBox::editingFinished, this, &ImageViewer::editSize);

    // workaround for incorrect highlight bug on macOS
    auto *dummy1 = new QPushButton(QIcon(), "");
    dummy1->hide();
    auto *dummy2 = new QPushButton(QIcon(), "");
    dummy2->hide();

    auto *dossao = new QPushButton(QIcon(":/icons/hd-img.png"), "");
    dossao->setCheckable(true);
    dossao->setToolTip("Toggle SSAO rendering");
    dossao->setObjectName("ssao");
    auto buttonhint = dossao->minimumSizeHint();
    buttonhint.setWidth(buttonhint.height() * 4 / 3);
    dossao->setMinimumSize(buttonhint);
    dossao->setMaximumSize(buttonhint);
    auto *doanti = new QPushButton(QIcon(":/icons/antialias.png"), "");
    doanti->setCheckable(true);
    doanti->setToolTip("Toggle anti-aliasing");
    doanti->setObjectName("antialias");
    doanti->setMinimumSize(buttonhint);
    doanti->setMaximumSize(buttonhint);
    auto *doshiny = new QPushButton(QIcon(":/icons/image-shiny.png"), "");
    doshiny->setCheckable(true);
    doshiny->setToolTip("Toggle shininess");
    doshiny->setObjectName("shiny");
    doshiny->setMinimumSize(buttonhint);
    doshiny->setMaximumSize(buttonhint);
    auto *dovdw = new QPushButton(QIcon(":/icons/vdw-style.png"), "");
    dovdw->setCheckable(true);
    dovdw->setToolTip("Toggle VDW style representation");
    dovdw->setObjectName("vdw");
    dovdw->setMinimumSize(buttonhint);
    dovdw->setMaximumSize(buttonhint);
    auto *dobond = new QPushButton(QIcon(":/icons/autobonds.png"), "");
    dobond->setCheckable(true);
    dobond->setToolTip("Toggle dynamic bond representation");
    dobond->setObjectName("autobond");
    dobond->setEnabled(false);
    dobond->setMinimumSize(buttonhint);
    dobond->setMaximumSize(buttonhint);
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
    dobox->setMinimumSize(buttonhint);
    dobox->setMaximumSize(buttonhint);
    auto *doaxes = new QPushButton(QIcon(":/icons/axes-img.png"), "");
    doaxes->setCheckable(true);
    doaxes->setToolTip("Toggle displaying axes");
    doaxes->setObjectName("axes");
    doaxes->setMinimumSize(buttonhint);
    doaxes->setMaximumSize(buttonhint);
    auto *zoomin = new QPushButton(QIcon(":/icons/gtk-zoom-in.png"), "");
    zoomin->setToolTip("Zoom in by 10 percent");
    zoomin->setMinimumSize(buttonhint);
    zoomin->setMaximumSize(buttonhint);
    auto *zoomout = new QPushButton(QIcon(":/icons/gtk-zoom-out.png"), "");
    zoomout->setToolTip("Zoom out by 10 percent");
    zoomout->setMinimumSize(buttonhint);
    zoomout->setMaximumSize(buttonhint);
    auto *rotleft = new QPushButton(QIcon(":/icons/object-rotate-left.png"), "");
    rotleft->setToolTip("Rotate left by 10 degrees");
    rotleft->setMinimumSize(buttonhint);
    rotleft->setMaximumSize(buttonhint);
    auto *rotright = new QPushButton(QIcon(":/icons/object-rotate-right.png"), "");
    rotright->setToolTip("Rotate right by 10 degrees");
    rotright->setMinimumSize(buttonhint);
    rotright->setMaximumSize(buttonhint);
    auto *rotup = new QPushButton(QIcon(":/icons/gtk-go-up.png"), "");
    rotup->setToolTip("Rotate up by 10 degrees");
    rotup->setMinimumSize(buttonhint);
    rotup->setMaximumSize(buttonhint);
    auto *rotdown = new QPushButton(QIcon(":/icons/gtk-go-down.png"), "");
    rotdown->setToolTip("Rotate down by 10 degrees");
    rotdown->setMinimumSize(buttonhint);
    rotdown->setMaximumSize(buttonhint);
    auto *recenter = new QPushButton(QIcon(":/icons/move-recenter.png"), "");
    recenter->setToolTip("Recenter on group");
    recenter->setMinimumSize(buttonhint);
    recenter->setMaximumSize(buttonhint);
    auto *reset = new QPushButton(QIcon(":/icons/gtk-zoom-fit.png"), "");
    reset->setToolTip("Reset view to defaults");
    reset->setMinimumSize(buttonhint);
    reset->setMaximumSize(buttonhint);

    auto *setviz = new QPushButton("G&lobal");
    setviz->setToolTip("Open dialog for global graphics settings");
    setviz->setObjectName("settings");
    auto *atomviz = new QPushButton("&Atoms/Bonds");
    atomviz->setToolTip("Open dialog for atom and bond settings");
    atomviz->setObjectName("atoms");
    auto *fixviz = new QPushButton("&Compute/Fix");
    fixviz->setToolTip("Open dialog for visualizing extra graphics from computes and fixes");
    fixviz->setObjectName("image_styles");
    fixviz->setEnabled(false);
    auto *regviz = new QPushButton("&Regions");
    regviz->setToolTip("Open dialog for visualizing regions");
    regviz->setObjectName("regions");
    regviz->setEnabled(false);
    auto *colviz = new QPushButton("C&olors");
    colviz->setToolTip("Open dialog for customizing colors");
    colviz->setObjectName("colors");
    auto *help = new QPushButton("Help");
    help->setToolTip("Open online help");
    help->setObjectName("visualization.html");

    constexpr int BUFLEN = 256;
    char gname[BUFLEN];
    auto *combo = new QComboBox;
    combo->setToolTip("Select group to display");
    combo->setObjectName("group");
    int ngroup = lammps->idCount("group");
    for (int i = 0; i < ngroup; ++i) {
        memset(gname, 0, BUFLEN);
        lammps->idName("group", i, gname, BUFLEN);
        combo->addItem(gname);
    }

    auto *molbox = new QComboBox;
    molbox->setToolTip("Select molecule to display");
    molbox->setObjectName("molecule");
    molbox->addItem("none");
    int nmols = lammps->idCount("molecule");
    for (int i = 0; i < nmols; ++i) {
        memset(gname, 0, BUFLEN);
        lammps->idName("molecule", i, gname, BUFLEN);
        molbox->addItem(gname);
    }

    auto *menuLayout   = new QHBoxLayout;
    auto *buttonLayout = new QHBoxLayout;
    auto *topLayout    = new QVBoxLayout;
    topLayout->addLayout(menuLayout);
    topLayout->addLayout(buttonLayout);
    topLayout->setSpacing(LAYOUT_SPACING);

    menuLayout->addWidget(menuBar);
    menuLayout->insertStretch(1, 10);
    menuLayout->addWidget(renderstatus);
    menuLayout->addWidget(new QLabel(" Atom Size: "));
    // hide item initially
    menuLayout->itemAt(3)->widget()->setObjectName("AtomLabel");
    menuLayout->itemAt(3)->widget()->hide();
    menuLayout->addWidget(asize);
    menuLayout->addWidget(new QLabel(" Bond Size: "));
    // hide item initially
    menuLayout->itemAt(5)->widget()->setObjectName("BondLabel");
    menuLayout->itemAt(5)->widget()->hide();
    menuLayout->addWidget(bsize);
    menuLayout->addWidget(new QLabel(" <u>W</u>idth: "));
    menuLayout->addWidget(xval);
    menuLayout->addWidget(new QLabel(" <u>H</u>eight: "));
    menuLayout->addWidget(yval);
    menuLayout->insertStretch(-1, 50);
    menuLayout->setSpacing(LAYOUT_SPACING);

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
    buttonLayout->setSizeConstraint(QLayout::SetMinimumSize);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(LAYOUT_SPACING);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(new QLabel("<u>G</u>roup:"));
    settingsLayout->addWidget(combo);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(new QLabel("<u>M</u>olecule:"));
    settingsLayout->addWidget(molbox);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(new QLabel("Settings:"));
    settingsLayout->addWidget(setviz);
    settingsLayout->addWidget(atomviz);
    settingsLayout->addWidget(regviz);
    settingsLayout->addWidget(fixviz);
    settingsLayout->addWidget(colviz);
    settingsLayout->addWidget(new QHline);
    settingsLayout->addWidget(help);
    settingsLayout->insertStretch(-1, 10);
    settingsLayout->setSizeConstraint(QLayout::SetMinimumSize);
    settingsLayout->setSpacing(LAYOUT_SPACING);

    connect(dossao, &QPushButton::released, this, &ImageViewer::toggleSsao);
    connect(doanti, &QPushButton::released, this, &ImageViewer::toggleAnti);
    connect(doshiny, &QPushButton::released, this, &ImageViewer::toggleShiny);
    connect(dovdw, &QPushButton::released, this, &ImageViewer::toggleVdw);
    connect(dobond, &QPushButton::released, this, &ImageViewer::toggleBond);
    connect(bondcut, &QLineEdit::editingFinished, this, &ImageViewer::setBondcut);
    connect(dobox, &QPushButton::released, this, &ImageViewer::toggleBox);
    connect(doaxes, &QPushButton::released, this, &ImageViewer::toggleAxes);
    connect(zoomin, &QPushButton::released, this, &ImageViewer::doZoomIn);
    connect(zoomout, &QPushButton::released, this, &ImageViewer::doZoomOut);
    connect(rotleft, &QPushButton::released, this, &ImageViewer::doRotLeft);
    connect(rotright, &QPushButton::released, this, &ImageViewer::doRotRight);
    connect(rotup, &QPushButton::released, this, &ImageViewer::doRotUp);
    connect(rotdown, &QPushButton::released, this, &ImageViewer::doRotDown);
    connect(recenter, &QPushButton::released, this, &ImageViewer::doRecenter);
    connect(reset, &QPushButton::released, this, &ImageViewer::resetView);
    connect(setviz, &QPushButton::released, this, &ImageViewer::globalSettings);
    connect(atomviz, &QPushButton::released, this, &ImageViewer::atomSettings);
    connect(fixviz, &QPushButton::released, this, &ImageViewer::fixSettings);
    connect(regviz, &QPushButton::released, this, &ImageViewer::regionSettings);
    connect(colviz, &QPushButton::released, this, &ImageViewer::colorSettings);
    connect(help, &QPushButton::released, this, &ImageViewer::resetColors);
    connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::changeGroup);
    connect(molbox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::changeMolecule);

    mainLayout->addLayout(topLayout);
    imageLayout->addWidget(scrollArea, 10);
    imageLayout->addLayout(settingsLayout, 0);
    imageLayout->setSpacing(LAYOUT_SPACING);
    mainLayout->addLayout(imageLayout);
    mainLayout->setSpacing(LAYOUT_SPACING);
    setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setWindowTitle(QString("LAMMPS-GUI - Image Viewer - ") + QFileInfo(fileName).fileName());
    createActions();

    resetView();
    // layout has not yet been established, so we need to fix up some pushbutton
    // properties directly since lookup in resetView() will have failed
    dobox->setChecked(showbox);
    doshiny->setChecked(shinyfactor > SHINY_CUT);
    dovdw->setChecked(vdwfactor > VDW_CUT);
    dovdw->setEnabled(showatoms && (useelements || usediameter || usesigma));
    dobond->setChecked(autobond);
    dobond->setEnabled(hasAutobonds());
    doaxes->setChecked(showaxes);
    dossao->setChecked(usessao);
    doanti->setChecked(antialias);

    scrollArea->setVisible(true);
    updateActions();
    setLayout(mainLayout);
    mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    adjustWindowSize();
    updateFixes();
    updateRegions();
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
    // must add maximize button for macOS to allow resizing, but remove on other platforms
#if defined(Q_OS_MACOS)
    flags |= Qt::WindowMaximizeButtonHint;
#else
    flags &= ~Qt::WindowMaximizeButtonHint;
#endif
    setWindowFlags(flags);
}

ImageViewer::~ImageViewer()
{
    shutdown = true;

    // clear dynamically allocated storage

    for (auto &comp : computes)
        delete comp.second;
    for (auto &ifix : fixes)
        delete ifix.second;
    for (auto &ireg : regions)
        delete ireg.second;
}

void ImageViewer::readImageSettings()
{
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
    atomcustom     = false;
    atomtrans      = 1.0;
    atomcolor      = settings.value("color", "type").toString();
    atomdiam       = settings.value("diameter", "type").toString();
    bondcolor      = settings.value("bondcolor", "atom").toString();
    bonddiam       = settings.value("bonddiam", "type").toString();
    bodycolor      = "atom";
    ellipsoidcolor = "atom";
    linecolor      = "atom";
    tricolor       = "atom";
    colormap       = settings.value("colormap", "BWR").toString();
    mapmin         = "auto";
    mapmax         = "auto";

    showatoms      = true;
    showbonds      = lammps->extractSetting("molecule_flag") == 1;
    showbodies     = true;
    bodydiam       = 0.2;
    bodyflag       = TRIANGLES;
    showellipsoids = true;
    ellipsoidflag  = TRIANGLES;
    ellipsoidlevel = 3;
    ellipsoiddiam  = 0.2;
    showlines      = true;
    linediam       = 0.2;
    showtris       = true;
    tridiam        = 0.2;
    triflag        = CYLINDERS;
    xcenter = ycenter = zcenter = 0.5;
    if (lammps->extractSetting("dimension") == 2) zcenter = 0.0;
    settings.endGroup();

    if (color_list.isEmpty()) resetColors(); // create list of default colors
}

void ImageViewer::resetView()
{
    readImageSettings();

    // reset state of checkable push buttons and combo box (if accessible)
    // also flag that atom/bond customizations should be ignored
    atomcustom = false;

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
        button->setEnabled(hasAutobonds());
        button->setChecked(autobond && hasAutobonds());
    }
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) {
        cutoff->setEnabled(autobond && hasAutobonds());
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

void ImageViewer::setAtomSize()
{
    auto *field = qobject_cast<QLineEdit *>(sender());
    if (!field) return;
    atomSize = 0.5 * field->text().toDouble();
    atomdiam = field->text();
    createImage();
}

void ImageViewer::setBondSize()
{
    auto *field = qobject_cast<QLineEdit *>(sender());
    if (!field) return;
    bondSize = field->text().toDouble();
    bonddiam = field->text();
    createImage();
}

void ImageViewer::editSize()
{
    auto *field = qobject_cast<QSpinBox *>(sender());
    if (!field) return;
    if (field->objectName() == "xsize") {
        xsize = field->value();
    } else if (field->objectName() == "ysize") {
        ysize = field->value();
    }
    createImage();
}

void ImageViewer::toggleSsao()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
    usessao = !usessao;
    button->setChecked(usessao);
    createImage();
}

void ImageViewer::toggleAnti()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
    antialias = !antialias;
    button->setChecked(antialias);
    createImage();
}

void ImageViewer::toggleShiny()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
    if (shinyfactor > SHINY_CUT)
        shinyfactor = SHINY_OFF;
    else
        shinyfactor = SHINY_ON;
    button->setChecked(shinyfactor > SHINY_CUT);
    createImage();
}

void ImageViewer::toggleVdw()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
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

void ImageViewer::toggleBond()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (button) autobond = button->isChecked();
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) cutoff->setEnabled(autobond);
    setBondcut();

    // when enabling autobond, we must turn off VDW
    if (autobond) {
        vdwfactor = VDW_OFF;
        auto *vdw = findChild<QPushButton *>("vdw");
        if (vdw) vdw->setChecked(false);
    }

    auto *edit  = findChild<QLineEdit *>("bondSize");
    auto *label = findChild<QLabel *>("BondLabel");
    if ((showbonds || autobond) && (bonddiam != "type") && (bonddiam != "atom") &&
        (bonddiam != "none")) {
        if (edit) {
            edit->setEnabled(true);
            edit->show();
            bondSize = bonddiam.toDouble();
            edit->setText(bonddiam);
        }
        if (label) {
            label->setEnabled(true);
            label->show();
        }
    } else {
        if (edit) {
            edit->setEnabled(false);
            edit->hide();
        }
        if (label) {
            label->setEnabled(false);
            label->hide();
        }
    }

    button->setChecked(autobond);
    createImage();
}

void ImageViewer::vdwbondSync()
{
    auto *src    = qobject_cast<QCheckBox *>(sender());
    auto *dialog = src->parent();
    auto *vdw    = dialog->findChild<QCheckBox *>("vdwbutton");
    auto *ab     = dialog->findChild<QCheckBox *>("autobutton");

    if (src == vdw) {
        if (vdw->isChecked() && ab->isChecked()) ab->setChecked(false);
    } else {
        if (vdw->isChecked() && ab->isChecked()) vdw->setChecked(false);
    }
}

void ImageViewer::acolorSync()
{
    auto *src = qobject_cast<QComboBox *>(sender());
    if (!src) return;
    auto *dialog = qobject_cast<QWidget *>(src->parent());

    // enable/disable colormap selector depending on atom coloring selection
    auto *amap = dialog->findChild<QComboBox *>("amap");
    if (amap) {
        if ((src->currentText() == "type") || (src->currentText() == "element"))
            amap->setEnabled(false);
        else
            amap->setEnabled(true);
    }
    auto *acolor = dialog->findChild<QComboBox *>("acolor");
    auto *bcolor = dialog->findChild<QComboBox *>("bcolor");
    auto *ecolor = dialog->findChild<QComboBox *>("ecolor");
    auto *lcolor = dialog->findChild<QComboBox *>("lcolor");
    auto *tcolor = dialog->findChild<QComboBox *>("tcolor");

    if (src && acolor && bcolor && ecolor && lcolor && tcolor) {
        if (src == acolor) {
            if (src->currentText() != "type") {
                for (auto *box : {bcolor, ecolor, lcolor, tcolor}) {
                    for (int idx = 0; idx < box->count(); ++idx)
                        if (box->itemText(idx) == "atom") box->setCurrentIndex(idx);
                }
            }
        } else {
            if (src->currentText() != "atom") {
                for (int idx = 0; idx < acolor->count(); ++idx)
                    if (acolor->itemText(idx) == "type") acolor->setCurrentIndex(idx);
            }
        }
    }
}

void ImageViewer::setBondcut()
{
    auto *cutoff = findChild<QLineEdit *>("bondcut");
    if (cutoff) {
        auto *dptr            = (double *)lammps->extractGlobal("neigh_cutmax");
        double max_bondcutoff = (dptr) ? *dptr : 0.0;
        double new_bondcutoff = cutoff->text().toDouble();

        if ((max_bondcutoff > 0.1) && (new_bondcutoff > max_bondcutoff))
            new_bondcutoff = max_bondcutoff;
        if (new_bondcutoff > 0.1) bondcutoff = new_bondcutoff;

        cutoff->setText(QString::number(bondcutoff));
    }
    createImage();
}

void ImageViewer::toggleBox()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
    showbox = !showbox;
    button->setChecked(showbox);
    createImage();
}

void ImageViewer::toggleAxes()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) return;
    showaxes = !showaxes;
    button->setChecked(showaxes);
    createImage();
}

void ImageViewer::doZoomIn()
{
    zoom = zoom * 1.1;
    zoom = std::min(zoom, 10.0);
    createImage();
}

void ImageViewer::doZoomOut()
{
    zoom = zoom / 1.1;
    zoom = std::max(zoom, 0.1);
    createImage();
}

void ImageViewer::doRotLeft()
{
    vrot -= 10;
    if (vrot < -180) vrot += 360;
    createImage();
}

void ImageViewer::doRotRight()
{
    vrot += 10;
    if (vrot > 180) vrot -= 360;
    createImage();
}

void ImageViewer::doRotDown()
{
    hrot -= 10;
    if (hrot < 0) hrot += 360;
    createImage();
}

void ImageViewer::doRotUp()
{
    hrot += 10;
    if (hrot > 360) hrot -= 360;
    createImage();
}

void ImageViewer::doRecenter()
{
    QString commands = QString("variable LAMMPSGUI_CX delete\n"
                               "variable LAMMPSGUI_CY delete\n"
                               "variable LAMMPSGUI_CZ delete\n"
                               "variable LAMMPSGUI_CX equal (xcm(%1,x)-xlo)/lx\n"
                               "variable LAMMPSGUI_CY equal (xcm(%1,y)-ylo)/ly\n"
                               "variable LAMMPSGUI_CZ equal (xcm(%1,z)-zlo)/lz\n")
                           .arg(group);
    lammps->commandsString(commands);
    xcenter = lammps->extractVariable("LAMMPSGUI_CX");
    ycenter = lammps->extractVariable("LAMMPSGUI_CY");
    zcenter = lammps->extractVariable("LAMMPSGUI_CZ");
    if (lammps->extractSetting("dimension") == 2) zcenter = 0.0;
    lammps->commandsString("variable LAMMPSGUI_CX delete\n"
                           "variable LAMMPSGUI_CY delete\n"
                           "variable LAMMPSGUI_CZ delete\n");
    createImage();
}

void ImageViewer::cmdToClipboard()
{
    auto words = splitLine(last_dump_cmd.toStdString());
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

    if (lammps->configHasPngSupport()) {
        dumpcmd += " image 100 myimage-*.png";
    } else if (lammps->configHasJpegSupport()) {
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
    auto *clip = QGuiApplication::clipboard();
    if (clip) {
        clip->setText(dumpcmd.c_str(), QClipboard::Clipboard);
        if (clip->supportsSelection()) clip->setText(dumpcmd.c_str(), QClipboard::Selection);
    } else
        fprintf(stderr, "# customized dump image command:\n%s", dumpcmd.c_str());
#else
    fprintf(stderr, "# customized dump image command:\n%s", dumpcmd.c_str());
#endif
}

void ImageViewer::globalSettings()
{
    QDialog setview;
    setview.setWindowTitle(QString("LAMMPS-GUI - Global image settings"));
    setview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    setview.setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);

    auto *title = new QLabel("Global image settings:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    auto *colorcompleter    = new QColorCompleter(this);
    auto *colorvalidator    = new QColorValidator(this);
    auto *transvalidator    = new QDoubleValidator(0.0, 1.0, 3, this);
    auto *fractionvalidator = new QDoubleValidator(0.00001, 5.0, 5, this);
    auto fwidth             = setview.fontMetrics().size(Qt::TextSingleLine, "0.00000000").width();

    auto *layout          = new QGridLayout;
    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 7;
    layout->addWidget(title, idx++, 0, 1, MAXCOLS, Qt::AlignCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    for (int i = 0; i < MAXCOLS; ++i)
        layout->setColumnStretch(i, 5);
    layout->setColumnStretch(0, 4);
    layout->setColumnStretch(1, 4);
    layout->setColumnStretch(MAXCOLS - 1, 3);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    auto *axesbutton = new QCheckBox("Axes ", this);
    axesbutton->setChecked(showaxes);
    axesbutton->setMaximumWidth(fwidth);
    layout->addWidget(axesbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Location: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *llbutton = new QRadioButton("Lower Left", this);
    llbutton->setChecked(axesloc == "yes");
    layout->addWidget(llbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *ulbutton = new QRadioButton("Upper Left", this);
    ulbutton->setChecked(axesloc == "upperleft");
    layout->addWidget(ulbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *lrbutton = new QRadioButton("Lower Right", this);
    lrbutton->setChecked(axesloc == "lowerright");
    layout->addWidget(lrbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *urbutton = new QRadioButton("Upper Right", this);
    urbutton->setChecked(axesloc == "upperright");
    layout->addWidget(urbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *cbutton = new QRadioButton("Center", this);
    cbutton->setChecked(axesloc == "center");
    layout->addWidget(cbutton, idx++, n++, 1, 1);

    n = 1;
    layout->addWidget(new QLabel("Length: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *alval = new QLineEdit(QString::number(axeslen));
    alval->setValidator(fractionvalidator);
    alval->setMaximumWidth(fwidth);
    layout->addWidget(alval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *adval = new QLineEdit(QString::number(axesdiam));
    adval->setValidator(fractionvalidator);
    adval->setMaximumWidth(fwidth);
    layout->addWidget(adval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Opacity: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *atval = new QLineEdit(QString::number(axestrans));
    atval->setValidator(transvalidator);
    atval->setMaximumWidth(fwidth * 3 / 2);
    layout->addWidget(atval, idx++, n++, 1, 1);

    n = 0;

    auto *boxbutton = new QCheckBox("Box ", this);
    boxbutton->setChecked(showbox);
    boxbutton->setMaximumWidth(fwidth);
    layout->addWidget(boxbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Color: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *bcolor = new QLineEdit(boxcolor);
    bcolor->setCompleter(colorcompleter);
    bcolor->setValidator(colorvalidator);
    bcolor->setMaximumWidth(fwidth);
    layout->addWidget(bcolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *bdiam = new QLineEdit(QString::number(boxdiam));
    bdiam->setValidator(fractionvalidator);
    bdiam->setMaximumWidth(fwidth);
    layout->addWidget(bdiam, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Opacity: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *btrans = new QLineEdit(QString::number(boxtrans));
    btrans->setValidator(transvalidator);
    btrans->setMaximumWidth(fwidth * 3 / 2);
    layout->addWidget(btrans, idx++, n++, 1, 1);

    n = 0;

    auto *subboxbutton = new QCheckBox("Subbox ", this);
    subboxbutton->setChecked(showsubbox);
    subboxbutton->setMaximumWidth(fwidth);
    layout->addWidget(subboxbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Diameter: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *subdiam = new QLineEdit(QString::number(subboxdiam));
    subdiam->setValidator(fractionvalidator);
    subdiam->setMaximumWidth(fwidth);
    layout->addWidget(subdiam, idx++, n++, 1, 1);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    n = 0;

    layout->addWidget(new QLabel("Background:"), idx, n++, 1, 1);
    layout->addWidget(new QLabel("Bottom: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *bgcolor = new QLineEdit(backcolor);
    bgcolor->setCompleter(colorcompleter);
    bgcolor->setValidator(colorvalidator);
    bgcolor->setMaximumWidth(fwidth);
    layout->addWidget(bgcolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Topcolor: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *b2color = new QLineEdit(backcolor2);
    b2color->setCompleter(colorcompleter);
    b2color->setValidator(colorvalidator);
    b2color->setMaximumWidth(fwidth);
    layout->addWidget(b2color, idx++, n++, 1, 1);

    n = 0;
    layout->addWidget(new QLabel("Quality:"), idx, n++, 1, 1);
    n++;
    auto *fsaa = new QCheckBox("FSAA  ", this);
    fsaa->setChecked(antialias);
    layout->addWidget(fsaa, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *ssao = new QCheckBox("SSAO: ", this);
    ssao->setChecked(usessao);
    layout->addWidget(ssao, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *aoval = new QLineEdit(QString::number(ssaoval));
    aoval->setValidator(transvalidator);
    aoval->setMaximumWidth(fwidth);
    layout->addWidget(aoval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Shiny: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *shiny = new QLineEdit(QString::number(shinyfactor));
    shiny->setValidator(transvalidator);
    shiny->setMaximumWidth(fwidth);
    layout->addWidget(shiny, idx++, n++, 1, 1);

    n = 0;
    layout->addWidget(new QLabel("Center:"), idx, n++, 1, 1);
    layout->addWidget(new QLabel("X-direction: "), idx, n++, 1, 1,
                      Qt::AlignVCenter | Qt::AlignRight);
    auto *xval = new QLineEdit(QString::number(xcenter));
    xval->setValidator(transvalidator);
    xval->setMaximumWidth(fwidth);
    layout->addWidget(xval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Y-direction: "), idx, n++, 1, 1,
                      Qt::AlignVCenter | Qt::AlignRight);
    auto *yval = new QLineEdit(QString::number(ycenter));
    yval->setValidator(transvalidator);
    yval->setMaximumWidth(fwidth);
    layout->addWidget(yval, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Z-direction: "), idx, n++, 1, 1,
                      Qt::AlignVCenter | Qt::AlignRight);
    auto *zval = new QLineEdit(QString::number(zcenter));
    zval->setValidator(transvalidator);
    zval->setMaximumWidth(fwidth);
    layout->addWidget(zval, idx++, n++, 1, 1);

    n = 0;
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    auto *lightlayout = new QHBoxLayout;
    lightlayout->setSpacing(LAYOUT_SPACING);
    lightlayout->addWidget(new QLabel("Lights: "), 3, Qt::AlignLeft);
    lightlayout->addWidget(new QLabel("Ambient: "), 2, Qt::AlignRight);
    auto *ambient = new QLineEdit(QString::number(ambientlight));
    ambient->setValidator(transvalidator);
    ambient->setMaximumWidth(fwidth);
    lightlayout->addWidget(ambient, 2);
    lightlayout->addWidget(new QLabel("Key: "), 2, Qt::AlignRight);
    auto *key = new QLineEdit(QString::number(keylight));
    key->setValidator(transvalidator);
    key->setMaximumWidth(fwidth);
    lightlayout->addWidget(key, 2);
    lightlayout->addWidget(new QLabel("Fill: "), 2, Qt::AlignRight);
    auto *fill = new QLineEdit(QString::number(filllight));
    fill->setValidator(transvalidator);
    fill->setMaximumWidth(fwidth);
    lightlayout->addWidget(fill, 2);
    lightlayout->addWidget(new QLabel("Back: "), 2, Qt::AlignRight);
    auto *back = new QLineEdit(QString::number(backlight));
    back->setValidator(transvalidator);
    back->setMaximumWidth(fwidth);
    lightlayout->addWidget(back, 2);
    // only allow modifying lights for LAMMPS versions after 30 March 2026
    if (lammps->version() > 20260330) {
        layout->addLayout(lightlayout, idx++, 0, 1, MAXCOLS, Qt::AlignHCenter);
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    }

    n = 0;

    auto *bottomlayout = new QHBoxLayout;
    bottomlayout->setSpacing(LAYOUT_SPACING);
    auto *cancel = new QPushButton(QIcon(":/icons/dialog-cancel.png"), "&Cancel");
    auto *apply  = new QPushButton(QIcon(":/icons/dialog-ok.png"), "&Apply");
    auto *help   = new QPushButton(QIcon(":/icons/help-browser.png"), "&Help");
    help->setObjectName("dump_image.html");
    cancel->setAutoDefault(false);
    help->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    apply->setFocus();
    connect(cancel, &QPushButton::released, &setview, &QDialog::reject);
    connect(apply, &QPushButton::released, &setview, &QDialog::accept);
    connect(help, &QPushButton::released, this, &ImageViewer::getHelp);

    bottomlayout->addWidget(cancel, Qt::AlignHCenter);
    bottomlayout->addWidget(apply, Qt::AlignHCenter);
    bottomlayout->addWidget(help, Qt::AlignHCenter);
    layout->addLayout(bottomlayout, idx++, 0, 1, MAXCOLS, Qt::AlignHCenter);
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
    button = findChild<QPushButton *>("shiny");
    if (button) button->setChecked(shinyfactor > SHINY_CUT);

    if (xval->hasAcceptableInput()) xcenter = xval->text().toDouble();
    if (yval->hasAcceptableInput()) ycenter = yval->text().toDouble();
    if (zval->hasAcceptableInput()) zcenter = zval->text().toDouble();

    if (ambient->hasAcceptableInput()) ambientlight = ambient->text().toDouble();
    if (key->hasAcceptableInput()) keylight = key->text().toDouble();
    if (fill->hasAcceptableInput()) filllight = fill->text().toDouble();
    if (back->hasAcceptableInput()) backlight = back->text().toDouble();

    // update image with new settings
    createImage();
}

void ImageViewer::atomSettings()
{
    updatePeratom();
    QDialog setview;
    setview.setWindowTitle(QString("LAMMPS-GUI - Atom and bond settings for images"));
    setview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    setview.setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);

    auto *title = new QLabel("Atom and bond settings for images:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    auto *transvalidator  = new QDoubleValidator(0.0, 1.0, 3, this);
    auto *diamvalidator   = new QDoubleValidator(0.001, 5.0, 4, this);
    auto *layout          = new QGridLayout;
    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 7;
    layout->addWidget(title, idx++, 0, 1, MAXCOLS, Qt::AlignCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    layout->setColumnStretch(0, 7);
    layout->setColumnStretch(1, 4);
    layout->setColumnStretch(2, 7);
    layout->setColumnStretch(3, 3);
    layout->setColumnStretch(4, 7);
    layout->setColumnStretch(5, 7);
    layout->setColumnStretch(6, 4);

    n = 0;

    auto *atombutton = new QCheckBox("Atoms ", this);
    atombutton->setChecked(showatoms);
    layout->addWidget(atombutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Color: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *acolor = new QComboBox;
    acolor->setObjectName("acolor");
    acolor->addItems(atom_properties);
    if (atomcustom) { // select item that was selected the last time
        for (int idx = 0; idx < acolor->count(); ++idx) {
            if (acolor->itemText(idx) == atomcolor) acolor->setCurrentIndex(idx);
        }
    }
    connect(acolor, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    layout->addWidget(acolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Size: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);

    QRegularExpression validatom(R"((element|diameter|sigma|type|none|^\d+\.?\d*|^\d*\.?\d+))");
    QStringList aditems;
    if (useelements) aditems << "element";
    if (usediameter) aditems << "diameter";
    if (usesigma) aditems << "sigma";
    aditems << "type" << "3.50" << "5.00" << "3.00" << "2.00";
    if ((atomSize > 0.1) && (atomSize < 5.0)) {
        aditems << QString::number(2.0 * atomSize, 'f', 2);
    } else {
        aditems << QString::number(2.0 * atomSize, 'g', 3);
    }
    if (atomdiam != "none") aditems << atomdiam;
    aditems.removeDuplicates();

    auto *adiam = new QComboBox;
    adiam->setObjectName("adiam");
    adiam->addItems(aditems);
    adiam->setEditable(true);
    adiam->setValidator(new QRegularExpressionValidator(validatom, this));
    if (atomcustom) { // select item that was selected the last time
        for (int idx = 0; idx < adiam->count(); ++idx) {
            if (adiam->itemText(idx) == atomdiam) adiam->setCurrentIndex(idx);
        }
    }
    layout->addWidget(adiam, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Opacity: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *atrans = new QLineEdit(QString::number(atomtrans));
    atrans->setValidator(transvalidator);
    layout->addWidget(atrans, idx++, n++, 1, 1);

    n = 0;

    auto *vdwbutton = new QCheckBox("VDW style ", this);
    vdwbutton->setChecked(vdwfactor > VDW_CUT);
    vdwbutton->setObjectName("vdwbutton");
    layout->addWidget(vdwbutton, idx, n++, 1, 1, Qt::AlignCenter);
    layout->addWidget(new QLabel("Map: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *amap = new QComboBox;
    amap->setObjectName("amap");
    amap->addItem(gradient_icon({QColor(0, 57, 109), "white", QColor(117, 14, 19)}), "BWR");
    amap->addItem(gradient_icon({QColor(117, 14, 19), "white", QColor(0, 57, 109)}), "RWB");
    amap->addItem(gradient_icon({QColor(73, 29, 141), "white", QColor(0, 65, 68)}), "PWT");
    amap->addItem(gradient_icon({"blue", "white", "green"}), "BWG");
    amap->addItem(gradient_icon({"blue", "green", "red"}), "BGR");
    amap->addItem(gradient_icon({"black", "white"}), "Grayscale");
    // clang-format off
    amap->addItem(gradient_icon({QColor(72, 33, 115), QColor(111, 111, 142), QColor(41, 175, 127),
                                 QColor(189, 223, 174)}), "Viridis");
    amap->addItem(gradient_icon({QColor(13, 8, 135), QColor(156, 23, 150), QColor(237, 121, 83),
                                 QColor(240, 249, 33)}), "Plasma");
    amap->addItem(gradient_icon({QColor(8, 8, 12), QColor(81, 18, 124), QColor(183, 55, 121),
                                 QColor(252, 137, 97), QColor(252, 253, 191)}), "Inferno");
    amap->addItem(gradient_icon({QColor(18, 39, 64), QColor(27, 72, 94), QColor(86, 139, 135),
                                 QColor(181, 209, 174)}), "Teal");
    amap->addItem(gradient_icon({"red", "yellow", "green", "cyan", "blue", "purple"}), "Rainbow");
    amap->addItem(sequence_icon({QColor(206, 206, 206), QColor(165, 89, 170), QColor(81, 168, 156),
                                 QColor(240, 197, 113), QColor(224, 43, 53), QColor(8, 42, 84)}),
                  "Sequential");
    amap->addItem(sequence_icon({QColor(37, 102, 118), QColor(100, 221, 150), QColor(146, 49, 36),
                                 QColor(100, 212, 253), QColor(5, 110, 18), QColor(253, 89, 37),
                                 QColor(70, 243, 62), QColor(186, 134, 92), QColor(201, 221, 135),
                                 QColor(62, 76, 20)}), "Landscape");
    amap->addItem(sequence_icon({"red", "cyan", "green", "black", "magenta", "blue", "yellow",
                                 "purple", "white", "orange"}), "Basic");
    // clang-format on
    for (int idx = 0; idx < amap->count(); ++idx) {
        if (amap->itemText(idx) == colormap) amap->setCurrentIndex(idx);
    }
    if ((atomcolor == "type") || (atomcolor == "element")) amap->setEnabled(false);
    QRegularExpression validminmax(
        R"((auto|min|max|[+-]?\d+\.?\d*|[+-]?\d*\.?\d+)|[+-]?\d+\.?\d*[eE][+-]?\d+|[+-]?\d*\.?\d+[eE][+-]?\d+)");
    auto *minmaxvalidator = new QRegularExpressionValidator(validminmax, this);

    layout->addWidget(amap, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Min: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *amapmin = new QLineEdit(mapmin);
    amapmin->setValidator(minmaxvalidator);
    layout->addWidget(amapmin, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Max: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *amapmax = new QLineEdit(mapmax);
    amapmax->setValidator(minmaxvalidator);
    layout->addWidget(amapmax, idx++, n++, 1, 1);

    n = 0;

    auto *bondbutton = new QCheckBox("Bonds ", this);
    bondbutton->setChecked(showbonds);
    layout->addWidget(bondbutton, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Color: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *bncolor = new QComboBox;
    bncolor->setObjectName("bncolor");
    bncolor->addItems({"atom", "type"});
    if (atomcustom) { // select item that was selected the last time
        if (bondcolor == "none") {
            bondbutton->setChecked(false);
        } else {
            for (int idx = 0; idx < bncolor->count(); ++idx) {
                if (bncolor->itemText(idx) == bondcolor) bncolor->setCurrentIndex(idx);
            }
        }
    }
    layout->addWidget(bncolor, idx, n++, 1, 1);
    layout->addWidget(new QLabel("Size: "), idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);

    QRegularExpression validbond(R"((atom|type|none|^\d+\.?\d*|^\d*\.?\d+))");
    QStringList bnitems{"type", "atom", "0.10", "0.20", "0.40", "0.75"};
    if (bonddiam != "none") {
        if ((bondSize > 0.1) && (bondSize < 5.0)) {
            bnitems << QString::number(bondSize, 'f', 2);
        } else {
            bnitems << QString::number(bondSize, 'g', 3);
        }
    }
    bnitems.removeDuplicates();

    auto *bndiam = new QComboBox;
    bndiam->setObjectName("bndiam");
    bndiam->addItems(bnitems);
    bndiam->setEditable(true);
    bndiam->setValidator(new QRegularExpressionValidator(validbond, this));
    if (atomcustom) {             // select item that was selected the last time
        if (bonddiam == "none") { // none means bonds are disabled
            bondbutton->setChecked(false);
        } else {
            for (int idx = 0; idx < bndiam->count(); ++idx) {
                if (bndiam->itemText(idx) == bonddiam) bndiam->setCurrentIndex(idx);
            }
        }
    }
    layout->addWidget(bndiam, idx, n++, 1, 1);
    auto *autobutton = new QCheckBox("AutoBonds:", this);
    autobutton->setChecked(autobond);
    autobutton->setEnabled(hasAutobonds());
    autobutton->setObjectName("autobutton");
    layout->addWidget(autobutton, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *bcutoff = new QLineEdit(QString::number(bondcutoff));
    bcutoff->setValidator(new QDoubleValidator(0.001, 10.0, 100, this));
    bcutoff->setEnabled(hasAutobonds());
    layout->addWidget(bcutoff, idx++, n++, 1, 1);
    if (lammps->extractSetting("molecule_flag") != 1) {
        bondbutton->setEnabled(false);
        bondbutton->setChecked(false);
        showbonds = false;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 7, 0)
    connect(vdwbutton, &QCheckBox::stateChanged, this, &ImageViewer::vdwbondSync);
    connect(autobutton, &QCheckBox::stateChanged, this, &ImageViewer::vdwbondSync);
#else
    connect(vdwbutton, &QCheckBox::checkStateChanged, this, &ImageViewer::vdwbondSync);
    connect(autobutton, &QCheckBox::checkStateChanged, this, &ImageViewer::vdwbondSync);
#endif
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    n = 0;

    layout->addWidget(new QLabel("Shape:"), idx, n++, 1, 1, Qt::AlignCenter);
    layout->addWidget(new QLabel("Color:"), idx, n++, 1, 1, Qt::AlignCenter);
    layout->addWidget(new QLabel("Style:"), idx, n++, 1, 4, Qt::AlignCenter);
    n += 3;
    layout->addWidget(new QLabel("Refine:"), idx++, n++, 1, 1, Qt::AlignCenter);

    n = 0;

    auto *bodybutton = new QCheckBox("Bodies ", this);
    bodybutton->setChecked(showbodies);
    layout->addWidget(bodybutton, idx, n++, 1, 1);
    auto *bcolor = new QComboBox;
    bcolor->addItems({"atom", "type", "index"});
    for (int idx = 0; idx < bcolor->count(); ++idx) {
        if (bcolor->itemText(idx) == bodycolor) bcolor->setCurrentIndex(idx);
    }
    bcolor->setObjectName("bcolor");
    connect(bcolor, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    layout->addWidget(bcolor, idx, n++, 1, 1);
    auto *bgroup   = new QButtonGroup(this);
    auto *bcbutton = new QRadioButton("Cylinders", this);
    bcbutton->setChecked(bodyflag == CYLINDERS);
    bgroup->addButton(bcbutton);
    layout->addWidget(bcbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *bdiam = new QLineEdit(QString::number(bodydiam));
    bdiam->setValidator(diamvalidator);
    layout->addWidget(bdiam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *btbutton = new QRadioButton("Triangles", this);
    btbutton->setChecked(bodyflag == TRIANGLES);
    bgroup->addButton(btbutton);
    layout->addWidget(btbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *bbbutton = new QRadioButton("Both", this);
    bbbutton->setChecked(bodyflag == BOTH);
    bgroup->addButton(bbbutton);
    layout->addWidget(bbbutton, idx++, n++, 1, 1, Qt::AlignCenter);
    if (lammps->extractSetting("body_flag") != 1) {
        bodybutton->setEnabled(false);
        bodybutton->setChecked(false);
        bcolor->setEnabled(false);
        bdiam->setEnabled(false);
        bcbutton->setEnabled(false);
        btbutton->setEnabled(false);
        bbbutton->setEnabled(false);
    }

    n = 0;

    auto *ellipsoidbutton = new QCheckBox("Ellipsoids ", this);
    ellipsoidbutton->setChecked(showellipsoids);
    layout->addWidget(ellipsoidbutton, idx, n++, 1, 1);
    auto *ecolor = new QComboBox;
    ecolor->addItems({"atom", "type", "index"});
    for (int idx = 0; idx < ecolor->count(); ++idx) {
        if (ecolor->itemText(idx) == ellipsoidcolor) ecolor->setCurrentIndex(idx);
    }
    ecolor->setObjectName("ecolor");
    connect(ecolor, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    layout->addWidget(ecolor, idx, n++, 1, 1);
    auto *egroup   = new QButtonGroup(this);
    auto *ecbutton = new QRadioButton("Cylinders", this);
    ecbutton->setChecked(ellipsoidflag == CYLINDERS);
    egroup->addButton(ecbutton);
    layout->addWidget(ecbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *ediam = new QLineEdit(QString::number(ellipsoiddiam));
    ediam->setValidator(diamvalidator);
    layout->addWidget(ediam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *etbutton = new QRadioButton("Triangles", this);
    etbutton->setChecked(ellipsoidflag == TRIANGLES);
    egroup->addButton(etbutton);
    layout->addWidget(etbutton, idx, n++, 1, 1, Qt::AlignCenter);
    // skip location for "Both" since ellipsoids don't need it
    ++n;
    auto *elevel = new QSpinBox;
    elevel->setRange(1, 6);
    elevel->setStepType(QAbstractSpinBox::DefaultStepType);
    elevel->setValue(ellipsoidlevel);
    elevel->setWrapping(false);
    layout->addWidget(elevel, idx++, n++, 1, 1);
    ++n;
    if (lammps->extractSetting("ellipsoid_flag") != 1) {
        ellipsoidbutton->setEnabled(false);
        ellipsoidbutton->setChecked(false);
        ecolor->setEnabled(false);
        elevel->setEnabled(false);
        ediam->setEnabled(false);
        ecbutton->setEnabled(false);
        etbutton->setEnabled(false);
    }

    n = 0;

    auto *linebutton = new QCheckBox("Lines ", this);
    linebutton->setChecked(showlines);
    layout->addWidget(linebutton, idx, n++, 1, 1);
    auto *lcolor = new QComboBox;
    lcolor->addItems({"atom", "type", "index"});
    for (int idx = 0; idx < lcolor->count(); ++idx) {
        if (lcolor->itemText(idx) == linecolor) lcolor->setCurrentIndex(idx);
    }
    lcolor->setObjectName("lcolor");
    connect(lcolor, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    layout->addWidget(lcolor, idx, n++, 1, 1);
    ++n;
    auto *ldiam = new QLineEdit(QString::number(linediam));
    ldiam->setValidator(diamvalidator);
    layout->addWidget(ldiam, idx++, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    if (lammps->extractSetting("line_flag") != 1) {
        linebutton->setEnabled(false);
        linebutton->setChecked(false);
        lcolor->setEnabled(false);
        ldiam->setEnabled(false);
    }

    n = 0;

    auto *tributton = new QCheckBox("Triangles ", this);
    tributton->setChecked(showtris);
    layout->addWidget(tributton, idx, n++, 1, 1);
    auto *tcolor = new QComboBox;
    tcolor->addItems({"atom", "type", "index"});
    for (int idx = 0; idx < tcolor->count(); ++idx) {
        if (tcolor->itemText(idx) == tricolor) tcolor->setCurrentIndex(idx);
    }
    tcolor->setObjectName("tcolor");
    connect(tcolor, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    layout->addWidget(tcolor, idx, n++, 1, 1);
    auto *tgroup   = new QButtonGroup(this);
    auto *tcbutton = new QRadioButton("Cylinders", this);
    tcbutton->setChecked(triflag == CYLINDERS);
    tgroup->addButton(tcbutton);
    layout->addWidget(tcbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *tdiam = new QLineEdit(QString::number(tridiam));
    tdiam->setValidator(diamvalidator);
    layout->addWidget(tdiam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *ttbutton = new QRadioButton("Triangles", this);
    ttbutton->setChecked(triflag == TRIANGLES);
    tgroup->addButton(ttbutton);
    layout->addWidget(ttbutton, idx, n++, 1, 1, Qt::AlignCenter);
    auto *tbbutton = new QRadioButton("Both", this);
    tbbutton->setChecked(triflag == BOTH);
    tgroup->addButton(tbbutton);
    layout->addWidget(tbbutton, idx++, n++, 1, 1, Qt::AlignCenter);
    ++n;
    if (lammps->extractSetting("tri_flag") != 1) {
        tributton->setEnabled(false);
        tributton->setChecked(false);
        tcolor->setEnabled(false);
        tdiam->setEnabled(false);
        tcbutton->setEnabled(false);
        ttbutton->setEnabled(false);
        tbbutton->setEnabled(false);
    }

    n = 0;
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *bottomlayout = new QHBoxLayout;
    bottomlayout->setSpacing(LAYOUT_SPACING);
    auto *cancel = new QPushButton(QIcon(":/icons/dialog-cancel.png"), "&Cancel");
    auto *apply  = new QPushButton(QIcon(":/icons/dialog-ok.png"), "&Apply");
    auto *help   = new QPushButton(QIcon(":/icons/help-browser.png"), "&Help");
    help->setObjectName("dump_image.html");
    cancel->setAutoDefault(false);
    help->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    apply->setFocus();
    connect(cancel, &QPushButton::released, &setview, &QDialog::reject);
    connect(apply, &QPushButton::released, &setview, &QDialog::accept);
    connect(help, &QPushButton::released, this, &ImageViewer::getHelp);

    bottomlayout->addWidget(cancel, Qt::AlignHCenter);
    bottomlayout->addWidget(apply, Qt::AlignHCenter);
    bottomlayout->addWidget(help, Qt::AlignHCenter);
    layout->addLayout(bottomlayout, idx, 0, 1, MAXCOLS, Qt::AlignHCenter);
    setview.setLayout(layout);

    int rv = setview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve and apply data

    showatoms    = atombutton->isChecked();
    vdwfactor    = vdwbutton->isChecked() ? VDW_ON : VDW_OFF;
    auto *button = findChild<QPushButton *>("vdw");
    if (button) {
        if (showatoms) {
            button->setEnabled(true);
            button->setChecked(vdwfactor > VDW_CUT);
        } else {
            button->setEnabled(false);
            button->setChecked(false);
        }
    }
    auto value = acolor->currentText();
    if (!atomcustom) { // treat "element" property special if not customized
        if (value != "element") {
            atomcustom = true;
            atomcolor  = value;
        }
    } else {
        atomcolor = value;
    }
    atomdiam = adiam->currentText();

    if (atrans->hasAcceptableInput()) atomtrans = atrans->text().toDouble();
    colormap = amap->currentText();
    if (amapmin->hasAcceptableInput()) mapmin = amapmin->text();
    if (amapmax->hasAcceptableInput()) mapmax = amapmax->text();

    showbonds = bondbutton->isChecked();
    value     = bncolor->currentText();
    if (!atomcustom) { // treat "atom" property special if not yet customized
        if (value != "atom") {
            atomcustom = true;
            bondcolor  = value;
        }
    } else {
        bondcolor = value;
    }
    value = bndiam->currentText();
    if (!atomcustom) { // treat "atom" property special if not yet customized
        if (value != "atom") {
            atomcustom = true;
            bonddiam   = value;
        }
    } else {
        bonddiam = value;
    }

    // enable atom size input field in main window, if not set to symbolic value
    if (atomcustom) {
        auto *edit  = findChild<QLineEdit *>("atomSize");
        auto *label = findChild<QLabel *>("AtomLabel");
        if ((atomdiam != "element") && (atomdiam != "type") && (atomdiam != "diameter") &&
            (atomdiam != "sigma") && (atomdiam != "none")) {
            if (edit) {
                edit->setEnabled(true);
                edit->show();
                atomSize = 0.5 * atomdiam.toDouble();
                edit->setText(atomdiam);
            }
            if (label) {
                label->setEnabled(true);
                label->show();
            }
        } else {
            if (edit) {
                edit->setEnabled(false);
                edit->hide();
            }
            if (label) {
                label->setEnabled(false);
                label->hide();
            }
        }
    }

    if (hasAutobonds()) {
        autobond   = autobutton->isChecked();
        bondcutoff = bcutoff->text().toDouble();

        button = findChild<QPushButton *>("autobond");
        if (button) button->setChecked(autobond);
        auto *cutoff = findChild<QLineEdit *>("bondcut");
        if (cutoff) {
            cutoff->setEnabled(autobond);
            cutoff->setText(QString::number(bondcutoff));
        }
    }

    if ((showbonds || autobond) && (bonddiam != "type") && (bonddiam != "atom") &&
        (bonddiam != "none")) {
        auto *edit  = findChild<QLineEdit *>("bondSize");
        auto *label = findChild<QLabel *>("BondLabel");
        if (edit) {
            edit->setEnabled(true);
            edit->show();
            bondSize = bonddiam.toDouble();
            edit->setText(bonddiam);
        }
        if (label) {
            label->setEnabled(true);
            label->show();
        }
    } else {
        auto *edit  = findChild<QLineEdit *>("bondSize");
        auto *label = findChild<QLabel *>("BondLabel");
        if (edit) {
            edit->setEnabled(false);
            edit->hide();
        }
        if (label) {
            label->setEnabled(false);
            label->hide();
        }
    }

    showbodies = bodybutton->isChecked();
    bodycolor  = bcolor->currentText();
    // diameter for body cylinders
    if (bdiam->hasAcceptableInput()) bodydiam = bdiam->text().toDouble();
    if (bcbutton->isChecked()) {
        bodyflag = CYLINDERS;
    } else if (btbutton->isChecked()) {
        bodyflag = TRIANGLES;
    } else if (bbbutton->isChecked()) {
        bodyflag    = BOTH;
        shinyfactor = 0.0;
        button      = findChild<QPushButton *>("shiny");
        if (button) button->setChecked(shinyfactor > SHINY_CUT);
    }

    showellipsoids = ellipsoidbutton->isChecked();
    if (ediam->hasAcceptableInput()) ellipsoiddiam = ediam->text().toDouble();
    if (elevel->hasAcceptableInput()) ellipsoidlevel = elevel->text().toInt();
    if (ecbutton->isChecked()) {
        ellipsoidflag = CYLINDERS;
    } else {
        ellipsoidflag = TRIANGLES;
    }
    ellipsoidcolor = ecolor->currentText();

    showlines = linebutton->isChecked();
    if (ldiam->hasAcceptableInput()) linediam = ldiam->text().toDouble();
    linecolor = lcolor->currentText();

    showtris = tributton->isChecked();
    if (tdiam->hasAcceptableInput()) tridiam = tdiam->text().toDouble();
    if (tcbutton->isChecked()) {
        triflag = CYLINDERS;
    } else if (ttbutton->isChecked()) {
        triflag = TRIANGLES;
    } else if (tbbutton->isChecked()) {
        triflag = BOTH;
    }
    tricolor = tcolor->currentText();

    // update image with new settings
    createImage();
}

void ImageViewer::fixSettings()
{
    updateFixes();
    if ((computes.size() + fixes.size()) == 0) return;
    QDialog fixview;
    fixview.setWindowTitle(QString("LAMMPS-GUI - Visualize Compute and Fix Graphics Objects"));
    fixview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    fixview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    fixview.setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);

    auto *title = new QLabel("Visualize Compute and Fix Graphics Objects:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    auto *colorcompleter = new QColorCompleter(this);
    auto *colorvalidator = new QColorValidator(this);
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 3, this);
    QFontMetrics metrics(fixview.fontMetrics());

    int idx               = 0;
    int n                 = 0;
    constexpr int MAXCOLS = 9;
    auto *layout          = new QGridLayout;
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    layout->addWidget(title, idx++, n, 1, MAXCOLS, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    for (int i = 0; i < MAXCOLS; ++i)
        layout->setColumnStretch(i, 2);

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
        layout->addWidget(new QLabel("Flag #2:"), idx, n++, Qt::AlignHCenter);
        layout->addWidget(new QLabel("Help:"), idx++, n++, Qt::AlignHCenter);
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

        for (const auto &comp : computes) {
            n = 0;

            auto *label = new QLabel(comp.first.c_str());
            layout->addWidget(label, idx, n++);
            layout->addWidget(new QLabel(comp.second->style), idx, n++);
            auto *check = new QCheckBox("");
            check->setChecked(comp.second->enabled);
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
            auto *help = new QPushButton(QIcon(":/icons/system-help.png"), "");
            help->setObjectName(compute_map.value(comp.second->style, QString()));
            layout->addWidget(help, idx, n++);
            connect(help, &QPushButton::released, this, &ImageViewer::getHelp);
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
            check->setChecked(fix.second->enabled);
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
            auto *help = new QPushButton(QIcon(":/icons/system-help.png"), "");
            help->setObjectName(fix_map.value(fix.second->style, QString()));
            layout->addWidget(help, idx, n++);
            connect(help, &QPushButton::released, this, &ImageViewer::getHelp);
            ++idx;
        }
        layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);
    }

    auto *cancel = new QPushButton(QIcon(":/icons/dialog-cancel.png"), "&Cancel");
    auto *apply  = new QPushButton(QIcon(":/icons/dialog-ok.png"), "&Apply");
    auto *help   = new QPushButton(QIcon(":/icons/help-browser.png"), "&Help");
    help->setObjectName("dump_image.html");
    cancel->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    layout->addWidget(cancel, idx, 0, 1, MAXCOLS / 3, Qt::AlignHCenter);
    layout->addWidget(apply, idx, MAXCOLS / 3, 1, MAXCOLS / 3, Qt::AlignHCenter);
    layout->addWidget(help, idx, 2 * (MAXCOLS / 3), 1, MAXCOLS / 3, Qt::AlignHCenter);
    connect(cancel, &QPushButton::released, &fixview, &QDialog::reject);
    connect(apply, &QPushButton::released, &fixview, &QDialog::accept);
    connect(help, &QPushButton::released, this, &ImageViewer::getHelp);
    fixview.setLayout(layout);

    int rv = fixview.exec();

    // return immediately on cancel
    if (!rv) return;

    // retrieve compute data from dialog and store in map
    for (int idx = computes_offset; idx < computes_offset + (int)computes.size(); ++idx) {
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
        computes[id]->enabled    = (box->isChecked());
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
    for (int idx = fixes_offset; idx < fixes_offset + (int)fixes.size(); ++idx) {
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
        fixes[id]->enabled    = (box->isChecked());
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

void ImageViewer::regionSettings()
{
    updateRegions();
    if (regions.size() == 0) return;
    QDialog regionview;
    regionview.setWindowTitle(QString("LAMMPS-GUI - Visualize Regions"));
    regionview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    regionview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    regionview.setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);

    int idx     = 0;
    int n       = 0;
    auto *title = new QLabel("Visualize Regions:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);

    constexpr int MAXCOLS = 7;
    auto *layout          = new QGridLayout;
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    layout->addWidget(title, idx++, 0, 1, MAXCOLS, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    layout->addWidget(new QLabel("Region ID:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Show:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Style:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Color:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Size:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("# Points:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Opacity:"), idx++, n++, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *colorcompleter = new QColorCompleter(this);
    auto *colorvalidator = new QColorValidator(this);
    auto *framevalidator = new QDoubleValidator(1.0e-10, 1.0e10, 10, this);
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 3, this);
    auto *pointvalidator = new QIntValidator(100, 1000000, this);
    QFontMetrics metrics(regionview.fontMetrics());

    for (const auto &reg : regions) {
        n = 0;
        layout->addWidget(new QLabel(reg.first.c_str()), idx, n++);
        layout->setObjectName(QString(reg.first.c_str()));

        auto *check = new QCheckBox("");
        check->setChecked(reg.second->enabled);
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

    n = 0;
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *bottomlayout = new QHBoxLayout;
    bottomlayout->setSpacing(LAYOUT_SPACING);
    auto *cancel = new QPushButton(QIcon(":/icons/dialog-cancel.png"), "&Cancel");
    auto *apply  = new QPushButton(QIcon(":/icons/dialog-ok.png"), "&Apply");
    auto *help   = new QPushButton(QIcon(":/icons/help-browser.png"), "&Help");
    help->setObjectName("Howto_viz.html#visualizing-regions");
    cancel->setAutoDefault(false);
    help->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    apply->setFocus();
    connect(cancel, &QPushButton::released, &regionview, &QDialog::reject);
    connect(apply, &QPushButton::released, &regionview, &QDialog::accept);
    connect(help, &QPushButton::released, this, &ImageViewer::getHelp);

    bottomlayout->addWidget(cancel, Qt::AlignHCenter);
    bottomlayout->addWidget(apply, Qt::AlignHCenter);
    bottomlayout->addWidget(help, Qt::AlignHCenter);
    layout->addLayout(bottomlayout, idx, 0, 1, MAXCOLS, Qt::AlignHCenter);
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
        regions[id]->enabled = box->isChecked();
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

void ImageViewer::colorSettings()
{
    int numcolors = color_list.size();
    if (numcolors == 0) return; // color list is not initialized, nothing to do
    int numtypes = lammps->extractSetting("ntypes");
    if (numtypes < 1) return; // nothing to do

    QDialog colorview;
    colorview.setWindowTitle(QString("LAMMPS-GUI - Atom Type Colors"));
    colorview.setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    colorview.setContentsMargins(CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN, CONTENT_MARGIN);
    QFontMetrics metrics(colorview.fontMetrics());

    // Main outer layout for the dialog (title + scroll area + buttons)
    auto *mainLayout = new QVBoxLayout(&colorview);
    mainLayout->setSpacing(LAYOUT_SPACING);

    // Fixed title outside the scroll area
    auto *title = new QLabel("Customize colors:");
    title->setFrameStyle(QFrame::Panel | QFrame::Raised);
    title->setLineWidth(1);
    title->setMargin(TITLE_MARGIN);
    mainLayout->addWidget(title, 0, Qt::AlignHCenter);
    mainLayout->addWidget(new QHline);

    // Scrollable area: column headers + color-editing rows
    constexpr int MAXCOLS = 5;

    int idx      = 0;
    int n        = 0;
    auto *layout = new QGridLayout;
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    layout->addWidget(new QLabel("Type:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel(""), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Red:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Green:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Blue:"), idx++, n++, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *rgbvalidator = new QDoubleValidator(0.0, 1.0, 3, &colorview);

    // record the row index where the colors start
    int colorstart = idx - 1;

    for (int i = 0; i < numtypes; ++i) {
        int icolor = i % numcolors;
        auto red   = color_list[icolor].redF();
        auto green = color_list[icolor].greenF();
        auto blue  = color_list[icolor].blueF();

        n       = 0;
        auto *t = new QLabel(QString::number(i + 1));
        t->setFixedSize(metrics.averageCharWidth() * 4, metrics.height() + 4);
        t->setAlignment(Qt::AlignRight);
        layout->addWidget(t, idx, n++, Qt::AlignHCenter);

        auto *icon = new QLabel("");
        icon->setPixmap(color_icon(QColor::fromRgbF(red, green, blue)));
        icon->setFrameStyle(QFrame::Panel | QFrame::Raised);

        auto iconhint = icon->minimumSizeHint();
        icon->setMinimumSize(iconhint);
        icon->setMaximumSize(iconhint);

        layout->addWidget(icon, idx, n++, Qt::AlignHCenter);

        auto *r = new QLineEdit(QString::number(red, 'f', 3));
        r->setValidator(rgbvalidator);
        r->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        r->setObjectName(QString("red%1").arg(i + 1));
        layout->addWidget(r, idx, n++);

        auto *g = new QLineEdit(QString::number(green, 'f', 3));
        g->setValidator(rgbvalidator);
        g->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        g->setObjectName(QString("green%1").arg(i + 1));
        layout->addWidget(g, idx, n++);

        auto *b = new QLineEdit(QString::number(blue, 'f', 3));
        b->setValidator(rgbvalidator);
        b->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        b->setObjectName(QString("blue%1").arg(i + 1));
        layout->addWidget(b, idx++, n++);
    }

    auto *scrollWidget = new QWidget;
    scrollWidget->setLayout(layout);
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidget(scrollWidget);
    scrollArea->setWidgetResizable(true);
    mainLayout->addWidget(scrollArea, 1);

    // Fixed buttons outside the scroll area
    mainLayout->addWidget(new QHline);

    // Load/Save JSON row (above Cancel/Apply/Reset)
    auto *jsonlayout = new QHBoxLayout;
    jsonlayout->setSpacing(LAYOUT_SPACING);
    auto *loadJson = new QPushButton(QIcon(":/icons/document-open.png"), "&Load from JSON...");
    auto *saveJson = new QPushButton(QIcon(":/icons/document-save.png"), "&Save to JSON...");
    loadJson->setAutoDefault(false);
    saveJson->setAutoDefault(false);
    jsonlayout->addWidget(loadJson, Qt::AlignHCenter);
    jsonlayout->addWidget(saveJson, Qt::AlignHCenter);
    mainLayout->addLayout(jsonlayout);

    auto *bottomlayout = new QHBoxLayout;
    bottomlayout->setSpacing(LAYOUT_SPACING);
    auto *cancel = new QPushButton(QIcon(":/icons/dialog-cancel.png"), "&Cancel");
    auto *apply  = new QPushButton(QIcon(":/icons/dialog-ok.png"), "&Apply");
    auto *reset  = new QPushButton(QIcon(":/icons/system-restart.png"), "&Reset");
    reset->setObjectName("dump_image.html");
    cancel->setAutoDefault(false);
    reset->setAutoDefault(false);
    apply->setAutoDefault(true);
    apply->setDefault(true);
    apply->setFocus();
    auto *mydialog = &colorview;
    connect(cancel, &QPushButton::released, &colorview, &QDialog::reject);
    connect(apply, &QPushButton::released, &colorview, &QDialog::accept);
    connect(reset, &QPushButton::released, this, [mydialog]() {
        mydialog->done(RESET_ALL_COLORS);
    });

    // Connect Load JSON button: read a JSON file and update dialog widgets
    connect(loadJson, &QPushButton::released, &colorview,
            [&colorview, layout, colorstart, numtypes, this]() {
                // read and validate file
                auto root = loadJsonColors(&colorview);
                if (root.isEmpty()) return;

                auto arr = root.value("colors").toArray();
                if (arr.isEmpty()) return;

                for (int i = 1; i <= numtypes; ++i) {
                    auto obj = arr[(i - 1) % arr.size()].toObject();
                    double r = std::clamp(obj.value("red").toDouble(1.0), 0.0, 1.0);
                    double g = std::clamp(obj.value("green").toDouble(1.0), 0.0, 1.0);
                    double b = std::clamp(obj.value("blue").toDouble(1.0), 0.0, 1.0);

                    auto *iconItem = layout->itemAtPosition(i + colorstart, 1);
                    if (auto *lbl = qobject_cast<QLabel *>(iconItem ? iconItem->widget() : nullptr))
                        lbl->setPixmap(color_icon(QColor::fromRgbF(r, g, b)));

                    auto *item = layout->itemAtPosition(i + colorstart, 2);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(QString::number(r, 'f', 3));
                    item = layout->itemAtPosition(i + colorstart, 3);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(QString::number(g, 'f', 3));
                    item = layout->itemAtPosition(i + colorstart, 4);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(QString::number(b, 'f', 3));
                }

                auto lights = root.value("lights").toObject();
                if (lights.isEmpty()) return;
                ambientlight = lights.value("ambient").toDouble();
                keylight     = lights.value("key").toDouble();
                filllight    = lights.value("fill").toDouble();
                backlight    = lights.value("back").toDouble();
            });

    // Connect Save JSON button: read current dialog widget values and save to JSON
    connect(saveJson, &QPushButton::released, &colorview,
            [&colorview, layout, colorstart, numtypes, this]() {
                QJsonArray colors;
                for (int i = 1; i <= numtypes; ++i) {
                    double r = 1.0, g = 1.0, b = 1.0;
                    auto *item = layout->itemAtPosition(i + colorstart, 2);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) r = w->text().toDouble();
                    item = layout->itemAtPosition(i + colorstart, 3);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) g = w->text().toDouble();
                    item = layout->itemAtPosition(i + colorstart, 4);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) b = w->text().toDouble();
                    QJsonObject obj;
                    obj["red"]   = r;
                    obj["green"] = g;
                    obj["blue"]  = b;
                    colors.append(obj);
                }
                QJsonObject lights;
                lights["ambient"] = ambientlight;
                lights["key"]     = keylight;
                lights["fill"]    = filllight;
                lights["back"]    = backlight;

                QJsonObject root;
                root["colors"] = colors;
                root["lights"] = lights;
                saveJsonColors(&colorview, colors, lights);
            });

    bottomlayout->addWidget(cancel, Qt::AlignHCenter);
    bottomlayout->addWidget(apply, Qt::AlignHCenter);
    bottomlayout->addWidget(reset, Qt::AlignHCenter);
    mainLayout->addLayout(bottomlayout);

    // Size the dialog relative to screen dimensions (same approach as AboutDialog)
    auto *screen = QGuiApplication::primaryScreen();
    if (screen) {
        auto screenSize = screen->availableSize();
        int rowHeight   = metrics.height() + 8;
        // Estimate total desired height: fixed overhead + one row per atom type
        int desiredHeight = rowHeight * (numtypes + 5) + 5 * (LAYOUT_SPACING + CONTENT_MARGIN);
        desiredHeight += title->sizeHint().height() + 2 * cancel->sizeHint().height();
        int desiredWidth = std::max(
            MINIMUM_WIDTH, metrics.averageCharWidth() * 8 * 3 +
                               scrollArea->verticalScrollBar()->sizeHint().width() + EXTRA_WIDTH);
        int maxWidth  = std::min(desiredWidth, screenSize.width() * 3 / 4);
        int maxHeight = std::min(desiredHeight, screenSize.height() * 4 / 5);
        colorview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
        colorview.resize(maxWidth, maxHeight);
    } else {
        colorview.setMinimumSize(MINIMUM_WIDTH, MINIMUM_HEIGHT);
    }

    int cv = colorview.exec();

    // return immediately on cancel
    if (!cv) return;

    if (cv == RESET_ALL_COLORS) {
        resetColors();
    } else {
        // expand color_list if more types than existing colors
        int old_size = color_list.size();
        for (int i = color_list.size(); i < numtypes; ++i)
            color_list.append(color_list[i % old_size]);

        for (int i = 1; i <= numtypes; ++i) {
            double r = color_list[i - 1].redF();
            double g = color_list[i - 1].greenF();
            double b = color_list[i - 1].blueF();

            auto *item = layout->itemAtPosition(i + colorstart, 2);
            auto *rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
            if (rgb && rgb->hasAcceptableInput()) r = rgb->text().toDouble();

            item = layout->itemAtPosition(i + colorstart, 3);
            rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
            if (rgb && rgb->hasAcceptableInput()) g = rgb->text().toDouble();

            item = layout->itemAtPosition(i + colorstart, 4);
            rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
            if (rgb && rgb->hasAcceptableInput()) b = rgb->text().toDouble();

            color_list[i - 1] = QColor::fromRgbF(r, g, b);
        }
    }
    createImage();
}

// our custom list of default colors for per-type colors
constexpr double DEFAULT_RGB[][3] = {
    {0.9, 0.0, 0.0}, {0.0, 0.9, 0.0}, {0.0, 0.0, 0.9}, {0.9, 0.9, 0.0}, {0.0, 0.9, 0.9},
    {0.9, 0.0, 0.9}, {0.9, 0.5, 0.0}, {0.5, 0.0, 0.9}, {0.8, 0.0, 0.4}, {0.0, 0.9, 0.5},
    {0.5, 0.2, 0.2}, {0.0, 0.5, 0.0}, {0.1, 0.1, 0.5}, {0.6, 0.9, 0.0}, {0.0, 0.5, 0.9},
    {0.5, 0.5, 0.0}, {0.0, 0.5, 0.5}, {0.5, 0.0, 0.5}, {0.9, 0.9, 0.9}, {0.5, 0.5, 0.5},
    {0.2, 0.2, 0.2}};

void ImageViewer::resetColors()
{
    color_list.clear();
    for (const auto &rgb : DEFAULT_RGB)
        color_list.append(QColor::fromRgbF(rgb[0], rgb[1], rgb[2]));

    ambientlight = 0.0;
    keylight     = 0.9;
    filllight    = 0.45;
    backlight    = 0.9;
}

void ImageViewer::loadColors()
{
    auto root = loadJsonColors(this);
    if (root.isEmpty()) return;

    auto arr = root["colors"].toArray();
    if (arr.isEmpty()) return;

    color_list.clear();
    for (const auto &item : arr) {
        auto obj = item.toObject();
        double r = std::clamp(obj.value("red").toDouble(1.0), 0.0, 1.0);
        double g = std::clamp(obj.value("green").toDouble(1.0), 0.0, 1.0);
        double b = std::clamp(obj.value("blue").toDouble(1.0), 0.0, 1.0);
        color_list.append(QColor::fromRgbF(r, g, b));
    }
    if (color_list.isEmpty()) resetColors();

    auto lights = root.value("lights").toObject();
    if (!lights.isEmpty()) {
        ambientlight = lights.value("ambient").toDouble();
        keylight     = lights.value("key").toDouble();
        filllight    = lights.value("fill").toDouble();
        backlight    = lights.value("back").toDouble();
    }

    createImage();
}

void ImageViewer::saveColors()
{
    QJsonArray colors;
    for (const auto &c : color_list) {
        QJsonObject obj;
        obj["red"]   = c.redF();
        obj["green"] = c.greenF();
        obj["blue"]  = c.blueF();
        colors.append(obj);
    }
    QJsonObject lights;
    lights["ambient"] = ambientlight;
    lights["key"]     = keylight;
    lights["fill"]    = filllight;
    lights["back"]    = backlight;

    saveJsonColors(this, colors, lights);
}

void ImageViewer::changeGroup(int)
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

void ImageViewer::changeMolecule(int)
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
        // don't handle any more key press events after entering destructor
        if (shutdown) return false;
        QKeyEvent *kev = static_cast<QKeyEvent *>(event);
        if ((kev->key() == Qt::Key_G) && (kev->modifiers() == Qt::AltModifier)) {
            auto *box = findChild<QComboBox *>("molecule");
            if (box) box->hidePopup();

            box = findChild<QComboBox *>("group");
            if (box) {
                box->setFocus();
                box->showPopup();
                return true;
            } else
                return false;
        } else if ((kev->key() == Qt::Key_M) && (kev->modifiers() == Qt::AltModifier)) {
            auto *box = findChild<QComboBox *>("group");
            if (box) box->hidePopup();

            box = findChild<QComboBox *>("molecule");
            if (box) {
                box->setFocus();
                box->showPopup();
                return true;
            } else
                return false;
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
            } else
                return false;
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
            } else
                return false;
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
    // no point in trying to update the image when triggered after the destructor started
    if (shutdown) return;

    auto *renderstatus = findChild<QLabel *>("renderstatus");
    if (renderstatus) renderstatus->setEnabled(true);
    repaint();

    QString oldgroup = group;
    if (molecule != "none") {
        // get center of box
        double *boxlo, *boxhi, xmid, ymid, zmid;
        boxlo = (double *)lammps->extractGlobal("boxlo");
        boxhi = (double *)lammps->extractGlobal("boxhi");
        if (boxlo && boxhi) {
            xmid = 0.5 * (boxhi[0] + boxlo[0]);
            ymid = 0.5 * (boxhi[1] + boxlo[1]);
            zmid = 0.5 * (boxhi[2] + boxlo[2]);
        } else {
            xmid = ymid = zmid = 0.0;
        }

        silenceStdout();
        QString molcreate = "create_atoms 0 single %1 %2 %3 mol %4 312944 group %5 units box";
        group             = "imgviewer_tmp_mol";
        lammps->command(molcreate.arg(xmid).arg(ymid).arg(zmid).arg(molecule).arg(group));
        lammps->command(QString("neigh_modify exclude group all %1").arg(group));
        lammps->command("run 0 post no");
        restoreStdout();
        if (lammps->hasError()) lammps->getLastErrorMessage(nullptr, 0);
    }

    QSettings settings;
    // attempt to clean up if a previous write_dump command failed
    lammps->command("if $(is_defined(dump,WRITE_DUMP)) then 'undump WRITE_DUMP'");
    QString dumpcmd = QString("write_dump ") + group + " image ";
    QDir dumpdir(QDir::tempPath());
    QFile dumpfile(dumpdir.absoluteFilePath(filename + ".ppm"));
    dumpcmd += "'" + dumpfile.fileName() + "'";

    int hhrot = (hrot > 180) ? 360 - hrot : hrot;

    // determine elements from masses and set their covalent radii
    int ntypes             = lammps->extractSetting("ntypes");
    int nbondtypes         = lammps->extractSetting("nbondtypes");
    auto *masses           = (double *)lammps->extractAtom("mass");
    const char *pair_style = (const char *)lammps->extractGlobal("pair_style");
    QString units          = (const char *)lammps->extractGlobal("units");
    QString elements{"element "};
    QString adiams;

    // detect if we can use element information, otherwise set atom color to "type" by default
    useelements = false;
    if (masses && ((units == "real") || (units == "metal"))) {
        useelements = true;
        if (!atomcustom) atomcolor = "element";
        for (int i = 1; i <= ntypes; ++i) {
            int idx = get_pte_from_mass(masses[i]);
            if (idx == 0) useelements = false;
            elements += QString(pte_label[idx]) + blank;
            adiams += QString("adiam %1 %2 ").arg(i).arg(vdwfactor * pte_vdw_radius[idx]);
        }
    } else {
        if (!atomcustom) atomcolor = "type";
    }

    usediameter = lammps->extractSetting("radius_flag") != 0;
    usesigma    = false;
    // if we cannot use element info or diameter data, try to use Lennard-Jones sigma for radius
    if (!useelements && !usediameter && pair_style && (strncmp(pair_style, "lj/", 3) == 0)) {
        auto **sigma = (double **)lammps->extractPair("sigma");
        if (sigma) {
            usesigma = true;
            for (int i = 1; i <= ntypes; ++i) {
                if (sigma[i][i] > 0.0)
                    adiams += QString("adiam %1 %2 ").arg(i).arg(vdwfactor * sigma[i][i]);
            }
        }
    }

    // adjust atomsize setting and pushbutton state and reset adiams string, if needed
    if (showatoms) {
        auto *edit   = findChild<QLineEdit *>("atomSize");
        auto *label  = findChild<QLabel *>("AtomLabel");
        auto *button = findChild<QPushButton *>("vdw");
        if (edit && label && button) {
            button->setEnabled(true);
            if (atomcustom) {
                if ((atomdiam != "element") && (atomdiam != "diameter") && (atomdiam != "sigma")) {
                    edit->setEnabled(true);
                    edit->show();
                    label->setEnabled(true);
                    label->show();
                    adiams.clear();
                    for (int i = 1; i <= ntypes; ++i) {
                        adiams += QString("adiam %1 %2 ").arg(i).arg(vdwfactor * atomSize);
                    }
                } else {
                    edit->setEnabled(false);
                    edit->hide();
                    label->setEnabled(false);
                    label->hide();
                }
            } else {
                if (useelements || usediameter || usesigma) {
                    edit->setEnabled(false);
                    edit->hide();
                    label->setEnabled(false);
                    label->hide();
                } else {
                    edit->setEnabled(true);
                    edit->show();
                    label->setEnabled(true);
                    label->show();
                }
            }
        }
    } else {
        adiams.clear();
        auto *edit   = findChild<QLineEdit *>("atomSize");
        auto *label  = findChild<QLabel *>("AtomLabel");
        auto *button = findChild<QPushButton *>("vdw");
        if (edit && label && button) {
            button->setEnabled(true);
            edit->setEnabled(false);
            edit->hide();
            label->setEnabled(false);
            label->hide();
        }
    }

    // atom color for dump
    if (!atomcustom && useelements)
        dumpcmd += blank + "element";
    else
        dumpcmd += blank + atomcolor;

    bool do_vdw = vdwfactor > VDW_CUT;
    // atom diameter for dump
    if (!atomcustom) {
        if (usediameter && do_vdw)
            dumpcmd += blank + "diameter";
        else
            dumpcmd += " type";
    } else {
        if ((atomdiam == "diameter") && usediameter && do_vdw)
            dumpcmd += blank + "diameter";
        else
            dumpcmd += " type";
    }

    if (!showatoms) dumpcmd += " atom no";
    if (showbodies && (lammps->extractSetting("body_flag") == 1)) {
        dumpcmd += QString(" body %1 %2 %3").arg(bodycolor).arg(bodydiam).arg(bodyflag);
    } else if (showlines && (lammps->extractSetting("line_flag") == 1))
        dumpcmd += QString(" line %1 %2").arg(linecolor).arg(linediam);
    else if (showtris && (lammps->extractSetting("tri_flag") == 1))
        dumpcmd += QString(" tri %1 %2 %3").arg(tricolor).arg(triflag).arg(tridiam);
    else if (showellipsoids && (lammps->extractSetting("ellipsoid_flag") == 1)) {
        dumpcmd += QString(" ellipsoid %1 %2 %3 %4")
                       .arg(ellipsoidcolor)
                       .arg(ellipsoidflag)
                       .arg(ellipsoidlevel)
                       .arg(ellipsoiddiam);
    }
    dumpcmd += QString(" size %1 %2").arg(xsize).arg(ysize);
    dumpcmd += QString(" zoom %1").arg(zoom);
    dumpcmd += QString(" shiny %1 ").arg(shinyfactor);
    dumpcmd += QString(" fsaa %1").arg(antialias ? "yes" : "no");
    if (nbondtypes > 0) {
        if (do_vdw || !showbonds)
            dumpcmd += " bond none none ";
        else
            dumpcmd += QString(" bond %1 %2 ").arg(bondcolor).arg(bonddiam);
    }
    if (lammps->extractSetting("dimension") == 3) {
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

    if (autobond && pair_style && (strcmp(pair_style, "none") != 0)) {
        // use custom bond diameter value, if present
        QRegularExpression validnum(R"((^\d+\.?\d*|^\d*\.?\d+))");
        auto match = validnum.match(bonddiam);
        if (match.hasMatch()) {
            dumpcmd += blank + "autobond" + blank + QString::number(bondcutoff) + blank + bonddiam;
        } else {
            dumpcmd += blank + "autobond" + blank + QString::number(bondcutoff) + " 0.5";
        }
    }

    if (regions.size() > 0) {
        for (const auto &reg : regions) {
            if (reg.second->enabled) {
                QString id(reg.first.c_str());
                QString color(reg.second->color.c_str());
                switch (reg.second->style) {
                    case FRAME:
                        dumpcmd += " region " + id + blank + color;
                        dumpcmd += " frame " + QString::number(reg.second->diameter);
                        if (lammps->version() > 20260330)
                            dumpcmd += " hull_points " + QString::number(reg.second->npoints);
                        break;
                    case FILLED:
                        dumpcmd += " region " + id + blank + color + " filled";
                        if (lammps->version() > 20260330)
                            dumpcmd += " hull_points " + QString::number(reg.second->npoints);
                        break;
                    case TRANSPARENT:
                        dumpcmd += " region " + id + blank + color;
                        dumpcmd += " transparent " + QString::number(reg.second->opacity);
                        if (lammps->version() > 20260330)
                            dumpcmd += " hull_points " + QString::number(reg.second->npoints);
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
    dumpcmd += " modify";

    // must change global colors first so they apply everywhere
    int numcolors = color_list.size();

    for (int i = 0; i < numcolors; ++i) {
        auto namekey = QString("type%1").arg(i + 1);
        dumpcmd += QString(" color %1 %2 %3 %4")
                       .arg(namekey)
                       .arg(color_list[i].redF())
                       .arg(color_list[i].greenF())
                       .arg(color_list[i].blueF());
    }
    int numtypes = lammps->extractSetting("ntypes");
    for (int i = 1; i <= numtypes; ++i) {
        int icolor   = ((i - 1) % numcolors) + 1;
        auto namekey = QString("type%1").arg(icolor);
        dumpcmd += QString(" acolor %1 %2").arg(i).arg(namekey);
    }

    dumpcmd += " boxcolor " + boxcolor;
    dumpcmd += " backcolor " + backcolor;
    dumpcmd += " backcolor2 " + backcolor2;
    dumpcmd += QString(" axestrans %1").arg(axestrans);
    dumpcmd += QString(" boxtrans %1").arg(boxtrans);
    dumpcmd += QString(" atrans * %1").arg(atomtrans);
    if (lammps->extractSetting("bond_flag") == 1) dumpcmd += QString(" btrans * %1").arg(atomtrans);
    if (lammps->version() > 20260330)
        dumpcmd += QString(" lights %1 %2 %3 %4")
                       .arg(ambientlight)
                       .arg(keylight)
                       .arg(filllight)
                       .arg(backlight);

    if (useelements) dumpcmd += blank + elements + blank + adiams + blank;
    if (usesigma) dumpcmd += blank + adiams + blank;
    if (!useelements && !usesigma && (atomSize != 1.0)) dumpcmd += blank + adiams + blank;
    // apply selected colormap
    QString mmin = mapmin;
    if (mmin == "auto") mmin = "min";
    QString mmax = mapmax;
    if (mmax == "auto") mmax = "max";
    if (colormap == "RWB") {
        dumpcmd += " color map1 0.459 0.055 0.075";
        dumpcmd += " color map2 0.000 0.227 0.427";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "5 min map1 0.1 map1 0.5 white 0.9 map2 max map2";
    } else if (colormap == "PWT") {
        dumpcmd += " color map1 0.286 0.114 0.553";
        dumpcmd += " color map2 0.000 0.255 0.267";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "5 min map1 0.1 map1 0.5 white 0.9 map2 max map2";
    } else if (colormap == "BGR") {
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "5 min blue 0.05 blue 0.5 green 0.95 red max red";
    } else if (colormap == "BWG") {
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "5 min blue 0.1 blue 0.5 white 0.9 green max green";
    } else if (colormap == "Grayscale") {
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "2 min black max white";
    } else if (colormap == "Rainbow") {
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "6 min red 0.25 yellow 0.45 green 0.65 cyan 0.85 blue max purple";
    } else if (colormap == "Sequential") {
        dumpcmd += " color map1 0.808 0.808 0.808";
        dumpcmd += " color map2 0.647 0.349 0.667";
        dumpcmd += " color map3 0.349 0.659 0.612";
        dumpcmd += " color map4 0.941 0.772 0.443";
        dumpcmd += " color map5 0.878 0.169 0.208";
        dumpcmd += " color map6 0.031 0.165 0.329";
        dumpcmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        dumpcmd += "6 map1 map2 map3 map4 map5 map6";
    } else if (colormap == "Landscape") {
        dumpcmd += " color map0 0.145 0.400 0.463";
        dumpcmd += " color map1 0.392 0.867 0.588";
        dumpcmd += " color map2 0.572 0.192 0.141";
        dumpcmd += " color map3 0.392 0.831 0.992";
        dumpcmd += " color map4 0.020 0.431 0.071";
        dumpcmd += " color map5 0.992 0.349 0.145";
        dumpcmd += " color map6 0.275 0.953 0.243";
        dumpcmd += " color map7 0.729 0.525 0.361";
        dumpcmd += " color map8 0.780 0.867 0.529";
        dumpcmd += " color map9 0.243 0.298 0.078";
        dumpcmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        dumpcmd += "10 map0 map1 map2 map3 map4 map5 map6 map7 map8 map9";
    } else if (colormap == "Basic") {
        dumpcmd += QString(" amap %1 %2 sa 1.0 ").arg(mmin).arg(mmax);
        dumpcmd += "10 red cyan green black magenta blue yellow purple white orange";
    } else if (colormap == "Teal") {
        dumpcmd += " color map1 0.071 0.153 0.251";
        dumpcmd += " color map2 0.106 0.282 0.369";
        dumpcmd += " color map3 0.337 0.545 0.529";
        dumpcmd += " color map4 0.710 0.820 0.682";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "4 min map1 0.25 map2 0.5 map3 max map4";
    } else if (colormap == "Viridis") {
        dumpcmd += " color map1 0.282 0.129 0.451";
        dumpcmd += " color map2 0.435 0.435 0.556";
        dumpcmd += " color map3 0.161 0.686 0.498";
        dumpcmd += " color map4 0.741 0.875 0.149";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "4 min map1 0.333 map2 0.667 map3 max map4";
    } else if (colormap == "Inferno") {
        dumpcmd += " color map1 0.032 0.032 0.048";
        dumpcmd += " color map2 0.318 0.071 0.486";
        dumpcmd += " color map3 0.718 0.216 0.475";
        dumpcmd += " color map4 0.988 0.537 0.380";
        dumpcmd += " color map5 0.988 0.992 0.749";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "5 min map1 0.25 map2 0.5 map3 0.75 map4 max map5";
    } else if (colormap == "Plasma") {
        dumpcmd += " color map1 0.051 0.031 0.529";
        dumpcmd += " color map2 0.612 0.090 0.620";
        dumpcmd += " color map3 0.929 0.475 0.325";
        dumpcmd += " color map4 0.941 0.976 0.129";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "4 min map1 0.333 map2 0.667 map3 max map4";
    } else { // default is "BWR"
        dumpcmd += " color map1 0.000 0.227 0.427";
        dumpcmd += " color map2 0.459 0.055 0.075";
        dumpcmd += QString(" amap %1 %2 cf 0.0 ").arg(mmin).arg(mmax);
        dumpcmd += "3 min map1 0.5 white max map2";
    }

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

    silenceStdout();
    last_dump_cmd = dumpcmd;
    lammps->command(dumpcmd);
    restoreStdout();

    // display error message
    if (lammps->hasError()) {
        char errormesg[DEFAULT_BUFLEN];
        lammps->getLastErrorMessage(errormesg, DEFAULT_BUFLEN);
        // ignore "Invalid LAMMPS handle", but report other errors
        if (!strstr(errormesg, "Invalid LAMMPS handle"))
            warning(this, "Image Viewer File Creation Error", "LAMMPS failed to create the image:",
                    QString("<code>%1</code>").arg(errormesg));
        return;
    }

    QImageReader reader(dumpfile.fileName());
    reader.setAutoTransform(true);
    const QImage newImage = reader.read();
    dumpfile.remove();

    // read of new image failed. nothing left to do.
    if (newImage.isNull()) return;

    // show show image
    image = newImage;
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->setMinimumSize(image.width(), image.height());
    imageLabel->resize(image.width(), image.height());
    adjustWindowSize();
    if (renderstatus) renderstatus->setEnabled(false);
    repaint();
    adjustWindowSize();

    if (molecule != "none") {
        lammps->command("neigh_modify exclude none");
        lammps->command(QString("delete_atoms group %1 compress no").arg(group));
        lammps->command(QString("group %1 delete").arg(group));
        group = oldgroup;
    }
}

void ImageViewer::saveAs()
{
    exportImage(this, &image, "ImageViewer");
}

void ImageViewer::copy()
{
#if QT_CONFIG(clipboard)
    auto *clip = QGuiApplication::clipboard();
    if (clip && !image.isNull()) {
        clip->setImage(image, QClipboard::Clipboard);
        if (clip->supportsSelection()) clip->setImage(image, QClipboard::Selection);
    } else
        fprintf(stderr, "Copy image to clipboard currently not available\n");
#else
    fprintf(stderr, "Copy image to clipboard not supported on this platform\n");
#endif
}

void ImageViewer::quit()
{
    if (lammpsgui) lammpsgui->quit();
}

void ImageViewer::getHelp()
{
    auto *src = sender();
    if (src) {
        QString page   = src->objectName();
        QString docver = "/";
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

        if (page == "visualization.html") {
            // lammps-gui docs
            QDesktopServices::openUrl(QUrl(QString("https://lammps-gui.lammps.org/%1").arg(page)));
        } else if (page.startsWith("compute_") || page.startsWith("fix_")) {
            // jump to the "Dump image info" section for computes and fixes
            QDesktopServices::openUrl(
                QUrl(QString("https://docs.lammps.org%1%2#dump-image-info").arg(docver).arg(page)));
        } else {
            // general LAMMPS doc page
            QDesktopServices::openUrl(
                QUrl(QString("https://docs.lammps.org%1%2").arg(docver).arg(page)));
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
    copyAct = fileMenu->addAction("Copy &Image", this, &ImageViewer::copy);
    copyAct->setIcon(QIcon(":/icons/edit-copy.png"));
    copyAct->setShortcut(QKeySequence::Copy);
    copyAct->setEnabled(false);
    cmdAct = fileMenu->addAction("Copy &dump image command", this, &ImageViewer::cmdToClipboard);
    cmdAct->setIcon(QIcon(":/icons/file-clipboard.png"));
    cmdAct->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    fileMenu->addSeparator();
    QAction *loadColorsAct =
        fileMenu->addAction("&Load Colors from JSON...", this, &ImageViewer::loadColors);
    loadColorsAct->setIcon(QIcon(":/icons/document-open.png"));
    QAction *saveColorsAct =
        fileMenu->addAction("S&ave Colors to JSON...", this, &ImageViewer::saveColors);
    saveColorsAct->setIcon(QIcon(":/icons/document-save.png"));
    QAction *resetColorsAct = fileMenu->addAction("&Reset Colors");
    connect(resetColorsAct, &QAction::triggered, this, [this]() {
        resetColors();
        createImage();
    });
    resetColorsAct->setIcon(QIcon(":/icons/system-restart.png"));
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

    auto *hbar        = scrollArea->horizontalScrollBar();
    auto *vbar        = scrollArea->verticalScrollBar();
    int desiredWidth  = image.width() + 2 + (vbar->isVisible() ? vbar->width() : 0);
    int desiredHeight = image.height() + 2 + (hbar->isVisible() ? hbar->height() : 0);

    // make sure the scroll area is not resized beyond a certain fraction of the screen
    auto *screen = QGuiApplication::primaryScreen();
    if (screen) {
        auto screenSize = screen->availableSize();
        desiredWidth    = std::min(desiredWidth, (screenSize.width() * 4 / 5) - EXTRA_WIDTH);
        desiredHeight   = std::min(desiredHeight, (screenSize.height() * 9 / 10) - EXTRA_HEIGHT);
    }
    scrollArea->setMinimumSize(desiredWidth, desiredHeight);
    scrollArea->resize(desiredWidth, desiredHeight);
    adjustSize();
}

void ImageViewer::updatePeratom()
{
    atom_properties.clear();
    if (useelements) atom_properties << "element";
    atom_properties << "type";

    if (lammps) {
        silenceStdout();

        if (lammps->extractSetting("molecule_flag")) atom_properties << "mol";
        if (lammps->extractSetting("q_flag")) atom_properties << "q";
        if (lammps->extractSetting("mu_flag")) atom_properties << "mu";
        if (lammps->extractSetting("radius_flag")) atom_properties << "diameter";

        void *ptr = nullptr;
        int type  = 0;
        char name[256];

        // add atom style variables to the list
        int num = lammps->idCount("variable");
        for (int idx = 0; idx < num; ++idx) {
            lammps->idName("variable", idx, name, 256);
            type = lammps->extractVariableDatatype(name);
            if (type == LammpsWrapper::ATOM_STYLE) atom_properties << QString("v_%1").arg(name);
        }

        // add compatible computes to the list
        num = lammps->idCount("compute");
        for (int idx = 0; idx < num; ++idx) {
            lammps->idName("compute", idx, name, 256);
            ptr =
                lammps->extractCompute(name, LammpsWrapper::ATOM_STYLE, LammpsWrapper::VECTOR_TYPE);
            if (ptr) {
                atom_properties << QString("c_%1").arg(name);
                continue;
            }
            ptr =
                lammps->extractCompute(name, LammpsWrapper::ATOM_STYLE, LammpsWrapper::ARRAY_TYPE);
            if (ptr) {
                ptr  = lammps->extractCompute(name, LammpsWrapper::ATOM_STYLE,
                                              LammpsWrapper::NUM_COLS);
                type = *(int *)ptr;
                for (int col = 1; col <= type; ++col) {
                    atom_properties << QString("c_%1[%2]").arg(name).arg(col);
                }
                continue;
            }

            // clear error status, if needed:
            lammps->getLastErrorMessage(nullptr, 0);
        }

        // add compatible fixes to the list
        num = lammps->idCount("fix");
        for (int idx = 0; idx < num; ++idx) {
            lammps->idName("fix", idx, name, 256);
            ptr = lammps->extractFix(name, LammpsWrapper::ATOM_STYLE, LammpsWrapper::ARRAY_TYPE, -1,
                                     -1);
            if (ptr) {
                ptr  = lammps->extractFix(name, LammpsWrapper::ATOM_STYLE, LammpsWrapper::NUM_COLS,
                                          -1, -1);
                type = *(int *)ptr;
                if (type == 0) {
                    atom_properties << QString("f_%1").arg(name);
                } else {
                    for (int col = 1; col <= type; ++col) {
                        atom_properties << QString("f_%1[%2]").arg(name).arg(col);
                    }
                }
                continue;
            }

            // clear error status, if needed:
            lammps->getLastErrorMessage(nullptr, 0);
        }
        restoreStdout();
    }
    // some more general dump custom properties
    atom_properties << "id" << "mass" << "x" << "y" << "z" << "vx" << "vy" << "vz" << "fx"
                    << "fy"
                    << "fz";
}

void ImageViewer::updateFixes()
{
    if (!lammps) return;

    // remove any fixes that no longer exist. to avoid inconsistencies while looping
    // over the fixes, we first collect the list of missing ids and then apply it.
    std::unordered_set<std::string> oldkeys;
    for (const auto &istyle : computes) {
        if (!lammps->hasId("compute", istyle.first.c_str())) oldkeys.insert(istyle.first);
    }
    for (const auto &id : oldkeys) {
        delete computes[id];
        computes.erase(id);
    }
    oldkeys.clear();
    for (const auto &istyle : fixes) {
        if (!lammps->hasId("fix", istyle.first.c_str())) oldkeys.insert(istyle.first);
    }
    for (const auto &id : oldkeys) {
        delete fixes[id];
        fixes.erase(id);
    }

    // map compute and fix styles to their ids by parsing info command output.
    StdCapture capturer;
    capturer.beginCapture();
    lammps->command("info computes fixes");
    capturer.endCapture();
    QString styleinfo(capturer.getCapture().c_str());
    QRegularExpression infoline(
        QStringLiteral(R"(^(Compute|Fix)\[.*\]: *([^,]+), *style = ([^,]+).*)"));
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

void ImageViewer::updateRegions()
{
    if (!lammps) return;

    // remove any regions that no longer exist. to avoid inconsistencies while looping
    // over the regions, we first collect the list of missing ids and then apply it.
    std::unordered_set<std::string> oldkeys;
    for (const auto &reg : regions) {
        if (!lammps->hasId("region", reg.first.c_str())) oldkeys.insert(reg.first);
    }
    for (const auto &id : oldkeys) {
        delete regions[id];
        regions.erase(id);
    }

    // add any new regions
    char buffer[DEFAULT_BUFLEN];
    int nregions = lammps->idCount("region");
    for (int i = 0; i < nregions; ++i) {
        if (lammps->idName("region", i, buffer, DEFAULT_BUFLEN)) {
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

bool ImageViewer::hasAutobonds()
{
    if (!lammps) return false;
    const auto *pair_style = (const char *)lammps->extractGlobal("pair_style");
    if (!pair_style) return false;
    return strcmp(pair_style, "none") != 0;
}

// Local Variables:
// c-basic-offset: 4
// End:
