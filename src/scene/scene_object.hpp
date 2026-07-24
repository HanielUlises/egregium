#pragma once
#include "gl/gpu_mesh.hpp"
#include "gl/shader.hpp"
#include "geometry/differential_geometry.hpp"
#include "geometry/surface_spec.hpp"
#include <glm/glm.hpp>
#include <optional>
#include <string>
#include <vector>

namespace scene {

struct GeodesicRun {
    gl::GPULineStrip line;
    double u0, v0, dirAngle, arcLength;
};

// One entry in the scene: either a tessellated mesh (Explicit/Parametric/
// Metric) or a raymarched implicit surface. Move-only (owns GPU resources).
struct SceneObject {
    std::string name;
    geo::SurfaceKind kind = geo::SurfaceKind::Parametric;
    bool visible = true;
    glm::vec3 baseColor{0.55f, 0.65f, 0.85f};

    // mesh-type objects (Explicit/Parametric/Metric)
    bool wireframe = false;
    bool colorByCurvature = false;
    bool curvatureHeatmap = false; // when colorByCurvature: sequential heatmap of |K| instead of the diverging signed map
    std::optional<geo::DiffGeoSurface> diffGeo; // present for Explicit/Parametric/Metric; used for geodesic shooting
    gl::GPUMesh gpuMesh;
    double scalarAbsMax = 1.0; // for colormap normalization (max(|min|,|max|) of the mesh's curvature range)
    std::vector<GeodesicRun> geodesics;
    glm::vec3 geodesicColor{1.0f, 0.82f, 0.10f};

    // implicit-type objects
    gl::GPUMesh implicitBoundingCube;
    gl::Shader implicitShader;
    glm::vec3 implicitBoxMin{-2, -2, -2}, implicitBoxMax{2, 2, 2};

    // formulas that produced this object, kept so the inspector can show
    // "how was this made" and so geodesic shooting has sane domain bounds
    geo::SurfaceFormulas formulas;
};

} // namespace scene
