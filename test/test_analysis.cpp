// Unit tests for the post-processing analyses (src/analysis.cpp),
// exercised without a GUI.

#include "analysis.h"

#include "gtest/gtest.h"

#include <cmath>
#include <vector>

namespace {

TEST(Autocorrelation, ExactSmallCase)
{
    // y = [1,2,3], mean 2, deviations [-1,0,1], denom = 2
    //   lag0 = 2/2 = 1 ; lag1 = 0/2 = 0 ; lag2 = -1/2 = -0.5
    const std::vector<double> acf = autocorrelation({1.0, 2.0, 3.0}, 2);
    ASSERT_EQ(acf.size(), 3u);
    EXPECT_NEAR(acf[0], 1.0, 1.0e-12);
    EXPECT_NEAR(acf[1], 0.0, 1.0e-12);
    EXPECT_NEAR(acf[2], -0.5, 1.0e-12);
}

TEST(Autocorrelation, LagZeroIsOne)
{
    const std::vector<double> acf = autocorrelation({3.0, 1.0, 4.0, 1.0, 5.0, 9.0}, 3);
    ASSERT_FALSE(acf.empty());
    EXPECT_NEAR(acf[0], 1.0, 1.0e-12);
    for (double v : acf)
        EXPECT_LE(std::fabs(v), 1.0 + 1.0e-12);
}

TEST(Autocorrelation, ConstantSeriesIsEmpty)
{
    EXPECT_TRUE(autocorrelation({5.0, 5.0, 5.0, 5.0}, 2).empty());
}

TEST(Autocorrelation, TooShortIsEmpty)
{
    EXPECT_TRUE(autocorrelation({}, 5).empty());
    EXPECT_TRUE(autocorrelation({1.0}, 5).empty());
}

TEST(Autocorrelation, MaxlagClampedToLength)
{
    const std::vector<double> y = {1.0, 2.0, 3.0, 4.0, 5.0};
    // non-positive maxlag clamps to n-1
    EXPECT_EQ(autocorrelation(y, 0).size(), 5u);
    // oversized maxlag clamps to n-1
    EXPECT_EQ(autocorrelation(y, 100).size(), 5u);
    // in-range maxlag is honored
    EXPECT_EQ(autocorrelation(y, 2).size(), 3u);
}

TEST(Autocorrelation, AlternatingIsAnticorrelated)
{
    std::vector<double> y(100);
    for (std::size_t i = 0; i < y.size(); ++i)
        y[i] = (i % 2 == 0) ? 1.0 : -1.0;

    const std::vector<double> acf = autocorrelation(y, 4);
    ASSERT_EQ(acf.size(), 5u);
    EXPECT_NEAR(acf[0], 1.0, 1.0e-12);
    EXPECT_LT(acf[1], 0.0); // odd lags: anticorrelated
    EXPECT_GT(acf[2], 0.0); // even lags: correlated
    EXPECT_LT(acf[3], 0.0);
}

} // namespace
