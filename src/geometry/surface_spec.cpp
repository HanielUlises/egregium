#include "geometry/surface_spec.hpp"
#include "expr/parser.hpp"
#include "expr/glsl_codegen.hpp"
#include "geometry/mesh_tessellate.hpp"
#include <stdexcept>

namespace geo {

using expr::parseExpression;
using expr::ExprPtr;
using expr::collectVariables;
using expr::renameVariable;
using expr::var;

namespace {

// Parses `formula`, and if it fails, writes a message like
// "<label>: <parser error>" into *error and returns nullptr.
ExprPtr parseOrError(const std::string& formula, const char* label, std::string* error) {
    auto r = parseExpression(formula);
    if (!r.ok) {
        *error = std::string(label) + ": " + r.error;
        return nullptr;
    }
    return r.expr;
}

bool validateVars(const ExprPtr& e, const char* label, std::initializer_list<const char*> allowed, std::string* error) {
    auto vars = collectVariables(e);
    for (auto& name : vars) {
        bool ok = false;
        for (auto* a : allowed)
            if (name == a) { ok = true; break; }
        if (!ok) {
            std::string allowedList;
            for (auto* a : allowed) { if (!allowedList.empty()) allowedList += ","; allowedList += a; }
            *error = std::string(label) + " uses unknown variable '" + name + "' (expected: " + allowedList + ")";
            return false;
        }
    }
    return true;
}

TessellationParams makeParams(const SurfaceFormulas& f, double uMin, double uMax, double vMin, double vMax,
                               bool periodicU, bool periodicV) {
    TessellationParams tp;
    tp.uMin = uMin; tp.uMax = uMax; tp.vMin = vMin; tp.vMax = vMax;
    tp.uSegments = f.resolutionU;
    tp.vSegments = f.resolutionV;
    tp.periodicU = periodicU;
    tp.periodicV = periodicV;
    tp.colorByCurvature = f.colorByCurvature;
    return tp;
}

} // namespace

BuildResult buildSurface(const SurfaceFormulas& f) {
    BuildResult r;
    r.kind = f.kind;
    try {
        switch (f.kind) {
            case SurfaceKind::Explicit: {
                ExprPtr height = parseOrError(f.heightFormula, "height formula z=f(x,y)", &r.error);
                if (!height) return r;
                if (!validateVars(height, "height formula", {"x", "y"}, &r.error)) return r;
                ExprPtr zOfUV = renameVariable(renameVariable(height, "x", "u"), "y", "v");
                DiffGeoSurface dg = DiffGeoSurface::fromEmbedding({var("u"), var("v"), zOfUV});
                r.mesh = tessellate(dg, makeParams(f, f.xMin, f.xMax, f.yMin, f.yMax, false, false));
                r.diffGeo = std::move(dg);
                r.ok = true;
                break;
            }
            case SurfaceKind::Parametric: {
                ExprPtr x = parseOrError(f.xFormula, "x(u,v)", &r.error); if (!x) return r;
                ExprPtr y = parseOrError(f.yFormula, "y(u,v)", &r.error); if (!y) return r;
                ExprPtr z = parseOrError(f.zFormula, "z(u,v)", &r.error); if (!z) return r;
                if (!validateVars(x, "x(u,v)", {"u", "v"}, &r.error)) return r;
                if (!validateVars(y, "y(u,v)", {"u", "v"}, &r.error)) return r;
                if (!validateVars(z, "z(u,v)", {"u", "v"}, &r.error)) return r;
                DiffGeoSurface dg = DiffGeoSurface::fromEmbedding({x, y, z});
                r.mesh = tessellate(dg, makeParams(f, f.uMin, f.uMax, f.vMin, f.vMax, f.periodicU, f.periodicV));
                r.diffGeo = std::move(dg);
                r.ok = true;
                break;
            }
            case SurfaceKind::Implicit: {
                ExprPtr F = parseOrError(f.implicitFormula, "F(x,y,z)", &r.error);
                if (!F) return r;
                if (!validateVars(F, "F(x,y,z)", {"x", "y", "z"}, &r.error)) return r;
                r.implicitGLSL = expr::toGLSL(F);
                r.bboxMin = f.bboxMin;
                r.bboxMax = f.bboxMax;
                r.ok = true;
                break;
            }
            case SurfaceKind::Metric: {
                ExprPtr E = parseOrError(f.metricE, "E(u,v)", &r.error); if (!E) return r;
                ExprPtr F2 = parseOrError(f.metricF, "F(u,v)", &r.error); if (!F2) return r;
                ExprPtr G = parseOrError(f.metricG, "G(u,v)", &r.error); if (!G) return r;
                if (!validateVars(E, "E(u,v)", {"u", "v"}, &r.error)) return r;
                if (!validateVars(F2, "F(u,v)", {"u", "v"}, &r.error)) return r;
                if (!validateVars(G, "G(u,v)", {"u", "v"}, &r.error)) return r;

                DiffGeoSurface dg = DiffGeoSurface::fromMetric({E, F2, G});
                if (f.metricHasEmbedding) {
                    ExprPtr ex = parseOrError(f.embedXFormula, "x(u,v)", &r.error); if (!ex) return r;
                    ExprPtr ey = parseOrError(f.embedYFormula, "y(u,v)", &r.error); if (!ey) return r;
                    ExprPtr ez = parseOrError(f.embedZFormula, "z(u,v)", &r.error); if (!ez) return r;
                    if (!validateVars(ex, "x(u,v)", {"u", "v"}, &r.error)) return r;
                    if (!validateVars(ey, "y(u,v)", {"u", "v"}, &r.error)) return r;
                    if (!validateVars(ez, "z(u,v)", {"u", "v"}, &r.error)) return r;
                    dg = DiffGeoSurface::fromEmbeddingAndMetric({ex, ey, ez}, {E, F2, G});
                }
                r.mesh = tessellate(dg, makeParams(f, f.uMin, f.uMax, f.vMin, f.vMax, f.periodicU, f.periodicV));
                r.diffGeo = std::move(dg);
                r.ok = true;
                break;
            }
        }
    } catch (const std::exception& ex) {
        r.ok = false;
        r.error = ex.what();
    }
    return r;
}

} // namespace geo
