// Validates DiffGeoSurface against closed-form results derived by hand
// (see comments). No graphics dependency -- pure math, run from the shell.
#include "expr/parser.hpp"
#include "geometry/differential_geometry.hpp"
#include <cmath>
#include <cstdio>

using namespace geo;
using expr::parseExpression;

static int g_failures = 0;

static expr::ExprPtr P(const std::string& formula) {
    auto r = parseExpression(formula);
    if (!r.ok) {
        std::printf("  [FAIL] could not parse '%s': %s\n", formula.c_str(), r.error.c_str());
        ++g_failures;
        return expr::konst(0.0);
    }
    return r.expr;
}

static void checkNear(double got, double want, double tol, const std::string& what) {
    bool ok = std::fabs(got - want) <= tol;
    std::printf("  [%s] %s (got %.10g, want %.10g, tol %.1e)\n", ok ? " ok " : "FAIL", what.c_str(), got, want, tol);
    if (!ok) ++g_failures;
}

static void checkTrue(bool ok, const std::string& what) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what.c_str());
    if (!ok) ++g_failures;
}

int main() {
    std::printf("== Sphere (R=2): constant curvature 1/R^2 = 0.25, intrinsic==extrinsic ==\n");
    {
        double R = 2.0;
        DiffGeoSurface sph = DiffGeoSurface::fromEmbedding({
            P("2*sin(u)*cos(v)"), P("2*sin(u)*sin(v)"), P("2*cos(u)")
        });
        for (auto [u, v] : {std::pair{1.0, 0.5}, std::pair{2.0, 3.0}, std::pair{0.7, -1.1}}) {
            double ki = sph.gaussianCurvature(u, v);
            double ke = sph.gaussianCurvatureExtrinsic(u, v);
            checkNear(ki, 1.0 / (R * R), 1e-6, "K_intrinsic == 1/R^2 at (" + std::to_string(u) + "," + std::to_string(v) + ")");
            checkNear(ke, 1.0 / (R * R), 1e-6, "K_extrinsic == 1/R^2 (Theorema Egregium cross-check)");
        }
    }

    std::printf("\n== Pseudosphere (tractricoid): constant curvature -1 (Beltrami) ==\n");
    {
        // X=sech(t)cos(v), Y=sech(t)sin(v), Z=t-tanh(t); hand-derived E=tanh^2(t), F=0, G=sech^2(t).
        // sech(t) isn't a builtin function name -- express it as 1/cosh(t).
        DiffGeoSurface pseudo = DiffGeoSurface::fromEmbedding({
            P("(1/cosh(u))*cos(v)"), P("(1/cosh(u))*sin(v)"), P("u - tanh(u)")
        });
        for (double u : {0.5, 1.0, 1.8}) {
            double ki = pseudo.gaussianCurvature(u, 0.4);
            double ke = pseudo.gaussianCurvatureExtrinsic(u, 0.4);
            checkNear(ki, -1.0, 1e-5, "K_intrinsic == -1 at u=" + std::to_string(u));
            checkNear(ke, -1.0, 1e-5, "K_extrinsic == -1 (cross-check) at u=" + std::to_string(u));
        }
    }

    std::printf("\n== Poincare disk: metric-only (no embedding), constant curvature -1 ==\n");
    {
        // ds^2 = 4(du^2+dv^2)/(1-u^2-v^2)^2 -- hand-verified via K=-(1/lambda^2)*Laplacian(ln lambda) = -1
        DiffGeoSurface disk = DiffGeoSurface::fromMetric({
            P("4/(1-u^2-v^2)^2"), P("0"), P("4/(1-u^2-v^2)^2")
        });
        checkTrue(!disk.hasEmbedding(), "Poincare disk correctly has no embedding (flat-model rendering)");
        for (auto [u, v] : {std::pair{0.3, 0.2}, std::pair{-0.5, 0.1}, std::pair{0.0, 0.6}}) {
            checkNear(disk.gaussianCurvature(u, v), -1.0, 1e-6,
                      "K == -1 at (" + std::to_string(u) + "," + std::to_string(v) + ")");
        }
    }

    std::printf("\n== Flat plane: curvature 0 everywhere ==\n");
    {
        DiffGeoSurface flat = DiffGeoSurface::fromMetric({P("1"), P("0"), P("1")});
        checkNear(flat.gaussianCurvature(0.3, -2.1), 0.0, 1e-9, "K == 0");
    }

    std::printf("\n== Hyperboloid model: Minkowski-metric K=-1 vs Euclidean-embedding K>0 (deliberately disagree) ==\n");
    {
        // Euclidean embedding of the upper sheet of x^2+y^2-z^2=-1: X=sinh(u)cos(v), Y=sinh(u)sin(v), Z=cosh(u).
        // Hand-derived Minkowski-induced metric (ds^2 with signature (+,+,-)): E=1, F=0, G=sinh^2(u).
        DiffGeoSurface hyp = DiffGeoSurface::fromEmbeddingAndMetric(
            {P("sinh(u)*cos(v)"), P("sinh(u)*sin(v)"), P("cosh(u)")},
            {P("1"), P("0"), P("sinh(u)^2")});
        double kIntrinsic = hyp.gaussianCurvature(0.8, 1.2);
        double kExtrinsic = hyp.gaussianCurvatureExtrinsic(0.8, 1.2);
        checkNear(kIntrinsic, -1.0, 1e-6, "Minkowski-metric K_intrinsic == -1 (this IS a hyperbolic-plane model)");
        checkTrue(kExtrinsic > 0.05, "Euclidean K_extrinsic is clearly positive (shape is a convex bowl) -- " +
                                          std::to_string(kExtrinsic) + " != -1, as expected");
    }

    std::printf("\n== Torus: intrinsic/extrinsic agree, curvature changes sign outer vs inner ==\n");
    {
        // Standard torus, tube radius r=0.4, ring radius R=1.5, u=around tube, v=around ring.
        DiffGeoSurface torus = DiffGeoSurface::fromEmbedding({
            P("(1.5 + 0.4*cos(u))*cos(v)"), P("(1.5 + 0.4*cos(u))*sin(v)"), P("0.4*sin(u)")
        });
        double kOuter = torus.gaussianCurvature(0.0, 0.0);      // outer equator, u=0 -> should be > 0
        double kInner = torus.gaussianCurvature(M_PI, 0.0);     // inner equator, u=pi -> should be < 0
        checkNear(kOuter, torus.gaussianCurvatureExtrinsic(0.0, 0.0), 1e-6, "outer: intrinsic==extrinsic");
        checkNear(kInner, torus.gaussianCurvatureExtrinsic(M_PI, 0.0), 1e-6, "inner: intrinsic==extrinsic");
        checkTrue(kOuter > 0.1, "outer equator curvature is positive (" + std::to_string(kOuter) + ")");
        checkTrue(kInner < -0.1, "inner equator curvature is negative (" + std::to_string(kInner) + ")");
    }

    std::printf("\n== Geodesics: flat plane -> straight line ==\n");
    {
        DiffGeoSurface flat = DiffGeoSurface::fromMetric({P("1"), P("0"), P("1")});
        auto path = flat.integrateGeodesic(0.0, 0.0, 0.37, 5.0, 200);
        // straight line through origin at angle 0.37: v = u*tan(0.37) exactly
        double slope = std::tan(0.37);
        double maxDev = 0.0;
        for (auto& p : path) maxDev = std::max(maxDev, std::fabs(p.v - slope * p.u));
        checkNear(maxDev, 0.0, 1e-6, "geodesic stays on the line v=tan(0.37)*u");
    }

    std::printf("\n== Geodesics: sphere -> great circle (points coplanar with sphere center) ==\n");
    {
        DiffGeoSurface sph = DiffGeoSurface::fromEmbedding({P("2*sin(u)*cos(v)"), P("2*sin(u)*sin(v)"), P("2*cos(u)")});
        auto path = sph.integrateGeodesic(1.0, 0.4, 1.1, 6.0, 300);
        Vec3 p0 = sph.renderPosition(path[0].u, path[0].v);
        Vec3 p1 = sph.renderPosition(path[1].u, path[1].v);
        Vec3 planeNormal = p0.cross(p1).normalized();
        double maxAbsDot = 0.0; // should stay ~0: every point lies in the plane through the origin with this normal
        for (auto& gp : path) {
            Vec3 p = sph.renderPosition(gp.u, gp.v);
            maxAbsDot = std::max(maxAbsDot, std::fabs(p.normalized().dot(planeNormal)));
        }
        checkNear(maxAbsDot, 0.0, 1e-3, "all points lie in a single plane through the center (great circle)");
    }

    std::printf("\n== Geodesics: Poincare disk radial line through origin stays a Euclidean straight line ==\n");
    {
        DiffGeoSurface disk = DiffGeoSurface::fromMetric({P("4/(1-u^2-v^2)^2"), P("0"), P("4/(1-u^2-v^2)^2")});
        auto path = disk.integrateGeodesic(0.0, 0.0, 0.9, 1.5, 200);
        double slope = std::tan(0.9);
        double maxDev = 0.0;
        for (auto& p : path) maxDev = std::max(maxDev, std::fabs(p.v - slope * p.u));
        checkNear(maxDev, 0.0, 1e-6, "diameter-through-origin geodesic is a straight line in disk coordinates");
    }

    std::printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
