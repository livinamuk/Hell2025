#version 460 core

#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"

layout(binding = 0) uniform sampler2D u_scratchTexture;

layout(location = 0) out vec4 outColor;

#define FXAA_REDUCE_MIN (1.0 / 128.0)
#define FXAA_REDUCE_MUL (1.0 / 8.0)
#define FXAA_SPAN_MAX   8.0

readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };

uint FilmGrainHash(uint value) {
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

float FilmGrain(ivec2 px, float time) {
    uint frame = uint(floor(time * 15.0));
    uint seed = uint(px.x) + uint(px.y) * 0x9e3779b9u + frame * 0x85ebca6bu;

    // Use the upper 24 bits so conversion to float is exact.
    return float(FilmGrainHash(seed) >> 8) * (1.0 / 16777216.0);
}

vec3 FilmNoise(vec3 inputColor, ivec2 px) {
    float noise = FilmGrain(px, rendererData.time);
    return inputColor + (0.5 - noise) * 0.0175;
}

vec4 Fxaa(sampler2D tex, vec2 uv, vec2 texelSize) {
    vec3 rgbNW = texture(tex, uv + vec2(-1.0, -1.0) * texelSize).rgb;
    vec3 rgbNE = texture(tex, uv + vec2( 1.0, -1.0) * texelSize).rgb;
    vec3 rgbSW = texture(tex, uv + vec2(-1.0,  1.0) * texelSize).rgb;
    vec3 rgbSE = texture(tex, uv + vec2( 1.0,  1.0) * texelSize).rgb;
    vec4 texColor = texture(tex, uv);

    const vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM = dot(texColor.rgb, luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    vec2 dir;
    dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
    dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

    float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-FXAA_SPAN_MAX), vec2(FXAA_SPAN_MAX)) * texelSize;

    vec3 rgbA = 0.5 * (
        texture(tex, uv + dir * (1.0 / 3.0 - 0.5)).rgb +
        texture(tex, uv + dir * (2.0 / 3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(tex, uv + dir * -0.5).rgb +
        texture(tex, uv + dir *  0.5).rgb);

    float lumaB = dot(rgbB, luma);
    vec3 color = ((lumaB < lumaMin) || (lumaB > lumaMax)) ? rgbA : rgbB;
    return vec4(color, texColor.a);
}

void main() {
    vec2 textureSizePx = vec2(textureSize(u_scratchTexture, 0));
    vec2 texelSize = 1.0 / textureSizePx;
    vec2 uv = gl_FragCoord.xy * texelSize;
    
    vec4 finalColor = Fxaa(u_scratchTexture, uv, texelSize);

    // The image is linearly downscaled by 2 after this pass 
    // Sharing one grain value across each 2x2 block prevents that blit averaging it away
    ivec2 grainPx = ivec2(gl_FragCoord.xy) / 2;
    //finalColor.rgb = FilmNoise(finalColor.rgb, grainPx);

    outColor = finalColor;
}
