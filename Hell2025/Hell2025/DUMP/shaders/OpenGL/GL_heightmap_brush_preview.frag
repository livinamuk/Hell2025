#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/reconstruction.glsl"
#include "../common/viewport.glsl"

layout(binding = 0) uniform sampler2D u_depthTexture;
layout(binding = 1) uniform sampler2D u_heightMapTexture;

layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) readonly restrict buffer viewportDataBuffer {
    ViewportData viewportData[];
};

uniform int u_viewportIndex;
uniform vec3 u_brushPosition;
uniform float u_brushRadius;
uniform bool u_validateTerrainHeight;

layout(location = 0) out vec4 outColor;

void main() {
    ivec2 pixel = ivec2(gl_FragCoord.xy);
    ivec2 gBufferSize = textureSize(u_depthTexture, 0);
    float depth = texelFetch(u_depthTexture, pixel, 0).r;
    if (depth <= 0.0) discard;

    ivec4 viewportRect = ivec4(viewportData[u_viewportIndex].xOffset, viewportData[u_viewportIndex].yOffset, viewportData[u_viewportIndex].width, viewportData[u_viewportIndex].height);
    vec2 viewportUV = ViewportUVFromPixel(pixel, gBufferSize, viewportRect);
    vec3 worldPosition = WorldPosFromDepth(viewportUV, depth, viewportData[u_viewportIndex].inverseJitteredProjectionViewReverseZ);

    if (u_validateTerrainHeight) {
        vec2 heightMapWorldSize = vec2(textureSize(u_heightMapTexture, 0)) * HEIGHTMAP_SCALE_XZ;
        vec2 heightMapUV = worldPosition.xz / heightMapWorldSize;
        if (any(lessThan(heightMapUV, vec2(0.0))) || any(greaterThan(heightMapUV, vec2(1.0)))) discard;
        float terrainHeight = texture(u_heightMapTexture, heightMapUV).r * HEIGHTMAP_SCALE_Y;
        if (abs(worldPosition.y - terrainHeight) > HEIGHTMAP_SCALE_XZ) discard;
    }

    float brushDistance = length(worldPosition.xz - u_brushPosition.xz);
    float antiAliasWidth = max(fwidth(brushDistance), 0.001);
    float ringDistance = abs(brushDistance - u_brushRadius);
    float ring = 1.0 - smoothstep(antiAliasWidth, antiAliasWidth * 2.0, ringDistance);
    float fill = (1.0 - smoothstep(u_brushRadius - antiAliasWidth, u_brushRadius + antiAliasWidth, brushDistance)) * 0.08;
    float alpha = max(ring * 0.9, fill);
    if (alpha <= 0.0) discard;

    outColor = vec4(1.0, 0.8, 0.05, alpha);
}
