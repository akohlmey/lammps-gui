// Unit tests for the customfunc module: evaluating user-supplied expressions
// (via the vendored LeptonMini parser) over an x range for custom-function
// plotting in the chart post-processing dialog.

#include "customfunc.h"

#include "gtest/gtest.h"

#include <cmath>
#include <vector>

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

// ---- nonlinear fitting via fitCustomCurve --------------------------------

namespace {
// build (x, y) samples from a model functor over an index range
template <typename F>
void makeData(std::vector<double> &xs, std::vector<double> &ys, int npts, double x0, double dx,
              F model)
{
    xs.clear();
    ys.clear();
    for (int i = 0; i < npts; ++i) {
        const double x = x0 + dx * i;
        xs.push_back(x);
        ys.push_back(model(x));
    }
}
} // namespace

// fit an exponential decay A*exp(-k*x) and recover A and k
TEST(CustomFit, ExponentialDecay)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 20, 0.0, 0.2, [](double x) {
        return 5.0 * std::exp(-0.7 * x);
    });

    const QList<FitParam> init = {{"A", 1.0}, {"k", 0.1}};
    const CustomFit f          = fitCustomCurve("A*exp(-k*x)", init, xs, ys, 0.0, 3.8, 50);

    ASSERT_TRUE(f.ok) << f.error.toStdString();
    ASSERT_EQ(f.params.size(), 2);
    EXPECT_EQ(f.params[0].name, "A");
    EXPECT_EQ(f.params[1].name, "k");
    EXPECT_NEAR(f.params[0].value, 5.0, 1e-3);
    EXPECT_NEAR(f.params[1].value, 0.7, 1e-3);
    EXPECT_NEAR(f.rms, 0.0, 1e-4);
    EXPECT_EQ(f.curve.size(), 51);
}

// fit a quadratic with named coefficients
TEST(CustomFit, Quadratic)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 15, -2.0, 0.3, [](double x) {
        return 2.0 * x * x - 3.0 * x + 1.0;
    });

    const QList<FitParam> init = {{"a", 1.0}, {"b", 1.0}, {"c", 0.0}};
    const CustomFit f          = fitCustomCurve("a*x^2 + b*x + c", init, xs, ys, -2.0, 2.2, 10);

    ASSERT_TRUE(f.ok) << f.error.toStdString();
    ASSERT_EQ(f.params.size(), 3);
    EXPECT_NEAR(f.params[0].value, 2.0, 1e-4);
    EXPECT_NEAR(f.params[1].value, -3.0, 1e-4);
    EXPECT_NEAR(f.params[2].value, 1.0, 1e-4);
}

// a parameter that clashes with the independent variable is rejected
TEST(CustomFit, RejectsVariableClash)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 5, 0.0, 1.0, [](double x) {
        return x;
    });
    const QList<FitParam> init = {{"x", 1.0}};
    const CustomFit f          = fitCustomCurve("a*x", init, xs, ys, 0.0, 4.0, 4);
    EXPECT_FALSE(f.ok);
    EXPECT_FALSE(f.error.isEmpty());
}

// duplicate parameter names are rejected
TEST(CustomFit, RejectsDuplicateParam)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 5, 0.0, 1.0, [](double x) {
        return x;
    });
    const QList<FitParam> init = {{"a", 1.0}, {"a", 2.0}};
    const CustomFit f          = fitCustomCurve("a*x", init, xs, ys, 0.0, 4.0, 4);
    EXPECT_FALSE(f.ok);
    EXPECT_FALSE(f.error.isEmpty());
}

// an expression referencing an undeclared symbol fails with a message
TEST(CustomFit, RejectsUndeclaredSymbol)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 6, 0.0, 1.0, [](double x) {
        return x;
    });
    const QList<FitParam> init = {{"a", 1.0}};
    // 'q' is neither the variable nor a declared parameter
    const CustomFit f = fitCustomCurve("a*x + q", init, xs, ys, 0.0, 5.0, 5);
    EXPECT_FALSE(f.ok);
    EXPECT_FALSE(f.error.isEmpty());
}

// too few data points for the number of parameters is rejected
TEST(CustomFit, RejectsTooFewPoints)
{
    std::vector<double> xs, ys;
    makeData(xs, ys, 1, 0.0, 1.0, [](double x) {
        return x;
    });
    const QList<FitParam> init = {{"a", 1.0}, {"b", 1.0}};
    const CustomFit f          = fitCustomCurve("a*x + b", init, xs, ys, 0.0, 1.0, 4);
    EXPECT_FALSE(f.ok);
    EXPECT_FALSE(f.error.isEmpty());
}
