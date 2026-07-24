#include "geometry/builtin_demos.hpp"
#include "geometry/surface_spec.hpp"
#include <cmath>
#include <cstdio>

using namespace geo;

static int g_failures = 0;

static void checkTrue(bool ok, const std::string& what) {
    std::printf("  [%s] %s\n", ok ? " ok " : "FAIL", what.c_str());
    if (!ok) ++g_failures;
}
static void checkNear(double got, double want, double tol, const std::string& what) {
    bool ok = std::fabs(got - want) <= tol;
    std::printf("  [%s] %s (got %.6g, want %.6g, tol %.1e)\n", ok ? " ok " : "FAIL", what.c_str(), got, want, tol);
    if (!ok) ++g_failures;
}

int main() {
    auto demos = builtinDemos();
    std::printf("== building all %zu built-in demos ==\n", demos.size());
    for (auto& d : demos) {
        BuildResult r = buildSurface(d.formulas);
        if (!r.ok) {
            checkTrue(false, d.name + " builds without error -- " + r.error);
            continue;
        }
        checkTrue(true, d.name + " builds without error");
        if (d.formulas.kind != SurfaceKind::Implicit) {
            checkTrue(!r.mesh.vertices.empty() && !r.mesh.indices.empty(),
                      d.name + " produced a non-empty mesh (" + std::to_string(r.mesh.vertices.size()) +
                          " verts, " + std::to_string(r.mesh.indices.size() / 3) + " tris)");
        }
    }

    std::printf("\n== spot-checking curvature on the non-Euclidean demos' own mesh ranges ==\n");
    auto findDemo = [&](const std::string& name) -> const DemoEntry& {
        for (auto& d : demos)
            if (d.name == name) return d;
        throw std::runtime_error("demo not found: " + name);
    };
    auto meshCurvatureRange = [](const std::string& name, const DemoEntry& d) {
        BuildResult r = buildSurface(d.formulas);
        std::printf("  %-22s scalar range [%.6f, %.6f]\n", name.c_str(), r.mesh.scalarMin, r.mesh.scalarMax);
        return std::make_pair(r.mesh.scalarMin, r.mesh.scalarMax);
    };

    {
        auto [lo, hi] = meshCurvatureRange("Sphere", findDemo("Sphere"));
        checkNear(lo, 0.25, 1e-3, "Sphere K ~= 1/R^2 = 0.25 (min)");
        checkNear(hi, 0.25, 1e-3, "Sphere K ~= 1/R^2 = 0.25 (max, i.e. truly constant)");
    }
    {
        auto [lo, hi] = meshCurvatureRange("Pseudosphere", findDemo("Pseudosphere"));
        checkNear(lo, -1.0, 1e-3, "Pseudosphere K ~= -1 (min)");
        checkNear(hi, -1.0, 1e-3, "Pseudosphere K ~= -1 (max)");
    }
    {
        auto [lo, hi] = meshCurvatureRange("Hyperboloid Model", findDemo("Hyperboloid Model"));
        checkNear(lo, -1.0, 1e-3, "Hyperboloid (Minkowski-metric) K ~= -1 (min)");
        checkNear(hi, -1.0, 1e-3, "Hyperboloid (Minkowski-metric) K ~= -1 (max)");
    }
    {
        // This independently checks the polar-form reparametrization used in
        // builtin_demos.cpp (E=4/(1-u^2)^2, F=0, G=4u^2/(1-u^2)^2 with u=r,
        // v=theta) against the same K=-1 result the Cartesian form gave in
        // diffgeo_test.cpp -- confirms the by-hand change-of-variables was done
        // correctly, not just that *a* formula happens to produce -1.
        auto [lo, hi] = meshCurvatureRange("Poincare Disk", findDemo("Poincare Disk"));
        checkNear(lo, -1.0, 1e-3, "Poincare disk (polar reparametrization) K ~= -1 (min)");
        checkNear(hi, -1.0, 1e-3, "Poincare disk (polar reparametrization) K ~= -1 (max)");
    }
    {
        auto [lo, hi] = meshCurvatureRange("Poincare Half-Plane", findDemo("Poincare Half-Plane"));
        checkNear(lo, -1.0, 1e-3, "Poincare half-plane K ~= -1 (min)");
        checkNear(hi, -1.0, 1e-3, "Poincare half-plane K ~= -1 (max)");
    }
    {
        auto [lo, hi] = meshCurvatureRange("Torus", findDemo("Torus"));
        checkTrue(lo < -0.1, "Torus min curvature is negative (inner throat), got " + std::to_string(lo));
        checkTrue(hi > 0.1, "Torus max curvature is positive (outer rim), got " + std::to_string(hi));
    }

    std::printf("\n== formula error reporting is usable ==\n");
    {
        SurfaceFormulas f;
        f.kind = SurfaceKind::Parametric;
        f.xFormula = "sin(u";  // unbalanced paren
        f.yFormula = "v"; f.zFormula = "0";
        BuildResult r = buildSurface(f);
        checkTrue(!r.ok && !r.error.empty(), "unbalanced paren reported as a friendly error: " + r.error);
    }
    {
        SurfaceFormulas f;
        f.kind = SurfaceKind::Parametric;
        f.xFormula = "u"; f.yFormula = "v"; f.zFormula = "w"; // 'w' is not a valid variable here
        BuildResult r = buildSurface(f);
        checkTrue(!r.ok && !r.error.empty(), "unknown variable reported as a friendly error: " + r.error);
    }
    {
        SurfaceFormulas f;
        f.kind = SurfaceKind::Metric;
        f.metricE = "1"; f.metricF = "0"; f.metricG = "max(u,v)"; // not smooth -> differentiate() throws
        BuildResult r = buildSurface(f);
        checkTrue(!r.ok && !r.error.empty(), "non-smooth metric (max) reported as a friendly error: " + r.error);
    }

    std::printf("\n%d failure(s)\n", g_failures);
    return g_failures == 0 ? 0 : 1;
}
