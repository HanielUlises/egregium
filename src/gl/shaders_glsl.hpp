#pragma once
// All GLSL lives here as inline string literals (C++17 inline variables --
// safe to include from multiple translation units). Embedding rather than
// loading from disk at runtime means the executable has no working-directory
// dependency: it runs the same whether launched from build/, a symlink, or
// anywhere else.
#include <string>

namespace gl::shaders {

inline const char* kMeshVert = R"glsl(
#version 410 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in float aScalar;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

out vec3 vWorldPos;
out vec3 vNormal;
out float vScalar;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = mat3(uModel) * aNormal;
    vScalar = aScalar;
    gl_Position = uProj * uView * worldPos;
}
)glsl";

inline const char* kMeshFrag = R"glsl(
#version 410 core
in vec3 vWorldPos;
in vec3 vNormal;
in float vScalar;
out vec4 FragColor;

uniform vec3 uBaseColor;
uniform vec3 uCameraPos;
// 0 = flat uBaseColor, 1 = diverging (signed) curvature colormap, 2 = sequential heatmap of |curvature|
uniform int uColorMode;
uniform float uScalarAbsMax;
uniform bool uFlatColorMode;
uniform vec3 uFlatColor;

vec3 divergingColormap(float t) {
    t = clamp(t, -1.0, 1.0);
    vec3 neg = vec3(0.20, 0.40, 0.85);
    vec3 mid = vec3(0.93, 0.93, 0.91);
    vec3 pos = vec3(0.85, 0.25, 0.20);
    return t < 0.0 ? mix(mid, neg, -t) : mix(mid, pos, t);
}

// Classic black -> red -> orange -> yellow -> white "heat" ramp. Unlike the
// diverging map above (which encodes the SIGN of curvature), this encodes
// only MAGNITUDE -- t is expected already-unsigned (e.g. |curvature|/max).
vec3 heatmapColormap(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 c0 = vec3(0.02, 0.02, 0.05);
    vec3 c1 = vec3(0.65, 0.06, 0.06);
    vec3 c2 = vec3(0.95, 0.55, 0.05);
    vec3 c3 = vec3(0.98, 0.92, 0.35);
    vec3 c4 = vec3(1.00, 1.00, 0.92);
    if (t < 0.333) return mix(c0, c1, t / 0.333);
    if (t < 0.666) return mix(c1, c2, (t - 0.333) / 0.333);
    if (t < 0.85)  return mix(c2, c3, (t - 0.666) / (0.85 - 0.666));
    return mix(c3, c4, (t - 0.85) / (1.0 - 0.85));
}

void main() {
    if (uFlatColorMode) { FragColor = vec4(uFlatColor, 1.0); return; }

    vec3 N = normalize(vNormal);
    if (!gl_FrontFacing) N = -N; // two-sided shading (open patches: Mobius, pseudosphere, half-plane, ...)
    vec3 L = normalize(vec3(0.5, 0.8, 0.6));
    vec3 V = normalize(uCameraPos - vWorldPos);
    vec3 H = normalize(L + V);

    vec3 base = uBaseColor;
    float m = max(uScalarAbsMax, 1e-6);
    if (uColorMode == 1) base = divergingColormap(vScalar / m);
    else if (uColorMode == 2) base = heatmapColormap(abs(vScalar) / m);

    float ambient = 0.25;
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.35;
    vec3 color = base * (ambient + diff * 0.75) + vec3(spec);
    FragColor = vec4(color, 1.0);
}
)glsl";

inline const char* kLineVert = R"glsl(
#version 410 core
layout(location=0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
void main() { gl_Position = uProj * uView * uModel * vec4(aPos, 1.0); }
)glsl";

inline const char* kLineFrag = R"glsl(
#version 410 core
out vec4 FragColor;
uniform vec3 uColor;
void main() { FragColor = vec4(uColor, 1.0); }
)glsl";

inline const char* kImplicitVert = R"glsl(
#version 410 core
layout(location=0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
out vec3 vWorldPos;
void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    gl_Position = uProj * uView * wp;
}
)glsl";

