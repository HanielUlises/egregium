#include "expr/parser.hpp"
#include <cctype>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

namespace expr {

namespace {

enum class Tok { Number, Ident, Plus, Minus, Star, Slash, Caret, LParen, RParen, Comma, End, Invalid };

struct Token {
    Tok kind;
    double num = 0.0;
    std::string text;
    size_t pos = 0;
};

struct ParseException : std::runtime_error {
    size_t pos;
    ParseException(std::string msg, size_t p) : std::runtime_error(std::move(msg)), pos(p) {}
};

std::vector<Token> tokenize(const std::string& s) {
    std::vector<Token> out;
    size_t i = 0, n = s.size();
    while (i < n) {
        char c = s[i];
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        size_t start = i;
        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '.' && i + 1 < n && std::isdigit(static_cast<unsigned char>(s[i + 1])))) {
            size_t j = i;
            while (j < n && std::isdigit(static_cast<unsigned char>(s[j]))) ++j;
            if (j < n && s[j] == '.') { ++j; while (j < n && std::isdigit(static_cast<unsigned char>(s[j]))) ++j; }
            if (j < n && (s[j] == 'e' || s[j] == 'E')) {
                size_t k = j + 1;
                if (k < n && (s[k] == '+' || s[k] == '-')) ++k;
                if (k < n && std::isdigit(static_cast<unsigned char>(s[k]))) {
                    ++k; while (k < n && std::isdigit(static_cast<unsigned char>(s[k]))) ++k;
                    j = k;
                }
            }
            std::string numText = s.substr(i, j - i);
            Token t; t.kind = Tok::Number; t.num = std::stod(numText); t.pos = start;
            out.push_back(t);
            i = j;
            continue;
        }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            size_t j = i;
            while (j < n && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_')) ++j;
            Token t; t.kind = Tok::Ident; t.text = s.substr(i, j - i); t.pos = start;
            out.push_back(t);
            i = j;
            continue;
        }
        Token t; t.pos = start;
        switch (c) {
            case '+': t.kind = Tok::Plus; break;
            case '-': t.kind = Tok::Minus; break;
            case '*': t.kind = Tok::Star; break;
            case '/': t.kind = Tok::Slash; break;
            case '^': t.kind = Tok::Caret; break;
            case '(': t.kind = Tok::LParen; break;
            case ')': t.kind = Tok::RParen; break;
            case ',': t.kind = Tok::Comma; break;
            default: t.kind = Tok::Invalid; t.text = std::string(1, c); break;
        }
        if (t.kind == Tok::Invalid) throw ParseException("unexpected character '" + t.text + "'", start);
        out.push_back(t);
        ++i;
    }
    Token end; end.kind = Tok::End; end.pos = n;
    out.push_back(end);
    return out;
}

struct FnSpec { int arity; };
const std::map<std::string, FnSpec>& functionTable() {
    static const std::map<std::string, FnSpec> table = {
        {"sin", {1}}, {"cos", {1}}, {"tan", {1}},
        {"asin", {1}}, {"acos", {1}}, {"atan", {1}}, {"atan2", {2}},
        {"sinh", {1}}, {"cosh", {1}}, {"tanh", {1}},
        {"exp", {1}}, {"log", {1}}, {"sqrt", {1}}, {"abs", {1}}, {"floor", {1}},
        {"min", {2}}, {"max", {2}}, {"pow", {2}},
    };
    return table;
}

class Parser {
public:
    explicit Parser(std::vector<Token> toks) : toks_(std::move(toks)) {}

    ExprPtr parseTop() {
        ExprPtr e = parseSum();
        if (!check(Tok::End)) {
            throw ParseException("unexpected trailing input", peek().pos);
        }
        return e;
    }

private:
    std::vector<Token> toks_;
    size_t pos_ = 0;

    const Token& peek() const { return toks_[pos_]; }
    const Token& advance() { return toks_[pos_++]; }
    bool check(Tok k) const { return peek().kind == k; }
    bool match(Tok k) { if (check(k)) { advance(); return true; } return false; }
    void expect(Tok k, const char* what) {
        if (!check(k)) throw ParseException(std::string("expected ") + what, peek().pos);
        advance();
    }

    // expr := term (('+'|'-') term)*
    ExprPtr parseSum() {
        ExprPtr lhs = parseProduct();
        for (;;) {
            if (match(Tok::Plus)) lhs = add(lhs, parseProduct());
            else if (match(Tok::Minus)) lhs = sub(lhs, parseProduct());
            else break;
        }
        return lhs;
    }

    // term := unary (('*'|'/') unary)*
    ExprPtr parseProduct() {
        ExprPtr lhs = parseUnary();
        for (;;) {
            if (match(Tok::Star)) lhs = mul(lhs, parseUnary());
            else if (match(Tok::Slash)) lhs = div_(lhs, parseUnary());
            else break;
        }
        return lhs;
    }

    // unary := '-' unary | '+' unary | power
    ExprPtr parseUnary() {
        if (match(Tok::Minus)) return neg(parseUnary());
        if (match(Tok::Plus)) return parseUnary();
        return parsePower();
    }

    // power := atom ('^' unary)?      (right-assoc; exponent may be unary so 2^-2 works)
    ExprPtr parsePower() {
        ExprPtr base = parseAtom();
        if (match(Tok::Caret)) {
            ExprPtr exponent = parseUnary();
            return pow_(base, exponent);
        }
        return base;
    }

    std::vector<ExprPtr> parseArgList() {
        std::vector<ExprPtr> args;
        if (check(Tok::RParen)) return args; // shouldn't normally happen (no 0-arity fns) but stay lenient
        args.push_back(parseSum());
        while (match(Tok::Comma)) args.push_back(parseSum());
        return args;
    }

    ExprPtr parseAtom() {
        if (check(Tok::Number)) {
            double v = advance().num;
            return konst(v);
        }
        if (check(Tok::LParen)) {
            advance();
            ExprPtr e = parseSum();
            expect(Tok::RParen, "')'");
            return e;
        }
        if (check(Tok::Ident)) {
            Token id = advance();
            if (id.text == "pi") return konst(M_PI);
            if (id.text == "e" && !check(Tok::LParen)) return konst(M_E);
            if (check(Tok::LParen)) {
                advance();
                std::vector<ExprPtr> args = parseArgList();
                expect(Tok::RParen, "')' to close function call");
                auto it = functionTable().find(id.text);
                if (it == functionTable().end()) {
                    throw ParseException("unknown function '" + id.text + "'", id.pos);
                }
                if (static_cast<int>(args.size()) != it->second.arity) {
                    throw ParseException(
                        id.text + " expects " + std::to_string(it->second.arity) +
                            " argument(s), got " + std::to_string(args.size()),
                        id.pos);
                }
                if (id.text == "pow") return pow_(args[0], args[1]);
                return call(id.text, std::move(args));
            }
            return var(id.text);
        }
        throw ParseException("expected a number, variable, or '('", peek().pos);
    }
};

} // namespace

ParseResult parseExpression(const std::string& text) {
    ParseResult r;
    try {
        std::vector<Token> toks = tokenize(text);
        Parser p(std::move(toks));
        r.expr = p.parseTop();
        r.ok = true;
    } catch (const ParseException& ex) {
        r.ok = false;
        r.error = ex.what();
        r.errorPos = ex.pos;
    } catch (const std::exception& ex) {
        r.ok = false;
        r.error = ex.what();
        r.errorPos = text.size();
    }
    return r;
}

} // namespace expr
