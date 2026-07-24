#include "expr/differentiate.hpp"
#include "expr/interpreter.hpp"
#include <cmath>
#include <stdexcept>

namespace expr {

namespace {
ExprPtr sqrt_(ExprPtr a) { return call1("sqrt", std::move(a)); }
ExprPtr sin_(ExprPtr a) { return call1("sin", std::move(a)); }
ExprPtr cos_(ExprPtr a) { return call1("cos", std::move(a)); }
ExprPtr sinh_(ExprPtr a) { return call1("sinh", std::move(a)); }
ExprPtr cosh_(ExprPtr a) { return call1("cosh", std::move(a)); }
ExprPtr tanh_(ExprPtr a) { return call1("tanh", std::move(a)); }
ExprPtr exp_(ExprPtr a) { return call1("exp", std::move(a)); }
ExprPtr log_(ExprPtr a) { return call1("log", std::move(a)); }
ExprPtr abs_(ExprPtr a) { return call1("abs", std::move(a)); }
} // namespace

ExprPtr differentiate(const ExprPtr& e, const std::string& v) {
    if (!e) return konst(0.0);
    switch (e->op) {
        case Op::Const:
            return konst(0.0);
        case Op::Var:
            return konst(e->name == v ? 1.0 : 0.0);
        case Op::Add:
            return add(differentiate(e->args[0], v), differentiate(e->args[1], v));
        case Op::Sub:
            return sub(differentiate(e->args[0], v), differentiate(e->args[1], v));
        case Op::Neg:
            return neg(differentiate(e->args[0], v));
        case Op::Mul: {
            const ExprPtr& a = e->args[0];
            const ExprPtr& b = e->args[1];
            ExprPtr da = differentiate(a, v);
            ExprPtr db = differentiate(b, v);
            // product rule: (a*b)' = a'*b + a*b'
            return add(mul(da, deepCopy(b)), mul(deepCopy(a), db));
        }
        case Op::Div: {
            const ExprPtr& a = e->args[0];
            const ExprPtr& b = e->args[1];
            ExprPtr da = differentiate(a, v);
            ExprPtr db = differentiate(b, v);
            // quotient rule: (a/b)' = (a'*b - a*b') / b^2
            return div_(sub(mul(da, deepCopy(b)), mul(deepCopy(a), db)), mul(deepCopy(b), deepCopy(b)));
        }
        case Op::Pow: {
            const ExprPtr& a = e->args[0]; // base
            const ExprPtr& b = e->args[1]; // exponent
            if (b->op == Op::Const) {
                double n = b->value;
                if (n == 0.0) return konst(0.0);
                // power rule: (a^n)' = n * a^(n-1) * a'
                ExprPtr da = differentiate(a, v);
                return mul(mul(konst(n), pow_(deepCopy(a), konst(n - 1.0))), da);
            }
            // general case: (a^b)' = a^b * (b'*ln(a) + b*a'/a)      [requires a > 0]
            ExprPtr da = differentiate(a, v);
            ExprPtr db = differentiate(b, v);
            ExprPtr term = add(mul(db, log_(deepCopy(a))), mul(deepCopy(b), div_(da, deepCopy(a))));
            return mul(pow_(deepCopy(a), deepCopy(b)), term);
        }
        case Op::Call: {
            const std::string& f = e->name;
            if (f == "min" || f == "max") {
                throw std::runtime_error(
                    "cannot symbolically differentiate '" + f +
                    "' (not smooth) -- use it only in implicit-surface (F(x,y,z)=0) formulas, "
                    "not in embeddings or metrics that need curvature/geodesics");
            }
            if (f == "floor") return konst(0.0); // piecewise-constant a.e.

            if (f == "atan2") {
                // atan2(y,x)' = (x*y' - y*x') / (x^2+y^2)
                const ExprPtr& y = e->args[0];
                const ExprPtr& x = e->args[1];
                ExprPtr dy = differentiate(y, v);
                ExprPtr dx = differentiate(x, v);
                return div_(sub(mul(deepCopy(x), dy), mul(deepCopy(y), dx)),
                            add(mul(deepCopy(x), deepCopy(x)), mul(deepCopy(y), deepCopy(y))));
            }

            // Remaining supported functions are all arity 1 with a chain rule f(u)' = f'(u)*u'
            const ExprPtr& u = e->args[0];
            ExprPtr du = differentiate(u, v);
            ExprPtr uc = deepCopy(u); // keep an undifferentiated copy for use inside f'(u)

            if (f == "sin") return mul(cos_(uc), du);
            if (f == "cos") return neg(mul(sin_(uc), du));
            if (f == "tan") return div_(du, pow_(cos_(uc), konst(2.0)));
            if (f == "asin") return div_(du, sqrt_(sub(konst(1.0), pow_(uc, konst(2.0)))));
            if (f == "acos") return neg(div_(du, sqrt_(sub(konst(1.0), pow_(uc, konst(2.0))))));
            if (f == "atan") return div_(du, add(konst(1.0), pow_(uc, konst(2.0))));
            if (f == "sinh") return mul(cosh_(uc), du);
            if (f == "cosh") return mul(sinh_(uc), du);
            if (f == "tanh") return mul(du, sub(konst(1.0), pow_(tanh_(uc), konst(2.0))));
            if (f == "exp") return mul(exp_(uc), du);
            if (f == "log") return div_(du, uc);
            if (f == "sqrt") return div_(du, mul(konst(2.0), sqrt_(uc)));
            if (f == "abs") return mul(du, div_(uc, abs_(deepCopy(u))));

            throw std::runtime_error("differentiate: unhandled function '" + f + "'");
        }
    }
    throw std::runtime_error("differentiate: unhandled op");
}

ExprPtr simplify(const ExprPtr& e) {
    if (!e) return e;
    if (e->op == Op::Const || e->op == Op::Var) return e;

    if (e->op == Op::Neg) {
        ExprPtr a = simplify(e->args[0]);
        if (a->op == Op::Const) return konst(-a->value);
        if (a->op == Op::Neg) return a->args[0];
        return neg(a);
    }

    if (e->op == Op::Call) {
        std::vector<ExprPtr> newArgs;
        bool allConst = true;
        newArgs.reserve(e->args.size());
        for (auto& a : e->args) {
            ExprPtr sa = simplify(a);
            if (sa->op != Op::Const) allConst = false;
            newArgs.push_back(sa);
        }
        ExprPtr rebuilt = call(e->name, newArgs);
        if (allConst) {
            try {
                double v = evaluate(rebuilt, {});
                return konst(v);
            } catch (...) {
                // fall through, keep symbolic form
            }
        }
        return rebuilt;
    }

    // binary ops: Add, Sub, Mul, Div, Pow
    ExprPtr a = simplify(e->args[0]);
    ExprPtr b = simplify(e->args[1]);
    bool aC = a->op == Op::Const, bC = b->op == Op::Const;

    switch (e->op) {
        case Op::Add:
            if (aC && bC) return konst(a->value + b->value);
            if (aC && a->value == 0.0) return b;
            if (bC && b->value == 0.0) return a;
            return add(a, b);
        case Op::Sub:
            if (aC && bC) return konst(a->value - b->value);
            if (bC && b->value == 0.0) return a;
            if (aC && a->value == 0.0) return simplify(neg(b));
            return sub(a, b);
        case Op::Mul:
            if (aC && bC) return konst(a->value * b->value);
            if ((aC && a->value == 0.0) || (bC && b->value == 0.0)) return konst(0.0);
            if (aC && a->value == 1.0) return b;
            if (bC && b->value == 1.0) return a;
            if (aC && a->value == -1.0) return simplify(neg(b));
            if (bC && b->value == -1.0) return simplify(neg(a));
            return mul(a, b);
        case Op::Div:
            if (aC && bC) return konst(a->value / b->value);
            if (bC && b->value == 1.0) return a;
            if (aC && a->value == 0.0) return konst(0.0);
            return div_(a, b);
        case Op::Pow:
            if (aC && bC) return konst(std::pow(a->value, b->value));
            if (bC && b->value == 0.0) return konst(1.0);
            if (bC && b->value == 1.0) return a;
            return pow_(a, b);
        default:
            break;
    }
    return e;
}

} // namespace expr
