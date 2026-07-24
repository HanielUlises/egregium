#pragma once
#include "expr/ast.hpp"
#include <string>

namespace expr {

// Exact symbolic partial derivative d(e)/d(varName), treating every other
// Var name as a constant. This is what makes curvature/Christoffel-symbol
// computation exact rather than finite-difference approximations of
// finite-difference approximations (which is what you'd get if you tried to
// numerically differentiate a numerically-derived metric).
//
// Throws std::runtime_error if the expression contains a non-smooth
// operation (min/max) that has no symbolic derivative -- those are only
// meant for implicit-surface formulas (shaded via gradient, not used for
// curvature/geodesics), never for embeddings or metrics.
ExprPtr differentiate(const ExprPtr& e, const std::string& varName);

// Bottom-up algebraic simplification: constant folding plus identities
// (x+0, x*1, x*0, x^0, x^1, double negation). Idempotent. Not a full CAS
// simplifier -- just enough to keep repeated differentiation (second
// partials) from blowing up in size.
ExprPtr simplify(const ExprPtr& e);

} // namespace expr
