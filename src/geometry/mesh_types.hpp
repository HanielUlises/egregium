#pragma once
#include <cmath>
#include <cstdint>
#include <vector>

namespace geo {

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    double dot(const Vec3& o) const { return x * o.x + y * o.y + z * o.z; }
    Vec3 cross(const Vec3& o) const { return {y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x}; }
    double length() const { return std::sqrt(dot(*this)); }
    Vec3 normalized() const {
        double L = length();
        if (L < 1e-15) return {0, 0, 1};
        return {x / L, y / L, z / L};
    }
};

// GPU-ready vertex: float precision by design (this is the boundary where we
// drop from the double-precision differential-geometry computations down to
// what actually gets uploaded to a VBO).
struct Vertex {
    float px, py, pz;    // position
    float nx, ny, nz;    // unit normal
    float scalar;        // colormap driver (Gaussian curvature, or 0 if unused)
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices; // triangle list
    double scalarMin = 0.0, scalarMax = 0.0; // range of `scalar` across the mesh, for colormap normalization
};

// A polyline in 3D (used for rendered geodesics).
struct Polyline {
    std::vector<Vec3> points;
};

} // namespace geo
