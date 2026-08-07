// Unit tests for the block-structured fix ave/* file parsers
// (src/plotblockdata.cpp), exercised without a GUI.

#include "plotblockdata.h"

#include "gtest/gtest.h"

#include <QString>

namespace {

// --- sample files, matching what the LAMMPS fixes actually write -------

// fix ave/time in vector mode: block header "step nrows", rows "row v1 ... vN"
const char ave_time_vector[] = "# Time-averaged data for fix myvec\n"
                               "# TimeStep Number-of-rows\n"
                               "# Row c_rdf[1] c_rdf[2]\n"
                               "1000 3\n"
                               "1 0.05 0.1\n"
                               "2 0.15 0.5\n"
                               "3 0.25 1.2\n"
                               "2000 3\n"
                               "1 0.05 0.3\n"
                               "2 0.15 0.7\n"
                               "3 0.25 1.4\n";

// fix ave/histo: block header "step nbins total missing min max", rows
// "bin coord count count/total"
const char ave_histo[] =
    "# Histogrammed data for fix myhisto\n"
    "# TimeStep Number-of-bins Total-counts Missing-counts Min-value Max-value\n"
    "# Bin Coord Count Count/Total\n"
    "1000 4 100 0 -1.5 2.5\n"
    "1 -1 10 0.1\n"
    "2 0 20 0.2\n"
    "3 1 40 0.4\n"
    "4 2 30 0.3\n"
    "2000 4 200 5 -1.4 2.6\n"
    "1 -1 40 0.2\n"
    "2 0 40 0.2\n"
    "3 1 80 0.4\n"
    "4 2 40 0.2\n";

// fix ave/correlate: block header "step nwindows", rows
// "index timedelta ncount c1 ..."  (here with nevery 5)
const char ave_correlate[] = "# Time-correlated data for fix mycorr\n"
                             "# Timestep Number-of-time-windows\n"
                             "# Index TimeDelta Ncount v_vx*v_vx\n"
                             "1000 3\n"
                             "1 0 100 1\n"
                             "2 5 99 0.5\n"
                             "3 10 98 0.25\n"
                             "2000 3\n"
                             "1 0 200 1\n"
                             "2 5 199 0.6\n"
                             "3 10 198 0.3\n";

// fix ave/correlate/long: two title lines only, blocks delimited by a comment,
// first column already simulation time
const char ave_correlate_long[] = "# Time-correlated data for fix myclong\n"
                                  "# Time v_vx*v_vx\n"
                                  "# Timestep: 1000\n"
                                  "0 1 \n"
                                  "0.005 0.5 \n"
                                  "0.01 0.25 \n"
                                  "# Timestep: 2000\n"
                                  "0 1 \n"
                                  "0.005 0.6 \n"
                                  "0.01 0.3 \n";

// fix ave/time in scalar mode: a flat table, must not be taken for blocks
const char ave_time_scalar[] = "# Time-averaged data for fix mysca\n"
                               "# TimeStep v_a v_b\n"
                               "100 1 2\n"
                               "200 3 4\n"
                               "300 5 6\n";

// fix ave/time vector mode YAML, as written for a .yaml file argument
const char ave_time_yaml[] = "---\n"
                             "keywords: ['c_rdf[1]', 'c_rdf[2]', ]\n"
                             "data:\n"
                             "  1000:\n"
                             "  - [0.05, 0.1, ]\n"
                             "  - [0.15, 0.5, ]\n"
                             "  2000:\n"
                             "  - [0.05, 0.3, ]\n"
                             "  - [0.15, 0.7, ]\n";

// fix ave/time scalar mode YAML: a flat list of rows, no per-step keys
const char ave_time_yaml_scalar[] = "---\n"
                                    "keywords: ['Step', 'v_a', ]\n"
                                    "data:\n"
                                    "  - [100, 1, ]\n"
                                    "  - [200, 2, ]\n";

QStringList names(const PlotDataBlock &b)
{
    return b.rows.columnNames();
}

// --- native format ----------------------------------------------------

TEST(AveBlocks, TimeVector)
{
    const PlotBlockData d = parseAveBlocks(ave_time_vector);
    EXPECT_EQ(d.kind, AveFileKind::AveTimeVector);
    EXPECT_EQ(d.fixId, "myvec");
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[0].step, 1000);
    EXPECT_EQ(d.blocks[1].step, 2000);
    EXPECT_TRUE(d.blocks[0].scalars.empty());
    EXPECT_TRUE(d.scalarNames.isEmpty());

    EXPECT_EQ(names(d.blocks[0]), QStringList({"Row", "c_rdf[1]", "c_rdf[2]"}));
    ASSERT_EQ(d.blocks[0].rows.rowCount(), 3);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(0)[2], 3.0);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(2)[2], 1.2);
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(2)[0], 0.3);
}

TEST(AveBlocks, Histo)
{
    const PlotBlockData d = parseAveBlocks(ave_histo);
    EXPECT_EQ(d.kind, AveFileKind::AveHisto);
    EXPECT_EQ(d.fixId, "myhisto");
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Bin", "Coord", "Count", "Count/Total"}));
    EXPECT_EQ(d.scalarNames,
              QStringList({"Total-counts", "Missing-counts", "Min-value", "Max-value"}));

    ASSERT_EQ(d.blocks[1].scalars.size(), 4u);
    EXPECT_DOUBLE_EQ(d.blocks[1].scalars[0], 200.0);
    EXPECT_DOUBLE_EQ(d.blocks[1].scalars[1], 5.0);
    EXPECT_DOUBLE_EQ(d.blocks[1].scalars[2], -1.4);
    EXPECT_DOUBLE_EQ(d.blocks[1].scalars[3], 2.6);

    ASSERT_EQ(d.blocks[0].rows.rowCount(), 4);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(3)[2], 0.4);
}

TEST(AveBlocks, Correlate)
{
    const PlotBlockData d = parseAveBlocks(ave_correlate);
    EXPECT_EQ(d.kind, AveFileKind::AveCorrelate);
    EXPECT_EQ(d.fixId, "mycorr");
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Index", "TimeDelta", "Ncount", "v_vx*v_vx"}));
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(1)[2], 10.0);
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(3)[1], 0.6);
}

TEST(AveBlocks, CorrelateLong)
{
    const PlotBlockData d = parseAveBlocks(ave_correlate_long);
    EXPECT_EQ(d.kind, AveFileKind::AveCorrelateLong);
    EXPECT_EQ(d.fixId, "myclong");
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[0].step, 1000);
    EXPECT_EQ(d.blocks[1].step, 2000);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Time", "v_vx*v_vx"}));
    ASSERT_EQ(d.blocks[0].rows.rowCount(), 3);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(0)[1], 0.005);
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(1)[2], 0.3);
}

// A single value in vector mode makes the block header exactly as wide as a
// data row; the row count on the header is what still separates them.
TEST(AveBlocks, VectorWithOneValue)
{
    const char text[]     = "# Time-averaged data for fix v1\n"
                            "# TimeStep Number-of-rows\n"
                            "# Row c_x\n"
                            "100 2\n"
                            "1 1.5\n"
                            "2 2.5\n"
                            "200 2\n"
                            "1 3.5\n"
                            "2 4.5\n";
    const PlotBlockData d = parseAveBlocks(text);
    EXPECT_EQ(d.kind, AveFileKind::AveTimeVector);
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Row", "c_x"}));
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(1)[1], 4.5);
}

// "overwrite" truncates the file to the latest block
TEST(AveBlocks, OverwriteSingleBlock)
{
    const char text[] =
        "# Histogrammed data for fix h\n"
        "# TimeStep Number-of-bins Total-counts Missing-counts Min-value Max-value\n"
        "# Bin Coord Count Count/Total\n"
        "5000 2 10 0 0 1\n"
        "1 0.25 4 0.4\n"
        "2 0.75 6 0.6\n";
    const PlotBlockData d = parseAveBlocks(text);
    EXPECT_EQ(d.kind, AveFileKind::AveHisto);
    ASSERT_EQ(d.blockCount(), 1);
    EXPECT_EQ(d.blocks[0].step, 5000);
    EXPECT_EQ(d.blocks[0].rows.rowCount(), 2);
}

// a second fix instance re-opens the file and writes its headers again
TEST(AveBlocks, AppendedHeaderMidFile)
{
    const char text[]     = "# Time-averaged data for fix a\n"
                            "# TimeStep Number-of-rows\n"
                            "# Row c_x\n"
                            "100 2\n"
                            "1 1\n"
                            "2 2\n"
                            "# Time-averaged data for fix a\n"
                            "# TimeStep Number-of-rows\n"
                            "# Row c_x\n"
                            "200 2\n"
                            "1 3\n"
                            "2 4\n";
    const PlotBlockData d = parseAveBlocks(text);
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[1].step, 200);
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(1)[1], 4.0);
}

// a run killed mid-write leaves the last block short of its announced rows
TEST(AveBlocks, TruncatedLastBlock)
{
    const char text[]     = "# Time-averaged data for fix a\n"
                            "# TimeStep Number-of-rows\n"
                            "# Row c_x\n"
                            "100 3\n"
                            "1 1\n"
                            "2 2\n"
                            "3 3\n"
                            "200 3\n"
                            "1 4\n"
                            "2 5\n";
    const PlotBlockData d = parseAveBlocks(text);
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[0].rows.rowCount(), 3);
    EXPECT_EQ(d.blocks[1].rows.rowCount(), 2);
}

TEST(AveBlocks, ChangingRowCount)
{
    const char text[]     = "# Time-averaged data for fix a\n"
                            "# TimeStep Number-of-rows\n"
                            "# Row c_x\n"
                            "100 2\n"
                            "1 1\n"
                            "2 2\n"
                            "200 3\n"
                            "1 3\n"
                            "2 4\n"
                            "3 5\n";
    const PlotBlockData d = parseAveBlocks(text);
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[0].rows.rowCount(), 2);
    EXPECT_EQ(d.blocks[1].rows.rowCount(), 3);
}

// --- structural fallback when title1/2/3 replaced the headers ---------

TEST(AveBlocksFallback, HistoWithoutDefaultTitles)
{
    const char text[]     = "# my own histogram\n"
                            "# step bins total missing lo hi\n"
                            "# index position hits fraction\n"
                            "1000 2 10 0 0 1\n"
                            "1 0.25 4 0.4\n"
                            "2 0.75 6 0.6\n";
    const PlotBlockData d = parseAveBlocks(text);
    EXPECT_EQ(d.kind, AveFileKind::AveHisto);
    EXPECT_TRUE(d.fixId.isEmpty());
    // the column names still come from the comment as wide as a data row
    EXPECT_EQ(names(d.blocks[0]), QStringList({"index", "position", "hits", "fraction"}));
    EXPECT_EQ(d.scalarNames, QStringList({"total", "missing", "lo", "hi"}));
}

TEST(AveBlocksFallback, CorrelateWithoutDefaultTitles)
{
    const char text[]     = "# custom title\n"
                            "1000 3\n"
                            "1 0 100 1\n"
                            "2 5 99 0.5\n"
                            "3 10 98 0.25\n";
    const PlotBlockData d = parseAveBlocks(text);
    // recognized by the 0, nevery, 2*nevery progression in column 2
    EXPECT_EQ(d.kind, AveFileKind::AveCorrelate);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"column1", "column2", "column3", "column4"}));
}

TEST(AveBlocksFallback, VectorWithoutDefaultTitles)
{
    const char text[]     = "1000 3\n"
                            "1 10 100 1\n"
                            "2 20 99 0.5\n"
                            "3 30 98 0.25\n";
    const PlotBlockData d = parseAveBlocks(text);
    // same header width as correlate, but column 2 does not start at zero
    EXPECT_EQ(d.kind, AveFileKind::AveTimeVector);
}

// fix ave/chunk parses through the generic path without losing data
TEST(AveBlocksFallback, ChunkIsGenericBlockData)
{
    const char text[]     = "# Chunk-averaged data for fix mychunk and group all\n"
                            "# Timestep Number-of-chunks Total-count\n"
                            "# Chunk Coord1 Ncount density/mass\n"
                            "1000 2 500\n"
                            "1 0.5 250 0.8\n"
                            "2 1.5 250 0.9\n";
    const PlotBlockData d = parseAveBlocks(text);
    EXPECT_EQ(d.kind, AveFileKind::Unknown);
    EXPECT_EQ(d.fixId, "mychunk");
    ASSERT_EQ(d.blockCount(), 1);
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Chunk", "Coord1", "Ncount", "density/mass"}));
    EXPECT_EQ(d.scalarNames, QStringList({"Total-count"}));
    EXPECT_DOUBLE_EQ(d.blocks[0].scalars[0], 500.0);
}

// --- detection --------------------------------------------------------

TEST(AveBlocksDetect, RecognizesBlockFiles)
{
    EXPECT_TRUE(looksLikeAveBlocks(ave_time_vector));
    EXPECT_TRUE(looksLikeAveBlocks(ave_histo));
    EXPECT_TRUE(looksLikeAveBlocks(ave_correlate));
    EXPECT_TRUE(looksLikeAveBlocks(ave_correlate_long));
}

TEST(AveBlocksDetect, RejectsFlatTables)
{
    // a fix ave/time scalar file: its header comment says "Time-averaged data
    // for fix", but there is no block structure behind it
    EXPECT_FALSE(looksLikeAveBlocks(ave_time_scalar));
    EXPECT_FALSE(looksLikeAveBlocks("# Step Temp Press\n0 300 1.0\n100 310 1.1\n200 305 1.2\n"));
    EXPECT_FALSE(looksLikeAveBlocks(""));
    EXPECT_FALSE(looksLikeAveBlocks("just some text\nand more of it\n"));
}

TEST(AveBlocksDetect, KindNames)
{
    EXPECT_EQ(aveFileKindName(AveFileKind::AveHisto), "fix ave/histo");
    EXPECT_EQ(aveFileKindName(AveFileKind::AveCorrelateLong), "fix ave/correlate/long");
    EXPECT_FALSE(aveFileKindName(AveFileKind::Unknown).isEmpty());
}

// --- YAML -------------------------------------------------------------

TEST(AveBlocksYaml, VectorMode)
{
    const PlotBlockData d = parseAveBlocksYaml(ave_time_yaml);
    EXPECT_EQ(d.kind, AveFileKind::AveTimeVector);
    ASSERT_EQ(d.blockCount(), 2);
    EXPECT_EQ(d.blocks[0].step, 1000);
    EXPECT_EQ(d.blocks[1].step, 2000);
    // a Row column is synthesized so the table matches the native format
    EXPECT_EQ(names(d.blocks[0]), QStringList({"Row", "c_rdf[1]", "c_rdf[2]"}));
    ASSERT_EQ(d.blocks[0].rows.rowCount(), 2);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(0)[1], 2.0);
    EXPECT_DOUBLE_EQ(d.blocks[0].rows.column(2)[1], 0.5);
    EXPECT_DOUBLE_EQ(d.blocks[1].rows.column(2)[1], 0.7);
}

TEST(AveBlocksYaml, ScalarModeIsNotBlockData)
{
    QString err;
    const PlotBlockData d = parseAveBlocksYaml(ave_time_yaml_scalar, &err);
    EXPECT_TRUE(d.isEmpty());
    EXPECT_FALSE(err.isEmpty());
}

} // namespace
