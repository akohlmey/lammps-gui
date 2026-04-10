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

#include "urldownloader.h"
#include "helpers.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDialog>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>

URLDownloader::URLDownloader(QWidget *parent) :
    manager(new QNetworkAccessManager), parentWidget(parent)
{
    configureProxy();
}

URLDownloader::~URLDownloader()
{
    delete manager;
}

void URLDownloader::configureProxy()
{
    // prefer environment variable, then fall back to preferences value
    auto https_proxy = QString::fromLocal8Bit(qgetenv("https_proxy"));
    if (https_proxy.isEmpty()) https_proxy = QSettings().value("https_proxy", "").toString();

    if (!https_proxy.isEmpty()) {
        QUrl proxyUrl(https_proxy);
        QNetworkProxy proxy;
        proxy.setType(QNetworkProxy::HttpProxy);
        proxy.setHostName(proxyUrl.host());
        proxy.setPort(static_cast<quint16>(proxyUrl.port(8080)));
        if (!proxyUrl.userName().isEmpty()) proxy.setUser(proxyUrl.userName());
        if (!proxyUrl.password().isEmpty()) proxy.setPassword(proxyUrl.password());
        manager->setProxy(proxy);
    }
}

bool URLDownloader::download(const QString &url, const QString &file, bool showDialog)
{
    lastError.clear();

    QDialog *dlg = nullptr;
    if (showDialog) {
        // show a temporary dialog while the download is in progress
        if (parentWidget) {
            dlg = new QDialog(parentWidget);
            dlg->setWindowTitle("Downloading...");
            dlg->setMinimumWidth(400);
            auto *layout = new QVBoxLayout(dlg);
            layout->addWidget(new QLabel(QString("<b>Downloading:</b><br>%1").arg(url)));
            layout->addWidget(new QLabel(QString("<b>Saving to:</b><br>%1").arg(file)));
            dlg->setLayout(layout);
            dlg->show();
            QCoreApplication::processEvents();
        }
    }

    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager->get(request);

    // block until the download finishes
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // close the dialog now that the download is complete
    delete dlg;

    if (reply->error() != QNetworkReply::NoError) {
        lastError = reply->errorString();
        reply->deleteLater();
        return false;
    }

    // check for HTTP-level errors (e.g. 404)
    int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (httpStatus >= 400) {
        lastError = QString("HTTP error %1").arg(httpStatus);
        reply->deleteLater();
        return false;
    }

    // ensure parent directories exist
    QFileInfo fi(file);
    if (!QDir().mkpath(fi.absolutePath())) {
        lastError = QString("Cannot create directory: %1").arg(fi.absolutePath());
        reply->deleteLater();
        return false;
    }

    QFile outFile(file);
    if (!outFile.open(QIODevice::WriteOnly)) {
        lastError = QString("Cannot open file for writing: %1").arg(file);
        reply->deleteLater();
        return false;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();
    if (outFile.write(data) != data.size()) {
        lastError = QString("Failed to write data to file: %1").arg(file);
        outFile.close();
        return false;
    }
    outFile.close();

    // verify SHA-256 checksum if a SHA256SUMS file is available
    if (!verifyChecksum(url, file)) {
        QFile::remove(file);
        return false;
    }

    return true;
}

QByteArray URLDownloader::fetchRawContent(const QString &url)
{
    QNetworkRequest request{QUrl(url)};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager->get(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data;
    if (reply->error() == QNetworkReply::NoError) {
        int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (httpStatus < 400) data = reply->readAll();
    }
    reply->deleteLater();
    return data;
}

bool URLDownloader::verifyChecksum(const QString &url, const QString &file)
{
    // build URL for the SHA256SUMS file in the same remote directory
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0) return true;

    QString sumsUrl  = url.left(lastSlash + 1) + "SHA256SUMS";
    QString fileName = url.mid(lastSlash + 1);

    QByteArray sumsData = fetchRawContent(sumsUrl);
    if (sumsData.isEmpty()) return true; // no SHA256SUMS file — nothing to check

    // parse SHA256SUMS: each line is "<hex-hash>  <filename>" or "<hex-hash> <filename>"
    QString expectedHash;
    const QList<QByteArray> lines = sumsData.split('\n');
    for (const auto &line : lines) {
        QString sline = QString::fromUtf8(line).trimmed();
        if (sline.isEmpty()) continue;

        // split on first whitespace run (handles both single and double space)
        int spaceIdx = sline.indexOf(' ');
        if (spaceIdx < 0) continue;

        QString hash = sline.left(spaceIdx);
        QString name = sline.mid(spaceIdx).trimmed();
        // strip leading "./" or "*" prefix sometimes present in SHA256SUMS files
        if (name.startsWith("./")) name = name.mid(2);
        if (name.startsWith('*')) name = name.mid(1);

        if (name == fileName) {
            expectedHash = hash.toLower();
            break;
        }
    }

    if (expectedHash.isEmpty()) return true; // no entry for this file — nothing to check

    // compute SHA-256 of the downloaded file
    QFile localFile(file);
    if (!localFile.open(QIODevice::ReadOnly)) return true;

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(&localFile);
    localFile.close();

    QString actualHash = hasher.result().toHex().toLower();

    if (actualHash != expectedHash) {
        lastError = QString("SHA-256 checksum mismatch for %1").arg(fileName);
        critical(parentWidget, "Download Error",
                 QString("The SHA-256 checksum of the downloaded file \"%1\" does not match "
                         "the expected checksum.")
                     .arg(fileName),
                 QString("Expected: %1\nActual: %2").arg(expectedHash, actualHash));
        return false;
    }

    return true;
}

bool URLDownloader::checkNetwork()
{
    lastError.clear();

    QNetworkRequest request{QUrl("https://www.lammps.org")};
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply *reply = manager->head(request);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool ok = (reply->error() == QNetworkReply::NoError);
    if (!ok) lastError = reply->errorString();
    reply->deleteLater();
    return ok;
}

// Local Variables:
// c-basic-offset: 4
// End:
