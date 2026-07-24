#pragma once
// Expression AST -------------------------------------------------------
//
// A minimal computer-algebra tree. Every formula the user types (or every
// built-in demo formula) is parsed into this representation exactly once.
// Three independent operations then walk the SAME tree structure:
//
//   - interpreter.hpp   : evaluate(expr, bindings) -> double         (CPU)
//   - differentiate.hpp : differentiate(expr, "u") -> ExprPtr        (symbolic, exact)
//   - glsl_codegen.hpp  : toGLSL(expr) -> std::string                (GPU raymarching)
//
// Using shared_ptr (not unique_ptr) is deliberate: differentiation needs to
// reuse undifferentiated subtrees verbatim inside new nodes (e.g. the product
// rule d(u*v) = du*v + u*dv reuses u and v as-is), which requires structural
// sharing rather than deep copies.

#include <memory>
#include <string>
#include <vector>
#include <set>

namespace expr {

enum class Op {
    Const,  // value
    Var,    // name
    Add, Sub, Mul, Div, Neg, Pow,   // args.size() == 2 (Neg: 1)
    Call                              // name = function name, args = operands
};

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    Op op;
    double value = 0.0;        // valid iff op == Const
    std::string name;          // valid iff op == Var (variable name) or Call (function name)
    std::vector<ExprPtr> args; // operands

    explicit Expr(Op o) : op(o) {}
};

// --- constructors ------------------------------------------------------

inline ExprPtr konst(double v) {
    auto e = std::make_shared<Expr>(Op::Const);
    e->value = v;
    return e;
}
inline ExprPtr var(std::string n) {
    auto e = std::make_shared<Expr>(Op::Var);
    e->name = std::move(n);
    return e;
}
inline ExprPtr bin(Op op, ExprPtr a, ExprPtr b) {
    auto e = std::make_shared<Expr>(op);
    e->args = {std::move(a), std::move(b)};
    return e;
}
inline ExprPtr add(ExprPtr a, ExprPtr b) { return bin(Op::Add, std::move(a), std::move(b)); }
inline ExprPtr sub(ExprPtr a, ExprPtr b) { return bin(Op::Sub, std::move(a), std::move(b)); }
inline ExprPtr mul(ExprPtr a, ExprPtr b) { return bin(Op::Mul, std::move(a), std::move(b)); }
inline ExprPtr div_(ExprPtr a, ExprPtr b) { return bin(Op::Div, std::move(a), std::move(b)); }
inline ExprPtr pow_(ExprPtr a, ExprPtr b) { return bin(Op::Pow, std::move(a), std::move(b)); }
inline ExprPtr neg(ExprPtr a) {
    auto e = std::make_shared<Expr>(Op::Neg);
    e->args = {std::move(a)};
    return e;
}
inline ExprPtr call(std::string fn, std::vector<ExprPtr> args) {
    auto e = std::make_shared<Expr>(Op::Call);
    e->name = std::move(fn);
    e->args = std::move(args);
    return e;
}
inline ExprPtr call1(std::string fn, ExprPtr a) { return call(std::move(fn), {std::move(a)}); }
inline ExprPtr call2(std::string fn, ExprPtr a, ExprPtr b) { return call(std::move(fn), {std::move(a), std::move(b)}); }

inline void collectVariables(const ExprPtr& e, std::set<std::string>& out) {
    if (!e) return;
    if (e->op == Op::Var) out.insert(e->name);
    for (auto& a : e->args) collectVariables(a, out);
}

inline std::set<std::string> collectVariables(const ExprPtr& e) {
    std::set<std::string> s;
    collectVariables(e, s);
    return s;
}

// Deep structural copy (used sparingly; most code shares nodes intentionally).
ExprPtr deepCopy(const ExprPtr& e);

// Returns a new tree with every Var(from) replaced by Var(to). Used to treat
// an explicit height field f(x,y) as the parametric embedding (u, v, f(u,v))
// internally, so it can share all the DiffGeoSurface machinery.
ExprPtr renameVariable(const ExprPtr& e, const std::string& from, const std::string& to);

// Human-readable re-serialization, used in error messages / debug prints.
std::string toString(const ExprPtr& e);

} // namespace expr
