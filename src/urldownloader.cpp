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
#include "constants.h"
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
#include <QSaveFile>
#include <QSettings>
#include <QUrl>
#include <QVBoxLayout>
#include <QVariant>

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
    if (https_proxy.isEmpty()) https_proxy = QSettings().value(Keys::HTTPS_PROXY, "").toString();

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
            auto *hlayout = new QHBoxLayout(dlg);
            auto *pixmap  = new QLabel();
            pixmap->setPixmap(QPixmap(":/icons/lammps-plugin.png").scaled(96, 96));
            hlayout->addWidget(pixmap);
            auto *vlayout = new QVBoxLayout();
            vlayout->addWidget(new QLabel(QString("<b>Downloading:</b><br>%1").arg(url)));
            vlayout->addWidget(new QLabel(QString("<b>Saving to:</b><br>%1").arg(file)));
            hlayout->addLayout(vlayout);
            dlg->setLayout(hlayout);
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

    const QByteArray data     = reply->readAll();
    const QVariant clen       = reply->header(QNetworkRequest::ContentLengthHeader);
    const QByteArray encoding = reply->rawHeader("Content-Encoding");
    reply->deleteLater();

    // reject an incomplete transfer: when the server advertised a content length
    // and did not compress the body, it must match the number of bytes actually
    // received.  Writing a truncated shared library would crash the dynamic
    // linker when the file is later loaded via dlopen().
    if (encoding.isEmpty() && clen.isValid() &&
        clen.toLongLong() != static_cast<qint64>(data.size())) {
        lastError = QString("Incomplete download of %1: expected %2 bytes but received %3")
                        .arg(file)
                        .arg(clen.toLongLong())
                        .arg(data.size());
        return false;
    }

    // write to a temporary file that is atomically renamed into place only after
    // a complete, successful write, so an interrupted or failed write can never
    // leave a truncated file at the destination path.
    QSaveFile outFile(file);
    if (!outFile.open(QIODevice::WriteOnly)) {
        lastError = QString("Cannot open file for writing: %1").arg(file);
        return false;
    }
    if (outFile.write(data) != data.size()) {
        lastError = QString("Failed to write data to file: %1").arg(file);
        outFile.cancelWriting();
        return false;
    }
    if (!outFile.commit()) {
        lastError = QString("Failed to finalize file: %1").arg(file);
        return false;
    }

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

QString URLDownloader::getRemoteChecksum(const QString &url)
{
    // build URL for the SHA256SUMS file in the same remote directory
    int lastSlash = url.lastIndexOf('/');
    if (lastSlash < 0) return QString();

    QString sumsUrl  = url.left(lastSlash + 1) + "SHA256SUMS";
    QString fileName = url.mid(lastSlash + 1);

    QByteArray sumsData = fetchRawContent(sumsUrl);
    if (sumsData.isEmpty()) return QString();

    // parse SHA256SUMS: each line is "<hex-hash>  <filename>" or "<hex-hash> <filename>"
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

        if (name == fileName) return hash.toLower();
    }
    return QString();
}

QString URLDownloader::getLocalChecksum(const QString &file)
{
    QFile localFile(file);
    if (!localFile.open(QIODevice::ReadOnly)) return QString();

    QCryptographicHash hasher(QCryptographicHash::Sha256);
    hasher.addData(&localFile);
    localFile.close();

    return hasher.result().toHex().toLower();
}

bool URLDownloader::verifyChecksum(const QString &url, const QString &file)
{
    QString expectedHash = getRemoteChecksum(url);
    if (expectedHash.isEmpty()) return true; // no SHA256SUMS entry — nothing to check

    QString actualHash = getLocalChecksum(file);
    if (actualHash.isEmpty()) return true;

    if (actualHash != expectedHash) {
        int lastSlash    = url.lastIndexOf('/');
        QString fileName = url.mid(lastSlash + 1);
        lastError        = QString("SHA-256 checksum mismatch for %1").arg(fileName);
        critical(parentWidget, "Download Error",
                 QString("The SHA-256 checksum of the downloaded file \"%1\" does not match "
                         "the expected checksum.")
                     .arg(fileName),
                 QString("Expected: %1\nActual: %2").arg(expectedHash, actualHash));
        return false;
    }

    return true;
}

// Local Variables:
// c-basic-offset: 4
// End:
