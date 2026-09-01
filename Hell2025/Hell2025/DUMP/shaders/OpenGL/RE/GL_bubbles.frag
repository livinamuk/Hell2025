#version 460
#extension GL_ARB_bindless_texture : enable
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/lighting.glsl"
#include "../../common/types.glsl"

layout(location = 0) out vec4 ColorOut;
in vec2 v_uv;
uniform mat4 u_view;
uniform float u_time;

layout (binding = 0) uniform samplerCube cubeMap;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SPOT_LIGHTS) buffer spotLightsBuffer { SpotLight spotLights[]; };

// -90 degrees y rotation
const mat3 kRotateYMinus90 = mat3(
    0.0, 0.0, -1.0,
    0.0, 1.0,  0.0,
    1.0, 0.0,  0.0
);

vec3 mod289(vec3 x){ return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x){ return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x){ return mod289(((x * 34.0) + 10.0) * x); }
vec4 taylorInvSqrt(vec4 r){ return 1.79284291400159 - 0.85373472095314 * r; }

float snoise(vec3 v) {
    const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
    const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
    vec3 i  = floor(v + dot(v, C.yyy));
    vec3 x0 = v - i + dot(i, C.xxx);
    vec3 g  = step(x0.yzx, x0.xyz);
    vec3 l  = 1.0 - g;
    vec3 i1 = min(g.xyz, l.zxy);
    vec3 i2 = max(g.xyz, l.zxy);
    vec3 x1 = x0 - i1 + C.xxx;
    vec3 x2 = x0 - i2 + C.yyy;
    vec3 x3 = x0 - D.yyy;
    i = mod289(i);
    vec4 p = permute(permute(permute(
             i.z + vec4(0.0, i1.z, i2.z, 1.0))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0))
           + i.x + vec4(0.0, i1.x, i2.x, 1.0));
    float n_ = 0.142857142857;
    vec3  ns = n_ * D.wyz - D.xzx;
    vec4  j  = p - 49.0 * floor(p * ns.z * ns.z);
    vec4  x_ = floor(j * ns.z);
    vec4  y_ = floor(j - 7.0 * x_);
    vec4  x  = x_ * ns.x + vec4(ns.y);
    vec4  y  = y_ * ns.x + vec4(ns.y);
    vec4  h  = 1.0 - abs(x) - abs(y);
    vec4 b0  = vec4(x.xy, y.xy);
    vec4 b1  = vec4(x.zw, y.zw);
    vec4 s0  = floor(b0) * 2.0 + 1.0;
    vec4 s1  = floor(b1) * 2.0 + 1.0;
    vec4 sh  = -step(h, vec4(0.0));
    vec4 a0  = b0.xzyw + s0.xzyw * sh.xxyy;
    vec4 a1  = b1.xzyw + s1.xzyw * sh.zzww;
    vec3 p0  = vec3(a0.xy, h.x);
    vec3 p1  = vec3(a0.zw, h.y);
    vec3 p2  = vec3(a1.xy, h.z);
    vec3 p3  = vec3(a1.zw, h.w);
    vec4 norm = taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x; p1 *= norm.y; p2 *= norm.z; p3 *= norm.w;
    vec4 m = max(0.6 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 42.0 * dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}

vec3 bump3y(vec3 x, vec3 yo){ return clamp(1.0 - x * x - yo, 0.0, 1.0); }

vec3 spectral(float w) {
    float x = clamp((w - 400.0) / 300.0, 0.0, 1.0);
    const vec3 c1 = vec3(3.54585104, 2.93225262, 2.41593945);
    const vec3 x1 = vec3(0.69549072, 0.49228336, 0.27699880);
    const vec3 y1 = vec3(0.02312639, 0.15225084, 0.52607955);
    const vec3 c2 = vec3(3.90307140, 3.21182957, 3.96587128);
    const vec3 x2 = vec3(0.11748627, 0.86755042, 0.66077860);
    const vec3 y2 = vec3(0.84897130, 0.88445281, 0.73949448);
    return bump3y(c1 * (x - x1), y1) + bump3y(c2 * (x - x2), y2);
}

vec3 thinFilm(float d_nm, float cosTheta) {
    const float n = 1.33;
    float sinT2 = (1.0 - cosTheta * cosTheta) / (n * n);
    float cosT  = sqrt(max(0.0, 1.0 - sinT2));
    vec3 col = vec3(0.0);
    for (int i = 0; i < 8; i++) {
        float lambda = 400.0 + float(i) * 42.857;
        float opd    = 2.0 * n * d_nm * cosT;
        float phase  = 6.28318530718 * opd / lambda + 3.14159265359;
        float R      = 0.5 * (1.0 - cos(phase));
        col += spectral(lambda) * R;
    }
    return col * 0.1;
}

