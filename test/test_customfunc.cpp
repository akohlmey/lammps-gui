// Unit tests for the customfunc module: evaluating user-supplied expressions
// (via the vendored LeptonMini parser) over an x range for custom-function
// plotting in the chart post-processing dialog.

#include "customfunc.h"

#include "gtest/gtest.h"

#include <cmath>

// a polynomial sampled at the endpoints and midpoint
TEST(CustomFunc, Polynomial)
{
    const CustomCurve c = evalCustomCurve("2*x^2 + 1", 0.0, 2.0, 2);
    ASSERT_TRUE(c.ok);
    EXPECT_TRUE(c.error.isEmpty());
    ASSERT_EQ(c.points.size(), 3);
    EXPECT_DOUBLE_EQ(c.points[0].x(), 0.0);
    EXPECT_DOUBLE_EQ(c.points[0].y(), 1.0); // 2*0+1
    EXPECT_DOUBLE_EQ(c.points[1].x(), 1.0);
    EXPECT_DOUBLE_EQ(c.points[1].y(), 3.0); // 2*1+1
    EXPECT_DOUBLE_EQ(c.points[2].x(), 2.0);
    EXPECT_DOUBLE_EQ(c.points[2].y(), 9.0); // 2*4+1
}

// transcendental functions are recognized
TEST(CustomFunc, Trig)
{
    const CustomCurve c = evalCustomCurve("sin(x)", 0.0, M_PI, 2);
    ASSERT_TRUE(c.ok);
    ASSERT_EQ(c.points.size(), 3);
    EXPECT_NEAR(c.points[0].y(), 0.0, 1e-12);
    EXPECT_NEAR(c.points[1].y(), 1.0, 1e-12); // sin(pi/2)
    EXPECT_NEAR(c.points[2].y(), 0.0, 1e-12); // sin(pi)
}

// a constant expression yields a flat line
TEST(CustomFunc, Constant)
{
    const CustomCurve c = evalCustomCurve("42", -1.0, 1.0, 4);
    ASSERT_TRUE(c.ok);
    ASSERT_EQ(c.points.size(), 5);
    for (const auto &p : c.points)
        EXPECT_DOUBLE_EQ(p.y(), 42.0);
}

// a custom variable name can be used instead of x
TEST(CustomFunc, CustomVariable)
{
    const CustomCurve c = evalCustomCurve("v^2", 1.0, 3.0, 2, "v");
    ASSERT_TRUE(c.ok);
    ASSERT_EQ(c.points.size(), 3);
    EXPECT_DOUBLE_EQ(c.points[0].y(), 1.0);
    EXPECT_DOUBLE_EQ(c.points[1].y(), 4.0);
    EXPECT_DOUBLE_EQ(c.points[2].y(), 9.0);
}

// non-finite values are skipped: 1/x at x=0 is dropped, the rest are kept
TEST(CustomFunc, SkipsNonFinite)
{
    const CustomCurve c = evalCustomCurve("1/x", 0.0, 2.0, 2);
    ASSERT_TRUE(c.ok);
    // x = 0 gives inf and is dropped; x = 1 and x = 2 remain
    ASSERT_EQ(c.points.size(), 2);
    EXPECT_DOUBLE_EQ(c.points[0].x(), 1.0);
    EXPECT_DOUBLE_EQ(c.points[0].y(), 1.0);
    EXPECT_DOUBLE_EQ(c.points[1].x(), 2.0);
    EXPECT_DOUBLE_EQ(c.points[1].y(), 0.5);
}

// an empty expression is reported as an error, not a crash
TEST(CustomFunc, EmptyExpression)
{
    const CustomCurve c = evalCustomCurve("   ", 0.0, 1.0, 4);
    EXPECT_FALSE(c.ok);
    EXPECT_FALSE(c.error.isEmpty());
    EXPECT_TRUE(c.points.isEmpty());
}

// a syntactically invalid expression is reported as an error
TEST(CustomFunc, InvalidSyntax)
{
    const CustomCurve c = evalCustomCurve("2*(x+", 0.0, 1.0, 4);
    EXPECT_FALSE(c.ok);
    EXPECT_FALSE(c.error.isEmpty());
    EXPECT_TRUE(c.points.isEmpty());
}

// referencing an undefined variable is an error (only the declared variable is set)
TEST(CustomFunc, UndefinedVariable)
{
    const CustomCurve c = evalCustomCurve("a*x + b", 0.0, 1.0, 4);
    EXPECT_FALSE(c.ok);
    EXPECT_FALSE(c.error.isEmpty());
    EXPECT_TRUE(c.points.isEmpty());
}

// nsamples below 1 is clamped to 1 (two endpoints)
TEST(CustomFunc, ClampsSampleCount)
{
    const CustomCurve c = evalCustomCurve("x", 0.0, 1.0, 0);
    ASSERT_TRUE(c.ok);
    ASSERT_EQ(c.points.size(), 2);
    EXPECT_DOUBLE_EQ(c.points[0].x(), 0.0);
    EXPECT_DOUBLE_EQ(c.points[1].x(), 1.0);
}
