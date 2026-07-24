cmake_minimum_required(VERSION 3.16)
project(manifold_graph CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)

# ---- OpenGL ----------------------------------------------------------------
find_package(OpenGL REQUIRED)

# ---- GLFW: try system package first, else fetch+build from source ----------
find_package(glfw3 3.3 QUIET)
if(NOT TARGET glfw)
  message(STATUS "system glfw3 not found -- fetching from GitHub")
  set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glfw)
endif()

# ---- GLM: header-only, always trivial to fetch if not installed ------------
find_package(glm QUIET)
if(NOT TARGET glm::glm)
  message(STATUS "system glm not found -- fetching from GitHub")
  FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.1 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(glm)
  if(NOT TARGET glm::glm AND TARGET glm)
    add_library(glm::glm ALIAS glm)
  endif()
endif()

# ---- GLEW: system package only. Its build system doesn't FetchContent
# reliably enough to auto-build here, so this fails fast with clear next steps
# rather than silently doing something fragile.
find_package(GLEW QUIET)
if(NOT TARGET GLEW::GLEW)
  message(FATAL_ERROR
    "GLEW not found. Install it and re-run cmake:\n"
    "  Ubuntu/Debian : sudo apt-get install libglew-dev\n"
    "  Fedora        : sudo dnf install glew-devel\n"
    "  Arch          : sudo pacman -S glew\n"
    "  macOS         : brew install glew\n"
    "  Windows       : vcpkg install glew")
endif()

# ---- Dear ImGui (vendored source in external/imgui, no CMake of its own) ---
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
if(NOT EXISTS ${IMGUI_DIR}/imgui.h)
  message(FATAL_ERROR "external/imgui is missing. Clone it:\n"
    "  git clone --depth 1 --branch v1.91.0 https://github.com/ocornut/imgui.git external/imgui")
endif()
add_library(imgui STATIC
  ${IMGUI_DIR}/imgui.cpp
  ${IMGUI_DIR}/imgui_draw.cpp
  ${IMGUI_DIR}/imgui_widgets.cpp
  ${IMGUI_DIR}/imgui_tables.cpp
  ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
  ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
  ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ${IMGUI_DIR}/misc/cpp)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# ---- stb_image_write (single-header, vendored in external/stb) -------------
if(NOT EXISTS ${CMAKE_SOURCE_DIR}/external/stb/stb_image_write.h)
  message(FATAL_ERROR "external/stb/stb_image_write.h is missing. Fetch it:\n"
    "  curl -fsSL -o external/stb/stb_image_write.h "
    "https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h")
endif()

# ---- core math/geometry library (no GL dependency -- reused by tests) ------
add_library(mathgraph_core STATIC
  src/expr/ast.cpp
  src/expr/parser.cpp
  src/expr/interpreter.cpp
  src/expr/differentiate.cpp
  src/expr/glsl_codegen.cpp
  src/geometry/differential_geometry.cpp
  src/geometry/mesh_tessellate.cpp
  src/geometry/surface_spec.cpp
  src/geometry/builtin_demos.cpp
  src/geometry/implicit_surface.cpp
)
target_include_directories(mathgraph_core PUBLIC src)

# ---- main executable --------------------------------------------------------
add_executable(manifold_graph
  src/main.cpp
  src/app.cpp
  src/stb_impl.cpp
  src/gl/shader.cpp
  src/gl/gpu_mesh.cpp
  src/gl/camera.cpp
  src/scene/scene.cpp
  src/ui/ui_layer.cpp
)
target_include_directories(manifold_graph PRIVATE src external/stb)
target_link_libraries(manifold_graph PRIVATE mathgraph_core imgui glfw GLEW::GLEW OpenGL::GL glm::glm)

if(UNIX AND NOT APPLE)
  find_package(Threads REQUIRED)
  target_link_libraries(manifold_graph PRIVATE Threads::Threads dl)
endif()

target_compile_options(manifold_graph PRIVATE -Wall -Wextra)
target_compile_options(mathgraph_core PRIVATE -Wall -Wextra)

# ---- tests (pure-math, no GL/window deps) -----------------------------------
enable_testing()
foreach(t expr_test diffgeo_test builtin_demos_test)
  add_executable(${t} tests/${t}.cpp)
  target_link_libraries(${t} PRIVATE mathgraph_core)
  target_include_directories(${t} PRIVATE src)
  add_test(NAME ${t} COMMAND ${t})
endforeach()
