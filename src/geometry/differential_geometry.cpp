#include "geometry/differential_geometry.hpp"
#include "expr/differentiate.hpp"
#include "expr/interpreter.hpp"
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

using expr::ExprPtr;
using expr::Bindings;

namespace geo {

namespace {

void requireOnlyUV(const ExprPtr& e, const char* label) {
    std::set<std::string> vars = expr::collectVariables(e);
    for (auto& name : vars) {
        if (name != "u" && name != "v") {
            throw std::runtime_error(std::string(label) + " uses unknown variable '" + name +
                                      "' -- only u and v are allowed here");
        }
    }
}

ExprPtr d(const ExprPtr& e, const std::string& v) { return expr::simplify(expr::differentiate(e, v)); }

double det3(double a00, double a01, double a02, double a10, double a11, double a12, double a20, double a21, double a22) {
    return a00 * (a11 * a22 - a12 * a21) - a01 * (a10 * a22 - a12 * a20) + a02 * (a10 * a21 - a11 * a20);
}

} // namespace

DiffGeoSurface DiffGeoSurface::fromEmbedding(Embedding emb) {
    requireOnlyUV(emb.X, "embedding x(u,v)");
    requireOnlyUV(emb.Y, "embedding y(u,v)");
    requireOnlyUV(emb.Z, "embedding z(u,v)");

    DiffGeoSurface s;
    s.hasEmbedding_ = true;
    s.hasExplicitMetric_ = false;
    s.X_ = emb.X; s.Y_ = emb.Y; s.Z_ = emb.Z;

    s.Xu_ = d(s.X_, "u"); s.Xv_ = d(s.X_, "v");
    s.Yu_ = d(s.Y_, "u"); s.Yv_ = d(s.Y_, "v");
    s.Zu_ = d(s.Z_, "u"); s.Zv_ = d(s.Z_, "v");

    s.Xuu_ = d(s.Xu_, "u"); s.Xuv_ = d(s.Xu_, "v"); s.Xvv_ = d(s.Xv_, "v");
    s.Yuu_ = d(s.Yu_, "u"); s.Yuv_ = d(s.Yu_, "v"); s.Yvv_ = d(s.Yv_, "v");
    s.Zuu_ = d(s.Zu_, "u"); s.Zuv_ = d(s.Zu_, "v"); s.Zvv_ = d(s.Zv_, "v");

    // induced (Euclidean) first fundamental form
    using expr::add; using expr::mul;
    s.inducedE_ = expr::simplify(add(add(mul(s.Xu_, s.Xu_), mul(s.Yu_, s.Yu_)), mul(s.Zu_, s.Zu_)));
    s.inducedF_ = expr::simplify(add(add(mul(s.Xu_, s.Xv_), mul(s.Yu_, s.Yv_)), mul(s.Zu_, s.Zv_)));
    s.inducedG_ = expr::simplify(add(add(mul(s.Xv_, s.Xv_), mul(s.Yv_, s.Yv_)), mul(s.Zv_, s.Zv_)));

    // no explicit metric override -> active metric IS the induced metric
    s.E_ = s.inducedE_; s.F_ = s.inducedF_; s.G_ = s.inducedG_;
    s.buildActiveMetricDerivatives();
    return s;
}

DiffGeoSurface DiffGeoSurface::fromMetric(Metric metric) {
    requireOnlyUV(metric.E, "metric E(u,v)");
    requireOnlyUV(metric.F, "metric F(u,v)");
    requireOnlyUV(metric.G, "metric G(u,v)");

    DiffGeoSurface s;
    s.hasEmbedding_ = false;
    s.hasExplicitMetric_ = true;
    s.E_ = metric.E; s.F_ = metric.F; s.G_ = metric.G;
    s.buildActiveMetricDerivatives();
    return s;
}

DiffGeoSurface DiffGeoSurface::fromEmbeddingAndMetric(Embedding emb, Metric metric) {
    DiffGeoSurface s = fromEmbedding(emb); // sets induced metric + embedding derivatives
    requireOnlyUV(metric.E, "metric E(u,v)");
    requireOnlyUV(metric.F, "metric F(u,v)");
    requireOnlyUV(metric.G, "metric G(u,v)");
    s.hasExplicitMetric_ = true;
    s.E_ = metric.E; s.F_ = metric.F; s.G_ = metric.G; // override the active metric
    s.buildActiveMetricDerivatives();
    return s;
}

void DiffGeoSurface::buildActiveMetricDerivatives() {
    Eu_ = d(E_, "u"); Ev_ = d(E_, "v");
    Fu_ = d(F_, "u"); Fv_ = d(F_, "v");
    Gu_ = d(G_, "u"); Gv_ = d(G_, "v");
    Evv_ = d(Ev_, "v");
    Fuv_ = d(Fu_, "v");
    Guu_ = d(Gu_, "u");
}

Vec3 DiffGeoSurface::renderPosition(double u, double v) const {
    if (!hasEmbedding_) return Vec3(u, v, 0.0);
    Bindings b{{"u", u}, {"v", v}};
    return Vec3(expr::evaluate(X_, b), expr::evaluate(Y_, b), expr::evaluate(Z_, b));
}

Vec3 DiffGeoSurface::renderNormal(double u, double v) const {
    if (!hasEmbedding_) return Vec3(0, 0, 1);
    Bindings b{{"u", u}, {"v", v}};
    Vec3 xu(expr::evaluate(Xu_, b), expr::evaluate(Yu_, b), expr::evaluate(Zu_, b));
    Vec3 xv(expr::evaluate(Xv_, b), expr::evaluate(Yv_, b), expr::evaluate(Zv_, b));
    return xu.cross(xv).normalized();
}

void DiffGeoSurface::firstFundamentalForm(double u, double v, double& E, double& F, double& G) const {
    Bindings b{{"u", u}, {"v", v}};
    E = expr::evaluate(E_, b);
    F = expr::evaluate(F_, b);
    G = expr::evaluate(G_, b);
}

double DiffGeoSurface::gaussianCurvature(double u, double v) const {
    Bindings b{{"u", u}, {"v", v}};
    double E = expr::evaluate(E_, b), F = expr::evaluate(F_, b), G = expr::evaluate(G_, b);
    double Eu = expr::evaluate(Eu_, b), Ev = expr::evaluate(Ev_, b);
    double Fu = expr::evaluate(Fu_, b), Fv = expr::evaluate(Fv_, b);
    double Gu = expr::evaluate(Gu_, b), Gv = expr::evaluate(Gv_, b);
    double Evv = expr::evaluate(Evv_, b), Fuv = expr::evaluate(Fuv_, b), Guu = expr::evaluate(Guu_, b);

    // Brioschi formula: K = (det(M1) - det(M2)) / (EG-F^2)^2
    double det1 = det3(-0.5 * Evv + Fuv - 0.5 * Guu, 0.5 * Eu, Fu - 0.5 * Ev,
                        Fv - 0.5 * Gu, E, F,
                        0.5 * Gv, F, G);
    double det2 = det3(0.0, 0.5 * Ev, 0.5 * Gu,
                        0.5 * Ev, E, F,
                        0.5 * Gu, F, G);
    double denom = E * G - F * F;
    return (det1 - det2) / (denom * denom);
}

double DiffGeoSurface::gaussianCurvatureExtrinsic(double u, double v) const {
    if (!hasEmbedding_) throw std::runtime_error("gaussianCurvatureExtrinsic requires an embedding");
    Bindings b{{"u", u}, {"v", v}};
    Vec3 xu(expr::evaluate(Xu_, b), expr::evaluate(Yu_, b), expr::evaluate(Zu_, b));
    Vec3 xv(expr::evaluate(Xv_, b), expr::evaluate(Yv_, b), expr::evaluate(Zv_, b));
    Vec3 n = xu.cross(xv).normalized();
    Vec3 xuu(expr::evaluate(Xuu_, b), expr::evaluate(Yuu_, b), expr::evaluate(Zuu_, b));
    Vec3 xuv(expr::evaluate(Xuv_, b), expr::evaluate(Yuv_, b), expr::evaluate(Zuv_, b));
    Vec3 xvv(expr::evaluate(Xvv_, b), expr::evaluate(Yvv_, b), expr::evaluate(Zvv_, b));
    double L = xuu.dot(n), M = xuv.dot(n), N = xvv.dot(n);
    double E = expr::evaluate(inducedE_, b), F = expr::evaluate(inducedF_, b), G = expr::evaluate(inducedG_, b);
    return (L * N - M * M) / (E * G - F * F);
}

DiffGeoSurface::Christoffel DiffGeoSurface::christoffelSymbols(double u, double v) const {
    Bindings b{{"u", u}, {"v", v}};
    double E = expr::evaluate(E_, b), F = expr::evaluate(F_, b), G = expr::evaluate(G_, b);
    double Eu = expr::evaluate(Eu_, b), Ev = expr::evaluate(Ev_, b);
    double Fu = expr::evaluate(Fu_, b), Fv = expr::evaluate(Fv_, b);
    double Gu = expr::evaluate(Gu_, b), Gv = expr::evaluate(Gv_, b);
    double denom = 2.0 * (E * G - F * F);

    Christoffel c;
    c.G111 = (G * Eu - 2 * F * Fu + F * Ev) / denom;
    c.G112 = (G * Ev - F * Gu) / denom;
    c.G122 = (2 * G * Fv - G * Gu - F * Gv) / denom;
    c.G211 = (2 * E * Fu - E * Ev - F * Eu) / denom;
    c.G212 = (E * Gu - F * Ev) / denom;
    c.G222 = (E * Gv - 2 * F * Fv + F * Gu) / denom;
    return c;
}

std::vector<DiffGeoSurface::GeodesicPoint> DiffGeoSurface::integrateGeodesic(double u0, double v0, double dirAngle,
                                                                              double arcLength, int steps) const {
    double E, F, G;
    firstFundamentalForm(u0, v0, E, F, G);

    // Gram-Schmidt orthonormal frame {e1,e2} at (u0,v0) w.r.t. the metric,
    // then take the unit-speed direction at angle dirAngle within that frame.
    double sqrtE = std::sqrt(std::max(E, 1e-12));
    double e1u = 1.0 / sqrtE, e1v = 0.0;
    double proj = F / sqrtE; // <d_v, e1>
    double wu = -proj * e1u, wv = 1.0 - proj * e1v;
    double normW2 = std::max(E * wu * wu + 2 * F * wu * wv + G * wv * wv, 1e-12);
    double normW = std::sqrt(normW2);
    double e2u = wu / normW, e2v = wv / normW;

    double pu = std::cos(dirAngle) * e1u + std::sin(dirAngle) * e2u;
    double pv = std::cos(dirAngle) * e1v + std::sin(dirAngle) * e2v;

    std::vector<GeodesicPoint> path;
    steps = std::max(steps, 1);
    path.reserve(static_cast<size_t>(steps) + 1);
    path.push_back({u0, v0});

    double h = arcLength / steps;
    double u = u0, v = v0;

    auto rhs = [&](double uu, double vv, double ppu, double ppv, double out[4]) {
        Christoffel c = christoffelSymbols(uu, vv);
        out[0] = ppu;
        out[1] = ppv;
        out[2] = -(c.G111 * ppu * ppu + 2 * c.G112 * ppu * ppv + c.G122 * ppv * ppv);
        out[3] = -(c.G211 * ppu * ppu + 2 * c.G212 * ppu * ppv + c.G222 * ppv * ppv);
    };

    for (int i = 0; i < steps; ++i) {
        double k1[4], k2[4], k3[4], k4[4];
        rhs(u, v, pu, pv, k1);
        rhs(u + 0.5 * h * k1[0], v + 0.5 * h * k1[1], pu + 0.5 * h * k1[2], pv + 0.5 * h * k1[3], k2);
        rhs(u + 0.5 * h * k2[0], v + 0.5 * h * k2[1], pu + 0.5 * h * k2[2], pv + 0.5 * h * k2[3], k3);
        rhs(u + h * k3[0], v + h * k3[1], pu + h * k3[2], pv + h * k3[3], k4);
        u += (h / 6.0) * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]);
        v += (h / 6.0) * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]);
        pu += (h / 6.0) * (k1[2] + 2 * k2[2] + 2 * k3[2] + k4[2]);
        pv += (h / 6.0) * (k1[3] + 2 * k2[3] + 2 * k3[3] + k4[3]);
        path.push_back({u, v});
    }
    return path;
}

} // namespace geo
