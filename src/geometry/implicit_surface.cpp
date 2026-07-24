#include "geometry/implicit_surface.hpp"

namespace geo {

Mesh makeBoundingCube(double lo, double hi) {
    Mesh mesh;
    float l = static_cast<float>(lo), h = static_cast<float>(hi);
    // 8 corners; normals are unused by the raymarch shader, left at 0.
    float corners[8][3] = {
        {l, l, l}, {h, l, l}, {h, h, l}, {l, h, l},
        {l, l, h}, {h, l, h}, {h, h, h}, {l, h, h},
    };
    for (auto& c : corners) {
        Vertex v{};
        v.px = c[0]; v.py = c[1]; v.pz = c[2];
        v.nx = v.ny = v.nz = 0.0f;
        v.scalar = 0.0f;
        mesh.vertices.push_back(v);
    }
    // 12 triangles, 2 per face, wound so the OUTWARD face is front-facing.
    uint32_t idx[36] = {
        0, 2, 1, 0, 3, 2,   // bottom (z=l), viewed from -z
        4, 5, 6, 4, 6, 7,   // top (z=h), viewed from +z
        0, 1, 5, 0, 5, 4,   // front (y=l)
        3, 7, 6, 3, 6, 2,   // back (y=h)
        0, 4, 7, 0, 7, 3,   // left (x=l)
        1, 2, 6, 1, 6, 5,   // right (x=h)
    };
    mesh.indices.assign(idx, idx + 36);
    return mesh;
}

} // namespace geo
