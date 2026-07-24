#include "expr/ast.hpp"
#include <sstream>

namespace expr {

ExprPtr deepCopy(const ExprPtr& e) {
    if (!e) return nullptr;
    auto c = std::make_shared<Expr>(e->op);
    c->value = e->value;
    c->name = e->name;
    c->args.reserve(e->args.size());
    for (auto& a : e->args) c->args.push_back(deepCopy(a));
    return c;
}

ExprPtr renameVariable(const ExprPtr& e, const std::string& from, const std::string& to) {
    if (!e) return e;
    if (e->op == Op::Var) return e->name == from ? var(to) : e;
    if (e->op == Op::Const) return e;
    auto c = std::make_shared<Expr>(e->op);
    c->value = e->value;
    c->name = e->name;
    c->args.reserve(e->args.size());
    for (auto& a : e->args) c->args.push_back(renameVariable(a, from, to));
    return c;
}

static void writeNum(std::ostringstream& os, double v) {
    os << v;
}

std::string toString(const ExprPtr& e) {
    if (!e) return "<null>";
    std::ostringstream os;
    switch (e->op) {
        case Op::Const: writeNum(os, e->value); break;
        case Op::Var: os << e->name; break;
        case Op::Add: os << "(" << toString(e->args[0]) << " + " << toString(e->args[1]) << ")"; break;
        case Op::Sub: os << "(" << toString(e->args[0]) << " - " << toString(e->args[1]) << ")"; break;
        case Op::Mul: os << "(" << toString(e->args[0]) << " * " << toString(e->args[1]) << ")"; break;
        case Op::Div: os << "(" << toString(e->args[0]) << " / " << toString(e->args[1]) << ")"; break;
        case Op::Pow: os << "(" << toString(e->args[0]) << " ^ " << toString(e->args[1]) << ")"; break;
        case Op::Neg: os << "(-" << toString(e->args[0]) << ")"; break;
        case Op::Call: {
            os << e->name << "(";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) os << ", ";
                os << toString(e->args[i]);
            }
            os << ")";
            break;
        }
    }
    return os.str();
}

} // namespace expr
