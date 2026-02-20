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

#include "slideshow.h"

#include "helpers.h"
#include "lammpsgui.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QImageReader>
#include <QKeySequence>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QProcess>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QSlider>
#include <QSpacerItem>
#include <QSpinBox>
#include <QTemporaryFile>
#include <QTimer>
#include <QTransform>
#include <QVBoxLayout>

#include <algorithm>

SlideShow::SlideShow(const QString &fileName, QWidget *parent) :
    QDialog(parent), playtimer(nullptr), imageLabel(new QLabel), scrollArea(new QScrollArea),
    scrollBar(new QSlider), imageName(new QLabel("(none)")), timer_delay(100), do_loop(true),
    imageRotation(0), imageFlipH(false), imageFlipV(false)
{
    imageLabel->setBackgroundRole(QPalette::Base);
    imageLabel->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    imageLabel->setScaledContents(false);
    imageLabel->minimumSizeHint();

    scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setWidget(imageLabel);
    scrollArea->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    scrollArea->setVisible(false);

    scrollBar->setMinimum(0);
    scrollBar->setMaximum(1);
    scrollBar->setOrientation(Qt::Horizontal);
    scrollBar->setToolTip("Select Image to display");
    connect(scrollBar, &QSlider::valueChanged, this, &SlideShow::loadImage);

    imageName->setFrameStyle(QFrame::Raised);
    imageName->setFrameShape(QFrame::Panel);
    imageName->setAlignment(Qt::AlignCenter);
    imageName->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    imageName->setTextInteractionFlags(Qt::TextSelectableByMouse);

    auto *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_W), this);
    QObject::connect(shortcut, &QShortcut::activated, this, &QWidget::close);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Slash), this);
    QObject::connect(shortcut, &QShortcut::activated, this, &SlideShow::stop_run);
    shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Q), this);
    QObject::connect(shortcut, &QShortcut::activated, this, &SlideShow::quit);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *mainLayout = new QVBoxLayout;
    auto *navLayout  = new QHBoxLayout;
    auto *botLayout  = new QHBoxLayout;

    // workaround for incorrect highlight bug on macOS
    auto *dummy = new QPushButton(QIcon(), "");
    dummy->hide();

    auto *tomovie = new QPushButton(QIcon(":/icons/export-movie.png"), "");
    tomovie->setToolTip("Export to movie file");
    tomovie->setEnabled(has_exe("ffmpeg") || has_exe("magick") || has_exe("convert"));

    auto *toimage = new QPushButton(QIcon(":/icons/document-save-as.png"), "");
    toimage->setToolTip("Export to image file");

    auto *totrash = new QPushButton(QIcon(":/icons/trash.png"), "");
    totrash->setToolTip("Delete all image files");

    auto dsize  = QFontMetrics(QApplication::font()).size(Qt::TextSingleLine, "Delay:  100");
    // need some extra space on Windows
#if Q_OS_WIN32
    dsize = dsize * 3 / 2;
