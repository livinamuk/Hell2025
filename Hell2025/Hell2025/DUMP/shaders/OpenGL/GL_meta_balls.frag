#version 460
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/types.glsl"
#include "../common/util.glsl"
#include "../common/viewport.glsl"

#define USE_POLY_FALLOFF
#define USE_NOISE

layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 3) out vec4 WorldPositionOut;
layout (location = 4) out vec4 EmissiveOut;

readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportData[]; };
readonly restrict layout(std430, binding = SSBO_IDX_META_BALLS) buffer metaBallBuffer { MetaBall metaballs[]; };

layout (binding = 0) uniform sampler3D u_noiseTexture;

uniform int u_metaBallCount;
uniform mat4 u_projectionView;
uniform mat4 u_model;
uniform float u_stepSize;

const float kThreshold = 0.65;
const int   kMaxSteps  = 8;
const float kPolyScale = 0.33;
const float kNoiseFrequency = 3.0;
const float kNoiseAmplitude = 0.5;

// Ray vs Sphere intersction
bool RaySphere(vec3 ro, vec3 rd, vec3 center, float radius, out float t0, out float t1) {
    vec3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b*b - c;
    if (h < 0.0) return false;
    h = sqrt(h);
    t0 = -b - h;
    t1 = -b + h;
    return t1 > 0.0;
}

// Calculate the total influence of all metaballs at a specific point
float MetaballField(vec3 p) {
    float fieldValue = 0.0;

    for (int i = 0; i < u_metaBallCount; ++i) {
        vec3 position = metaballs[i].posAndInvSigma2.xyz;
        float invSigma2 = metaballs[i].posAndInvSigma2.w;

        // Distance squared
        vec3 d = p - position;
        float d2 = dot(d, d);

        // Uses a cubic polynomial influence curve
        #ifdef USE_POLY_FALLOFF
            float r2_over_R2 = d2 * invSigma2 * kPolyScale;
            float term = max(0.0, 1.0 - r2_over_R2);
            fieldValue += (term * term * term);
        // Uses a exponential influence curve
        #else
            fieldValue += exp(-d2 * invSigma2);
        #endif
    }

    // Noisey influence
    #ifdef USE_NOISE
        float noiseVal = texture(u_noiseTexture, p * kNoiseFrequency).r;
        fieldValue -= noiseVal * kNoiseAmplitude;
    #endif

    // Noiseless influence
    return fieldValue;
}

vec3 NormalFromHitPosition(vec3 hitPosition) {
    // Central difference via 3D texture
    #ifdef USE_NOISE
        vec2 e = vec2(0.01, 0.0);
        float nx = MetaballField(hitPosition + e.xyy) - MetaballField(hitPosition - e.xyy);
        float ny = MetaballField(hitPosition + e.yxy) - MetaballField(hitPosition - e.yxy);
        float nz = MetaballField(hitPosition + e.yyx) - MetaballField(hitPosition - e.yyx);
        return normalize(vec3(-nx, -ny, -nz));

    // Analytical gradient for smooth metaballs
    #else
        vec3 gradient = vec3(0.0);
        for (int i = 0; i < u_metaBallCount; ++i) {
            vec3 position = metaballs[i].posAndInvSigma2.xyz;
            float invSigma2 = metaballs[i].posAndInvSigma2.w;

            // Distance squared
            vec3 d = hitPosition - position;
            float d2 = dot(d, d);

            // Polynomial gradient contribution
            #ifdef USE_POLY_FALLOFF
                float r2_over_R2 = d2 * invSigma2 * kPolyScale;
                float term = max(0.0, 1.0 - r2_over_R2);
                gradient += (-6.0 * term * term * invSigma2 * kPolyScale) * d;

            // Exponential gradient contribution
            #else
                float w = exp(-d2 * invSigma2);
                gradient += (-2.0 * invSigma2) * d * w;
            #endif
        }
        return normalize(-gradient);
    #endif
}

void main() {
    ivec2 pixelCoords = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uint viewportIndex = ViewportIndexFromPixel(pixelCoords, resolution, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));
    ivec4 viewportRect = ivec4(viewportData[viewportIndex].xOffset, viewportData[viewportIndex].yOffset, viewportData[viewportIndex].width, viewportData[viewportIndex].height);
    mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;
    mat4 inverseProjectionView = viewportData[viewportIndex].inverseJitteredProjectionViewReverseZ;
    vec3 viewPos = viewportData[viewportIndex].viewPos.xyz;

    vec3 rayOrigin = viewPos;
    vec3 rayDir = WorldRayFromPixel(pixelCoords, resolution, viewportRect, viewPos, inverseProjectionView);

    vec3 proxyCenter = u_model[3].xyz;
    float proxyRadius = length(u_model[0].xyz);

    // Calculate entry and exit points for the current proxy sphere volume
    float tStart, tStop;
    if (!RaySphere(rayOrigin, rayDir, proxyCenter, proxyRadius, tStart, tStop)) discard;

    tStart = max(tStart, 0.0);
    if (tStop <= tStart) discard;

    bool hit = false;
    float tHit = 0.0;
    float t = tStart;
    float previousFieldValue = MetaballField(rayOrigin + rayDir * t);
    float previousT = t;

    vec3 p = rayOrigin + rayDir * tStart;
    vec3 stepVec = rayDir * u_stepSize;

    // Ray march through the proxy volume to find the isosurface threshold
    for (int i = 0; i < kMaxSteps; ++i) {
        t += u_stepSize;
        p += stepVec;
        if (t > tStop) break;

        float fieldValue = MetaballField(p);

        if (previousFieldValue < kThreshold && fieldValue >= kThreshold) {
            float fraction = (kThreshold - previousFieldValue) / (fieldValue - previousFieldValue); // Linearly interpolate to find a more accurate hit surface
            tHit = previousT + fraction * u_stepSize;
            hit = true;
            break;
        }
        previousFieldValue = fieldValue;
        previousT = t;
    }

    // Bail if the ray never reached the threshold density
    if (!hit) discard;

    vec3 hitWorldPosition = rayOrigin + rayDir * tHit;

    // Transform hit position to clip space
    vec4 clip = projectionView * vec4(hitWorldPosition, 1.0);
    if (clip.w <= 0.0) discard;

    // Manually write the calculated depth to the depth buffer
    gl_FragDepth = clamp(clip.z / clip.w, 0.0, 1.0);

    // Write to the GBuffer
    vec3 normal = NormalFromHitPosition(hitWorldPosition);

    BaseColorOut = vec4(1.0, 0.0, 0.0, 1.0);
    NormalOut = vec4(normal, 1.0);
    RMAOut = vec4(0.05, 0.25, 1.0, 0.0);
    WorldPositionOut = vec4(hitWorldPosition, 1.0);
    EmissiveOut = vec4(0.0, 0.0, 0.0, 1.0);

    // BaseColorOut = vec4(0.4, 0.0, 0.0, 1.0);
    //
    // float fresnel = 1.0 - clamp(dot(normal, -rayDir), 0.0, 1.0);
    // float roughness = mix(0.15, 0.02, pow(fresnel, 5.0)); // Sharper at edges
    // RMAOut = vec4(roughness, 0.0, 1.0, 1.0);
}
