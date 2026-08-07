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

#include "plotblockdata.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>

/* -------------------------------------------------------------------- */

namespace {

// Whitespace splitter shared by the line scanners.
const QRegularExpression &wsRe()
{
    static const QRegularExpression re("\\s+");
    return re;
}

// Convert whitespace-separated tokens to numbers.  Returns false on the first
// token that is not a number, which is how comment text and stray log output
// are told apart from data.
bool numericTokens(const QStringList &toks, std::vector<double> &row)
{
    row.clear();
    row.reserve(toks.size());
    for (const QString &t : toks) {
        bool good      = false;
        const double v = t.toDouble(&good);
        if (!good) return false;
        row.push_back(v);
    }
    return !row.empty();
}

// Placeholder names matching those of the flat parsers in plotdata.cpp.
QStringList genericNames(int ncol)
{
    QStringList names;
    names.reserve(ncol);
    for (int i = 0; i < ncol; ++i)
        names << QStringLiteral("column%1").arg(i + 1);
    return names;
}

// From a run of comment lines (the leading '#' already stripped), return the
// fields of the last one that has exactly @p count of them.
//
// This is what makes the parser independent of the individual fix styles: the
// comment naming the data columns is simply the one as wide as a data row, and
// the comment naming the block header fields the one as wide as a block header.
// Both fall back to nothing when title1/2/3 replaced the default headers.
QStringList commentWithFields(const QStringList &comments, int count)
{
    for (auto it = comments.crbegin(); it != comments.crend(); ++it) {
        const QStringList fields = it->split(wsRe(), Qt::SkipEmptyParts);
        if (fields.size() == count) return fields;
    }
    return {};
}

// Value that can serve as a row count on a block header line.
bool isRowCount(double v)
{
    return (v >= 1.0) && (v < 1.0e9) && (std::fabs(v - std::round(v)) < 1.0e-9);
}

// Does column 0 of every block count 1, 2, ... n?  Every native block format
// writes such a row index, and no plain data table does, so this is what keeps
// a flat file from being mistaken for a block file no matter what its comment
// lines claim.
bool hasRowIndexColumns(const PlotBlockData &data)
{
    for (const PlotDataBlock &b : data.blocks) {
        if (b.rows.columnCount() < 1) return false;
        const std::vector<double> &idx = b.rows.column(0);
        for (std::size_t r = 0; r < idx.size(); ++r)
            if (std::fabs(idx[r] - static_cast<double>(r + 1)) > 1.0e-9) return false;
    }
    return true;
}

// Whether a parse result is really block-structured output and not an
// accidental reading of some other file.
bool plausibleBlocks(const PlotBlockData &data)
{
    if (data.blocks.empty()) return false;
    for (const PlotDataBlock &b : data.blocks)
        if (b.rows.rowCount() < 1) return false;
    // the comment-delimited format has no row index, but its "# Timestep: N"
    // delimiter is unambiguous by itself
    if (data.kind == AveFileKind::AveCorrelateLong) return true;
    return hasRowIndexColumns(data);
}

// Second column of the rows counting 0, k, 2k, ... is the time-delta column of
// fix ave/correlate, and tells it apart from a fix ave/time vector whose block
// header happens to be as wide.
bool looksLikeTimeDeltaColumn(const PlotData &rows)
{
    if ((rows.columnCount() < 4) || (rows.rowCount() < 3)) return false;
    const std::vector<double> &d = rows.column(1);
    if (d[0] != 0.0) return false;
    const double step = d[1];
    if (step <= 0.0) return false;
    for (std::size_t r = 0; r < d.size(); ++r)
        if (std::fabs(d[r] - static_cast<double>(r) * step) > 1.0e-6 * step) return false;
    return true;
}

// Identify the writing fix from its default first header line, falling back to
// the block structure when title1/2/3 replaced the default headers.
AveFileKind detectKind(const QString &firstComment, bool commentDelimited, int headerWidth,
                       const PlotData &rows)
{
    if (commentDelimited) return AveFileKind::AveCorrelateLong;
    if (firstComment.contains("Histogrammed data for fix")) return AveFileKind::AveHisto;
    if (firstComment.contains("Time-correlated data for fix")) return AveFileKind::AveCorrelate;
    if (firstComment.contains("Time-averaged data for fix")) return AveFileKind::AveTimeVector;
    // fix ave/chunk is recognized as block structured and imports through the
    // generic path, but has no defaults of its own yet
    if (firstComment.contains("Chunk-averaged data for fix")) return AveFileKind::Unknown;

    if ((headerWidth == 6) && (rows.columnCount() == 4)) return AveFileKind::AveHisto;
    if (headerWidth == 2)
        return looksLikeTimeDeltaColumn(rows) ? AveFileKind::AveCorrelate
                                              : AveFileKind::AveTimeVector;
    return AveFileKind::Unknown;
}

// The fix ID out of any of the default first header lines.
QString detectFixId(const QString &firstComment)
{
    static const QRegularExpression re("data for fix (\\S+)");
    const auto match = re.match(firstComment);
    return match.hasMatch() ? match.captured(1) : QString();
}

// Strip a single layer of matching quotes from a YAML token.
QString unquote(QString t)
{
    t = t.trimmed();
    if (t.size() >= 2) {
        const QChar f = t.front();
        const QChar l = t.back();
        if ((f == l) && ((f == '\'') || (f == '"'))) t = t.mid(1, t.size() - 2);
    }
    return t.trimmed();
}

// Text between the first '[' and the last ']' (empty if not found).
QString bracketContents(const QString &s)
{
    const int a = s.indexOf('[');
    const int b = s.lastIndexOf(']');
    if ((a < 0) || (b < 0) || (b <= a)) return {};
    return s.mid(a + 1, b - a - 1);
}

} // namespace

/* -------------------------------------------------------------------- */

QString aveFileKindName(AveFileKind kind)
{
    switch (kind) {
        case AveFileKind::AveTimeVector:
            return QStringLiteral("fix ave/time (vector)");
        case AveFileKind::AveHisto:
            return QStringLiteral("fix ave/histo");
        case AveFileKind::AveCorrelate:
            return QStringLiteral("fix ave/correlate");
        case AveFileKind::AveCorrelateLong:
            return QStringLiteral("fix ave/correlate/long");
        case AveFileKind::Unknown:
            break;
    }
    return QStringLiteral("generic block data");
}

/* -------------------------------------------------------------------- */

PlotBlockData parseAveBlocks(const QString &text, QString *error)
{
    PlotBlockData out;
    const QStringList lines = text.split('\n');
    static const QRegularExpression timestepRe("^#\\s*Timestep:\\s*(-?\\d+)");

    QStringList commentRun;      // comment lines seen since the last data line
    QStringList sectionComments; // header comments the current blocks belong to
    QStringList firstSectionComments;
    QString firstComment;
    bool commentDelimited = false; // "# Timestep: N" delimits blocks (correlate/long)
    long long pendingStep = 0;

    PlotDataBlock cur;
    bool haveBlock       = false;
    int expectedRows     = 0;
    int rowWidth         = -1;
    int headerWidth      = -1;
    int firstHeaderWidth = -1;

    auto closeBlock = [&]() {
        if (haveBlock && (cur.rows.rowCount() > 0)) out.blocks.push_back(std::move(cur));
        cur       = PlotDataBlock();
        haveBlock = false;
        rowWidth  = -1;
    };

    // Name the columns of a freshly opened block from the header comment that
    // is as wide as its rows.
    auto startRows = [&](int width) {
        QStringList names = commentWithFields(sectionComments, width);
        if (names.isEmpty()) names = genericNames(width);
        cur.rows.setColumnNames(names);
        rowWidth = width;
    };

    // Open a block from a numeric block header line: step, row count, extras.
    auto openHeader = [&](const std::vector<double> &v) {
        if ((v.size() < 2) || !isRowCount(v[1])) return;
        cur      = PlotDataBlock();
        cur.step = static_cast<long long>(std::llround(v[0]));
        cur.scalars.assign(v.begin() + 2, v.end());
        expectedRows = static_cast<int>(std::llround(v[1]));
        haveBlock    = true;
        rowWidth     = -1;
        headerWidth  = static_cast<int>(v.size());
        if (firstHeaderWidth < 0) {
            firstHeaderWidth     = headerWidth;
            firstSectionComments = sectionComments;
        }
    };

    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith('#')) {
            if (firstComment.isEmpty()) firstComment = line;
            // any comment terminates the block being read
            closeBlock();
            const auto match = timestepRe.match(line);
            if (match.hasMatch()) {
                commentDelimited = true;
                pendingStep      = match.captured(1).toLongLong();
            } else {
                commentRun << line.mid(1).trimmed();
            }
            continue;
        }

        const QStringList toks = line.split(wsRe(), Qt::SkipEmptyParts);
        std::vector<double> v;
        if (!numericTokens(toks, v)) {
            // non-numeric, non-comment text (e.g. log output the file was
            // appended to) ends the current block and the current header run
            closeBlock();
            commentRun.clear();
            continue;
        }

        if (!commentRun.isEmpty()) {
            sectionComments = commentRun;
            commentRun.clear();
        }

        if (commentDelimited) {
            if (!haveBlock) {
                cur       = PlotDataBlock();
                cur.step  = pendingStep;
                haveBlock = true;
                rowWidth  = -1;
                if (firstSectionComments.isEmpty()) firstSectionComments = sectionComments;
            }
            if (rowWidth < 0) startRows(static_cast<int>(v.size()));
            if (static_cast<int>(v.size()) == rowWidth) cur.rows.appendRow(v);
            continue;
        }

        if (!haveBlock) {
            openHeader(v);
            continue;
        }
        if (rowWidth < 0) {
            startRows(static_cast<int>(v.size()));
            cur.rows.appendRow(v);
            continue;
        }
        if ((static_cast<int>(v.size()) == rowWidth) && (cur.rows.rowCount() < expectedRows)) {
            cur.rows.appendRow(v);
            continue;
        }
        // the block is complete (or was cut short by an interrupted run): this
        // line is the header of the next one
        closeBlock();
        openHeader(v);
    }
    closeBlock();

    if (out.blocks.empty()) {
        if (error) *error = QStringLiteral("no block-structured data found");
        return out;
    }

    out.kind =
        detectKind(firstComment, commentDelimited, firstHeaderWidth, out.blocks.front().rows);
    out.fixId = detectFixId(firstComment);
    if (firstHeaderWidth > 2) {
        const QStringList fields = commentWithFields(firstSectionComments, firstHeaderWidth);
        for (int i = 2; i < firstHeaderWidth; ++i)
            out.scalarNames << (fields.isEmpty() ? QStringLiteral("value%1").arg(i - 1)
                                                 : fields[i]);
    }
    return out;
}

/* -------------------------------------------------------------------- */

PlotBlockData parseAveBlocksYaml(const QString &text, QString *error)
{
    PlotBlockData out;
    const QStringList lines = text.split('\n');

    QStringList keywords;
    PlotDataBlock cur;
    bool haveBlock = false;
    int rowWidth   = -1;

    auto closeBlock = [&]() {
        if (haveBlock && (cur.rows.rowCount() > 0)) out.blocks.push_back(std::move(cur));
        cur       = PlotDataBlock();
        haveBlock = false;
        rowWidth  = -1;
    };

    for (const QString &raw : lines) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) continue;

        if (line.startsWith("keywords:")) {
            keywords.clear();
            const QStringList toks = bracketContents(line).split(',');
            for (const QString &t : toks) {
                // tolerate the trailing comma LAMMPS writes inside flow sequences
                const QString name = unquote(t);
                if (!name.isEmpty()) keywords << name;
            }
            continue;
        }

        if (line.startsWith('-') && line.contains('[')) {
            // a data row; without an open block this is the scalar-mode shape,
            // which is a flat table parsePlotYaml() already handles
            if (!haveBlock) continue;
            const QStringList toks = bracketContents(line).split(',');
            std::vector<double> v;
            v.reserve(toks.size() + 1);
            bool ok = true;
            for (const QString &t : toks) {
                const QString tok = t.trimmed();
                if (tok.isEmpty()) continue;
                bool good      = false;
                const double d = tok.toDouble(&good);
                if (!good) {
                    ok = false;
                    break;
                }
                v.push_back(d);
            }
            if (!ok || v.empty()) continue;
            // the YAML rows carry no row index; synthesize one so that the
            // table has the same shape as the native format
            v.insert(v.begin(), static_cast<double>(cur.rows.rowCount() + 1));
            if (rowWidth < 0) {
                rowWidth          = static_cast<int>(v.size());
                QStringList names = QStringList() << QStringLiteral("Row");
                for (int i = 0; i + 1 < rowWidth; ++i)
                    names << (i < keywords.size() ? keywords[i]
                                                  : QStringLiteral("column%1").arg(i + 2));
                cur.rows.setColumnNames(names);
            }
            if (static_cast<int>(v.size()) == rowWidth) cur.rows.appendRow(v);
            continue;
        }

        if (line.endsWith(':')) {
            const QString key = line.chopped(1).trimmed();
            bool ok           = false;
            const long long s = key.toLongLong(&ok);
            if (!ok) continue; // "data:" and any other mapping key
            closeBlock();
            cur       = PlotDataBlock();
            cur.step  = s;
            haveBlock = true;
        }
    }
    closeBlock();

    if (out.blocks.empty()) {
        if (error) *error = QStringLiteral("no block-structured YAML data found");
        return out;
    }
    // only fix ave/time writes YAML, and only its vector mode is block structured
    out.kind = AveFileKind::AveTimeVector;
    return out;
}

/* -------------------------------------------------------------------- */

bool looksLikeAveBlocks(const QString &text)
{
    return plausibleBlocks(parseAveBlocks(text));
}

/* -------------------------------------------------------------------- */

PlotBlockData loadPlotBlockData(const QString &filename, QString *error)
{
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = QStringLiteral("cannot open file: %1").arg(filename);
        return {};
    }
    const QString text = QString::fromUtf8(f.readAll());
    f.close();

    const QString suffix = QFileInfo(filename).suffix().toLower();
    bool yaml            = (suffix == "yaml") || (suffix == "yml");
    if (!yaml) {
        for (const QString &line : text.split('\n'))
            if (line.trimmed().startsWith("keywords:")) {
                yaml = true;
                break;
            }
    }
    if (yaml) return parseAveBlocksYaml(text, error);

    PlotBlockData data = parseAveBlocks(text, error);
    if (!plausibleBlocks(data)) {
        if (error && error->isEmpty()) *error = QStringLiteral("not block-structured data");
        return {};
    }
    return data;
}

// Local Variables:
// c-basic-offset: 4
// End:
