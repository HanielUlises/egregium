// Standalone sanity test for expr/*. No OpenGL/window dependency at all --
// this exercises parsing, evaluation, symbolic differentiation, and
// simplification against hand-computed expected values.
#include "expr/parser.hpp"
#include "expr/interpreter.hpp"
#include "expr/differentiate.hpp"
#include "expr/glsl_codegen.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

using namespace expr;

static int g_failures = 0;

static void check(bool cond, const std::string& what) {
    if (!cond) {
        std::printf("  [FAIL] %s\n", what.c_str());
        ++g_failures;
    } else {
        std::printf("  [ ok ] %s\n", what.c_str());
    }
}

static void checkNear(double got, double want, double tol, const std::string& what) {
    bool ok = std::fabs(got - want) <= tol;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s (got %.10g, want %.10g, tol %.1e)", what.c_str(), got, want, tol);
    check(ok, buf);
}

int main() {
    std::printf("== parsing + evaluation ==\n");
    {
        auto r = parseExpression("2 + 3 * 4");
        check(r.ok, "parses '2 + 3 * 4'");
        if (r.ok) checkNear(evaluate(r.expr, {}), 14.0, 1e-12, "precedence: 2+3*4 == 14");
    }
    {
        auto r = parseExpression("-2^2");
        check(r.ok, "parses '-2^2'");
        if (r.ok) checkNear(evaluate(r.expr, {}), -4.0, 1e-12, "-2^2 == -4 (unary minus looser than ^)");
    }
    {
        auto r = parseExpression("2^-2");
        check(r.ok, "parses '2^-2'");
        if (r.ok) checkNear(evaluate(r.expr, {}), 0.25, 1e-12, "2^-2 == 0.25 (negative exponent)");
    }
    {
        auto r = parseExpression("2^3^2"); // right-assoc: 2^(3^2) = 2^9 = 512, NOT (2^3)^2=64
        check(r.ok, "parses '2^3^2'");
        if (r.ok) checkNear(evaluate(r.expr, {}), 512.0, 1e-9, "^ is right-associative");
    }
    {
        auto r = parseExpression("sin(pi/2) + sqrt(4) - abs(-3)");
        check(r.ok, "parses function calls + constants");
        if (r.ok) checkNear(evaluate(r.expr, {}), 1.0 + 2.0 - 3.0, 1e-9, "sin(pi/2)+sqrt(4)-abs(-3) == 0");
    }
    {
        auto r = parseExpression("sech(t)"); // sech isn't in the function table
        check(!r.ok, "rejects unknown function 'sech'");
    }
    {
        auto r = parseExpression("sin(1,2)"); // wrong arity
        check(!r.ok, "rejects wrong arity for sin()");
    }
    {
        Bindings b{{"u", 0.3}, {"v", -1.2}};
        auto r = parseExpression("u*u + v*v");
        check(r.ok, "parses 'u*u + v*v'");
        if (r.ok) checkNear(evaluate(r.expr, b), 0.3 * 0.3 + 1.2 * 1.2, 1e-12, "u*u+v*v matches bindings");
    }

    std::printf("\n== symbolic differentiation ==\n");
    auto d_at = [](const std::string& formula, const std::string& wrt, Bindings at) -> double {
        auto r = parseExpression(formula);
        if (!r.ok) throw std::runtime_error("parse failed: " + r.error);
        ExprPtr d = simplify(differentiate(r.expr, wrt));
        return evaluate(d, at);
    };
    checkNear(d_at("x^2", "x", {{"x", 3.0}}), 6.0, 1e-9, "d/dx(x^2) at x=3 == 6");
    checkNear(d_at("x^3", "x", {{"x", 2.0}}), 12.0, 1e-9, "d/dx(x^3) at x=2 == 12  (3x^2)");
    checkNear(d_at("sin(x)", "x", {{"x", 0.0}}), 1.0, 1e-9, "d/dx(sin(x)) at x=0 == 1  (cos 0)");
    checkNear(d_at("cos(x)", "x", {{"x", 0.0}}), 0.0, 1e-9, "d/dx(cos(x)) at x=0 == 0  (-sin 0)");
    checkNear(d_at("exp(x)", "x", {{"x", 1.234}}), std::exp(1.234), 1e-9, "d/dx(exp(x)) == exp(x)");
    checkNear(d_at("log(x)", "x", {{"x", 5.0}}), 0.2, 1e-9, "d/dx(log(x)) at x=5 == 1/5");
    checkNear(d_at("x*y", "x", {{"x", 2.0}, {"y", 5.0}}), 5.0, 1e-9, "d/dx(x*y) == y  (product rule)");
    checkNear(d_at("x*y", "y", {{"x", 2.0}, {"y", 5.0}}), 2.0, 1e-9, "d/dy(x*y) == x  (partial derivative)");
    checkNear(d_at("x/y", "x", {{"x", 3.0}, {"y", 4.0}}), 0.25, 1e-9, "d/dx(x/y) == 1/y  (quotient rule)");
    checkNear(d_at("sqrt(x)", "x", {{"x", 4.0}}), 0.25, 1e-9, "d/dx(sqrt(x)) at x=4 == 1/(2*sqrt(4))=0.25");
    checkNear(d_at("tanh(x)", "x", {{"x", 0.0}}), 1.0, 1e-9, "d/dx(tanh(x)) at x=0 == 1  (sech^2 0)");
    checkNear(d_at("sin(x)*cos(x)", "x", {{"x", 0.5}}), std::cos(1.0), 1e-9, "d/dx(sin x cos x) == cos(2x)");

    // second derivative
    {
        auto r = parseExpression("sin(x)");
        ExprPtr d1 = simplify(differentiate(r.expr, "x"));
        ExprPtr d2 = simplify(differentiate(d1, "x"));
        checkNear(evaluate(d2, {{"x", 0.7}}), -std::sin(0.7), 1e-9, "d2/dx2(sin(x)) == -sin(x)");
    }

    // min/max must refuse symbolic differentiation
    {
        auto r = parseExpression("min(x,y)");
        bool threw = false;
        try {
            differentiate(r.expr, "x");
        } catch (const std::exception&) {
            threw = true;
        }
        check(threw, "differentiate() throws on min() (not smooth)");
    }

    std::printf("\n== GLSL codegen (structural spot-check) ==\n");
    {
        auto r = parseExpression("x^2 + y^2 + z^2 - 1");
        std::string glsl = toGLSL(r.expr);
        check(glsl.find("pow(x, 2") != std::string::npos, "GLSL codegen uses pow() for ^");
        std::printf("       -> %s\n", glsl.c_str());
    }

    std::printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
