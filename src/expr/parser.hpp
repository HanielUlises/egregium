#pragma once
#include "expr/ast.hpp"
#include <string>

namespace expr {

struct ParseResult {
    bool ok = false;
    ExprPtr expr;        // valid iff ok
    std::string error;   // human-readable message, valid iff !ok
    size_t errorPos = 0;  // character offset into the input, for caret display
};

// Grammar (precedence high -> low): atoms/calls/parens, '^' (right-assoc,
// exponent may itself start with unary '-'), unary '-', '*','/', '+','-'.
// Supported functions: sin cos tan asin acos atan atan2 sinh cosh tanh
// exp log sqrt abs min max pow floor. Named constants: pi, e.
// Note: implicit multiplication ("2x") is NOT supported -- write "2*x".
ParseResult parseExpression(const std::string& text);

} // namespace expr
