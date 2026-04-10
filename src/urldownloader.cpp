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

#include <QByteArray>
#include <QCoreApplication>
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

bool URLDownloader::download(const QString &url, const QString &file)
{
    lastError.clear();

    // show a temporary dialog while the download is in progress
    QDialog *dlg = nullptr;
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
