// Unit tests for the PlotData column model and file parsers
// (src/plotdata.cpp), exercised without a GUI.

#include "plotdata.h"

#include "gtest/gtest.h"

#include <QByteArray>
#include <QDir>
#include <QString>
#include <QTemporaryFile>

namespace {

int colIndex(const PlotData &d, const QString &name)
{
    for (int i = 0; i < d.columnCount(); ++i)
        if (d.columnName(i) == name) return i;
    return -1;
}

TEST(PlotDataModel, AppendRowAndColumn)
{
    PlotData d;
    d.setColumnNames({"a", "b"});
    EXPECT_EQ(d.columnCount(), 2);
    EXPECT_EQ(d.rowCount(), 0);
    EXPECT_TRUE(d.isEmpty());

    EXPECT_TRUE(d.appendRow({1.0, 2.0}));
    EXPECT_TRUE(d.appendRow({3.0, 4.0}));
    EXPECT_FALSE(d.appendRow({5.0})); // wrong width
    EXPECT_EQ(d.rowCount(), 2);
    EXPECT_DOUBLE_EQ(d.column(0)[1], 3.0);
    EXPECT_DOUBLE_EQ(d.column(1)[0], 2.0);

    d.addColumn("c", {7.0, 8.0});
    EXPECT_EQ(d.columnCount(), 3);
    EXPECT_EQ(colIndex(d, "c"), 2);
}

TEST(PlotDataCsv, WithHeader)
{
    const QString text = "Step,Temp,Press\n0,300,1.0\n1,310,1.1\n";
    const PlotData d   = parsePlotCsv(text);
    ASSERT_EQ(d.columnCount(), 3);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "Step");
    EXPECT_EQ(d.columnName(1), "Temp");
    EXPECT_DOUBLE_EQ(d.column(1)[0], 300.0);
    EXPECT_DOUBLE_EQ(d.column(2)[1], 1.1);
}

TEST(PlotDataCsv, WithoutHeaderGeneratesNames)
{
    const QString text = "0,1,2\n3,4,5\n";
    const PlotData d   = parsePlotCsv(text);
    ASSERT_EQ(d.columnCount(), 3);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "column1");
    EXPECT_DOUBLE_EQ(d.column(0)[0], 0.0);
    EXPECT_DOUBLE_EQ(d.column(2)[1], 5.0);
}

TEST(PlotDataCsv, EmptyIsError)
{
    QString err;
    const PlotData d = parsePlotCsv("", &err);
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(PlotDataWhitespace, LammpsDatHeader)
{
    // mirrors ChartWindow::exportDat(): a description comment then a column
    // header comment, followed by aligned numeric columns
    const QString text = "# Thermodynamic data from foo\n"
                         "#          Step Temp Press\n"
                         "           0   300  1.0\n"
                         "           1   310  1.1\n";
    const PlotData d   = parsePlotWhitespace(text);
    ASSERT_EQ(d.columnCount(), 3);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "Step");
    EXPECT_EQ(d.columnName(2), "Press");
    EXPECT_DOUBLE_EQ(d.column(1)[1], 310.0);
}

TEST(PlotDataWhitespace, NoHeaderGeneratesNames)
{
    const QString text = "1.0 2.0\n3.0 4.0\n";
    const PlotData d   = parsePlotWhitespace(text);
    ASSERT_EQ(d.columnCount(), 2);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "column1");
    EXPECT_DOUBLE_EQ(d.column(1)[1], 4.0);
}

TEST(PlotDataYaml, KeywordsAndData)
{
    // mirrors ChartWindow::exportYaml()
    const QString text = "---\n"
                         "keywords: ['Step', Temp, Press]\n"
                         "data: \n"
                         "  - [0, 300, 1.0]\n"
                         "  - [1, 310, 1.1]\n"
                         "...\n";
    const PlotData d   = parsePlotYaml(text);
    ASSERT_EQ(d.columnCount(), 3);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "Step");
    EXPECT_EQ(d.columnName(1), "Temp");
    EXPECT_DOUBLE_EQ(d.column(2)[0], 1.0);
    EXPECT_DOUBLE_EQ(d.column(0)[1], 1.0);
}

TEST(PlotDataJson, ArrayOfRows)
{
    const QByteArray json = "[[0,300,1.0],[1,310,1.1]]";
    const PlotData d      = parsePlotJson(json);
    ASSERT_EQ(d.columnCount(), 3);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(0), "column1");
    EXPECT_DOUBLE_EQ(d.column(1)[1], 310.0);
}

TEST(PlotDataJson, ObjectOfArrays)
{
    const QByteArray json = R"({"Step":[0,1],"Temp":[300,310]})";
    const PlotData d      = parsePlotJson(json);
    ASSERT_EQ(d.columnCount(), 2);
    ASSERT_EQ(d.rowCount(), 2);
    const int s = colIndex(d, "Step");
    const int t = colIndex(d, "Temp");
    ASSERT_GE(s, 0);
    ASSERT_GE(t, 0);
    EXPECT_DOUBLE_EQ(d.column(s)[1], 1.0);
    EXPECT_DOUBLE_EQ(d.column(t)[0], 300.0);
}

TEST(PlotDataJson, UnequalColumnsIsError)
{
    QString err;
    const PlotData d = parsePlotJson(R"({"a":[1,2],"b":[3]})", &err);
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(PlotDataJson, MalformedIsError)
{
    QString err;
    const PlotData d = parsePlotJson("{not json", &err);
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

TEST(LoadPlotData, DispatchesByExtension)
{
    QTemporaryFile f(QDir::tempPath() + "/plotdataXXXXXX.csv");
    ASSERT_TRUE(f.open());
    f.write("Step,Val\n0,1.5\n1,2.5\n");
    f.flush();
    const QString name = f.fileName();

    QString err;
    const PlotData d = loadPlotData(name, &err);
    EXPECT_TRUE(err.isEmpty()) << err.toStdString();
    ASSERT_EQ(d.columnCount(), 2);
    ASSERT_EQ(d.rowCount(), 2);
    EXPECT_EQ(d.columnName(1), "Val");
    EXPECT_DOUBLE_EQ(d.column(1)[1], 2.5);
}

} // namespace
