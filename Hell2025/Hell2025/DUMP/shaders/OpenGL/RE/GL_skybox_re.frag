#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"
#include "../../common/util.glsl"
#include "../../common/viewport.glsl"

layout (location = 0) out vec4 FinalLightingOut;

layout (binding = 0) uniform samplerCube u_skyboxCubeMap;

readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

mat3 GetSkyboxRotationMatrix() {
    float angle = radians(-90.0);
    float c = cos(angle);
    float s = sin(angle);
    return mat3(c, 0.0, s, 0.0, 1.0, 0.0, -s, 0.0, c );
}

void main() {
    ivec2 px = ivec2(gl_FragCoord.xy);
    ivec2 resolution = ivec2(rendererData.gBufferWidth, rendererData.gBufferHeight);

    uint viewportIndex = ViewportIndexFromPixel(px, resolution, rendererData.viewportLayout, vec2(rendererData.viewportSplitX, rendererData.viewportSplitY));

    // Get world ray
    ivec4 viewportRect = ivec4(viewportDataArr[viewportIndex].xOffset, viewportDataArr[viewportIndex].yOffset, viewportDataArr[viewportIndex].width, viewportDataArr[viewportIndex].height);
    vec3 viewPosition = viewportDataArr[viewportIndex].viewPos.xyz;
    mat4 inverseProjectionView = viewportDataArr[viewportIndex].inverseJitteredProjectionViewReverseZ;
    vec3 rayDir = WorldRayFromPixel(px, resolution, viewportRect, viewPosition, inverseProjectionView);

    // Sample skybox cubemap
    mat3 skyboxRotation = GetSkyboxRotationMatrix();
    vec3 skyboxSampleDir = normalize(skyboxRotation * rayDir);
    vec3 skyColor = texture(u_skyboxCubeMap, skyboxSampleDir).rgb;
    vec3 skyLinear = pow(skyColor, vec3(2.2));

    // Fade some color in based on view dir
    vec3 horizonColor = vec3(0.6, 0.2, 0.6);
    vec3 downColor = vec3(0.4);
    float amount = 0.02;
    float colorCurve = 0.95;
    float fadeCurve = 0.69;
    float downwardness = clamp(-rayDir.y, 0.0, 1.0);
    float colorT = pow(downwardness, colorCurve);
    float fogT = pow(downwardness, fadeCurve);
    vec3 rayFogColor = mix(horizonColor, downColor, colorT) * amount;
    vec3 outColor = mix(skyLinear, rayFogColor, fogT);

    // outColor *= 0.75;

    // Write output
    FinalLightingOut = vec4(outColor, 1.0);
}
