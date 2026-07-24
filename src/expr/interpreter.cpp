#include "expr/interpreter.hpp"
#include <cmath>
#include <stdexcept>

namespace expr {

double evaluate(const ExprPtr& e, const Bindings& vars) {
    if (!e) throw std::runtime_error("evaluate: null expression");
    switch (e->op) {
        case Op::Const: return e->value;
        case Op::Var: {
            auto it = vars.find(e->name);
            if (it == vars.end()) throw std::runtime_error("unbound variable: " + e->name);
            return it->second;
        }
        case Op::Add: return evaluate(e->args[0], vars) + evaluate(e->args[1], vars);
        case Op::Sub: return evaluate(e->args[0], vars) - evaluate(e->args[1], vars);
        case Op::Mul: return evaluate(e->args[0], vars) * evaluate(e->args[1], vars);
        case Op::Div: return evaluate(e->args[0], vars) / evaluate(e->args[1], vars);
        case Op::Neg: return -evaluate(e->args[0], vars);
        case Op::Pow: return std::pow(evaluate(e->args[0], vars), evaluate(e->args[1], vars));
        case Op::Call: {
            const std::string& f = e->name;
            auto a0 = [&] { return evaluate(e->args[0], vars); };
            if (f == "sin") return std::sin(a0());
            if (f == "cos") return std::cos(a0());
            if (f == "tan") return std::tan(a0());
            if (f == "asin") return std::asin(a0());
            if (f == "acos") return std::acos(a0());
            if (f == "atan") return std::atan(a0());
            if (f == "sinh") return std::sinh(a0());
            if (f == "cosh") return std::cosh(a0());
            if (f == "tanh") return std::tanh(a0());
            if (f == "exp") return std::exp(a0());
            if (f == "log") return std::log(a0());
            if (f == "sqrt") return std::sqrt(a0());
            if (f == "abs") return std::fabs(a0());
            if (f == "floor") return std::floor(a0());
            if (f == "atan2") return std::atan2(evaluate(e->args[0], vars), evaluate(e->args[1], vars));
            if (f == "min") return std::min(evaluate(e->args[0], vars), evaluate(e->args[1], vars));
            if (f == "max") return std::max(evaluate(e->args[0], vars), evaluate(e->args[1], vars));
            throw std::runtime_error("evaluate: unknown function '" + f + "'");
        }
    }
    throw std::runtime_error("evaluate: unhandled op");
}

} // namespace expr
