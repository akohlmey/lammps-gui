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

#ifndef URLDOWNLOADER_H
#define URLDOWNLOADER_H

#include <QString>

class QNetworkAccessManager;
class QWidget;

/**
 * @brief Utility class for downloading files over HTTPS
 *
 * URLDownloader provides a synchronous interface for downloading files from
 * HTTPS URLs.  It respects the "https_proxy" preference setting (or the
 * https_proxy environment variable) when configured.
 *
 * @see Preferences for the proxy setting
 */
class URLDownloader {
public:
    /**
     * @brief Construct a new URLDownloader
     * @param parent Optional parent widget for progress dialogs
     */
    explicit URLDownloader(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~URLDownloader();

    URLDownloader(const URLDownloader &)            = delete;
    URLDownloader(URLDownloader &&)                 = delete;
    URLDownloader &operator=(const URLDownloader &) = delete;
    URLDownloader &operator=(URLDownloader &&)      = delete;

    /**
     * @brief Download a file from the given HTTPS URL to a local file
     *
     * Performs a synchronous (blocking) download of the resource at \p url,
     * writing the result to the local file \p file.  Respects the configured
     * HTTPS proxy setting.  Optionally display a dialog with the downloaded
     * URL and the location of the downloaded file.
     *
     * @param url   The HTTPS URL to download from
     * @param file  The local file path to write to
     * @param showDialog  Display a dialog with the downloaded URL and target file location while downloading
     * @return true if the download completed successfully, false otherwise
     */
    bool download(const QString &url, const QString &file, bool showDialog=false);

    /**
     * @brief Check if a website can be accessed over the network
     *
     * Attempts an HTTP HEAD request to www.lammps.org to verify that the
     * network is reachable.
     *
     * @return true if the site can be reached, false otherwise
     */
    bool checkNetwork();

    /**
     * @brief Return the last error message
     * @return Human-readable error description or empty string
     */
    QString errorString() const { return lastError; }

private:
    void configureProxy();

    QNetworkAccessManager *manager; ///< Qt network access manager
    QWidget *parentWidget;          ///< Parent widget for dialogs
    QString lastError;              ///< Last error message
};

#endif // URLDOWNLOADER_H

// Local Variables:
// c-basic-offset: 4
// End:
