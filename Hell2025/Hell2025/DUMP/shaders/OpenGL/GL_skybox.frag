#version 460
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/types.glsl"
#include "../common/viewport.glsl"

layout (location = 0) out vec4 FinalLightingOut;

layout (binding = 0) uniform samplerCube cubeMap;

in vec3 TexCoords;
in flat int ViewportIndex;

readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer {
    RendererData rendererData;
};

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
    ViewportData viewportData[];
};

void main() {
    vec3 skyColor = texture(cubeMap, TexCoords).rgb;
    vec3 skyLinear = pow(skyColor, vec3(2.6));

    ivec2 pixelCoords = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);
    ivec4 viewportRect = ivec4(viewportData[ViewportIndex].xOffset, viewportData[ViewportIndex].yOffset, viewportData[ViewportIndex].width, viewportData[ViewportIndex].height);
    vec3 viewPosition = viewportData[ViewportIndex].viewPos.xyz;
    mat4 inverseProjectionView = viewportData[ViewportIndex].inverseJitteredProjectionViewReverseZ;
    vec3 rayDir = WorldRayFromPixel(pixelCoords, resolution, viewportRect, viewPosition, inverseProjectionView);

    vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    vec3 downColor = vec3(0.4);

    float amount = 0.02;
    float colorCurve = 0.5;
    float fadeCurve = 0.9;

    float downwardness = clamp(-rayDir.y, 0.0, 1.0);
    float colorT = pow(downwardness, colorCurve);
    float fogT = pow(downwardness, fadeCurve);

    vec3 rayFogColor = mix(horizonColor, downColor, colorT) * amount;
    vec3 outColor = mix(skyLinear, rayFogColor, fogT);

    FinalLightingOut = vec4(outColor, 1.0);
}
