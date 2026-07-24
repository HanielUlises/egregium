#pragma once
#include <GL/glew.h>
#include "geometry/mesh_types.hpp"

namespace gl {

// Uploads a geo::Mesh (position + normal + scalar per vertex, triangle
// indices) to a VAO/VBO/EBO. Move-only.
class GPUMesh {
public:
    GPUMesh() = default;
    ~GPUMesh();
    GPUMesh(GPUMesh&& o) noexcept;
    GPUMesh& operator=(GPUMesh&& o) noexcept;
    GPUMesh(const GPUMesh&) = delete;
    GPUMesh& operator=(const GPUMesh&) = delete;

    static GPUMesh upload(const geo::Mesh& mesh);
    void draw() const;
    GLsizei indexCount() const { return indexCount_; }
    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0, vbo_ = 0, ebo_ = 0;
    GLsizei indexCount_ = 0;
    void destroy();
};

// Uploads a geo::Polyline (rendered geodesic) as a GL_LINE_STRIP.
class GPULineStrip {
public:
    GPULineStrip() = default;
    ~GPULineStrip();
    GPULineStrip(GPULineStrip&& o) noexcept;
    GPULineStrip& operator=(GPULineStrip&& o) noexcept;
    GPULineStrip(const GPULineStrip&) = delete;
    GPULineStrip& operator=(const GPULineStrip&) = delete;

    static GPULineStrip upload(const geo::Polyline& poly);
    void draw(GLenum mode = GL_LINE_STRIP) const;
    bool valid() const { return vao_ != 0; }

private:
    GLuint vao_ = 0, vbo_ = 0;
    GLsizei count_ = 0;
    void destroy();
};

} // namespace gl
