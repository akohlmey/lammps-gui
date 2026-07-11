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

#include "movieimport.h"

#include "constants.h"
#include "helpers.h"
#include "qaddon.h"

#include <QApplication>
#include <QByteArray>
#include <QDialogButtonBox>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QLabel>
#include <QLocale>
#include <QPalette>
#include <QProcess>
#include <QProgressDialog>
#include <QPushButton>
#include <QSpinBox>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

/* ---------------------------------------------------------------------- */

namespace {
constexpr int LAYOUT_SPACING = 6;
constexpr int NOTE_ICON_SIZE = 32;
constexpr int KILL_TIMEOUT   = 1000;
constexpr int PROGRESS_TICK  = 100;
constexpr int PROGRESS_DELAY = 500; // ms before the extraction progress dialog appears

// ffprobe reports numbers as JSON numbers or as strings, depending on the key
int jsonToInt(const QJsonValue &val)
{
    if (val.isDouble()) return static_cast<int>(val.toDouble());
    bool ok         = false;
    const int value = val.toString().toInt(&ok);
    return ok ? value : 0;
}

double jsonToDouble(const QJsonValue &val)
{
    if (val.isDouble()) return val.toDouble();
    bool ok            = false;
    const double value = val.toString().toDouble(&ok);
    return ok ? value : 0.0;
}

// Count the packets of the video stream. This does not decode the frames and
// is thus fast, but it only works for containers where each packet is a frame.
int countPackets(const QString &filename)
{
    QProcess proc;
    proc.start("ffprobe", {"-v", "error", "-select_streams", "v:0", "-count_packets",
                           "-show_entries", "stream=nb_read_packets", "-of", "csv=p=0", filename});
    if (!proc.waitForFinished(Cfg::MOVIE_PROBE_TIMEOUT)) {
        proc.kill();
        proc.waitForFinished(KILL_TIMEOUT);
        return 0;
    }
    if ((proc.exitStatus() != QProcess::NormalExit) || (proc.exitCode() != 0)) return 0;

    bool ok           = false;
    const int packets = QString::fromLocal8Bit(proc.readAllStandardOutput()).trimmed().toInt(&ok);
    return ok ? packets : 0;
}

// Decode a single frame near the given time to a PNG file and return its size.
// The seek is approximate (to the preceding key frame) since the exact frame
// does not matter for estimating how much space a whole sequence will need.
qint64 sampleFrameSize(const QString &filename, double seconds)
{
    QTemporaryFile tmp(QDir::tempPath() + "/lammpsgui_sampleXXXXXX.png");
    if (!tmp.open()) return 0;
    const QString pngname = tmp.fileName();
    tmp.close();

    QProcess proc;
    proc.start("ffmpeg",
               {"-y", "-nostdin", "-loglevel", "error", "-ss", QString::number(seconds, 'f', 3),
                "-i", filename, "-frames:v", "1", "-vsync", "0", pngname});
    if (!proc.waitForFinished(Cfg::MOVIE_PROBE_TIMEOUT)) {
        proc.kill();
        proc.waitForFinished(KILL_TIMEOUT);
        return 0;
    }
    if ((proc.exitStatus() != QProcess::NormalExit) || (proc.exitCode() != 0)) return 0;
    return QFileInfo(pngname).size();
}

// Remove the frames of an aborted or failed extraction
void purgeFrames(const QString &outdir)
{
    QDir dir(outdir);
    const auto files = dir.entryList({"frame_*.png"}, QDir::Files);
    for (const auto &file : files)
        dir.remove(file);
}
} // namespace

/* ---------------------------------------------------------------------- */

double parseFrameRate(const QString &rate)
{
    bool ok                 = false;
    const QStringList parts = rate.split('/');
    if (parts.size() == 2) {
        bool okden       = false;
        const double num = parts[0].toDouble(&ok);
        const double den = parts[1].toDouble(&okden);
        if (ok && okden && (den != 0.0)) return num / den;
        return 0.0;
    }
    const double value = rate.toDouble(&ok);
    return ok ? value : 0.0;
}

int selectedFrameCount(int first, int last, int interval)
{
    if ((interval < 1) || (last < first)) return 0;
    return (last - first) / interval + 1;
}

MovieInfo parseProbeOutput(const QByteArray &json)
{
    MovieInfo info;

    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(json, &err);
    if ((err.error != QJsonParseError::NoError) || !doc.isObject()) {
        info.error = QString("Cannot parse the output of ffprobe: %1").arg(err.errorString());
        return info;
    }

    const auto root    = doc.object();
    const auto streams = root.value("streams").toArray();
    if (streams.isEmpty()) {
        info.error = "The file does not contain a video stream.";
        return info;
    }

    const auto stream = streams.at(0).toObject();
    info.width        = jsonToInt(stream.value("width"));
    info.height       = jsonToInt(stream.value("height"));
    info.fps          = parseFrameRate(stream.value("r_frame_rate").toString());
    if ((info.width <= 0) || (info.height <= 0)) {
        info.error = "The video stream has no usable frame size.";
        return info;
    }

    // both entries are absent or "N/A" for containers that do not store them
    const int frames = jsonToInt(stream.value("nb_frames"));
    if (frames > 0) info.frames = frames;

    double duration = jsonToDouble(stream.value("duration"));
    if (duration <= 0.0) duration = jsonToDouble(root.value("format").toObject().value("duration"));
    if (duration > 0.0) info.duration = duration;

    info.valid = true;
    return info;
}

MovieInfo probeMovie(const QString &filename)
{
    MovieInfo info;

    if (!QFileInfo::exists(filename)) {
        info.error = QString("The file \"%1\" does not exist.").arg(filename);
        return info;
    }
    if (!hasExe("ffprobe") || !hasExe("ffmpeg")) {
        info.error = "Importing the frames of a movie file requires the ffmpeg and ffprobe "
                     "programs, but they were not found in the executable search path.";
        return info;
    }

    QProcess proc;
    proc.start("ffprobe", {"-v", "error", "-select_streams", "v:0", "-show_entries",
                           "stream=width,height,nb_frames,r_frame_rate,duration", "-show_entries",
                           "format=duration", "-of", "json", filename});
    if (!proc.waitForFinished(Cfg::MOVIE_PROBE_TIMEOUT)) {
        proc.kill();
        proc.waitForFinished(KILL_TIMEOUT);
        info.error = "The ffprobe program did not finish in time.";
        return info;
    }
    if ((proc.exitStatus() != QProcess::NormalExit) || (proc.exitCode() != 0)) {
        info.error = QString("The ffprobe program failed to read the movie file:\n%1")
                         .arg(QString::fromLocal8Bit(proc.readAllStandardError()).trimmed());
        return info;
    }

    info = parseProbeOutput(proc.readAllStandardOutput());
    if (!info.valid) return info;

    // Some containers (webm, mkv) do not store the number of frames. Counting
    // the video packets is exact and fast; the frame rate times the duration
    // is the last resort and may be off by a frame for a variable frame rate.
    if (info.frames <= 0) info.frames = countPackets(filename);
    if ((info.frames <= 0) && (info.fps > 0.0) && (info.duration > 0.0))
        info.frames = static_cast<int>(std::lround(info.duration * info.fps));

    if (info.frames <= 0) {
        info.valid = false;
        info.error = "Could not determine the number of frames of the movie file.";
    }
    return info;
}

