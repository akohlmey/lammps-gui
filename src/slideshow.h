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

#ifndef SLIDESHOW_H
#define SLIDESHOW_H

#include <QDialog>
#include <QImage>
#include <QString>
#include <QStringList>

class QDialogButtonBox;
class QLabel;
class QTimer;

class SlideShow : public QDialog {
    Q_OBJECT

public:
    explicit SlideShow(const QString &fileName, QWidget *parent = nullptr);
    ~SlideShow() override = default;

    SlideShow()                             = delete;
    SlideShow(const SlideShow &)            = delete;
    SlideShow(SlideShow &&)                 = delete;
    SlideShow &operator=(const SlideShow &) = delete;
    SlideShow &operator=(SlideShow &&)      = delete;

    void add_image(const QString &filename);
    void clear();

private slots:
    void quit();
    void delete_images();
    void stop_run();
    void movie();
    void first();
    void last();
    void next();
    void prev();
    void play();
    void loop();
    void zoomIn();
    void zoomOut();
    void normalSize();

private:
    void scaleImage(double factor);
    void loadImage(int idx);

private:
    QImage image;
    QTimer *playtimer;
    QLabel *imageLabel, *imageName;
    QDialogButtonBox *buttonBox;
    double scaleFactor = 1.0;

    int current;
    int maxwidth, maxheight;
    bool do_loop;
    QStringList imagefiles;
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
