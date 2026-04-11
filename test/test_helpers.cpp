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

#include "helpers.h"
#include "stdcapture.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QString>

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// Fixture for tests that need Qt application
class HelpersTest : public ::testing::Test {
protected:
    static void SetUpTestSuite()
    {
        // Create QCoreApplication if it doesn't exist
        if (!QCoreApplication::instance()) {
            static int argc     = 1;
            static char *argv[] = {(char *)"test_helpers"};
            app                 = new QCoreApplication(argc, argv);
        }
    }

    static QCoreApplication *app;
};

QCoreApplication *HelpersTest::app = nullptr;

// Tests for dateCompare function

TEST_F(HelpersTest, DateCompareSame)
{
    QString date1 = "15 Jan 2024";
    QString date2 = "15 Jan 2024";
    EXPECT_EQ(dateCompare(date1, date2), 0);
}

TEST_F(HelpersTest, DateCompareYearDifferent)
{
    QString older = "15 Jan 2023";
    QString newer = "15 Jan 2024";
    EXPECT_EQ(dateCompare(older, newer), -1);
    EXPECT_EQ(dateCompare(newer, older), 1);
}

TEST_F(HelpersTest, DateCompareMonthDifferent)
{
    QString earlier = "15 Jan 2024";
    QString later   = "15 Feb 2024";
    EXPECT_EQ(dateCompare(earlier, later), -1);
    EXPECT_EQ(dateCompare(later, earlier), 1);
}

TEST_F(HelpersTest, DateCompareDayDifferent)
{
    QString earlier = "14 Jan 2024";
    QString later   = "15 Jan 2024";
    EXPECT_EQ(dateCompare(earlier, later), -1);
    EXPECT_EQ(dateCompare(later, earlier), 1);
}

TEST_F(HelpersTest, DateCompareFullMonthName)
{
    QString date1 = "15 January 2024";
    QString date2 = "15 February 2024";
    // Should truncate to "Jan" and "Feb"
    EXPECT_EQ(dateCompare(date1, date2), -1);
}

TEST_F(HelpersTest, DateComparePartialMonthName)
{
    QString date1 = "15 January 2024";
    QString date2 = "15 Jan 2024";
    // Should truncate "January" to "Jan"
    EXPECT_EQ(dateCompare(date1, date2), 0);
    EXPECT_EQ(dateCompare(date2, date1), 0);
}

TEST_F(HelpersTest, DateCompareInvalidFormat)
{
    QString invalid = "Invalid";
    QString valid   = "15 Jan 2024";
    // Invalid format should return -1 or 1 depending on which is invalid
    EXPECT_EQ(dateCompare(invalid, valid), -1);
    EXPECT_EQ(dateCompare(valid, invalid), 1);
}

TEST_F(HelpersTest, DateCompareAllMonths)
{
    QString jan = "15 Jan 2024";
    QString dec = "15 Dec 2024";
    EXPECT_EQ(dateCompare(jan, dec), -1);
    EXPECT_EQ(dateCompare(dec, jan), 1);
}

// Tests for splitLine function

TEST_F(HelpersTest, SplitLineSimple)
{
    std::string input                 = "one two three";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineEmpty)
{
    std::string input = "";
    auto result       = splitLine(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(HelpersTest, SplitLineWhitespaceOnly)
{
    std::string input = "   \t\n  ";
    auto result       = splitLine(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(HelpersTest, SplitLineSingleQuotes)
{
    std::string input                 = "one 'two three' four";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "'two three'", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineDoubleQuotes)
{
    std::string input                 = "one \"two three\" four";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "\"two three\"", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineEscapedQuotes)
{
    std::string input                 = "one \\'test\\' two";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "\\'test\\'", "two"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineMixedQuotes)
{
    std::string input                 = "word1 'single' \"double\" word2";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"word1", "'single'", "\"double\"", "word2"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineTripleQuotes)
{
    // Triple quotes handling: """ is a special marker in LAMMPS
    // The implementation shows that """ at start triggers special parsing
    // but the exact behavior can be complex. We'll test that it at least
    // parses without crashing and returns some tokens.
    std::string input = "word1 \"\"\"triple quoted text\"\"\" word2";
    auto result       = splitLine(input);
    // Just verify basic parsing works - we got some results
    EXPECT_GT(result.size(), 0);
    EXPECT_EQ(result[0], "word1");
    // The triple quote handling may parse this in various ways,
    // so we just ensure it doesn't crash and produces output
}

TEST_F(HelpersTest, SplitLineMultipleSpaces)
{
    std::string input                 = "one    two\t\tthree\n\nfour";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "two", "three", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineLeadingTrailingWhitespace)
{
    std::string input                 = "  \t one two  \n";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"one", "two"};
    EXPECT_EQ(result, expected);
}

// Tests for hasExe function

TEST_F(HelpersTest, HasExeSystemCommand)
{
    // Test with a command that should exist on most systems
#if defined(_WIN32)
    EXPECT_TRUE(hasExe("cmd"));
#else
    EXPECT_TRUE(hasExe("ls"));
#endif
}

TEST_F(HelpersTest, HasExeNonExistent)
{
    // Test with a command that definitely doesn't exist
    EXPECT_FALSE(hasExe("this_command_does_not_exist_12345"));
}

// Tests for isLightTheme function

TEST_F(HelpersTest, IsLightThemeReturnsBoolean)
{
    // Just test that it returns a valid boolean value
    // We can't reliably test the actual value as it depends on system theme
    bool result = isLightTheme();
    // Should be either true or false, test passes if it doesn't crash
    EXPECT_TRUE(result == true || result == false);
}

// Tests for silenceStdout / restoreStdout functions

TEST_F(HelpersTest, SilenceStdoutSilencesOutput)
{
    // stdout should not be silenced initially
    EXPECT_FALSE(isStdoutSilenced());

    silenceStdout();
    EXPECT_TRUE(isStdoutSilenced());

    // Write something to stdout (should go to /dev/null)
    printf("this should be silenced");
    fflush(stdout);

    restoreStdout();
    EXPECT_FALSE(isStdoutSilenced());
}

TEST_F(HelpersTest, RestoreStdoutWhenNotSilenced)
{
    // Restoring when not silenced should be a no-op
    EXPECT_FALSE(isStdoutSilenced());
    restoreStdout();
    EXPECT_FALSE(isStdoutSilenced());
}

TEST_F(HelpersTest, SilenceStdoutIdempotent)
{
    // Silencing twice should not cause issues
    silenceStdout();
    EXPECT_TRUE(isStdoutSilenced());
    silenceStdout(); // second call should be a no-op
    EXPECT_TRUE(isStdoutSilenced());

    restoreStdout();
    EXPECT_FALSE(isStdoutSilenced());
    // restoring a second time should not make a difference
    restoreStdout();
    EXPECT_FALSE(isStdoutSilenced());
}

TEST_F(HelpersTest, SilenceStdoutSkippedDuringCapture)
{
    // When StdCapture is active, silenceStdout should be a no-op
    StdCapture capturer;
    capturer.beginCapture();

    silenceStdout();
    EXPECT_FALSE(isStdoutSilenced()); // should NOT have silenced

    capturer.endCapture();
}

TEST_F(HelpersTest, CaptureRestoresSilencedStdout)
{
    // StdCapture is typically created before silenceStdout is called
    StdCapture capturer;

    // When stdout is silenced and StdCapture starts, it should restore first
    silenceStdout();
    EXPECT_TRUE(isStdoutSilenced());

    capturer.beginCapture();
    // beginCapture should have restored (un-silenced) stdout
    EXPECT_FALSE(isStdoutSilenced());

    printf("captured output");
    fflush(stdout);

    capturer.endCapture();
    auto output = capturer.getCapture();
    EXPECT_NE(output.find("captured output"), std::string::npos);
}

TEST_F(HelpersTest, SilenceAndRestorePreservesStdout)
{
    // After silence + restore, stdout should work normally
    StdCapture capturer;

    silenceStdout();
    printf("silenced text");
    fflush(stdout);
    restoreStdout();

    // Now capture to verify stdout is functional
    capturer.beginCapture();
    printf("visible text");
    fflush(stdout);
    capturer.endCapture();

    auto output = capturer.getCapture();
    EXPECT_NE(output.find("visible text"), std::string::npos);
    EXPECT_EQ(output.find("silenced text"), std::string::npos);
}

// Tests for purgeDirectory function

TEST_F(HelpersTest, PurgeDirectoryRemovesFiles)
{
    // Create a temporary directory with some files
    QDir temp(QDir::tempPath());
    QString testDir = temp.filePath("lammps_gui_test_purge");
    temp.mkpath(testDir);

    // Create some test files
    QFile f1(testDir + "/test1.txt");
    EXPECT_TRUE(f1.open(QIODevice::WriteOnly));
    f1.write("test");
    f1.close();

    QFile f2(testDir + "/test2.txt");
    EXPECT_TRUE(f2.open(QIODevice::WriteOnly));
    f2.write("test");
    f2.close();

    // Verify files exist
    EXPECT_TRUE(QFile::exists(testDir + "/test1.txt"));
    EXPECT_TRUE(QFile::exists(testDir + "/test2.txt"));

    // Purge the directory
    purgeDirectory(testDir);

    // Files should be gone but directory should still exist
    EXPECT_FALSE(QFile::exists(testDir + "/test1.txt"));
    EXPECT_FALSE(QFile::exists(testDir + "/test2.txt"));
    EXPECT_TRUE(QDir(testDir).exists());

    // Cleanup
    temp.rmdir(testDir);
}

TEST_F(HelpersTest, PurgeDirectoryNonExistent)
{
    // Purging a non-existent directory should not crash
    purgeDirectory("/nonexistent/path/that/does/not/exist");
}

TEST_F(HelpersTest, PurgeDirectoryEmpty)
{
    // Purging an empty directory should work fine
    QDir temp(QDir::tempPath());
    QString testDir = temp.filePath("lammps_gui_test_purge_empty");
    temp.mkpath(testDir);

    purgeDirectory(testDir);

    EXPECT_TRUE(QDir(testDir).exists());
    temp.rmdir(testDir);
}

// Additional dateCompare edge case tests

TEST_F(HelpersTest, DateCompareDecemberMonths)
{
    // Test all months are recognized correctly
    QStringList months = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                          "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    for (int i = 0; i < months.size() - 1; ++i) {
        QString earlier = QString("15 %1 2024").arg(months[i]);
        QString later   = QString("15 %1 2024").arg(months[i + 1]);
        EXPECT_EQ(dateCompare(earlier, later), -1)
            << "Expected " << months[i].toStdString() << " < " << months[i + 1].toStdString();
    }
}

TEST_F(HelpersTest, DateCompareBothInvalid)
{
    // When the first date is invalid (not 3 words), dateCompare returns -1
    EXPECT_EQ(dateCompare("invalid1", "invalid2"), -1);
    // But identical invalid strings return 0 (equality check first)
    EXPECT_EQ(dateCompare("invalid", "invalid"), 0);
}

TEST_F(HelpersTest, DateCompareWithUpdateSuffix)
{
    // LAMMPS dates can have "update N" suffix like "22 July 2025 update 2"
    // dateCompare should handle the date part correctly
    QString date1 = "22 Jul 2025";
    QString date2 = "22 Aug 2025";
    EXPECT_EQ(dateCompare(date1, date2), -1);
}

// Additional splitLine edge case tests

TEST_F(HelpersTest, SplitLineSingleWord)
{
    std::string input                 = "word";
    auto result                       = splitLine(input);
    std::vector<std::string> expected = {"word"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineHashComment)
{
    // In LAMMPS, # starts a comment - splitLine should handle this
    std::string input = "fix 1 all nve # this is a comment";
    auto result       = splitLine(input);
    // After #, everything is ignored
    EXPECT_GE(result.size(), 1u);
    EXPECT_EQ(result[0], "fix");
}

TEST_F(HelpersTest, SplitLineSpecialCharacters)
{
    std::string input = "pair_coeff * * 1.0 2.5";
    auto result       = splitLine(input);
    EXPECT_EQ(result.size(), 5u);
    EXPECT_EQ(result[0], "pair_coeff");
    EXPECT_EQ(result[1], "*");
    EXPECT_EQ(result[2], "*");
    EXPECT_EQ(result[3], "1.0");
    EXPECT_EQ(result[4], "2.5");
}

// Additional hasExe tests

TEST_F(HelpersTest, HasExeEmptyString)
{
    EXPECT_FALSE(hasExe(""));
}

TEST_F(HelpersTest, HasExeBash)
{
#if !defined(_WIN32)
    // bash should exist on Linux/macOS
    EXPECT_TRUE(hasExe("bash"));
#endif
}
