#include "gl/gpu_mesh.hpp"
#include <cstddef>

namespace gl {

GPUMesh GPUMesh::upload(const geo::Mesh& mesh) {
    GPUMesh g;
    glGenVertexArrays(1, &g.vao_);
    glGenBuffers(1, &g.vbo_);
    glGenBuffers(1, &g.ebo_);

    glBindVertexArray(g.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, g.vbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.vertices.size() * sizeof(geo::Vertex)),
                 mesh.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g.ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(mesh.indices.size() * sizeof(uint32_t)),
                 mesh.indices.data(), GL_STATIC_DRAW);

    // layout(location=0) vec3 position, location=1 vec3 normal, location=2 float scalar
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(geo::Vertex), reinterpret_cast<void*>(offsetof(geo::Vertex, px)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(geo::Vertex), reinterpret_cast<void*>(offsetof(geo::Vertex, nx)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(geo::Vertex), reinterpret_cast<void*>(offsetof(geo::Vertex, scalar)));

    glBindVertexArray(0);
    g.indexCount_ = static_cast<GLsizei>(mesh.indices.size());
    return g;
}

void GPUMesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void GPUMesh::destroy() {
    if (ebo_) glDeleteBuffers(1, &ebo_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = ebo_ = 0;
    indexCount_ = 0;
}
GPUMesh::~GPUMesh() { destroy(); }
GPUMesh::GPUMesh(GPUMesh&& o) noexcept : vao_(o.vao_), vbo_(o.vbo_), ebo_(o.ebo_), indexCount_(o.indexCount_) {
    o.vao_ = o.vbo_ = o.ebo_ = 0;
    o.indexCount_ = 0;
}
GPUMesh& GPUMesh::operator=(GPUMesh&& o) noexcept {
    if (this != &o) {
        destroy();
        vao_ = o.vao_; vbo_ = o.vbo_; ebo_ = o.ebo_; indexCount_ = o.indexCount_;
        o.vao_ = o.vbo_ = o.ebo_ = 0;
        o.indexCount_ = 0;
    }
    return *this;
}

// --- GPULineStrip ---------------------------------------------------------

GPULineStrip GPULineStrip::upload(const geo::Polyline& poly) {
    GPULineStrip g;
    glGenVertexArrays(1, &g.vao_);
    glGenBuffers(1, &g.vbo_);
    glBindVertexArray(g.vao_);
    glBindBuffer(GL_ARRAY_BUFFER, g.vbo_);

    std::vector<float> flat;
    flat.reserve(poly.points.size() * 3);
    for (auto& p : poly.points) {
        flat.push_back(static_cast<float>(p.x));
        flat.push_back(static_cast<float>(p.y));
        flat.push_back(static_cast<float>(p.z));
    }
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(flat.size() * sizeof(float)), flat.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glBindVertexArray(0);

    g.count_ = static_cast<GLsizei>(poly.points.size());
    return g;
}

void GPULineStrip::draw(GLenum mode) const {
    glBindVertexArray(vao_);
    glDrawArrays(mode, 0, count_);
    glBindVertexArray(0);
}

void GPULineStrip::destroy() {
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    vao_ = vbo_ = 0;
    count_ = 0;
}
GPULineStrip::~GPULineStrip() { destroy(); }
GPULineStrip::GPULineStrip(GPULineStrip&& o) noexcept : vao_(o.vao_), vbo_(o.vbo_), count_(o.count_) {
    o.vao_ = o.vbo_ = 0;
    o.count_ = 0;
}
GPULineStrip& GPULineStrip::operator=(GPULineStrip&& o) noexcept {
    if (this != &o) {
        destroy();
        vao_ = o.vao_; vbo_ = o.vbo_; count_ = o.count_;
        o.vao_ = o.vbo_ = 0;
        o.count_ = 0;
    }
    return *this;
}

} // namespace gl
