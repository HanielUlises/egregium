#pragma once
#include "geometry/differential_geometry.hpp"
#include "geometry/mesh_types.hpp"

namespace geo {

struct TessellationParams {
    double uMin = 0, uMax = 1, vMin = 0, vMax = 1;
    int uSegments = 64, vSegments = 64;
    bool periodicU = false, periodicV = false; // wrap last column/row back to the first instead of a boundary edge
    // Some immersions (Klein bottle, and half-angle-twist surfaces in general)
    // identify u=uMax with u=uMin under a REFLECTED v, i.e. renderPosition(uMax,v)
    // == renderPosition(uMin,-v), not renderPosition(uMin,v) -- a plain periodicU
    // wrap stitches the wrong vertices together and leaves a visible seam.
    // When set, this wraps u like periodicU but mirrors the v index at the seam
    // (requires periodicV, since the mirrored index must itself be a valid
    // sample); periodicU is ignored in that case.
    bool periodicUTwistV = false;
    bool colorByCurvature = false;              // fill Vertex::scalar with gaussianCurvature(u,v)
};

// Samples surf on a (uSegments x vSegments) grid over the given domain and
// builds a triangle mesh. Non-finite curvature samples (possible at
// coordinate singularities, e.g. u=0 in a sphere's (theta,phi) parametrization)
// are clamped to 0 and excluded from the min/max range so one bad sample
// doesn't wreck the colormap normalization for the whole mesh.
Mesh tessellate(const DiffGeoSurface& surf, const TessellationParams& params);

// Maps a geodesic's (u,v) path through the surface's renderPosition into a
// drawable 3D polyline.
Polyline geodesicToPolyline(const DiffGeoSurface& surf, const std::vector<DiffGeoSurface::GeodesicPoint>& path);

} // namespace geo