#endif
    auto *delay = new QSpinBox;
    delay->setRange(10, 10000);
    delay->setStepType(QAbstractSpinBox::AdaptiveDecimalStepType);
    delay->setValue(timer_delay);
    delay->setObjectName("delay");
    delay->setToolTip("Set delay between images in milliseconds");
    delay->setMinimumSize(dsize);

    auto *gofirst = new QPushButton(QIcon(":/icons/go-first.png"), "");
    gofirst->setToolTip("Go to first Image");
    gofirst->setObjectName("first");
    gofirst->setCheckable(false);
    auto *goprev = new QPushButton(QIcon(":/icons/go-previous-2.png"), "");
    goprev->setToolTip("Go to previous Image");
    auto *goplay = new QPushButton(QIcon(":/icons/media-playback-start-2.png"), "");
    goplay->setToolTip("Play animation");
    goplay->setCheckable(true);
    goplay->setChecked(playtimer);
    goplay->setObjectName("play");
    auto *gonext = new QPushButton(QIcon(":/icons/go-next-2.png"), "");
    gonext->setToolTip("Go to next Image");
    auto *golast = new QPushButton(QIcon(":/icons/go-last.png"), "");
    golast->setToolTip("Go to last Image");
    auto *goloop = new QPushButton(QIcon(":/icons/media-playlist-repeat.png"), "");
    goloop->setToolTip("Loop animation");
    goloop->setCheckable(true);
    goloop->setChecked(do_loop);

    auto *zoomin = new QPushButton(QIcon(":/icons/gtk-zoom-in.png"), "");
    zoomin->setToolTip("Zoom in by 10 percent");
    auto *zoomout = new QPushButton(QIcon(":/icons/gtk-zoom-out.png"), "");
    zoomout->setToolTip("Zoom out by 10 percent");
    auto *normal = new QPushButton(QIcon(":/icons/gtk-zoom-fit.png"), "");
    normal->setToolTip("Reset zoom to normal");

    auto *imgrotcw = new QPushButton(QIcon(":/icons/object-rotate-right.png"), "");
    imgrotcw->setToolTip("Rotate displayed image 90<sup>o</sup> clockwise");
    auto *imgrotccw = new QPushButton(QIcon(":/icons/object-rotate-left.png"), "");
    imgrotccw->setToolTip("Rotate displayed image 90<sup>o</sup> counter-clockwise");
    auto *imgfliph = new QPushButton(QIcon(":/icons/object-flip-horizontal.png"), "");
    imgfliph->setToolTip("Mirror displayed image horizontally");
    auto *imgflipv = new QPushButton(QIcon(":/icons/object-flip-vertical.png"), "");
    imgflipv->setToolTip("Mirror displayed image vertically");

    connect(tomovie, &QPushButton::released, this, &SlideShow::movie);
    connect(toimage, &QPushButton::released, this, &SlideShow::save_current_image);
    connect(totrash, &QPushButton::released, this, &SlideShow::delete_images);
    connect(delay, &QAbstractSpinBox::editingFinished, this, &SlideShow::set_delay);

    connect(gofirst, &QPushButton::released, this, &SlideShow::first);
    connect(goprev, &QPushButton::released, this, &SlideShow::prev);
    connect(goplay, &QPushButton::released, this, &SlideShow::play);
    connect(gonext, &QPushButton::released, this, &SlideShow::next);
    connect(golast, &QPushButton::released, this, &SlideShow::last);
    connect(goloop, &QPushButton::released, this, &SlideShow::loop);
    connect(zoomin, &QPushButton::released, this, &SlideShow::zoomIn);
    connect(zoomout, &QPushButton::released, this, &SlideShow::zoomOut);
    connect(gofirst, &QPushButton::released, this, &SlideShow::first);
    connect(normal, &QPushButton::released, this, &SlideShow::normalSize);
    connect(imgrotcw, &QPushButton::released, this, &SlideShow::do_image_rotate_cw);
    connect(imgrotccw, &QPushButton::released, this, &SlideShow::do_image_rotate_ccw);
    connect(imgfliph, &QPushButton::released, this, &SlideShow::do_image_flip_h);
    connect(imgflipv, &QPushButton::released, this, &SlideShow::do_image_flip_v);

    navLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    navLayout->addWidget(tomovie,1);
    navLayout->addWidget(toimage,1);
    navLayout->addWidget(totrash,1);
    navLayout->addWidget(new QLabel("Delay (ms):"));
    navLayout->addWidget(delay, 5);
    navLayout->addWidget(dummy);
    navLayout->addWidget(gofirst,1);
    navLayout->addWidget(goprev,1);
    navLayout->addWidget(goplay,1);
    navLayout->addWidget(gonext,1);
    navLayout->addWidget(golast,1);
    navLayout->addWidget(goloop,1);

    navLayout->addWidget(zoomin,1);
    navLayout->addWidget(zoomout,1);
    navLayout->addWidget(normal,1);
    navLayout->addWidget(imgrotcw,1);
    navLayout->addWidget(imgrotccw,1);
    navLayout->addWidget(imgfliph,1);
    navLayout->addWidget(imgflipv,1);
    navLayout->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Expanding, QSizePolicy::Minimum));
    navLayout->setSizeConstraint(QLayout::SetMinimumSize);

    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(scrollArea, 10);

    botLayout->addWidget(imageName);
    botLayout->addWidget(buttonBox);
    botLayout->setStretch(0, 3);
    botLayout->setSizeConstraint(QLayout::SetMinimumSize);
    mainLayout->addLayout(botLayout);
    mainLayout->addWidget(scrollBar, 10);

    setWindowIcon(QIcon(":/icons/lammps-gui-icon-128x128.png"));
    setWindowTitle(QString("LAMMPS-GUI - Slide Show: ") + QFileInfo(fileName).fileName());

    imagefiles.clear();
    scaleFactor = 1.0;
    current     = 0;

    scrollArea->setVisible(true);
    setLayout(mainLayout);
    mainLayout->setSizeConstraint(QLayout::SetMinAndMaxSize);
    adjustWindowSize();

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

void SlideShow::add_image(const QString &filename)
{
    if (!imagefiles.contains(filename)) {
        int lastidx = imagefiles.size();
        imagefiles.append(filename);
        loadImage(lastidx);
        scrollBar->setMaximum(lastidx);
        scrollBar->setValue(lastidx);
    }
}

void SlideShow::delete_images()
{
    for (const auto &file : imagefiles) {
        QFile::remove(file);
    }
    clear();
}

void SlideShow::clear()
{
    imagefiles.clear();
    image.fill(Qt::black);
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->setMinimumSize(image.width(), image.height());
    imageLabel->resize(image.width(), image.height());
    imageName->setText("(none)");
    scrollBar->setMaximum(1);
    adjustWindowSize();
    repaint();
}

void SlideShow::set_delay()
{
    auto *field = qobject_cast<QSpinBox *>(sender());
    if (field->objectName() == "delay") {
        timer_delay = field->value();
    }
}

void SlideShow::loadImage(int idx)
{
    if ((idx < 0) || (idx >= imagefiles.size())) return;

    do {
        QImageReader reader(imagefiles[idx]);
        reader.setAutoTransform(true);
        const QImage newImage = reader.read();

        // There was an error reading the image file. Try reading the previous image instead.
        if (newImage.isNull()) {
            --idx;
        } else {
            rawImage = newImage;
            applyImageTransform();
            imageName->setText(QString(" Image %1 / %2 : %3 ")
                                   .arg(idx + 1)
                                   .arg(imagefiles.size())
                                   .arg(imagefiles[idx]));
            current = idx;
            break;
        }
    } while (idx >= 0);
    scrollBar->setValue(idx);
    adjustWindowSize();
}

void SlideShow::quit()
{
    auto *main = dynamic_cast<LammpsGui *>(get_main_widget());
    if (main) main->quit();
}

void SlideShow::stop_run()
{
    auto *main = dynamic_cast<LammpsGui *>(get_main_widget());
    if (main) main->stop_run();
}

void SlideShow::save_current_image()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export Current Image to Image File", ".",
        "Image Files (*.png *.jpg *.jpeg *.gif *.bmp *.tga *.ppm *.tiff *.pgm *.xpm *.xbm)");
    if (fileName.isEmpty()) return;

    // try direct save and if it fails write to PNG and then convert with ImageMagick if available
    if (!image.save(fileName)) {
        if (has_exe("magick") || has_exe("convert")) {
            QTemporaryFile tmpfile(QDir::tempPath() + "/LAMMPS_GUI.XXXXXX.png");
            // open and close to generate temporary file name
            (void)tmpfile.open();
            (void)tmpfile.close();
            if (!image.save(tmpfile.fileName())) {
                QMessageBox::warning(this, "SlideShow Error",
                                     "Could not save image to file " + fileName);
                return;
            }

            QString cmd = "magick";
            QStringList args{tmpfile.fileName(), fileName};
            if (!has_exe("magick")) cmd = "convert";
            auto *convert = new QProcess(this);
            convert->start(cmd, args);
            if (!convert->waitForFinished(-1)) {
                const QString err = convert->errorString();
                delete convert;
                QFile::remove(fileName);
                QMessageBox::warning(this, "SlideShow Error",
                                     "ImageMagick conversion failed while saving to file " +
                                         fileName + ":\n" + err);
                return;
            }
            if (convert->exitStatus() != QProcess::NormalExit || convert->exitCode() != 0) {
                const QString stderrText = QString::fromLocal8Bit(convert->readAllStandardError());
                delete convert;
                QFile::remove(fileName);
                QString msg =
                    "ImageMagick conversion failed while saving to file " + fileName + ".";
                if (!stderrText.trimmed().isEmpty()) {
                    msg += "\n\nDetails:\n" + stderrText.trimmed();
                }
                QMessageBox::warning(this, "SlideShow Error", msg);
                return;
            }
            delete convert;
            if (!QFile::exists(fileName)) {
                QMessageBox::warning(this, "SlideShow Error",
                                     "ImageMagick reported success, but the output file " +
                                         fileName + " was not created.");
                return;
            }
        } else {
            QMessageBox::warning(this, "SlideShow Error",
                                 "Could not save image to file " + fileName);
        }
    }
}

void SlideShow::movie()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "Export to Movie File", ".", "Movie Files (*.mp4 *.mkv *.avi *.mpg *.mpeg *.gif)");
    if (fileName.isEmpty()) return;

    if (has_exe("ffmpeg")) {
        QDir curdir(".");
        QTemporaryFile concatfile;
        if (concatfile.open()) {
            for (const auto &img : imagefiles) {
                concatfile.write("file '");
                concatfile.write(curdir.absoluteFilePath(img).toLocal8Bit());
                concatfile.write("'\n");
            }
            concatfile.close();

            const auto fps = QString::number(1.0/((double)timer_delay/1000.0));
            QStringList args;
            args << "-y";
            args << "-safe"
                 << "0";
            args << "-r"
                 << fps;
            args << "-f"
                 << "concat";
            args << "-i" << concatfile.fileName();
            QString filters;
            if (scaleFactor != 1.0) filters += QString("scale=iw*%1:-1,").arg(scaleFactor);
            if (imageRotation == 90.0) {
                filters += "transpose=1,";
            } else if (imageRotation == 180.0) {
                filters += "transpose=1,transpose=1,";
            } else if (imageRotation == 270.0) {
                filters += "transpose=2,";
            }
            if (imageFlipH) filters += "hflip,";
            if (imageFlipV) filters += "vflip,";
            if (!filters.isEmpty()) {
                // chop off trailing comma
                filters.resize(filters.size() - 1);
                args << "-vf" << filters;
            }
            args << "-b:v"
                 << "2000k";
            args << "-r"
                 << fps;
            args << fileName;

            auto *ffmpeg = new QProcess(this);
            ffmpeg->start("ffmpeg", args);
            ffmpeg->waitForFinished(-1);
            delete ffmpeg;
        } else {
            QMessageBox::warning(this, "SlideShow Error",
                                 "Cannot create temporary file for generating movie " +
                                     concatfile.errorString());
        }
    } else {
        QString cmd = "magick";
        if (!has_exe("magick")) cmd = "convert";
        QStringList args;
        args << "-delay" << QString::number(timer_delay / 10);
        QDir curdir(".");
        for (const auto &img : imagefiles)
            args << curdir.absoluteFilePath(img);
        if (scaleFactor != 1.0) args << "-resize" << QString("%1%%").arg(100.0 * scaleFactor);
        if (imageRotation != 0.0) args << "-rotate" << QString("%1").arg(imageRotation);
        if (imageFlipH) args << "-flop";
        if (imageFlipV) args << "-flip";
        args << fileName;

        // run the conversion command
        auto *convert = new QProcess(this);
        convert->start(cmd, args);
        convert->waitForFinished(-1);
        delete convert;
    }
}

void SlideShow::first()
{
    current = 0;
    loadImage(current);
}

void SlideShow::last()
{
    current = imagefiles.size() - 1;
    loadImage(current);
}