vec3 sampleEnvironment(vec3 dir) {
    vec3 dir_rotated = kRotateYMinus90 * dir;
    vec3 env = texture(cubeMap, dir_rotated).rgb;
    return pow(env, vec3(2.2));
}

vec3 refractedEnv(vec3 rd, vec3 n, float ior) {
    vec3 r  = refract(rd, n, 1.0 / ior);         if (dot(r, r) < 0.01) r  = reflect(rd, n);
    vec3 rR = refract(rd, n, 1.0 / (ior - 0.018)); if (dot(rR, rR) < 0.01) rR = r;
    vec3 rB = refract(rd, n, 1.0 / (ior + 0.018)); if (dot(rB, rB) < 0.01) rB = r;
    
    return vec3(sampleEnvironment(rR).r, sampleEnvironment(r).g, sampleEnvironment(rB).b);
}

float filmThickness(vec3 p, vec3 center, float radius) {
    return 0;
    float localY = (p.y - center.y) / radius;
    float g      = clamp((-localY + 1.0) * 0.5, 0.0, 1.0);
    float base   = mix(150.0, 700.0, pow(g, 1.5)) * 1.0;
    
    vec3 localPos = (p - center) / radius;
    float slosh  = snoise(localPos * 1.05 + u_time * 0.7) * 55.0
                 + snoise(localPos * 2.1 - u_time * 1.1) * 22.0;
                 
    return clamp(base + slosh, 60.0, 950.0);
}

void main() {
    vec2 p = v_uv * 2.0 - 1.0;
    float r2 = dot(p, p);
    if (r2 > 1.0) {
        discard;
    }
    
    float z = sqrt(1.0 - r2);

    vec3 cameraRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
    vec3 cameraUp    = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
    vec3 cameraBack  = vec3(u_view[0][2], u_view[1][2], u_view[2][2]);

    vec3 n = normalize(p.x * cameraRight + p.y * cameraUp + z * cameraBack);
    vec3 vd = cameraBack;

    vec3 center = vec3(36.0, 32.5, 37.0);
    float radius = 0.1;
    vec3 worldPos = center + n * radius; 

    float cos0 = max(dot(n, vd), 0.0);
    float fresnel = pow(1.0 - cos0, 3.0);

    float thick = filmThickness(worldPos, center, radius);
    vec3 irid = thinFilm(thick, cos0);

    vec3 reflCol = sampleEnvironment(reflect(-vd, n));
    vec3 refrCol = refractedEnv(-vd, n, 1.33);

    vec3 totalSpecular = vec3(0.0);
    float totalOpacity = 0;

    vec3 L_moon = normalize(rendererData.moonLightDir.xyz);
    vec3 H_moon = normalize(L_moon + vd);
    float moonSpec = pow(max(dot(n, H_moon), 0.0), 512.0) * 5.0 * rendererData.moonLightColorStrength.a;
    
    totalSpecular += rendererData.moonLightColorStrength.rgb * moonSpec;
    totalOpacity += moonSpec * 0.4;
    totalOpacity += fresnel * 0.75 + length(irid) * 0.12; // Frensel

    //totalSpecular = vec3(0);
    //totalOpacity = 0;

    sampler2D flashlightIES = sampler2D(textureSamplers[max(rendererData.flashlightIESTextureIndex, 0)]);
    for (uint i = 0u; i < rendererData.spotLightCount; i++) {
            SpotLight spotLight = spotLights[i];
            vec3 spotLightPos = spotLight.positionModifier.xyz;
            vec3 L_flash = normalize(spotLightPos - worldPos);
            vec3 H_flash = normalize(L_flash + vd);
            float flashSpec = pow(max(dot(n, H_flash), 0.0), 512.0) * 5.0;
            float attenuation = GetSpotLightAttenuation(spotLight, rendererData, worldPos, flashlightIES);

            totalSpecular += rendererData.flashlightColor.rgb * flashSpec * attenuation;
            totalOpacity += flashSpec * 0.4 * attenuation;
            totalOpacity += fresnel * 0.75 + length(irid) * 0.12;
    }

    vec3 surface = refrCol * (1.0 - fresnel)
                 + reflCol * fresnel
                 + irid * 0.55
                 + totalSpecular;

    float opacity = clamp(totalOpacity, 0.03, 0.90);

    vec3 color = surface / (surface + 0.5) * 1.4;
    color = pow(max(color, 0.0), vec3(0.92));

    opacity *= 0.25;
    
    ColorOut = vec4(color, opacity);
}
