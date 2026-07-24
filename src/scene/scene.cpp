#include "scene/scene.hpp"
#include "gl/shaders_glsl.hpp"
#include "geometry/implicit_surface.hpp"
#include "geometry/mesh_tessellate.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace scene {

namespace {
geo::Polyline makeGridLines() {
    geo::Polyline poly;
    const double extent = 6.0, step = 1.0;
    for (double x = -extent; x <= extent + 1e-9; x += step) {
        poly.points.push_back({x, 0, -extent});
        poly.points.push_back({x, 0, extent});
    }
    for (double z = -extent; z <= extent + 1e-9; z += step) {
        poly.points.push_back({-extent, 0, z});
        poly.points.push_back({extent, 0, z});
    }
    return poly;
}
} // namespace

Scene::Scene()
    : meshShader_(gl::Shader::fromSource(gl::shaders::kMeshVert, gl::shaders::kMeshFrag)),
      lineShader_(gl::Shader::fromSource(gl::shaders::kLineVert, gl::shaders::kLineFrag)),
      gridLines_(gl::GPULineStrip::upload(makeGridLines())) {}

SceneObject& Scene::addFromBuildResult(const std::string& name, const geo::SurfaceFormulas& formulas,
                                        geo::BuildResult&& built, glm::vec3 color) {
    SceneObject obj;
    obj.name = name;
    obj.kind = built.kind;
    obj.baseColor = color;
    obj.formulas = formulas;

    if (built.kind == geo::SurfaceKind::Implicit) {
        geo::Mesh cube = geo::makeBoundingCube(built.bboxMin, built.bboxMax);
        obj.implicitBoundingCube = gl::GPUMesh::upload(cube);
        obj.implicitBoxMin = glm::vec3(built.bboxMin);
        obj.implicitBoxMax = glm::vec3(built.bboxMax);
        std::string fragSrc = gl::shaders::buildImplicitFragmentShader(built.implicitGLSL);
        obj.implicitShader = gl::Shader::fromSource(gl::shaders::kImplicitVert, fragSrc);
    } else {
        obj.colorByCurvature = formulas.colorByCurvature;
        obj.curvatureHeatmap = formulas.curvatureHeatmap;
        obj.gpuMesh = gl::GPUMesh::upload(built.mesh);
        obj.scalarAbsMax = std::max({std::fabs(built.mesh.scalarMin), std::fabs(built.mesh.scalarMax), 1e-6});
        obj.diffGeo = std::move(built.diffGeo);
    }

    objects_.push_back(std::move(obj));
    return objects_.back();
}

void Scene::remove(size_t index) {
    if (index < objects_.size()) objects_.erase(objects_.begin() + static_cast<long>(index));
}

void Scene::shootGeodesic(size_t objectIndex, double u0, double v0, double dirAngleRad, double arcLength, int steps) {
    if (objectIndex >= objects_.size()) return;
    SceneObject& obj = objects_[objectIndex];
    if (!obj.diffGeo) return;
    auto path = obj.diffGeo->integrateGeodesic(u0, v0, dirAngleRad, arcLength, steps);
    geo::Polyline poly = geo::geodesicToPolyline(*obj.diffGeo, path);
    GeodesicRun run;
    run.line = gl::GPULineStrip::upload(poly);
    run.u0 = u0; run.v0 = v0; run.dirAngle = dirAngleRad; run.arcLength = arcLength;
    obj.geodesics.push_back(std::move(run));
}

void Scene::clearGeodesics(size_t objectIndex) {
    if (objectIndex < objects_.size()) objects_[objectIndex].geodesics.clear();
}

void Scene::render(const gl::Camera& cam, float aspect) const {
    glm::mat4 view = cam.viewMatrix();
    glm::mat4 proj = cam.projectionMatrix(aspect);
    glm::vec3 eye = cam.eyePosition();
    glm::mat4 model(1.0f);

    // Pass 1: solid fills, nudged back very slightly so wireframe/geodesics
    // (drawn without the offset) sit cleanly on top without z-fighting.
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0f, 1.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    meshShader_.use();
    meshShader_.setMat4("uModel", model);
    meshShader_.setMat4("uView", view);
    meshShader_.setMat4("uProj", proj);
    meshShader_.setVec3("uCameraPos", eye);
    meshShader_.setInt("uFlatColorMode", 0);
    for (auto& obj : objects_) {
        if (!obj.visible || obj.kind == geo::SurfaceKind::Implicit) continue;
        meshShader_.setVec3("uBaseColor", obj.baseColor);
        int colorMode = !obj.colorByCurvature ? 0 : (obj.curvatureHeatmap ? 2 : 1);
        meshShader_.setInt("uColorMode", colorMode);
        meshShader_.setFloat("uScalarAbsMax", static_cast<float>(obj.scalarAbsMax));
        obj.gpuMesh.draw();
    }
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Pass 2: wireframe overlays.
    meshShader_.setInt("uFlatColorMode", 1);
    meshShader_.setVec3("uFlatColor", glm::vec3(0.05f, 0.05f, 0.08f));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    for (auto& obj : objects_) {
        if (!obj.visible || !obj.wireframe || obj.kind == geo::SurfaceKind::Implicit) continue;
        obj.gpuMesh.draw();
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Pass 3: reference grid + geodesics.
    lineShader_.use();
    lineShader_.setMat4("uModel", model);
    lineShader_.setMat4("uView", view);
    lineShader_.setMat4("uProj", proj);
    glLineWidth(2.0f);
    if (showGrid) {
        lineShader_.setVec3("uColor", glm::vec3(0.35f, 0.35f, 0.38f));
        glLineWidth(1.0f);
        gridLines_.draw(GL_LINES);
        glLineWidth(2.0f);
    }
    for (auto& obj : objects_) {
        if (!obj.visible) continue;
        lineShader_.setVec3("uColor", obj.geodesicColor);
        for (auto& g : obj.geodesics) g.line.draw();
    }

    // Pass 4: raymarched implicit surfaces. Front faces are culled so the
    // cube's BACK faces rasterize (giving a fragment to march from even when
    // the camera sits outside the box); each writes its own gl_FragDepth so
    // depth-compositing with passes 1-3 is correct regardless of draw order.
    glCullFace(GL_FRONT);
    for (auto& obj : objects_) {
        if (!obj.visible || obj.kind != geo::SurfaceKind::Implicit) continue;
        obj.implicitShader.use();
        obj.implicitShader.setMat4("uModel", model);
        obj.implicitShader.setMat4("uView", view);
        obj.implicitShader.setMat4("uProj", proj);
        obj.implicitShader.setVec3("uCameraPos", eye);
        obj.implicitShader.setVec3("uBoxMin", obj.implicitBoxMin);
        obj.implicitShader.setVec3("uBoxMax", obj.implicitBoxMax);
        obj.implicitShader.setVec3("uBaseColor", obj.baseColor);
        obj.implicitBoundingCube.draw();
    }
    glCullFace(GL_BACK);
}

} // namespace scene
