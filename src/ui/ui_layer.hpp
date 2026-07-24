#pragma once
#include "gl/camera.hpp"
#include "geometry/surface_spec.hpp"
#include "scene/scene.hpp"
#include <string>

namespace ui {

struct UIState {
    // Add-surface dialog
    bool showAddDialog = false;
    int draftKindIndex = 1; // 0=Explicit,1=Parametric,2=Implicit,3=Metric (matches geo::SurfaceKind)
    geo::SurfaceFormulas draft;
    std::string draftError;
    std::string draftName = "My Surface";

    // Outliner / inspector
    int selectedObject = -1;

    // Geodesic-shooting controls (apply to the selected object)
    float geoU0 = 0.3f, geoV0 = 0.3f, geoAngleDeg = 0.0f, geoArcLength = 2.0f;
    int geoSteps = 250;

    bool showHelp = false;
    bool showAbout = false;

    // Screenshot export (clean render, no ImGui overlay, optional supersampling)
    bool showScreenshotDialog = false;
    std::string screenshotFilename = "manifold_view.png";
    int screenshotScale = 2; // 1x/2x/4x multiplier on the current window size
    std::string screenshotRequest;  // set by the UI, consumed by App on the next frame
    std::string screenshotStatusMsg;
};

// Draws the full UI for one frame (main menu bar, demo gallery, add-surface
// dialog, outliner+inspector window). May mutate `scene` (add/remove
// objects, shoot geodesics) and `camera` (reset).
void draw(scene::Scene& scene, gl::Camera& camera, UIState& state);

} // namespace ui
