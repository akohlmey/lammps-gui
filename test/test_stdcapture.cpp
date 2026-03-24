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

// Basic capture via EndCapture/GetCapture

TEST_F(StdCaptureTest, CaptureSimpleOutput)
{
    capturer.BeginCapture();
    printf("Hello World");
    capturer.EndCapture();

    std::string result = capturer.GetCapture();
    EXPECT_EQ(result, "Hello World");
}

TEST_F(StdCaptureTest, CaptureMultipleLines)
{
    capturer.BeginCapture();
    printf("Line 1\n");
    printf("Line 2\n");
    printf("Line 3");
    capturer.EndCapture();

    std::string result = capturer.GetCapture();
    EXPECT_NE(result.find("Line 1"), std::string::npos);
    EXPECT_NE(result.find("Line 2"), std::string::npos);
    EXPECT_NE(result.find("Line 3"), std::string::npos);
}

TEST_F(StdCaptureTest, CaptureEmptyOutput)
{
    capturer.BeginCapture();
    // write nothing
    capturer.EndCapture();

    std::string result = capturer.GetCapture();
    EXPECT_TRUE(result.empty());
}

TEST_F(StdCaptureTest, MultipleCapturesSameInstance)
{
    // First capture
    capturer.BeginCapture();
    printf("First");
    capturer.EndCapture();
    std::string first = capturer.GetCapture();
    EXPECT_EQ(first, "First");

    // Second capture reuses the same pipe
    capturer.BeginCapture();
    printf("Second");
    capturer.EndCapture();
    std::string second = capturer.GetCapture();
    EXPECT_EQ(second, "Second");
}

// GetChunk tests

TEST_F(StdCaptureTest, GetChunkDuringCapture)
{
    capturer.BeginCapture();
    printf("chunk data");

    std::string chunk = capturer.GetChunk();
    EXPECT_EQ(chunk, "chunk data");

    capturer.EndCapture();
}

TEST_F(StdCaptureTest, GetChunkWhenNotCapturing)
{
    // GetChunk without active capture should return empty
    std::string chunk = capturer.GetChunk();
    EXPECT_TRUE(chunk.empty());
}

TEST_F(StdCaptureTest, GetChunkMultipleTimes)
{
    capturer.BeginCapture();

    printf("part1");
    std::string chunk1 = capturer.GetChunk();
    EXPECT_EQ(chunk1, "part1");

    printf("part2");
    std::string chunk2 = capturer.GetChunk();
    EXPECT_EQ(chunk2, "part2");

    capturer.EndCapture();
}

// EndCapture without BeginCapture

TEST_F(StdCaptureTest, EndCaptureWithoutBegin)
{
    EXPECT_FALSE(capturer.EndCapture());
}

// Buffer use tracking

TEST_F(StdCaptureTest, BufferUseInitiallyZero)
{
    EXPECT_DOUBLE_EQ(capturer.get_bufferuse(), 0.0);
}

TEST_F(StdCaptureTest, BufferUseAfterChunk)
{
    capturer.BeginCapture();
    printf("some data");
    capturer.GetChunk();

    double usage = capturer.get_bufferuse();
    EXPECT_GT(usage, 0.0);
    EXPECT_LT(usage, 1.0);

    capturer.EndCapture();
}

// Verify stdout is restored after capture

TEST_F(StdCaptureTest, StdoutRestoredAfterCapture)
{
    // Capture something
    capturer.BeginCapture();
    printf("captured");
    capturer.EndCapture();

    // After EndCapture, stdout should work normally again.
    // We verify indirectly: a second capture cycle should still work.
    capturer.BeginCapture();
    printf("also captured");
    capturer.EndCapture();
    std::string result = capturer.GetCapture();
    EXPECT_EQ(result, "also captured");
}

// Local Variables:
// c-basic-offset: 4
// End:
