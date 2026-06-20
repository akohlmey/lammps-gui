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

// ImageViewer settings/visualization dialog builders, split out of imageviewer.cpp
// to keep that translation unit manageable. Shared implementation-detail symbols
// live in imageviewer_internal.h.

#include "imageviewer.h"

#include "imageviewer_internal.h"

#include "constants.h"
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
#include <QColorDialog>
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
    auto *gradient = new QCheckBox("Gradient: ", this);
    gradient->setChecked(usegradient);
    gradient->setToolTip("Blend the background from the bottom to the top color");
    layout->addWidget(gradient, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignRight);
    auto *b2color = new QLineEdit(backcolor2);
    b2color->setCompleter(colorcompleter);
    b2color->setValidator(colorvalidator);
    b2color->setMaximumWidth(fwidth);
    b2color->setEnabled(usegradient);
    connect(gradient, &QCheckBox::toggled, b2color, &QLineEdit::setEnabled);
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
    auto *ambient = new QDoubleSpinBox;
    ambient->setObjectName("ambient");
    ambient->setRange(0.0, 1.0);
    ambient->setSingleStep(0.05);
    ambient->setValue(ambientlight);
    ambient->setMaximumWidth(fwidth);
    lightlayout->addWidget(ambient, 2);
    lightlayout->addWidget(new QLabel("Key: "), 2, Qt::AlignRight);
    auto *key = new QDoubleSpinBox;
    key->setObjectName("key");
    key->setRange(0.0, 1.0);
    key->setSingleStep(0.05);
    key->setValue(keylight);
    key->setMaximumWidth(fwidth);
    lightlayout->addWidget(key, 2);
    lightlayout->addWidget(new QLabel("Fill: "), 2, Qt::AlignRight);
    auto *fill = new QDoubleSpinBox;
    fill->setObjectName("fill");
    fill->setRange(0.0, 1.0);
    fill->setSingleStep(0.05);
    fill->setValue(filllight);
    fill->setMaximumWidth(fwidth);
    lightlayout->addWidget(fill, 2);
    lightlayout->addWidget(new QLabel("Back: "), 2, Qt::AlignRight);
    auto *back = new QDoubleSpinBox;
    back->setObjectName("back");
    back->setRange(0.0, 1.0);
    back->setSingleStep(0.05);
    back->setValue(backlight);
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
    usegradient = gradient->isChecked();

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

    ambientlight = ambient->value();
    keylight     = key->value();
    filllight    = fill->value();
    backlight    = back->value();

    // update image with new settings
    createImage();
}

QComboBox *ImageViewer::makeColorCombo(const QString &current, const QString &name)
{
    auto *box = new QComboBox;
    box->addItems({"atom", "type", "index"});
    selectComboItem(box, current);
    box->setObjectName(name);
    connect(box, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
            &ImageViewer::acolorSync);
    return box;
}

QRadioButton *ImageViewer::addShapeButton(QButtonGroup *group, const QString &label, bool checked,
                                          QGridLayout *layout, int row, int &col)
{
    auto *button = new QRadioButton(label, this);
    button->setChecked(checked);
    group->addButton(button);
    layout->addWidget(button, row, col++, 1, 1, Qt::AlignCenter);
    return button;
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
    // select item that was selected the last time
    if (atomcustom) selectComboItem(acolor, atomcolor);
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
    // select item that was selected the last time
    if (atomcustom) selectComboItem(adiam, atomdiam);
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
    selectComboItem(amap, colormap);
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
    bncolor->insertSeparator(bncolor->count());
    bncolor->addItems(bondLocalAttrs); // per-bond compute attributes -> color by value
    if (atomcustom) {                  // select item that was selected the last time
        if (bondcolor == "none") {
            bondbutton->setChecked(false);
        } else {
            selectComboItem(bncolor, bondcolor);
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
            selectComboItem(bndiam, bonddiam);
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
    auto *bcolor = makeColorCombo(bodycolor, "bcolor");
    layout->addWidget(bcolor, idx, n++, 1, 1);
    auto *bgroup   = new QButtonGroup(this);
    auto *bcbutton = addShapeButton(bgroup, "Cylinders", bodyflag == CYLINDERS, layout, idx, n);
    auto *bdiam    = new QLineEdit(QString::number(bodydiam));
    bdiam->setValidator(diamvalidator);
    layout->addWidget(bdiam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *btbutton = addShapeButton(bgroup, "Triangles", bodyflag == TRIANGLES, layout, idx, n);
    auto *bbbutton = addShapeButton(bgroup, "Both", bodyflag == BOTH, layout, idx, n);
    ++idx;
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
    auto *ecolor = makeColorCombo(ellipsoidcolor, "ecolor");
    layout->addWidget(ecolor, idx, n++, 1, 1);
    auto *egroup = new QButtonGroup(this);
    auto *ecbutton =
        addShapeButton(egroup, "Cylinders", ellipsoidflag == CYLINDERS, layout, idx, n);
    auto *ediam = new QLineEdit(QString::number(ellipsoiddiam));
    ediam->setValidator(diamvalidator);
    layout->addWidget(ediam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *etbutton =
        addShapeButton(egroup, "Triangles", ellipsoidflag == TRIANGLES, layout, idx, n);
    // skip location for "Both" since ellipsoids don't need it
    ++n;
    auto *elevel = new QSpinBox;
    elevel->setRange(1, 6);
    elevel->setStepType(QAbstractSpinBox::DefaultStepType);
    elevel->setValue(ellipsoidlevel);
    elevel->setWrapping(false);
    layout->addWidget(elevel, idx, n++, 1, 1);
    ++idx;
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
    auto *lcolor = makeColorCombo(linecolor, "lcolor");
    layout->addWidget(lcolor, idx, n++, 1, 1);
    ++n;
    auto *ldiam = new QLineEdit(QString::number(linediam));
    ldiam->setValidator(diamvalidator);
    layout->addWidget(ldiam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    ++idx;
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
    auto *tcolor = makeColorCombo(tricolor, "tcolor");
    layout->addWidget(tcolor, idx, n++, 1, 1);
    auto *tgroup   = new QButtonGroup(this);
    auto *tcbutton = addShapeButton(tgroup, "Cylinders", triflag == CYLINDERS, layout, idx, n);
    auto *tdiam    = new QLineEdit(QString::number(tridiam));
    tdiam->setValidator(diamvalidator);
    layout->addWidget(tdiam, idx, n++, 1, 1, Qt::AlignVCenter | Qt::AlignLeft);
    auto *ttbutton = addShapeButton(tgroup, "Triangles", triflag == TRIANGLES, layout, idx, n);
    auto *tbbutton = addShapeButton(tgroup, "Both", triflag == BOTH, layout, idx, n);
    ++idx;
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

void ImageViewer::buildFixComputeRows(QGridLayout *layout, int &idx,
                                      const std::map<std::string, ImageInfo *> &items,
                                      const QMap<QString, QString> &helpmap)
{
    auto *colorcompleter = new QColorCompleter(this);
    auto *colorvalidator = new QColorValidator(this);
    auto *transvalidator = new QDoubleValidator(0.0, 1.0, 3, this);
    QFontMetrics metrics(fontMetrics());
    int n = 0;
    for (const auto &item : items) {
        n = 0;

        auto *label = new QLabel(item.first.c_str());
        layout->addWidget(label, idx, n++);
        layout->addWidget(new QLabel(item.second->style), idx, n++);
        auto *check = new QCheckBox("");
        check->setChecked(item.second->enabled);
        layout->addWidget(check, idx, n++, Qt::AlignHCenter);
        auto *cstyle = new QComboBox;
        cstyle->setEditable(false);
        cstyle->addItem("type");
        cstyle->addItem("element");
        cstyle->addItem("const");
        cstyle->setCurrentIndex(item.second->colorstyle);
        layout->addWidget(cstyle, idx, n++);
        auto *color = new QLineEdit(item.second->color.c_str());
        color->setCompleter(colorcompleter);
        color->setValidator(colorvalidator);
        color->setFixedSize(metrics.averageCharWidth() * 12, metrics.height() + 4);
        color->setText(item.second->color.c_str());
        layout->addWidget(color, idx, n++);
        auto *trans = new QLineEdit(QString::number(item.second->opacity));
        trans->setValidator(transvalidator);
        trans->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        trans->setText(QString::number(item.second->opacity));
        layout->addWidget(trans, idx, n++);
        auto *flag1 = new QLineEdit(QString::number(item.second->flag1));
        flag1->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        flag1->setText(QString::number(item.second->flag1));
        layout->addWidget(flag1, idx, n++);
        auto *flag2 = new QLineEdit(QString::number(item.second->flag2));
        flag2->setFixedSize(metrics.averageCharWidth() * 8, metrics.height() + 4);
        flag2->setText(QString::number(item.second->flag2));
        layout->addWidget(flag2, idx, n++);
        auto *help = new QPushButton(QIcon(":/icons/system-help.png"), "");
        help->setObjectName(helpmap.value(item.second->style, QString()));
        layout->addWidget(help, idx, n++);
        connect(help, &QPushButton::released, this, &ImageViewer::getHelp);
        ++idx;
    }
}

void ImageViewer::readFixComputeRows(QGridLayout *layout, int offset,
                                     std::map<std::string, ImageInfo *> &items)
{
    int n = 0;
    for (int idx = offset; idx < offset + static_cast<int>(items.size()); ++idx) {
        n          = 0;
        auto *item = layout->itemAtPosition(idx, n++);
        if (!item) continue;
        auto *label = qobject_cast<QLabel *>(item->widget());

        auto id = label->text().toStdString();
        // compute ID is not registered with a widget; skip rest to avoid segfault
        if (items.count(id) == 0) continue;

        ++n; // nothing to do with label for style name
        item                  = layout->itemAtPosition(idx, n++);
        auto *box             = qobject_cast<QCheckBox *>(item->widget());
        items[id]->enabled    = (box->isChecked());
        item                  = layout->itemAtPosition(idx, n++);
        auto *combo           = qobject_cast<QComboBox *>(item->widget());
        items[id]->colorstyle = combo->currentIndex();
        item                  = layout->itemAtPosition(idx, n++);
        auto *line            = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) items[id]->color = line->text().toStdString();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) items[id]->opacity = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) items[id]->flag1 = line->text().toDouble();
        item = layout->itemAtPosition(idx, n++);
        line = qobject_cast<QLineEdit *>(item->widget());
        if (line && line->hasAcceptableInput()) items[id]->flag2 = line->text().toDouble();
    }
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

        buildFixComputeRows(layout, idx, computes, compute_map);
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

        buildFixComputeRows(layout, idx, fixes, fix_map);
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
    readFixComputeRows(layout, computes_offset, computes);

    // retrieve fix data from dialog and store in map
    readFixComputeRows(layout, fixes_offset, fixes);
    createImage();
}

void ImageViewer::readRegionRows(QGridLayout *layout)
{
    int n = 0;
    for (int idx = 4; idx < static_cast<int>(regions.size()) + 4; ++idx) {
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

    readRegionRows(layout);
    createImage();
}

void ImageViewer::readColorRows(QGridLayout *layout, int colorstart, int numtypes)
{
    // expand color_list if more types than existing colors
    int old_size = color_list.size();
    for (int i = color_list.size(); i < numtypes; ++i)
        color_list.append(color_list[i % old_size]);

    for (int i = 1; i <= numtypes; ++i) {
        QString n = color_list[i - 1].first;
        double r  = color_list[i - 1].second.redF();
        double g  = color_list[i - 1].second.greenF();
        double b  = color_list[i - 1].second.blueF();

        auto *item = layout->itemAtPosition(i + colorstart, 2);
        auto *rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
        if (rgb) n = rgb->text();

        item = layout->itemAtPosition(i + colorstart, 3);
        rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
        if (rgb && rgb->hasAcceptableInput()) r = rgb->text().toDouble();

        item = layout->itemAtPosition(i + colorstart, 4);
        rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
        if (rgb && rgb->hasAcceptableInput()) g = rgb->text().toDouble();

        item = layout->itemAtPosition(i + colorstart, 5);
        rgb  = item ? qobject_cast<QLineEdit *>(item->widget()) : nullptr;
        if (rgb && rgb->hasAcceptableInput()) b = rgb->text().toDouble();

        color_list[i - 1] = {n, QColor::fromRgbF(r, g, b)};
    }
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
    constexpr int MAXCOLS = 6;

    int idx      = 0;
    int n        = 0;
    auto *layout = new QGridLayout;
    layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

    layout->addWidget(new QLabel("Type:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel(""), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Name:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Red:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Green:"), idx, n++, Qt::AlignHCenter);
    layout->addWidget(new QLabel("Blue:"), idx++, n++, Qt::AlignHCenter);
    layout->addWidget(new QHline, idx++, 0, 1, MAXCOLS);

    auto *rgbvalidator = new QDoubleValidator(0.0, 1.0, 3, &colorview);

    // record the row index where the colors start
    int colorstart = idx - 1;

    for (int i = 0; i < numtypes; ++i) {
        int icolor = i % numcolors;
        auto cname = (i == icolor) ? color_list[icolor].first : QString("type%1").arg(i + 1);
        auto red   = color_list[icolor].second.redF();
        auto green = color_list[icolor].second.greenF();
        auto blue  = color_list[icolor].second.blueF();

        n       = 0;
        auto *t = new QLabel(QString::number(i + 1));
        t->setFixedSize(metrics.averageCharWidth() * 4, metrics.height() + 4);
        t->setAlignment(Qt::AlignRight);
        layout->addWidget(t, idx, n++, Qt::AlignHCenter);

        auto *icon = new QPushButton("");
        icon->setIcon(color_icon(QColor::fromRgbF(red, green, blue)));
        auto iconhint = icon->minimumSizeHint();
        auto isize    = iconhint.height();
        iconhint.setWidth(isize);
        icon->setIconSize(QSize(isize - 4, isize - 4));
        icon->setMinimumSize(iconhint);
        icon->setMaximumSize(iconhint);
        layout->addWidget(icon, idx, n++, Qt::AlignHCenter);

        auto *name = new QLineEdit(cname);
        name->setFixedSize(metrics.averageCharWidth() * 14, metrics.height() + 4);
        name->setObjectName(QString("type%1").arg(i + 1));
        layout->addWidget(name, idx, n++);

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

        connect(icon, &QPushButton::released, [=, &colorview]() {
            QColor initialColor =
                QColor::fromRgbF(r->text().toDouble(), g->text().toDouble(), b->text().toDouble());
            QColor selectedColor =
                QColorDialog::getColor(initialColor, &colorview, "Select Atom Type Color");
            if (selectedColor.isValid()) {
                r->setText(QString::number(selectedColor.redF(), 'f', 3));
                g->setText(QString::number(selectedColor.greenF(), 'f', 3));
                b->setText(QString::number(selectedColor.blueF(), 'f', 3));
                icon->setIcon(color_icon(selectedColor));
            }
        });
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
                    auto obj  = arr[(i - 1) % arr.size()].toObject();
                    QString n = obj.value("name").toString();
                    if (n.isEmpty()) n = QString("type%1").arg(i);
                    double r = std::clamp(obj.value("red").toDouble(1.0), 0.0, 1.0);
                    double g = std::clamp(obj.value("green").toDouble(1.0), 0.0, 1.0);
                    double b = std::clamp(obj.value("blue").toDouble(1.0), 0.0, 1.0);

                    auto *iconItem = layout->itemAtPosition(i + colorstart, 1);
                    if (auto *but =
                            qobject_cast<QPushButton *>(iconItem ? iconItem->widget() : nullptr))
                        but->setIcon(color_icon(QColor::fromRgbF(r, g, b)));

                    auto *item = layout->itemAtPosition(i + colorstart, 2);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(n);
                    item = layout->itemAtPosition(i + colorstart, 3);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(QString::number(r, 'f', 3));
                    item = layout->itemAtPosition(i + colorstart, 4);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        w->setText(QString::number(g, 'f', 3));
                    item = layout->itemAtPosition(i + colorstart, 5);
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
                    QString n;
                    auto *item = layout->itemAtPosition(i + colorstart, 2);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        n = w->text();
                    item = layout->itemAtPosition(i + colorstart, 3);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) r = w->text().toDouble();
                    item = layout->itemAtPosition(i + colorstart, 4);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) g = w->text().toDouble();
                    item = layout->itemAtPosition(i + colorstart, 5);
                    if (auto *w = qobject_cast<QLineEdit *>(item ? item->widget() : nullptr))
                        if (w->hasAcceptableInput()) b = w->text().toDouble();
                    QJsonObject obj;
                    obj["name"]  = n;
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
            MINIMUM_WIDTH, metrics.averageCharWidth() * 10 * 4 +
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
        readColorRows(layout, colorstart, numtypes);
    }
    createImage();
}

// Local Variables:
// c-basic-offset: 4
// End:
