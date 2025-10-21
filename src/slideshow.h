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

/**
 * @brief Slideshow viewer for displaying sequences of images
 * 
 * SlideShow provides a dialog for viewing and navigating through
 * sequences of images, typically from LAMMPS dump image commands.
 * It supports manual navigation (first/prev/next/last), automatic
 * playback with configurable timing, looping, and zoom controls.
 * Images can be exported as a movie file.
 */
class SlideShow : public QDialog {
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param fileName Path to first image file
     * @param parent Parent widget
     */
    explicit SlideShow(const QString &fileName, QWidget *parent = nullptr);
    
    /**
     * @brief Destructor
     */
    ~SlideShow() override = default;

    SlideShow()                             = delete;
    SlideShow(const SlideShow &)            = delete;
    SlideShow(SlideShow &&)                 = delete;
    SlideShow &operator=(const SlideShow &) = delete;
    SlideShow &operator=(SlideShow &&)      = delete;

    /**
     * @brief Add an image to the slideshow sequence
     * @param filename Path to image file to add
     */
    void add_image(const QString &filename);
    
    /**
     * @brief Clear all images from slideshow
     */
    void clear();

private slots:
    void quit();           ///< Close slideshow window
    void delete_images();  ///< Delete all image files in sequence
    void stop_run();       ///< Stop running simulation
    void movie();          ///< Export images as movie file
    void first();          ///< Jump to first image
    void last();           ///< Jump to last image
    void next();           ///< Advance to next image
    void prev();           ///< Go back to previous image
    void play();           ///< Start/stop automatic playback
    void loop();           ///< Toggle looping mode
    void zoomIn();         ///< Zoom in on current image
    void zoomOut();        ///< Zoom out on current image
    void normalSize();     ///< Reset zoom to 100%

private:
    /**
     * @brief Scale the displayed image
     * @param factor Scaling factor to apply
     */
    void scaleImage(double factor);
    
    /**
     * @brief Load and display image at given index
     * @param idx Image index in sequence
     */
    void loadImage(int idx);

private:
    QImage image;                ///< Currently displayed image
    QTimer *playtimer;           ///< Timer for automatic playback
    QLabel *imageLabel;          ///< Label displaying the image
    QLabel *imageName;           ///< Label showing image filename
    QDialogButtonBox *buttonBox; ///< Dialog control buttons
    double scaleFactor = 1.0;    ///< Current zoom scale factor

    int current;                 ///< Index of current image
    int maxwidth, maxheight;     ///< Maximum image dimensions
    bool do_loop;                ///< Loop playback flag
    QStringList imagefiles;      ///< List of image file paths
};
#endif

// Local Variables:
// c-basic-offset: 4
// End:
