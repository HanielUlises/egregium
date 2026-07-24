#include "geometry/mesh_tessellate.hpp"
#include <cmath>
#include <limits>

namespace geo {

Mesh tessellate(const DiffGeoSurface& surf, const TessellationParams& p) {
    Mesh mesh;
    bool uWraps = p.periodicU || p.periodicUTwistV;
    int uCount = uWraps ? p.uSegments : p.uSegments + 1;
    int vCount = p.periodicV ? p.vSegments : p.vSegments + 1;
    uCount = std::max(uCount, 2);
    vCount = std::max(vCount, 2);

    mesh.vertices.resize(static_cast<size_t>(uCount) * static_cast<size_t>(vCount));
    double scalarMin = std::numeric_limits<double>::infinity();
    double scalarMax = -std::numeric_limits<double>::infinity();

    for (int i = 0; i < uCount; ++i) {
        double u = p.uMin + (p.uMax - p.uMin) * (static_cast<double>(i) / p.uSegments);
        for (int j = 0; j < vCount; ++j) {
            double v = p.vMin + (p.vMax - p.vMin) * (static_cast<double>(j) / p.vSegments);
            Vec3 pos = surf.renderPosition(u, v);
            Vec3 n = surf.renderNormal(u, v);
            double scalar = 0.0;
            if (p.colorByCurvature) {
                double k = surf.gaussianCurvature(u, v);
                // Near a coordinate singularity the Brioschi denominator can be a
                // tiny nonzero float rather than exactly 0 (e.g. sin(M_PI) != 0
                // in floating point), giving a huge FINITE value that isfinite()
                // alone won't catch. A generous magnitude cap guards against that
                // without rejecting any curvature a real demo would produce.
                if (std::isfinite(k) && std::fabs(k) < 1e6) {
                    scalar = k;
                    scalarMin = std::min(scalarMin, k);
                    scalarMax = std::max(scalarMax, k);
                } // else leave scalar at 0 and don't let a singular sample poison the range
            }
            Vertex& vert = mesh.vertices[static_cast<size_t>(i) * vCount + j];
            vert.px = static_cast<float>(pos.x);
            vert.py = static_cast<float>(pos.y);
            vert.pz = static_cast<float>(pos.z);
            vert.nx = static_cast<float>(n.x);
            vert.ny = static_cast<float>(n.y);
            vert.nz = static_cast<float>(n.z);
            vert.scalar = static_cast<float>(scalar);
        }
    }

    if (p.colorByCurvature && scalarMin <= scalarMax) {
        mesh.scalarMin = scalarMin;
        mesh.scalarMax = scalarMax;
    }

    // For a twisted wrap, row 0's OWN vertices can't be reused as the seam's
    // "next ring": renderPosition(uMax,v) == renderPosition(uMin,-v) holds,
    // but the analogous identity for the normal is renderNormal(uMax,v) ==
    // -renderNormal(uMin,-v) -- differentiating the shared-point identity
    // w.r.t. v flips the sign of Xv (and hence of Xu x Xv) under v -> -v,
    // while Xu itself is unchanged. That sign flip is real, not a bug: it's
    // the Klein bottle's non-orientability showing up directly in the
    // formulas (there is no continuous, consistent normal field to compute).
    // Reusing row 0's vertices as-is would linearly blend that flipped
    // normal against row (uCount-1)'s correctly-facing one across each seam
    // triangle, producing spiky lit/unlit interpolation garbage. Duplicating
    // row 0 here with negated normals (same positions and curvature scalar,
    // which don't have this sign ambiguity) instead gives every seam
    // triangle a consistent orientation, so the identification shows up
    // honestly as a sharp lighting crease rather than as noise.
    size_t twistRowOffset = mesh.vertices.size();
    if (p.periodicUTwistV) {
        for (int j = 0; j < vCount; ++j) {
            int jm = (vCount - j) % vCount;
            Vertex dup = mesh.vertices[static_cast<size_t>(jm)]; // row 0
            dup.nx = -dup.nx; dup.ny = -dup.ny; dup.nz = -dup.nz;
            mesh.vertices.push_back(dup);
        }
    }

    int uQuadRows = uWraps ? uCount : uCount - 1;
    int vQuadCols = p.periodicV ? vCount : vCount - 1;
    mesh.indices.reserve(static_cast<size_t>(uQuadRows) * vQuadCols * 6);

    for (int i = 0; i < uQuadRows; ++i) {
        int i2 = (i + 1) % uCount;
        // Only the wraparound edge (last ring back to the first) needs the
        // twist; every other strip connects row i to row i+1 straightforwardly.
        bool twistEdge = p.periodicUTwistV && i2 == 0;
        for (int j = 0; j < vQuadCols; ++j) {
            int j2 = (j + 1) % vCount;
            uint32_t a = static_cast<uint32_t>(i * vCount + j);
            uint32_t dd = static_cast<uint32_t>(i * vCount + j2);
            uint32_t b = twistEdge ? static_cast<uint32_t>(twistRowOffset + static_cast<size_t>(j))
                                    : static_cast<uint32_t>(i2 * vCount + j);
            uint32_t c = twistEdge ? static_cast<uint32_t>(twistRowOffset + static_cast<size_t>(j2))
                                    : static_cast<uint32_t>(i2 * vCount + j2);
            mesh.indices.push_back(a);
            mesh.indices.push_back(b);
            mesh.indices.push_back(c);
            mesh.indices.push_back(a);
            mesh.indices.push_back(c);
            mesh.indices.push_back(dd);
        }
    }
    return mesh;
}

Polyline geodesicToPolyline(const DiffGeoSurface& surf, const std::vector<DiffGeoSurface::GeodesicPoint>& path) {
    Polyline poly;
    poly.points.reserve(path.size());
    for (auto& gp : path) poly.points.push_back(surf.renderPosition(gp.u, gp.v));
    return poly;
}

} // namespace geo
