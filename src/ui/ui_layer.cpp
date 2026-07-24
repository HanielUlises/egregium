#include "ui/ui_layer.hpp"
#include "geometry/builtin_demos.hpp"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <cmath>
#include <vector>

namespace ui {

namespace {

constexpr double kPi = 3.14159265358979323846;
const char* kKindNames[4] = {
    "Explicit   z = f(x,y)",
    "Parametric   (u,v) -> (x,y,z)",
    "Implicit   F(x,y,z) = 0",
    "Riemannian metric (advanced)",
};

void applyKindDefaults(geo::SurfaceFormulas& f, int kindIndex) {
    switch (kindIndex) {
        case 0:
            f.kind = geo::SurfaceKind::Explicit;
            f.heightFormula = "sin(x)*cos(y)";
            f.xMin = -3; f.xMax = 3; f.yMin = -3; f.yMax = 3;
            break;
        case 1:
            f.kind = geo::SurfaceKind::Parametric;
            f.xFormula = "u"; f.yFormula = "v"; f.zFormula = "sin(u)*cos(v)";
            f.uMin = -3; f.uMax = 3; f.vMin = -3; f.vMax = 3;
            f.periodicU = false; f.periodicV = false;
            break;
        case 2:
            f.kind = geo::SurfaceKind::Implicit;
            f.implicitFormula = "x^2+y^2+z^2-1";
            f.bboxMin = -2; f.bboxMax = 2;
            break;
        case 3:
            f.kind = geo::SurfaceKind::Metric;
            f.metricE = "1"; f.metricF = "0"; f.metricG = "1";
            f.metricHasEmbedding = false;
            f.uMin = 0; f.uMax = 1; f.vMin = 0; f.vMax = 1;
            break;
        default:
            break;
    }
}

void drawDemoMenu(scene::Scene& sc) {
    static std::vector<geo::DemoEntry> demos = geo::builtinDemos();
    if (ImGui::BeginMenu("Demos")) {
        std::string lastCategory;
        for (auto& d : demos) {
            if (d.category != lastCategory) {
                if (!lastCategory.empty()) ImGui::Separator();
                ImGui::TextDisabled("%s", d.category.c_str());
                lastCategory = d.category;
            }
            if (ImGui::MenuItem(d.name.c_str())) {
                geo::BuildResult built = geo::buildSurface(d.formulas);
                if (built.ok) {
                    glm::vec3 color(d.formulas.colorR, d.formulas.colorG, d.formulas.colorB);
                    sc.addFromBuildResult(d.name, d.formulas, std::move(built), color);
                }
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", d.description.c_str());
        }
        ImGui::EndMenu();
    }
}

void drawMainMenuBar(scene::Scene& sc, gl::Camera& cam, UIState& state) {
    if (ImGui::BeginMainMenuBar()) {
        drawDemoMenu(sc);
        if (ImGui::BeginMenu("Add")) {
            if (ImGui::MenuItem("New surface...")) {
                state.showAddDialog = true;
                applyKindDefaults(state.draft, state.draftKindIndex);
                state.draftError.clear();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::Checkbox("Show grid", &sc.showGrid);
            if (ImGui::MenuItem("Reset camera")) cam.reset();
            ImGui::Separator();
            if (ImGui::MenuItem("Save Screenshot...")) {
                state.showScreenshotDialog = true;
                state.screenshotStatusMsg.clear();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("Controls")) state.showHelp = true;
            if (ImGui::MenuItem("About")) state.showAbout = true;
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void drawAddSurfaceDialog(scene::Scene& sc, UIState& state) {
    if (!state.showAddDialog) return;
    ImGui::SetNextWindowSize(ImVec2(460, 0), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Add Surface", &state.showAddDialog)) {
        ImGui::End();
        return;
    }

    ImGui::InputText("Name", &state.draftName);

    int kindIdx = state.draftKindIndex;
    if (ImGui::Combo("Type", &kindIdx, kKindNames, 4)) {
        state.draftKindIndex = kindIdx;
        applyKindDefaults(state.draft, kindIdx);
        state.draftError.clear();
    }
    ImGui::Separator();

    switch (state.draft.kind) {
        case geo::SurfaceKind::Explicit:
            ImGui::InputText("f(x,y)  [z=]", &state.draft.heightFormula);
            ImGui::InputDouble("x min", &state.draft.xMin); ImGui::SameLine(); ImGui::InputDouble("x max", &state.draft.xMax);
            ImGui::InputDouble("y min", &state.draft.yMin); ImGui::SameLine(); ImGui::InputDouble("y max", &state.draft.yMax);
            break;
        case geo::SurfaceKind::Parametric:
            ImGui::InputText("x(u,v)", &state.draft.xFormula);
            ImGui::InputText("y(u,v)", &state.draft.yFormula);
            ImGui::InputText("z(u,v)", &state.draft.zFormula);
            ImGui::InputDouble("u min", &state.draft.uMin); ImGui::SameLine(); ImGui::InputDouble("u max", &state.draft.uMax);
            ImGui::InputDouble("v min", &state.draft.vMin); ImGui::SameLine(); ImGui::InputDouble("v max", &state.draft.vMax);
            ImGui::Checkbox("periodic in u", &state.draft.periodicU);
            ImGui::SameLine();
            ImGui::Checkbox("periodic in v", &state.draft.periodicV);
            ImGui::Checkbox("color by curvature", &state.draft.colorByCurvature);
            break;
        case geo::SurfaceKind::Implicit:
            ImGui::InputText("F(x,y,z)  [=0]", &state.draft.implicitFormula);
            ImGui::TextDisabled("Raymarched on the GPU -- min()/max() are fine here (metaballs, etc).");
            ImGui::InputDouble("box min", &state.draft.bboxMin); ImGui::SameLine(); ImGui::InputDouble("box max", &state.draft.bboxMax);
            break;
        case geo::SurfaceKind::Metric:
            ImGui::InputText("E(u,v)", &state.draft.metricE);
            ImGui::InputText("F(u,v)", &state.draft.metricF);
            ImGui::InputText("G(u,v)", &state.draft.metricG);
            ImGui::Checkbox("provide explicit embedding (optional)", &state.draft.metricHasEmbedding);
            if (state.draft.metricHasEmbedding) {
                ImGui::InputText("x(u,v)", &state.draft.embedXFormula);
                ImGui::InputText("y(u,v)", &state.draft.embedYFormula);
                ImGui::InputText("z(u,v)", &state.draft.embedZFormula);
            } else {
                ImGui::TextDisabled("No embedding: rendered as a flat (u,v,0) model (like the Poincare disk).");
            }
            ImGui::InputDouble("u min", &state.draft.uMin); ImGui::SameLine(); ImGui::InputDouble("u max", &state.draft.uMax);
            ImGui::InputDouble("v min", &state.draft.vMin); ImGui::SameLine(); ImGui::InputDouble("v max", &state.draft.vMax);
            ImGui::Checkbox("periodic in u", &state.draft.periodicU);
            ImGui::SameLine();
            ImGui::Checkbox("periodic in v", &state.draft.periodicV);
            ImGui::Checkbox("color by curvature", &state.draft.colorByCurvature);
            break;
    }

    if (state.draft.kind != geo::SurfaceKind::Implicit) {
        ImGui::SliderInt("resolution (u)", &state.draft.resolutionU, 4, 200);
        ImGui::SliderInt("resolution (v)", &state.draft.resolutionV, 4, 200);
    }

    float col[3] = {state.draft.colorR, state.draft.colorG, state.draft.colorB};
    if (ImGui::ColorEdit3("color", col)) {
        state.draft.colorR = col[0]; state.draft.colorG = col[1]; state.draft.colorB = col[2];
    }

    if (!state.draftError.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.35f, 0.35f, 1.0f));
        ImGui::TextWrapped("%s", state.draftError.c_str());
        ImGui::PopStyleColor();
    }

    if (ImGui::Button("Create", ImVec2(120, 0))) {
        geo::BuildResult built = geo::buildSurface(state.draft);
        if (!built.ok) {
            state.draftError = built.error;
        } else {
            try {
                glm::vec3 color(state.draft.colorR, state.draft.colorG, state.draft.colorB);
                sc.addFromBuildResult(state.draftName, state.draft, std::move(built), color);
                state.draftError.clear();
                state.showAddDialog = false;
            } catch (const std::exception& ex) {
                state.draftError = std::string("shader compile error: ") + ex.what();
            }
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) state.showAddDialog = false;

    ImGui::End();
}

void drawOutlinerAndInspector(scene::Scene& sc, UIState& state) {
    ImGui::SetNextWindowSize(ImVec2(360, 460), ImGuiCond_FirstUseEver);
    ImGui::Begin("Scene");

    auto& objs = sc.objects();
    ImGui::Text("%zu object(s)", objs.size());
    ImGui::Separator();

    int toRemove = -1;
    for (int i = 0; i < static_cast<int>(objs.size()); ++i) {
        ImGui::PushID(i);
        bool selected = (state.selectedObject == i);
        if (ImGui::Selectable(objs[static_cast<size_t>(i)].name.c_str(), selected, 0, ImVec2(180, 0))) {
            state.selectedObject = i;
        }
        ImGui::SameLine();
        ImGui::Checkbox("show", &objs[static_cast<size_t>(i)].visible);
        ImGui::SameLine();
        if (ImGui::SmallButton("remove")) toRemove = i;
        ImGui::PopID();
    }
    if (toRemove >= 0) {
        sc.remove(static_cast<size_t>(toRemove));
        if (state.selectedObject == toRemove) state.selectedObject = -1;
        else if (state.selectedObject > toRemove) state.selectedObject--;
    }

    ImGui::Separator();
    if (state.selectedObject >= 0 && state.selectedObject < static_cast<int>(objs.size())) {
        scene::SceneObject& obj = objs[static_cast<size_t>(state.selectedObject)];
        ImGui::TextColored(ImVec4(0.80f, 0.85f, 1.0f, 1.0f), "%s", obj.name.c_str());

        float col[3] = {obj.baseColor.r, obj.baseColor.g, obj.baseColor.b};
        if (ImGui::ColorEdit3("Color", col)) obj.baseColor = glm::vec3(col[0], col[1], col[2]);

        if (obj.kind != geo::SurfaceKind::Implicit) {
            ImGui::Checkbox("Wireframe", &obj.wireframe);
            ImGui::Checkbox("Color by curvature", &obj.colorByCurvature);

            if (obj.diffGeo) {
                ImGui::Separator();
                ImGui::TextUnformatted("Geodesics (exact RK4 integration of the geodesic ODE)");
                ImGui::SliderFloat("u0", &state.geoU0, static_cast<float>(obj.formulas.uMin), static_cast<float>(obj.formulas.uMax));
                ImGui::SliderFloat("v0", &state.geoV0, static_cast<float>(obj.formulas.vMin), static_cast<float>(obj.formulas.vMax));
                ImGui::SliderFloat("direction (deg)", &state.geoAngleDeg, 0.0f, 360.0f);
                ImGui::SliderFloat("arc length", &state.geoArcLength, 0.05f, 10.0f);
                if (ImGui::Button("Shoot geodesic")) {
                    sc.shootGeodesic(static_cast<size_t>(state.selectedObject), state.geoU0, state.geoV0,
                                      state.geoAngleDeg * kPi / 180.0, state.geoArcLength, state.geoSteps);
                }
                ImGui::SameLine();
                if (ImGui::Button("Clear geodesics")) sc.clearGeodesics(static_cast<size_t>(state.selectedObject));

                double k = obj.diffGeo->gaussianCurvature(state.geoU0, state.geoV0);
                if (std::isfinite(k)) ImGui::Text("Gaussian curvature K at (u0,v0) = %.4f", k);
                else ImGui::TextDisabled("K at (u0,v0): coordinate singularity here");
            }
        } else {
            ImGui::TextDisabled("Implicit surfaces are raymarched -- no mesh, curvature, or geodesics.");
        }
    } else {
        ImGui::TextDisabled("Select an object above to inspect it.");
    }

    ImGui::End();
}

void drawHelpWindow(UIState& state) {
    if (!state.showHelp) return;
    ImGui::Begin("Controls", &state.showHelp, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::BulletText("Left-drag: orbit camera");
    ImGui::BulletText("Right-drag: pan camera");
    ImGui::BulletText("Scroll wheel: zoom");
    ImGui::BulletText("Demos menu: load a built-in example");
    ImGui::BulletText("Add menu: define your own surface from a formula");
    ImGui::BulletText("Scene panel: select an object to inspect it or shoot a geodesic");
    ImGui::End();
}

void drawAboutWindow(UIState& state) {
    if (!state.showAbout) return;
    ImGui::Begin("About", &state.showAbout, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::TextWrapped(
        "A GeoGebra-like 3D grapher that also supports manifolds: arbitrary "
        "Riemannian metrics, exact symbolic curvature (Brioschi formula), and "
        "geodesics via RK4 integration of the geodesic ODE -- all driven by a "
        "small formula parser with exact symbolic differentiation, not finite "
        "differences.");
    ImGui::End();
}

void drawScreenshotDialog(UIState& state) {
    if (!state.showScreenshotDialog) return;
    ImGui::SetNextWindowSize(ImVec2(360, 0), ImGuiCond_FirstUseEver);
    ImGui::Begin("Save Screenshot", &state.showScreenshotDialog, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::InputText("Filename", &state.screenshotFilename);
    const char* scaleLabels[3] = {"1x (window size)", "2x (supersampled)", "4x (supersampled)"};
    int scaleIdx = state.screenshotScale == 4 ? 2 : (state.screenshotScale == 2 ? 1 : 0);
    if (ImGui::Combo("Resolution", &scaleIdx, scaleLabels, 3)) {
        state.screenshotScale = (scaleIdx == 2) ? 4 : (scaleIdx == 1 ? 2 : 1);
    }
    ImGui::TextDisabled("Captured before the UI is drawn, so the exported image is a clean render.");
    if (ImGui::Button("Save", ImVec2(120, 0))) {
        state.screenshotRequest = state.screenshotFilename;
        state.showScreenshotDialog = false;
    }
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 0))) state.showScreenshotDialog = false;
    if (!state.screenshotStatusMsg.empty()) {
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.6f, 0.9f, 0.6f, 1.0f), "%s", state.screenshotStatusMsg.c_str());
    }
    ImGui::End();
}

} // namespace

void draw(scene::Scene& sc, gl::Camera& cam, UIState& state) {
    drawMainMenuBar(sc, cam, state);
    drawAddSurfaceDialog(sc, state);
    drawOutlinerAndInspector(sc, state);
    drawHelpWindow(state);
    drawAboutWindow(state);
    drawScreenshotDialog(state);
}

} // namespace ui
