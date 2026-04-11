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

#include "stdcapture.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

// Test fixture for StdCapture tests
class StdCaptureTest : public ::testing::Test {
protected:
    StdCapture capturer;
};

// Basic capture via endCapture/getCapture

TEST_F(StdCaptureTest, CaptureSimpleOutput)
{
    capturer.beginCapture();
    printf("Hello World");
    capturer.endCapture();

    std::string result = capturer.getCapture();
    EXPECT_EQ(result, "Hello World");
}

TEST_F(StdCaptureTest, CaptureMultipleLines)
{
    capturer.beginCapture();
    printf("Line 1\n");
    printf("Line 2\n");
    printf("Line 3");
    capturer.endCapture();

    std::string result = capturer.getCapture();
    EXPECT_NE(result.find("Line 1"), std::string::npos);
    EXPECT_NE(result.find("Line 2"), std::string::npos);
    EXPECT_NE(result.find("Line 3"), std::string::npos);
}

TEST_F(StdCaptureTest, CaptureEmptyOutput)
{
    capturer.beginCapture();
    // write nothing
    capturer.endCapture();

    std::string result = capturer.getCapture();
    EXPECT_TRUE(result.empty());
}

TEST_F(StdCaptureTest, MultipleCapturesSameInstance)
{
    // First capture
    capturer.beginCapture();
    printf("First");
    capturer.endCapture();
    std::string first = capturer.getCapture();
    EXPECT_EQ(first, "First");

    // Second capture reuses the same pipe
    capturer.beginCapture();
    printf("Second");
    capturer.endCapture();
    std::string second = capturer.getCapture();
    EXPECT_EQ(second, "Second");
}

// getChunk tests

TEST_F(StdCaptureTest, GetChunkDuringCapture)
{
    capturer.beginCapture();
    printf("chunk data");

    std::string chunk = capturer.getChunk();
    EXPECT_EQ(chunk, "chunk data");

    capturer.endCapture();
}

TEST_F(StdCaptureTest, GetChunkWhenNotCapturing)
{
    // getChunk without active capture should return empty
    std::string chunk = capturer.getChunk();
    EXPECT_TRUE(chunk.empty());
}

TEST_F(StdCaptureTest, GetChunkMultipleTimes)
{
    capturer.beginCapture();

    printf("part1");
    std::string chunk1 = capturer.getChunk();
    EXPECT_EQ(chunk1, "part1");

    printf("part2");
    std::string chunk2 = capturer.getChunk();
    EXPECT_EQ(chunk2, "part2");

    capturer.endCapture();
}

// endCapture without beginCapture

TEST_F(StdCaptureTest, EndCaptureWithoutBegin)
{
    EXPECT_FALSE(capturer.endCapture());
}

// Buffer use tracking

TEST_F(StdCaptureTest, BufferUseInitiallyZero)
{
    EXPECT_DOUBLE_EQ(capturer.getBufferUse(), 0.0);
}

TEST_F(StdCaptureTest, BufferUseAfterChunk)
{
    capturer.beginCapture();
    printf("some data");
    capturer.getChunk();

    double usage = capturer.getBufferUse();
    EXPECT_GT(usage, 0.0);
    EXPECT_LT(usage, 1.0);

    capturer.endCapture();
}

// Verify stdout is restored after capture

TEST_F(StdCaptureTest, StdoutRestoredAfterCapture)
{
    // Capture something
    capturer.beginCapture();
    printf("captured");
    capturer.endCapture();

    // After endCapture, stdout should work normally again.
    // We verify indirectly: a second capture cycle should still work.
    capturer.beginCapture();
    printf("also captured");
    capturer.endCapture();
    std::string result = capturer.getCapture();
    EXPECT_EQ(result, "also captured");
}

// Local Variables:
// c-basic-offset: 4
// End:
