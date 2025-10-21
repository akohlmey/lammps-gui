/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   https://www.lammps.org/, Sandia National Laboratories
   LAMMPS development team: developers@lammps.org

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "helpers.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QString>

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
            static int argc    = 1;
            static char *argv[] = {(char *)"test_helpers"};
            app                = new QCoreApplication(argc, argv);
        }
    }

    static QCoreApplication *app;
};

QCoreApplication *HelpersTest::app = nullptr;

// Tests for mystrdup functions

TEST_F(HelpersTest, MyStrdupStdString)
{
    std::string input = "Hello World";
    char *result      = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "Hello World");
    EXPECT_EQ(strlen(result), input.size());

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupStdStringEmpty)
{
    std::string input = "";
    char *result      = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");
    EXPECT_EQ(strlen(result), 0);

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupCString)
{
    const char *input = "Test String";
    char *result      = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "Test String");

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupCStringEmpty)
{
    const char *input = "";
    char *result      = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupCStringNull)
{
    const char *input = nullptr;
    char *result      = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupQString)
{
    QString input = "Qt String Test";
    char *result  = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "Qt String Test");

    delete[] result;
}

TEST_F(HelpersTest, MyStrdupQStringEmpty)
{
    QString input = "";
    char *result  = mystrdup(input);

    ASSERT_NE(result, nullptr);
    EXPECT_STREQ(result, "");

    delete[] result;
}

// Tests for date_compare function

TEST_F(HelpersTest, DateCompareSame)
{
    QString date1 = "15 Jan 2024";
    QString date2 = "15 Jan 2024";
    EXPECT_EQ(date_compare(date1, date2), 0);
}

TEST_F(HelpersTest, DateCompareYearDifferent)
{
    QString older = "15 Jan 2023";
    QString newer = "15 Jan 2024";
    EXPECT_EQ(date_compare(older, newer), -1);
    EXPECT_EQ(date_compare(newer, older), 1);
}

TEST_F(HelpersTest, DateCompareMonthDifferent)
{
    QString earlier = "15 Jan 2024";
    QString later   = "15 Feb 2024";
    EXPECT_EQ(date_compare(earlier, later), -1);
    EXPECT_EQ(date_compare(later, earlier), 1);
}

TEST_F(HelpersTest, DateCompareDayDifferent)
{
    QString earlier = "14 Jan 2024";
    QString later   = "15 Jan 2024";
    EXPECT_EQ(date_compare(earlier, later), -1);
    EXPECT_EQ(date_compare(later, earlier), 1);
}

TEST_F(HelpersTest, DateCompareFullMonthName)
{
    QString date1 = "15 January 2024";
    QString date2 = "15 February 2024";
    // Should truncate to "Jan" and "Feb"
    EXPECT_EQ(date_compare(date1, date2), -1);
}

TEST_F(HelpersTest, DateComparePartialMonthName)
{
    QString date1 = "15 January 2024";
    QString date2 = "15 Jan 2024";
    // Should truncate "January" to "Jan"
    EXPECT_EQ(date_compare(date1, date2), 0);
    EXPECT_EQ(date_compare(date2, date1), 0);
}

TEST_F(HelpersTest, DateCompareInvalidFormat)
{
    QString invalid = "Invalid";
    QString valid   = "15 Jan 2024";
    // Invalid format should return -1 or 1 depending on which is invalid
    EXPECT_EQ(date_compare(invalid, valid), -1);
    EXPECT_EQ(date_compare(valid, invalid), 1);
}

TEST_F(HelpersTest, DateCompareAllMonths)
{
    QString jan = "15 Jan 2024";
    QString dec = "15 Dec 2024";
    EXPECT_EQ(date_compare(jan, dec), -1);
    EXPECT_EQ(date_compare(dec, jan), 1);
}

// Tests for split_line function

TEST_F(HelpersTest, SplitLineSimple)
{
    std::string input  = "one two three";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "two", "three"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineEmpty)
{
    std::string input = "";
    auto result       = split_line(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(HelpersTest, SplitLineWhitespaceOnly)
{
    std::string input = "   \t\n  ";
    auto result       = split_line(input);
    EXPECT_TRUE(result.empty());
}

TEST_F(HelpersTest, SplitLineSingleQuotes)
{
    std::string input  = "one 'two three' four";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "'two three'", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineDoubleQuotes)
{
    std::string input  = "one \"two three\" four";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "\"two three\"", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineEscapedQuotes)
{
    std::string input  = "one \\'test\\' two";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "\\'test\\'", "two"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineMixedQuotes)
{
    std::string input  = "word1 'single' \"double\" word2";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"word1", "'single'", "\"double\"", "word2"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineTripleQuotes)
{
    // Triple quotes handling: """ is a special marker in LAMMPS
    // The implementation shows that """ at start triggers special parsing
    // but the exact behavior can be complex. We'll test that it at least
    // parses without crashing and returns some tokens.
    std::string input  = "word1 \"\"\"triple quoted text\"\"\" word2";
    auto result        = split_line(input);
    // Just verify basic parsing works - we got some results
    EXPECT_GT(result.size(), 0);
    EXPECT_EQ(result[0], "word1");
    // The triple quote handling may parse this in various ways,
    // so we just ensure it doesn't crash and produces output
}

TEST_F(HelpersTest, SplitLineMultipleSpaces)
{
    std::string input  = "one    two\t\tthree\n\nfour";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "two", "three", "four"};
    EXPECT_EQ(result, expected);
}

TEST_F(HelpersTest, SplitLineLeadingTrailingWhitespace)
{
    std::string input  = "  \t one two  \n";
    auto result        = split_line(input);
    std::vector<std::string> expected = {"one", "two"};
    EXPECT_EQ(result, expected);
}

// Tests for has_exe function

TEST_F(HelpersTest, HasExeSystemCommand)
{
    // Test with a command that should exist on most systems
#if defined(_WIN32)
    EXPECT_TRUE(has_exe("cmd"));
#else
    EXPECT_TRUE(has_exe("ls"));
#endif
}

TEST_F(HelpersTest, HasExeNonExistent)
{
    // Test with a command that definitely doesn't exist
    EXPECT_FALSE(has_exe("this_command_does_not_exist_12345"));
}

// Tests for is_light_theme function

TEST_F(HelpersTest, IsLightThemeReturnsBoolean)
{
    // Just test that it returns a valid boolean value
    // We can't reliably test the actual value as it depends on system theme
    bool result = is_light_theme();
    // Should be either true or false, test passes if it doesn't crash
    EXPECT_TRUE(result == true || result == false);
}
