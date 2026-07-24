#pragma once
#include "geometry/surface_spec.hpp"
#include <string>
#include <vector>

namespace geo {

struct DemoEntry {
    std::string name;
    std::string category;     // "Classical Surfaces" or "Non-Euclidean & Manifolds"
    std::string description;  // one-line blurb shown as a tooltip in the demo menu
    SurfaceFormulas formulas;
};

// Returns the built-in demo gallery in a fixed, stable order (grouped by
// category in the UI, not necessarily in this vector's order).
std::vector<DemoEntry> builtinDemos();

} // namespace geo