QStringList extractMovieFrames(QWidget *parent, const QString &filename, const QString &outdir,
                               int first, int last, int interval, QString &error)
{
    error.clear();
    const int expected = selectedFrameCount(first, last, interval);
    if (expected < 1) {
        error = "No frames are selected for extraction.";
        return {};
    }
    if (!hasExe("ffmpeg")) {
        error = "The ffmpeg program was not found in the executable search path.";
        return {};
    }

    // FFmpeg counts frames from 0, the dialog and the slide show from 1. The
    // filter expression is quoted so that its commas do not separate filters.
    const int begin = first - 1;
    const int end   = last - 1;
    QString select  = QString("select='between(n,%1,%2)").arg(begin).arg(end);
    if (interval > 1) select += QString("*not(mod(n-%1,%2))").arg(begin).arg(interval);
    select += "'";

    QStringList args;
    args << "-y"
         << "-nostdin"
         << "-nostats"
         << "-loglevel"
         << "error";
    args << "-progress"
         << "pipe:1";
    args << "-i" << filename;
    args << "-vf" << select;
    // keep only the frames the filter selects instead of padding to a fixed
    // frame rate, and stop decoding once the last selected frame is written
    args << "-vsync"
         << "0";
    args << "-frames:v" << QString::number(expected);
    args << QDir(outdir).absoluteFilePath("frame_%05d.png");

    QProgressDialog progress(QString("Extracting %1 frames from %2 ...")
                                 .arg(expected)
                                 .arg(QFileInfo(filename).fileName()),
                             "Cancel", 0, expected, parent);
    progress.setWindowTitle("LAMMPS-GUI - Importing Movie Frames");
    progress.setWindowIcon(QIcon(Cfg::MAIN_ICON));
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(PROGRESS_DELAY);
    progress.setAutoClose(false);
    progress.setAutoReset(false);
    progress.setValue(0);

    QProcess proc;
    QEventLoop loop;
    QByteArray stdoutbuf;
    QByteArray stderrbuf;
    int done      = 0;
    bool canceled = false;
    bool finished = false;

    // The progress output of FFmpeg reports the number of written frames.  It
    // is collected here and shown by a timer, since updating the dialog from
    // the read handler would process events while reading from the process.
    QObject::connect(&proc, &QProcess::readyReadStandardOutput, &proc, [&]() {
        stdoutbuf += proc.readAllStandardOutput();
        int eol = -1;
        while ((eol = stdoutbuf.indexOf('\n')) >= 0) {
            const QByteArray line = stdoutbuf.left(eol).trimmed();
            stdoutbuf.remove(0, eol + 1);
            if (line.startsWith("frame=")) {
                bool ok           = false;
                const int written = line.mid(6).trimmed().toInt(&ok);
                if (ok) done = written;
            }
        }
    });
    QObject::connect(&proc, &QProcess::readyReadStandardError, &proc, [&]() {
        stderrbuf += proc.readAllStandardError();
    });
    QObject::connect(&proc, &QProcess::finished, &proc, [&]() {
        finished = true;
        loop.quit();
    });
    QObject::connect(&proc, &QProcess::errorOccurred, &proc, [&]() {
        finished = true;
        loop.quit();
    });
    QObject::connect(&progress, &QProgressDialog::canceled, &progress, [&]() {
        canceled = true;
        proc.kill();
    });

    QTimer ticker;
    QObject::connect(&ticker, &QTimer::timeout, &ticker, [&]() {
        progress.setValue(std::min(done, expected));
    });
    ticker.start(PROGRESS_TICK);

    proc.start("ffmpeg", args);
    if (!proc.waitForStarted()) {
        error = "The ffmpeg program could not be started.";
        return {};
    }
    // guard against a process that is already gone when the loop would start
    if (!finished) loop.exec();
    ticker.stop();
    // hide() rather than close(): QProgressDialog emits canceled() on a close event
    progress.hide();

    if (canceled) {
        purgeFrames(outdir);
        error = "The extraction of the movie frames was canceled.";
        return {};
    }
    if ((proc.exitStatus() != QProcess::NormalExit) || (proc.exitCode() != 0)) {
        purgeFrames(outdir);
        stderrbuf += proc.readAllStandardError();
        error = QString("The ffmpeg program failed to extract the movie frames:\n%1")
                    .arg(QString::fromLocal8Bit(stderrbuf).trimmed());
        return {};
    }

    QDir dir(outdir);
    QStringList frames = dir.entryList({"frame_*.png"}, QDir::Files, QDir::Name);
    if (frames.isEmpty()) {
        error = "The ffmpeg program did not extract any movie frames.";
        return {};
    }
    for (auto &frame : frames)
        frame = dir.absoluteFilePath(frame);
    return frames;
}

/* ---------------------------------------------------------------------- */

