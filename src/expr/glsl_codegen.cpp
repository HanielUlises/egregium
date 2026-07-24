#include "expr/glsl_codegen.hpp"
#include <cstdio>
#include <stdexcept>

namespace expr {

namespace {
std::string formatGLSLNumber(double v) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.9g", v);
    std::string s(buf);
    // GLSL requires float literals to look like floats (a '.' or exponent),
    // otherwise e.g. "2" is an int literal and "2 * x" (x a float) fails to compile.
    if (s.find('.') == std::string::npos && s.find('e') == std::string::npos &&
        s.find('E') == std::string::npos && s.find("inf") == std::string::npos &&
        s.find("nan") == std::string::npos) {
        s += ".0";
    }
    return s;
}
} // namespace

std::string toGLSL(const ExprPtr& e) {
    if (!e) return "0.0";
    switch (e->op) {
        case Op::Const:
            return formatGLSLNumber(e->value);
        case Op::Var:
            return e->name;
        case Op::Add:
            return "(" + toGLSL(e->args[0]) + " + " + toGLSL(e->args[1]) + ")";
        case Op::Sub:
            return "(" + toGLSL(e->args[0]) + " - " + toGLSL(e->args[1]) + ")";
        case Op::Mul:
            return "(" + toGLSL(e->args[0]) + " * " + toGLSL(e->args[1]) + ")";
        case Op::Div:
            return "(" + toGLSL(e->args[0]) + " / " + toGLSL(e->args[1]) + ")";
        case Op::Neg:
            return "(-" + toGLSL(e->args[0]) + ")";
        case Op::Pow:
            return "pow(" + toGLSL(e->args[0]) + ", " + toGLSL(e->args[1]) + ")";
        case Op::Call: {
            if (e->name == "atan2") {
                // GLSL's 2-argument atan(y, x) matches atan2(y, x) exactly.
                return "atan(" + toGLSL(e->args[0]) + ", " + toGLSL(e->args[1]) + ")";
            }
            std::string s = e->name + "(";
            for (size_t i = 0; i < e->args.size(); ++i) {
                if (i) s += ", ";
                s += toGLSL(e->args[i]);
            }
            s += ")";
            return s;
        }
    }
    throw std::runtime_error("toGLSL: unhandled op");
}

} // namespace expr
