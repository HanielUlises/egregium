#pragma once
#include "gl/camera.hpp"
#include "scene/scene.hpp"
#include "ui/ui_layer.hpp"
#include <string>
#include <vector>

struct GLFWwindow;

// Owns the window/GL context and drives the main loop: input -> camera,
// scene::render + ui::draw per frame, and clean (no-ImGui) screenshot export.
class App {
public:
    // One batch-capture step: load a builtin demo by name, optionally force
    // its colormap, place the camera, and save one clean screenshot.
    struct Shot {
        std::string demoName;
        std::string colormap; // "diverging", "heatmap", or "" to leave the demo's own default
        std::string outputPath;
        float yawDeg = -50.0f, pitchDeg = 22.0f, distance = 8.0f;
    };

    struct Options {
        int width = 1280, height = 800;
        std::string title = "egregium -- manifold grapher";
        // Batch screenshot automation (see main.cpp --help): all shots are
        // captured sequentially in this ONE process/window, then it exits --
        // no manual interaction and no relaunching per shot.
        std::vector<Shot> shots;
        int framesPerShot = 5; // frames rendered before each shot's capture, to let GL state settle
    };

    explicit App(const Options& opts);
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void run();

private:
    Options opts_;
    GLFWwindow* window_ = nullptr;
    gl::Camera camera_;
    scene::Scene* scene_ = nullptr; // constructed after the GL context exists
    ui::UIState uiState_;

    double lastMouseX_ = 0, lastMouseY_ = 0;
    bool orbiting_ = false, panning_ = false;

    void initWindow();
    void initImGui();
    void shutdownImGui();
    void framebufferSize(int& w, int& h) const;
    // Clears the scene and builds the given shot's demo + camera placement.
    void loadShot(const Shot& shot);
    // Renders the scene only (no ImGui overlay) into an offscreen FBO at
    // `scale`x the current framebuffer size, reads it back, and writes a PNG.
    void captureScreenshot(const std::string& path, int scale);

    static void cursorPosCallback(GLFWwindow* w, double x, double y);
    static void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods);
    static void scrollCallback(GLFWwindow* w, double xoff, double yoff);
};
