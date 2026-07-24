#pragma once
#include "expr/ast.hpp"
#include <unordered_map>

namespace expr {

using Bindings = std::unordered_map<std::string, double>;

// Evaluates e at the given variable bindings. Throws std::runtime_error if a
// Var node references a name that isn't in bindings, or an unknown Call name
// is encountered (the latter should be impossible for anything that came out
// of parseExpression, since the parser validates function names/arity).
double evaluate(const ExprPtr& e, const Bindings& vars);

} // namespace expr
