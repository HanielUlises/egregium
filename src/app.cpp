#include "app.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <stb_image_write.h>

#include "geometry/builtin_demos.hpp"
#include "geometry/surface_spec.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>
#include <cstdio>

App::App(const Options& opts) : opts_(opts) {
    initWindow();
    initImGui();
    camera_.reset();
    scene_ = new scene::Scene();
}

void App::loadShot(const Shot& shot) {
    scene_->clear();
    auto demos = geo::builtinDemos();
    const geo::DemoEntry* found = nullptr;
    for (auto& d : demos)
        if (d.name == shot.demoName) { found = &d; break; }
    if (!found) throw std::runtime_error("no such demo: " + shot.demoName);
    geo::BuildResult built = geo::buildSurface(found->formulas);
    if (!built.ok) throw std::runtime_error("failed to build demo '" + shot.demoName + "': " + built.error);
    glm::vec3 color(found->formulas.colorR, found->formulas.colorG, found->formulas.colorB);
    scene::SceneObject& obj = scene_->addFromBuildResult(found->name, found->formulas, std::move(built), color);
    if (!shot.colormap.empty()) {
        obj.colorByCurvature = true;
        obj.curvatureHeatmap = (shot.colormap == "heatmap");
    }
    camera_.setView(shot.yawDeg, shot.pitchDeg, shot.distance);
}

App::~App() {
    delete scene_;
    shutdownImGui();
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

void App::initWindow() {
    if (!glfwInit()) throw std::runtime_error("glfwInit failed");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(opts_.width, opts_.height, opts_.title.c_str(), nullptr, nullptr);
    if (!window_) { glfwTerminate(); throw std::runtime_error("glfwCreateWindow failed"); }
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    GLenum glewStatus = glewInit();
    if (glewStatus != GLEW_OK)
        throw std::runtime_error(std::string("glewInit failed: ") +
                                  reinterpret_cast<const char*>(glewGetErrorString(glewStatus)));
    glGetError(); // glewInit() reliably leaves a spurious GL_INVALID_ENUM on core profiles; clear it.

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    glfwSetWindowUserPointer(window_, this);
    glfwSetCursorPosCallback(window_, &App::cursorPosCallback);
    glfwSetMouseButtonCallback(window_, &App::mouseButtonCallback);
    glfwSetScrollCallback(window_, &App::scrollCallback);
}

void App::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 410");
}

void App::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void App::framebufferSize(int& w, int& h) const { glfwGetFramebufferSize(window_, &w, &h); }

void App::captureScreenshot(const std::string& path, int scale) {
    int baseW, baseH;
    framebufferSize(baseW, baseH);
    int w = baseW * scale, h = baseH * scale;

    GLuint fbo = 0, colorTex = 0, depthRb = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &colorTex);
    glBindTexture(GL_TEXTURE_2D, colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex, 0);

    glGenRenderbuffers(1, &depthRb);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRb);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRb);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("screenshot FBO incomplete");

    glViewport(0, 0, w, h);
    glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    scene_->render(camera_, static_cast<float>(w) / static_cast<float>(h));

    std::vector<unsigned char> pixels(static_cast<size_t>(w) * static_cast<size_t>(h) * 3);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

    // glReadPixels' origin is bottom-left; PNG expects top-left, so flip rows.
    std::vector<unsigned char> flipped(pixels.size());
    size_t rowBytes = static_cast<size_t>(w) * 3;
    for (int row = 0; row < h; ++row)
        std::copy_n(pixels.data() + static_cast<size_t>(row) * rowBytes, rowBytes,
                    flipped.data() + static_cast<size_t>(h - 1 - row) * rowBytes);

    stbi_write_png(path.c_str(), w, h, 3, flipped.data(), static_cast<int>(rowBytes));

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &depthRb);
    glDeleteTextures(1, &colorTex);
    glDeleteFramebuffers(1, &fbo);
    int restoreW, restoreH;
    framebufferSize(restoreW, restoreH);
    glViewport(0, 0, restoreW, restoreH);
}

void App::run() {
    bool batchMode = !opts_.shots.empty();
    size_t shotIndex = 0;
    int frameInShot = 0;
    if (batchMode) loadShot(opts_.shots[0]);

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        int w, h;
        framebufferSize(w, h);
        if (w == 0 || h == 0) continue; // minimized
        glViewport(0, 0, w, h);
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        scene_->render(camera_, static_cast<float>(w) / static_cast<float>(h));

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ui::draw(*scene_, camera_, uiState_);
        if (!uiState_.screenshotRequest.empty()) {
            captureScreenshot(uiState_.screenshotRequest, uiState_.screenshotScale);
            uiState_.screenshotStatusMsg = "Saved " + uiState_.screenshotRequest;
            uiState_.screenshotRequest.clear();
        }
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);

        if (batchMode && ++frameInShot >= opts_.framesPerShot) {
            const Shot& shot = opts_.shots[shotIndex];
            captureScreenshot(shot.outputPath, 1);
            std::printf("wrote %s (demo=%s)\n", shot.outputPath.c_str(), shot.demoName.c_str());
            ++shotIndex;
            frameInShot = 0;
            if (shotIndex >= opts_.shots.size()) {
                glfwSetWindowShouldClose(window_, GLFW_TRUE);
            } else {
                loadShot(opts_.shots[shotIndex]);
            }
        }
    }
}

void App::cursorPosCallback(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<App*>(glfwGetWindowUserPointer(w));
    double dx = x - self->lastMouseX_, dy = y - self->lastMouseY_;
    self->lastMouseX_ = x;
    self->lastMouseY_ = y;
    if (ImGui::GetIO().WantCaptureMouse) return;
    if (self->orbiting_) self->camera_.orbit(static_cast<float>(dx) * 0.3f, static_cast<float>(-dy) * 0.3f);
    if (self->panning_) {
        int fw, fh;
        self->framebufferSize(fw, fh);
        self->camera_.pan(static_cast<float>(dx) / static_cast<float>(fw),
                           static_cast<float>(dy) / static_cast<float>(fh));
    }
}

void App::mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
    (void)mods;
    auto* self = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (ImGui::GetIO().WantCaptureMouse && action == GLFW_PRESS) return;
    bool down = (action == GLFW_PRESS);
    if (button == GLFW_MOUSE_BUTTON_LEFT) self->orbiting_ = down;
    if (button == GLFW_MOUSE_BUTTON_RIGHT) self->panning_ = down;
}

void App::scrollCallback(GLFWwindow* w, double xoff, double yoff) {
    (void)xoff;
    auto* self = static_cast<App*>(glfwGetWindowUserPointer(w));
    if (ImGui::GetIO().WantCaptureMouse) return;
    self->camera_.zoom(static_cast<float>(yoff));
}
