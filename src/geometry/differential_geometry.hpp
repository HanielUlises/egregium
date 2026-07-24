#pragma once
// DiffGeoSurface -----------------------------------------------------------
//
// The core "manifold" abstraction. A 2-parameter (u,v) surface is described
// by an optional embedding (u,v) -> (x,y,z) into R^3, and/or an optional
// Riemannian metric E,F,G (first fundamental form). At least one must be
// given:
//
//   - embedding only : the metric used for curvature/geodesics is INDUCED
//                       from the embedding (E=Xu.Xu etc). This is the
//                       ordinary case for explicit/parametric surfaces
//                       (sphere, torus, pseudosphere, ...).
//   - metric only     : there's no 3D shape to draw -- rendered as a flat
//                        (u,v,0) "model" (e.g. the Poincare disk: you see a
//                        flat disk, but distances/geodesics/curvature follow
//                        the hyperbolic metric, not the flat Euclidean one
//                        of the picture you're looking at).
//   - both            : the given metric is used for curvature/geodesics
//                        EVEN THOUGH it may differ from the embedding's own
//                        Euclidean-induced metric. This is exactly the
//                        hyperboloid-model case: the shape drawn is the
//                        ordinary Euclidean hyperboloid (and its *Euclidean*
//                        curvature is positive -- it's a convex bowl), but
//                        with the Minkowski-induced metric imposed on top,
//                        it's a model of the hyperbolic plane (curvature
//                        -1). Both numbers are available and meant to be
//                        compared -- that disagreement is the whole point
//                        of that demo, not a bug.
//
// All derivatives are computed once, symbolically (expr::differentiate),
// at construction time -- not by finite-differencing an already
// finite-differenced quantity, which is the numerically fragile way most
// hobby implementations do this.

#include "expr/ast.hpp"
#include "geometry/mesh_types.hpp"
#include <vector>

namespace geo {

class DiffGeoSurface {
public:
    struct Embedding { expr::ExprPtr X, Y, Z; }; // functions of u,v
    struct Metric { expr::ExprPtr E, F, G; };     // functions of u,v

    static DiffGeoSurface fromEmbedding(Embedding emb);
    static DiffGeoSurface fromMetric(Metric metric);
    static DiffGeoSurface fromEmbeddingAndMetric(Embedding emb, Metric metric);

    bool hasEmbedding() const { return hasEmbedding_; }
    bool hasExplicitMetric() const { return hasExplicitMetric_; }

    // Position used for drawing: the real embedding if present, otherwise
    // the flat (u,v,0) placeholder (a deliberate, honest stand-in -- it is
    // NOT claiming that placeholder is an isometric embedding).
    Vec3 renderPosition(double u, double v) const;
    // Unit normal for shading: from the real embedding if present, else the
    // trivial (0,0,1) normal of the flat placeholder.
    Vec3 renderNormal(double u, double v) const;

    // The ACTIVE first fundamental form -- what Christoffel symbols,
    // geodesics, and gaussianCurvature() all use. Equal to the induced
    // metric unless an explicit metric override was supplied.
    void firstFundamentalForm(double u, double v, double& E, double& F, double& G) const;

    // Intrinsic Gaussian curvature via the Brioschi formula from the ACTIVE
    // metric alone (Theorema Egregium: no embedding needed). Always
    // available.
    double gaussianCurvature(double u, double v) const;

    // Extrinsic Gaussian curvature (L*N - M^2) / (E*G - F^2) using the
    // embedding's own second fundamental form and its OWN Euclidean-induced
    // first fundamental form (never the overridden metric). Only call when
    // hasEmbedding().
    double gaussianCurvatureExtrinsic(double u, double v) const;

    struct Christoffel { double G111, G112, G122, G211, G212, G222; };
    Christoffel christoffelSymbols(double u, double v) const;

    struct GeodesicPoint { double u, v; };
    // Integrates the unit-speed geodesic ODE via RK4 starting at (u0,v0),
    // heading in the direction at angle `dirAngle` measured in the metric's
    // own orthonormal frame at that point (angle 0 = the ∂u direction).
    // `arcLength` is measured w.r.t. the active metric.
    std::vector<GeodesicPoint> integrateGeodesic(double u0, double v0, double dirAngle,
                                                  double arcLength, int steps) const;

private:
    bool hasEmbedding_ = false;
    bool hasExplicitMetric_ = false;

    // embedding + its first/second symbolic partials (only populated if hasEmbedding_)
    expr::ExprPtr X_, Y_, Z_;
    expr::ExprPtr Xu_, Xv_, Yu_, Yv_, Zu_, Zv_;
    expr::ExprPtr Xuu_, Xuv_, Xvv_, Yuu_, Yuv_, Yvv_, Zuu_, Zuv_, Zvv_;
    // induced (Euclidean) first fundamental form, always populated if hasEmbedding_,
    // used only for gaussianCurvatureExtrinsic's denominator.
    expr::ExprPtr inducedE_, inducedF_, inducedG_;

    // ACTIVE metric (== induced metric unless hasExplicitMetric_) and its
    // symbolic partials needed for Christoffel symbols + Brioschi curvature.
    expr::ExprPtr E_, F_, G_;
    expr::ExprPtr Eu_, Ev_, Fu_, Fv_, Gu_, Gv_;
    expr::ExprPtr Evv_, Fuv_, Guu_;

    void buildActiveMetricDerivatives(); // fills Eu_..Guu_ by differentiating E_,F_,G_
};

} // namespace geo
