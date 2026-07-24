#pragma once
#include "geometry/mesh_types.hpp"

namespace geo {

// An axis-aligned cube [lo,hi]^3. Implicit surfaces are raymarched: this
// cube is rasterized normally (so ordinary depth testing against other
// scene objects works for free) and its fragment shader marches a ray from
// the camera through each fragment's world position, looking for a sign
// change in F(x,y,z). No UVs/normals needed here -- the shader reconstructs
// everything it needs from the world-space position alone.
Mesh makeBoundingCube(double lo, double hi);

} // namespace geo