// %%F_EXPR%% is substituted with the GLSL translation of the user's F(x,y,z)
// formula (see expr::toGLSL). Raymarches with fixed steps + bisection
// refinement on the first sign change (robust for arbitrary F, not just true
// signed-distance fields), and writes gl_FragDepth from the actual hit point
// so it composites correctly with ordinary mesh objects drawn in the same
// scene (front faces are culled at the C++ call site so the cube's back
// faces are what gets rasterized, giving a fragment to march from even when
// the camera is outside the box).
inline const char* kImplicitFragTemplate = R"glsl(
#version 410 core
in vec3 vWorldPos;
out vec4 FragColor;

uniform vec3 uCameraPos;
uniform mat4 uView;
uniform mat4 uProj;
uniform vec3 uBoxMin;
uniform vec3 uBoxMax;
uniform vec3 uBaseColor;

float F(float x, float y, float z) {
    return %%F_EXPR%%;
}

vec2 intersectBox(vec3 ro, vec3 rd, vec3 bmin, vec3 bmax) {
    vec3 invD = 1.0 / rd;
    vec3 t0 = (bmin - ro) * invD;
    vec3 t1 = (bmax - ro) * invD;
    vec3 tsmall = min(t0, t1);
    vec3 tbig = max(t0, t1);
    return vec2(max(max(tsmall.x, tsmall.y), tsmall.z), min(min(tbig.x, tbig.y), tbig.z));
}

vec3 gradF(vec3 p) {
    float h = 0.0015;
    float dx = F(p.x + h, p.y, p.z) - F(p.x - h, p.y, p.z);
    float dy = F(p.x, p.y + h, p.z) - F(p.x, p.y - h, p.z);
    float dz = F(p.x, p.y, p.z + h) - F(p.x, p.y, p.z - h);
    return normalize(vec3(dx, dy, dz) + 1e-8);
}

void main() {
    vec3 ro = uCameraPos;
    vec3 rd = normalize(vWorldPos - uCameraPos);

    vec2 tb = intersectBox(ro, rd, uBoxMin, uBoxMax);
    float tNear = max(tb.x, 0.0);
    float tFar = tb.y;
    if (tNear >= tFar) discard;

    const int STEPS = 256;
    float dt = (tFar - tNear) / float(STEPS);
    float t = tNear;
    float prevF = F(ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t);
    bool hit = false;
    float hitT = 0.0;

    for (int i = 1; i <= STEPS; ++i) {
        float tt = tNear + float(i) * dt;
        vec3 p = ro + rd * tt;
        float f = F(p.x, p.y, p.z);
        if ((prevF < 0.0) != (f < 0.0)) {
            float lo = t, hi = tt, flo = prevF;
            for (int b = 0; b < 24; ++b) {
                float mid = 0.5 * (lo + hi);
                vec3 pm = ro + rd * mid;
                float fm = F(pm.x, pm.y, pm.z);
                if ((flo < 0.0) == (fm < 0.0)) { lo = mid; flo = fm; } else { hi = mid; }
            }
            hitT = 0.5 * (lo + hi);
            hit = true;
            break;
        }
        prevF = f;
        t = tt;
    }
    if (!hit) discard;

    vec3 p = ro + rd * hitT;
    vec3 N = gradF(p);
    if (dot(N, -rd) < 0.0) N = -N;

    vec3 L = normalize(vec3(0.5, 0.8, 0.6));
    vec3 V = normalize(uCameraPos - p);
    vec3 H = normalize(L + V);
    float ambient = 0.25;
    float diff = max(dot(N, L), 0.0);
    float spec = pow(max(dot(N, H), 0.0), 32.0) * 0.35;
    vec3 color = uBaseColor * (ambient + diff * 0.75) + vec3(spec);
    FragColor = vec4(color, 1.0);

    vec4 clipPos = uProj * uView * vec4(p, 1.0);
    float ndcDepth = clipPos.z / clipPos.w;
    gl_FragDepth = 0.5 * ndcDepth + 0.5;
}
)glsl";

inline std::string buildImplicitFragmentShader(const std::string& fExprGLSL) {
    std::string src = kImplicitFragTemplate;
    size_t pos = src.find("%%F_EXPR%%");
    if (pos != std::string::npos) src.replace(pos, std::string("%%F_EXPR%%").size(), fExprGLSL);
    return src;
}

} // namespace gl::shaders
