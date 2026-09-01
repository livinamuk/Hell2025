#version 460 core

#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"

layout(location = 0) in vec3 a_position;

layout(binding = 0) uniform sampler2D u_decalTexture;

layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) readonly restrict buffer viewportDataBuffer {
    ViewportData viewportData[];
};

uniform int u_viewportIndex;
uniform mat4 u_modelMatrix;

void main() {
    const float decalDepthScale = 0.2;

    vec2 decalTextureSize = vec2(textureSize(u_decalTexture, 0));
    float shortestTextureSide = max(min(decalTextureSize.x, decalTextureSize.y), 1.0);
    vec2 decalAspectScale = decalTextureSize / shortestTextureSide;

    vec3 localPosition = a_position;
    localPosition.xy *= decalAspectScale;
    localPosition.z *= decalDepthScale;

    mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
    gl_Position = projectionView * u_modelMatrix * vec4(localPosition, 1.0);
}
