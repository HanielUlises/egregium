#pragma once
#include "geometry/differential_geometry.hpp"
#include "geometry/mesh_types.hpp"
#include <optional>
#include <string>

namespace geo {

enum class SurfaceKind { Explicit, Parametric, Implicit, Metric };

// The "recipe" for a surface: either hand-written by a built-in demo, or
// filled in from the Add-Surface dialog's text fields. Every field is a
// plain formula string or number so it round-trips cleanly through UI text
// boxes.
struct SurfaceFormulas {
    SurfaceKind kind = SurfaceKind::Parametric;
    std::string name = "Untitled Surface";

    // Explicit: z = heightFormula(x,y) over [xMin,xMax] x [yMin,yMax]
    std::string heightFormula = "sin(x)*cos(y)";
    double xMin = -3, xMax = 3, yMin = -3, yMax = 3;

    // Parametric: (xFormula,yFormula,zFormula) as functions of (u,v)
    std::string xFormula = "u", yFormula = "v", zFormula = "0";

    // Implicit: F(x,y,z) = 0, raymarched inside the cube [bboxMin,bboxMax]^3
    std::string implicitFormula = "x^2+y^2+z^2-1";
    double bboxMin = -2.0, bboxMax = 2.0;

    // Metric: E,F,G as functions of (u,v). Embedding optional -- if
    // metricHasEmbedding is false the surface renders as a flat (u,v,0) model.
    std::string metricE = "1", metricF = "0", metricG = "1";
    bool metricHasEmbedding = false;
    std::string embedXFormula, embedYFormula, embedZFormula;

    // shared by Parametric/Metric (Explicit converts internally to this too)
    double uMin = 0, uMax = 1, vMin = 0, vMax = 1;
    bool periodicU = false, periodicV = false;
    int resolutionU = 80, resolutionV = 80;
    bool colorByCurvature = false;

    float colorR = 0.55f, colorG = 0.65f, colorB = 0.85f;
};

struct BuildResult {
    bool ok = false;
    std::string error; // set iff !ok -- always a message meant to be shown directly in the UI
    SurfaceKind kind = SurfaceKind::Parametric;

    std::optional<DiffGeoSurface> diffGeo; // Explicit/Parametric/Metric only
    Mesh mesh;                              // tessellated mesh, Explicit/Parametric/Metric only

    std::string implicitGLSL;               // Implicit only: F(x,y,z) compiled to a GLSL expression
    double bboxMin = -2, bboxMax = 2;        // Implicit only
};

// Parses every formula involved, validates variable names, builds the
// DiffGeoSurface (if applicable) and tessellates it. Never throws -- parse
// or differentiation failures (e.g. min/max in a metric formula) come back
// as BuildResult::error instead, since this is called directly from UI
// "Create" button handlers.
BuildResult buildSurface(const SurfaceFormulas& formulas);

} // namespace geo
