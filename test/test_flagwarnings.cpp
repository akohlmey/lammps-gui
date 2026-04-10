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

#include "flagwarnings.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QLabel>
#include <QTextDocument>

// Fixture for tests that need QApplication (for widgets)
class FlagWarningsTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // QApplication needed for widget-based tests
        if (!QApplication::instance()) {
            static int argc     = 1;
            static char *argv[] = {(char *)"test_flagwarnings"};
            app                 = new QApplication(argc, argv);
        }
    }

    static QApplication *app;
};

QApplication *FlagWarningsTest::app = nullptr;

TEST_F(FlagWarningsTest, ConstructorInitializesZeroWarnings)
{
    QTextDocument doc;
    FlagWarnings fw(nullptr, &doc);
    EXPECT_EQ(fw.getNWarnings(), 0);
}

TEST_F(FlagWarningsTest, DetectsWarningLines)
{
    QTextDocument doc;
    QLabel label;
    FlagWarnings fw(&label, &doc);

    // Setting text triggers rehighlight which calls highlightBlock
    doc.setPlainText("WARNING: some warning message\nNormal line\nERROR: some error");

    // The highlighter should have found 2 warnings (WARNING + ERROR)
    EXPECT_EQ(fw.getNWarnings(), 2);
}

TEST_F(FlagWarningsTest, IgnoresNormalLines)
{
    QTextDocument doc;
    FlagWarnings fw(nullptr, &doc);

    doc.setPlainText("This is a normal line\nAnother normal line\nNo warnings here");

    EXPECT_EQ(fw.getNWarnings(), 0);
}

TEST_F(FlagWarningsTest, UpdatesSummaryLabel)
{
    QTextDocument doc;
    QLabel label;
    FlagWarnings fw(&label, &doc);

    doc.setPlainText("WARNING: test warning\nLine 2");

    // Label should contain the warning count and line count
    QString text = label.text();
    EXPECT_TRUE(text.contains("1 Warnings")) << text.toStdString();
    EXPECT_TRUE(text.contains("Lines")) << text.toStdString();
}

TEST_F(FlagWarningsTest, EmptyDocument)
{
    QTextDocument doc;
    FlagWarnings fw(nullptr, &doc);

    doc.setPlainText("");
    EXPECT_EQ(fw.getNWarnings(), 0);
}

TEST_F(FlagWarningsTest, WarningAtStartOfLine)
{
    QTextDocument doc;
    FlagWarnings fw(nullptr, &doc);

    // WARNING must be at start of line for the regex ^(ERROR|WARNING).*$
    doc.setPlainText("  WARNING: indented warning");
    EXPECT_EQ(fw.getNWarnings(), 0); // Should NOT match (not at start)

    QTextDocument doc2;
    FlagWarnings fw2(nullptr, &doc2);
    doc2.setPlainText("WARNING: proper warning");
    EXPECT_EQ(fw2.getNWarnings(), 1); // Should match
}

TEST_F(FlagWarningsTest, MultipleWarnings)
{
    QTextDocument doc;
    FlagWarnings fw(nullptr, &doc);

    doc.setPlainText(
        "WARNING: first warning\n"
        "WARNING: second warning\n"
        "ERROR: an error\n"
        "Normal line\n"
        "ERROR: another error");

    EXPECT_EQ(fw.getNWarnings(), 4);
}
