#pragma once
#include "gl/camera.hpp"
#include "gl/shader.hpp"
#include "geometry/surface_spec.hpp"
#include "scene/scene_object.hpp"
#include <vector>

namespace scene {

class Scene {
public:
    // Compiles the shared mesh/line shaders. Requires a current GL context.
    Scene();

    // Builds GPU resources from an already-successful geo::BuildResult and
    // appends a new SceneObject. Throws std::runtime_error if GLSL
    // compilation fails for an Implicit surface (rare -- a codegen bug,
    // not a user formula mistake, which would already have been caught by
    // geo::buildSurface itself).
    SceneObject& addFromBuildResult(const std::string& name, const geo::SurfaceFormulas& formulas,
                                     geo::BuildResult&& built, glm::vec3 color);

    void remove(size_t index);
    void clear() { objects_.clear(); }
    std::vector<SceneObject>& objects() { return objects_; }
    const std::vector<SceneObject>& objects() const { return objects_; }

    // Integrates a geodesic on objects_[objectIndex] (must have a diffGeo)
    // starting at (u0,v0) and appends it to that object's geodesics list.
    void shootGeodesic(size_t objectIndex, double u0, double v0, double dirAngleRad, double arcLength, int steps);
    void clearGeodesics(size_t objectIndex);

    void render(const gl::Camera& cam, float aspect) const;

    bool showGrid = true;

private:
    std::vector<SceneObject> objects_;
    gl::Shader meshShader_;
    gl::Shader lineShader_;
    gl::GPULineStrip gridLines_;
};

} // namespace scene
