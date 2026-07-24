#pragma once
#include "expr/ast.hpp"
#include <string>

namespace expr {

// Emits a GLSL expression string equivalent to e. Variable names are emitted
// verbatim (so the caller's shader must declare matching float locals/args,
// e.g. "x", "y", "z" for an implicit F(x,y,z)). GLSL's built-in function set
// (sin, cos, sqrt, pow, exp, log, min, max, abs, floor, and both 1- and
// 2-argument atan) matches this AST's Call names directly, so this is a
// close to line-for-line translation rather than a real compiler backend.
std::string toGLSL(const ExprPtr& e);

} // namespace expr