void SlideShow::play()
{
    // if we do not loop, start animation from beginning
    if (!do_loop) current = 0;
    auto *delay = findChild<QSpinBox *>("delay");

    if (playtimer) {
        playtimer->stop();
        delete playtimer;
        playtimer = nullptr;
        if (delay) delay->setEnabled(true);
    } else {
        playtimer = new QTimer(this);
        connect(playtimer, &QTimer::timeout, this, &SlideShow::next);
        playtimer->start(timer_delay);
        if (delay) delay->setEnabled(false);
    }

    // reset push button state. use findChild() if not triggered from button.
    auto *button = qobject_cast<QPushButton *>(sender());
    if (!button) button = findChild<QPushButton *>("play");
    if (button) button->setChecked(playtimer);
}

void SlideShow::next()
{
    ++current;
    if (current >= imagefiles.size()) {
        if (do_loop) {
            current = 0;
        } else {
            // stop animation
            if (playtimer) play();
            --current;
        }
    }
    loadImage(current);
}

void SlideShow::prev()
{
    --current;
    if (current < 0) {
        if (do_loop)
            current = imagefiles.size() - 1;
        else
            current = 0;
    }
    loadImage(current);
}

void SlideShow::loop()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    do_loop      = !do_loop;
    button->setChecked(do_loop);
}

void SlideShow::zoomIn()
{
    scaleImage(1.1);
}

void SlideShow::zoomOut()
{
    scaleImage(0.9);
}

void SlideShow::normalSize()
{
    scaleFactor = 1.0;
    scaleImage(1.0);
}

void SlideShow::scaleImage(double factor)
{
    scaleFactor *= factor;
    // don't let the image become smaller than 10%
    scaleFactor = std::max(scaleFactor, 0.1);

    loadImage(current);
}

void SlideShow::adjustWindowSize()
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
        desiredWidth    = std::min(desiredWidth, screenSize.width() * 3 / 4);
        desiredHeight   = std::min(desiredHeight, screenSize.height() * 9 / 10);
    }
    scrollArea->setMinimumSize(desiredWidth, desiredHeight);
    scrollArea->resize(desiredWidth, desiredHeight);
    adjustSize();
}

void SlideShow::do_image_rotate_cw()
{
    imageRotation = (imageRotation + 90) % 360;
    applyImageTransform();
}

void SlideShow::do_image_rotate_ccw()
{
    imageRotation = (imageRotation + 270) % 360;
    applyImageTransform();
}

void SlideShow::do_image_flip_h()
{
    imageFlipH = !imageFlipH;
    applyImageTransform();
}

void SlideShow::do_image_flip_v()
{
    imageFlipV = !imageFlipV;
    applyImageTransform();
}

void SlideShow::applyImageTransform()
{
    // If no raw image is available yet, use the current image
    if (rawImage.isNull()) {
        if (!image.isNull()) {
            rawImage = image;
        } else {
            return;
        }
    }

    QImage transformedImage = rawImage;

    // Apply rotation
    if (imageRotation != 0) {
        QTransform transform;
        transform.rotate(imageRotation);
        transformedImage = transformedImage.transformed(transform, Qt::SmoothTransformation);
    }

    // Apply horizontal flip
    if (imageFlipH) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        transformedImage = transformedImage.flipped(Qt::Horizontal);
#else
        transformedImage = transformedImage.mirrored(true, false);
#endif
    }

    // Apply vertical flip
    if (imageFlipV) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 9, 0)
        transformedImage = transformedImage.flipped(Qt::Vertical);
#else
        transformedImage = transformedImage.mirrored(false, true);
#endif
    }

    // Scale the transformed image
    int newheight = transformedImage.height() * scaleFactor;
    int newwidth  = transformedImage.width() * scaleFactor;
    image         = transformedImage.scaled(newwidth, newheight, Qt::IgnoreAspectRatio,
                                            Qt::SmoothTransformation);
    imageLabel->setPixmap(QPixmap::fromImage(image));
    imageLabel->adjustSize();
}

// Local Variables:
// c-basic-offset: 4
// End:
