#include "geometry/builtin_demos.hpp"

namespace geo {

std::vector<DemoEntry> builtinDemos() {
    std::vector<DemoEntry> demos;
    const std::string classical = "Classical Surfaces";
    const std::string nonEuclidean = "Non-Euclidean & Manifolds";

    // ---- Classical / Euclidean --------------------------------------------

    {
        DemoEntry e;
        e.name = "Monkey Saddle";
        e.category = classical;
        e.description = "z = x^3 - 3xy^2 -- three valleys, three ridges, saddle at the origin.";
        e.formulas.kind = SurfaceKind::Explicit;
        e.formulas.heightFormula = "x^3 - 3*x*y^2";
        e.formulas.xMin = -1.6; e.formulas.xMax = 1.6; e.formulas.yMin = -1.6; e.formulas.yMax = 1.6;
        e.formulas.resolutionU = 90; e.formulas.resolutionV = 90;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Gaussian Bump";
        e.category = classical;
        e.description = "z = 2*exp(-(x^2+y^2)) -- positive curvature at the peak, negative on the flanks.";
        e.formulas.kind = SurfaceKind::Explicit;
        e.formulas.heightFormula = "2*exp(-(x^2+y^2))";
        e.formulas.xMin = -3; e.formulas.xMax = 3; e.formulas.yMin = -3; e.formulas.yMax = 3;
        e.formulas.resolutionU = 90; e.formulas.resolutionV = 90;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Torus";
        e.category = classical;
        e.description = "Curvature flips sign: positive on the outer rim, negative on the inner throat.";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "(1.5 + 0.4*cos(u))*cos(v)";
        e.formulas.yFormula = "(1.5 + 0.4*cos(u))*sin(v)";
        e.formulas.zFormula = "0.4*sin(u)";
        e.formulas.uMin = 0; e.formulas.uMax = 2 * M_PI; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = true; e.formulas.periodicV = true;
        e.formulas.resolutionU = 100; e.formulas.resolutionV = 60;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Mobius Strip";
        e.category = classical;
        e.description = "One-sided surface with a single boundary curve. u = across the strip, v = along it.";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "(1 + (u/2)*cos(v/2))*cos(v)";
        e.formulas.yFormula = "(1 + (u/2)*cos(v/2))*sin(v)";
        e.formulas.zFormula = "(u/2)*sin(v/2)";
        e.formulas.uMin = -1; e.formulas.uMax = 1; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        // Deliberately NOT periodic in either direction: the half-twist means
        // v=2*pi reconnects to v=0 with u negated, which a simple wraparound
        // can't express. Leaving both open matches the strip's real boundary.
        e.formulas.periodicU = false; e.formulas.periodicV = false;
        e.formulas.resolutionU = 12; e.formulas.resolutionV = 120;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Klein Bottle";
        e.category = classical;
        e.description = "Figure-8 immersion (self-intersecting -- a true Klein bottle needs 4D to embed cleanly).";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "(2.5 + cos(u/2)*sin(v) - sin(u/2)*sin(2*v))*cos(u)";
        e.formulas.yFormula = "(2.5 + cos(u/2)*sin(v) - sin(u/2)*sin(2*v))*sin(u)";
        e.formulas.zFormula = "sin(u/2)*sin(v) + cos(u/2)*sin(2*v)";
        e.formulas.uMin = 0; e.formulas.uMax = 2 * M_PI; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        // Going around u by 2*pi maps v -> -v (the half-angle "twist"), which a
        // plain periodicU wraparound can't express -- periodicUTwistV mirrors
        // the v index at the seam instead, closing the mesh with no gap.
        e.formulas.periodicU = false; e.formulas.periodicV = true; e.formulas.periodicUTwistV = true;
        e.formulas.resolutionU = 100; e.formulas.resolutionV = 60;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Helicoid";
        e.category = classical;
        e.description = "Minimal surface swept by a rotating, rising line -- a ramp/screw shape.";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "u*cos(v)";
        e.formulas.yFormula = "u*sin(v)";
        e.formulas.zFormula = "0.3*v";
        e.formulas.uMin = -1; e.formulas.uMax = 1; e.formulas.vMin = 0; e.formulas.vMax = 4 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = false;
        e.formulas.resolutionU = 16; e.formulas.resolutionV = 140;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Catenoid";
        e.category = classical;
        e.description = "The only minimal surface of revolution besides the plane (Catalan's theorem).";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "0.6*cosh(u/0.6)*cos(v)";
        e.formulas.yFormula = "0.6*cosh(u/0.6)*sin(v)";
        e.formulas.zFormula = "u";
        e.formulas.uMin = -1.3; e.formulas.uMax = 1.3; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = true;
        e.formulas.resolutionU = 40; e.formulas.resolutionV = 80;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Enneper's Surface";
        e.category = classical;
        e.description = "A minimal surface built directly from a polynomial patch -- self-intersects near the edges.";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "u - (u^3)/3 + u*v^2";
        e.formulas.yFormula = "v - (v^3)/3 + v*u^2";
        e.formulas.zFormula = "u^2 - v^2";
        e.formulas.uMin = -1.2; e.formulas.uMax = 1.2; e.formulas.vMin = -1.2; e.formulas.vMax = 1.2;
        e.formulas.periodicU = false; e.formulas.periodicV = false;
        e.formulas.resolutionU = 70; e.formulas.resolutionV = 70;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Implicit: Metaball Pair";
        e.category = classical;
        e.description = "Two blended point-charges, F=0 raymarched on the GPU -- not a mesh at all.";
        e.formulas.kind = SurfaceKind::Implicit;
        e.formulas.implicitFormula = "1/((x-1)^2+y^2+z^2+0.01) + 1/((x+1)^2+y^2+z^2+0.01) - 1";
        e.formulas.bboxMin = -2.5; e.formulas.bboxMax = 2.5;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Implicit: Torus";
        e.category = classical;
        e.description = "(sqrt(x^2+y^2) - 1.2)^2 + z^2 = 0.25 -- same shape as the parametric torus, different technique.";
        e.formulas.kind = SurfaceKind::Implicit;
        e.formulas.implicitFormula = "(sqrt(x^2+y^2) - 1.2)^2 + z^2 - 0.25";
        e.formulas.bboxMin = -2.0; e.formulas.bboxMax = 2.0;
        demos.push_back(e);
    }

    // ---- Non-Euclidean & Manifolds -----------------------------------------

    {
        DemoEntry e;
        e.name = "Sphere";
        e.category = nonEuclidean;
        e.description = "Constant curvature +1/R^2 everywhere -- geodesics are great circles.";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "2*sin(u)*cos(v)";
        e.formulas.yFormula = "2*sin(u)*sin(v)";
        e.formulas.zFormula = "2*cos(u)";
        e.formulas.uMin = 0.0005; e.formulas.uMax = M_PI - 0.0005; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = true;
        e.formulas.resolutionU = 60; e.formulas.resolutionV = 100;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Pseudosphere";
        e.category = nonEuclidean;
        e.description = "Tractricoid: constant curvature -1, an actual embeddable patch of the hyperbolic plane (Beltrami).";
        e.formulas.kind = SurfaceKind::Parametric;
        e.formulas.xFormula = "(1/cosh(u))*cos(v)";
        e.formulas.yFormula = "(1/cosh(u))*sin(v)";
        e.formulas.zFormula = "u - tanh(u)";
        e.formulas.uMin = 0.01; e.formulas.uMax = 3.0; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = true;
        e.formulas.resolutionU = 60; e.formulas.resolutionV = 80;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Hyperboloid Model";
        e.category = nonEuclidean;
        e.description = "Same Euclidean shape as an ordinary (convex!) hyperboloid, but with the Minkowski-induced "
                         "metric imposed instead: curvature -1, a full model of the hyperbolic plane.";
        e.formulas.kind = SurfaceKind::Metric;
        e.formulas.metricHasEmbedding = true;
        e.formulas.embedXFormula = "sinh(u)*cos(v)";
        e.formulas.embedYFormula = "sinh(u)*sin(v)";
        e.formulas.embedZFormula = "cosh(u)";
        e.formulas.metricE = "1";
        e.formulas.metricF = "0";
        e.formulas.metricG = "sinh(u)^2";
        e.formulas.uMin = 0; e.formulas.uMax = 1.5; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = true;
        e.formulas.resolutionU = 50; e.formulas.resolutionV = 80;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Poincare Disk";
        e.category = nonEuclidean;
        e.description = "The whole hyperbolic plane squeezed into a unit disk -- straight lines are circular arcs "
                         "meeting the boundary at right angles. u=radius, v=angle.";
        e.formulas.kind = SurfaceKind::Metric;
        e.formulas.metricHasEmbedding = true;
        // Flat placement in polar form so the mesh is an actual disk (not an
        // invalid square with corners outside r<1); the hyperbolic metric is
        // then layered on top as an override, same mechanism as the hyperboloid.
        e.formulas.embedXFormula = "u*cos(v)";
        e.formulas.embedYFormula = "u*sin(v)";
        e.formulas.embedZFormula = "0";
        e.formulas.metricE = "4/(1-u^2)^2";
        e.formulas.metricF = "0";
        e.formulas.metricG = "4*u^2/(1-u^2)^2";
        e.formulas.uMin = 0; e.formulas.uMax = 0.95; e.formulas.vMin = 0; e.formulas.vMax = 2 * M_PI;
        e.formulas.periodicU = false; e.formulas.periodicV = true;
        e.formulas.resolutionU = 50; e.formulas.resolutionV = 100;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Poincare Half-Plane";
        e.category = nonEuclidean;
        e.description = "ds^2 = (du^2+dv^2)/v^2 on the upper half-plane -- another chart on the same hyperbolic plane.";
        e.formulas.kind = SurfaceKind::Metric;
        e.formulas.metricHasEmbedding = true;
        e.formulas.embedXFormula = "u";
        e.formulas.embedYFormula = "v";
        e.formulas.embedZFormula = "0";
        e.formulas.metricE = "1/v^2";
        e.formulas.metricF = "0";
        e.formulas.metricG = "1/v^2";
        e.formulas.uMin = -2; e.formulas.uMax = 2; e.formulas.vMin = 0.08; e.formulas.vMax = 2.5;
        e.formulas.periodicU = false; e.formulas.periodicV = false;
        e.formulas.resolutionU = 90; e.formulas.resolutionV = 60;
        e.formulas.colorByCurvature = true;
        demos.push_back(e);
    }
    {
        DemoEntry e;
        e.name = "Flat Torus";
        e.category = nonEuclidean;
        e.description = "Curvature exactly 0 -- opposite edges of this square are identified, like a video-game map "
                         "that wraps at the borders. Cannot be drawn as an ordinary donut without bending it.";
        e.formulas.kind = SurfaceKind::Metric;
        e.formulas.metricHasEmbedding = false; // rendered as the flat (u,v,0) model, honestly
        e.formulas.metricE = "1";
        e.formulas.metricF = "0";
        e.formulas.metricG = "1";
        e.formulas.uMin = 0; e.formulas.uMax = 1; e.formulas.vMin = 0; e.formulas.vMax = 1;
        e.formulas.periodicU = false; e.formulas.periodicV = false; // see note in mesh_tessellate about flat-model wrap
        e.formulas.resolutionU = 40; e.formulas.resolutionV = 40;
        e.formulas.colorByCurvature = false;
        demos.push_back(e);
    }

    return demos;
}

} // namespace geo