MovieImportDialog::MovieImportDialog(const QString &filename, const MovieInfo &info,
                                     QWidget *parent) :
    QDialog(parent), movieinfo(info), samplebytes(0), diskfree(0), firstBox(new QSpinBox),
    lastBox(new QSpinBox), stepBox(new QSpinBox), countLabel(new QLabel), sizeLabel(new QLabel),
    noteIcon(new QLabel), noteLabel(new QLabel)
{
    setWindowTitle("LAMMPS-GUI - Import Movie Frames");
    setWindowIcon(QIcon(Cfg::MAIN_ICON));

    // decoding one frame in the middle of the movie calibrates the size
    // estimate; the first frame is often a title or an empty box and thus
    // compresses far better than a typical frame of the trajectory
    QApplication::setOverrideCursor(Qt::WaitCursor);
    samplebytes = sampleFrameSize(filename, 0.5 * movieinfo.duration);
    QApplication::restoreOverrideCursor();
    const QStorageInfo storage(QDir::tempPath());
    if (storage.isValid() && storage.isReady()) diskfree = storage.bytesAvailable();

    auto *headIcon = new QLabel;
    headIcon->setPixmap(
        QIcon(":/icons/export-movie.svg").pixmap(QSize(64, 64), devicePixelRatioF()));
    headIcon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

    auto *headText =
        new QLabel(QString("The movie file \"%1\" must be decompressed into individual images "
                           "before they can be shown in the slide show viewer.  The images are "
                           "written to a temporary folder and are removed when the slide show "
                           "window is closed.")
                       .arg(QFileInfo(filename).fileName()));
    headText->setWordWrap(true);

    auto *headLayout = new QHBoxLayout;
    headLayout->addWidget(headIcon);
    headLayout->addWidget(headText, 10);
    headLayout->setSpacing(LAYOUT_SPACING);

    // properties of the movie
    auto *propLayout = new QGridLayout;
    int row          = 0;
    propLayout->addWidget(new QLabel("Frame size:"), row, 0, 1, 1, Qt::AlignRight);
    propLayout->addWidget(
        new QLabel(QString("%1 x %2 pixels").arg(movieinfo.width).arg(movieinfo.height)), row++, 1);
    propLayout->addWidget(new QLabel("Total frames:"), row, 0, 1, 1, Qt::AlignRight);
    propLayout->addWidget(new QLabel(QString::number(movieinfo.frames)), row++, 1);
    propLayout->addWidget(new QLabel("Frame rate:"), row, 0, 1, 1, Qt::AlignRight);
    propLayout->addWidget(new QLabel(QString("%1 frames per second").arg(movieinfo.fps, 0, 'f', 2)),
                          row++, 1);
    propLayout->addWidget(new QLabel("Duration:"), row, 0, 1, 1, Qt::AlignRight);
    propLayout->addWidget(new QLabel(QString("%1 seconds").arg(movieinfo.duration, 0, 'f', 2)),
                          row++, 1);
    propLayout->setColumnStretch(1, 10);
    propLayout->setSpacing(LAYOUT_SPACING);

    // selection of the frames to extract
    firstBox->setRange(1, movieinfo.frames);
    firstBox->setValue(1);
    firstBox->setToolTip("First frame of the movie to extract");

    lastBox->setRange(1, movieinfo.frames);
    lastBox->setValue(movieinfo.frames);
    lastBox->setToolTip("Last frame of the movie to extract");

    stepBox->setRange(1, movieinfo.frames);
    stepBox->setValue(1);
    stepBox->setToolTip("Extract only every n-th frame of the selected range");

    // keep the range ordered: the first frame must never follow the last one
    connect(firstBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (val > lastBox->value()) lastBox->setValue(val);
        updateEstimate();
    });
    connect(lastBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int val) {
        if (val < firstBox->value()) firstBox->setValue(val);
        updateEstimate();
    });
    connect(stepBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            &MovieImportDialog::updateEstimate);

    auto *selLayout = new QGridLayout;
    row             = 0;
    selLayout->addWidget(new QLabel("First frame:"), row, 0, 1, 1, Qt::AlignRight);
    selLayout->addWidget(firstBox, row, 1);
    selLayout->addWidget(new QLabel("Last frame:"), row, 2, 1, 1, Qt::AlignRight);
    selLayout->addWidget(lastBox, row++, 3);
    selLayout->addWidget(new QLabel("Frame interval:"), row, 0, 1, 1, Qt::AlignRight);
    selLayout->addWidget(stepBox, row, 1);
    selLayout->addWidget(new QLabel("Images to extract:"), row, 2, 1, 1, Qt::AlignRight);
    selLayout->addWidget(countLabel, row++, 3);
    selLayout->addWidget(new QLabel("Estimated size:"), row, 0, 1, 1, Qt::AlignRight);
    selLayout->addWidget(sizeLabel, row++, 1, 1, 3);
    selLayout->setColumnStretch(1, 5);
    selLayout->setColumnStretch(3, 5);
    selLayout->setSpacing(LAYOUT_SPACING);

    // the size estimate note, which turns into a highlighted warning
    noteIcon->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    noteLabel->setWordWrap(true);
    noteLabel->setMinimumWidth(NOTE_ICON_SIZE * 10);

    auto *noteLayout = new QHBoxLayout;
    noteLayout->addWidget(noteIcon);
    noteLayout->addWidget(noteLabel, 10);
    noteLayout->setSpacing(LAYOUT_SPACING);

    auto *noteFrame = new QFrame;
    noteFrame->setFrameStyle(QFrame::Sunken);
    noteFrame->setFrameShape(QFrame::Panel);
    noteFrame->setLayout(noteLayout);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Ok)->setText("&Import");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    styleDialogButtons(buttonBox);

    auto *mainLayout = new QVBoxLayout;
    mainLayout->addLayout(headLayout);
    mainLayout->addWidget(new QHline);
    mainLayout->addLayout(propLayout);
    mainLayout->addWidget(new QHline);
    mainLayout->addLayout(selLayout);
    mainLayout->addWidget(noteFrame);
    mainLayout->addWidget(buttonBox);
    mainLayout->setSpacing(LAYOUT_SPACING);
    setLayout(mainLayout);

    updateEstimate();
}

int MovieImportDialog::firstFrame() const
{
    return firstBox->value();
}

int MovieImportDialog::lastFrame() const
{
    return lastBox->value();
}

int MovieImportDialog::frameInterval() const
{
    return stepBox->value();
}

void MovieImportDialog::updateEstimate()
{
    const int count = selectedFrameCount(firstBox->value(), lastBox->value(), stepBox->value());
    countLabel->setText(QString::number(count));

    const qint64 estimate = samplebytes * count;
    if (samplebytes > 0)
        sizeLabel->setText(QString("%1 (about %2 per image)")
                               .arg(locale().formattedDataSize(estimate),
                                    locale().formattedDataSize(samplebytes)));
    else
        sizeLabel->setText("unknown");

    // the estimate extrapolates from a single frame, so it is approximate and
    // the thresholds below are deliberately generous
    QStringList reasons;
    if ((diskfree > 0) && (estimate > static_cast<qint64>(Cfg::MOVIE_WARN_DISKFRAC * diskfree)))
        reasons << QString("the images would use most of the %1 of free space on the volume "
                           "holding the temporary folder")
                       .arg(locale().formattedDataSize(diskfree));
    else if (estimate > Cfg::MOVIE_WARN_BYTES)
        reasons << QString("the images will need about %1 of temporary disk space")
                       .arg(locale().formattedDataSize(estimate));
    if (count > Cfg::MOVIE_WARN_FRAMES)
        reasons << QString("extracting %1 images will take a while").arg(count);

    const bool darkmode = palette().color(QPalette::Window).lightness() < 128;
    if (reasons.isEmpty()) {
        noteIcon->setPixmap(
            QIcon(":/icons/image-x-generic.svg")
                .pixmap(QSize(NOTE_ICON_SIZE, NOTE_ICON_SIZE), devicePixelRatioF()));
        noteLabel->setStyleSheet("");
        noteLabel->setText("The size estimate is extrapolated from a single decoded frame and "
                           "may differ from the actual size of the extracted images.");
    } else {
        noteIcon->setPixmap(
            QIcon(":/icons/warning.svg")
                .pixmap(QSize(NOTE_ICON_SIZE, NOTE_ICON_SIZE), devicePixelRatioF()));
        noteLabel->setStyleSheet(QString("QLabel { color: %1; font-weight: bold; }")
                                     .arg(darkmode ? "#ff8080" : "#a00000"));
        noteLabel->setText(QString("Warning: %1.  Consider a smaller frame range or a larger "
                                   "frame interval.")
                               .arg(reasons.join(", and ")));
    }
}

// Local Variables:
// c-basic-offset: 4
// End:
