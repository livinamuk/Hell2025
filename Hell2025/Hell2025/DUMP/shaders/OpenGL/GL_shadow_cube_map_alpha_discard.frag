#version 460 core
#extension GL_ARB_bindless_texture : require

#include "../common/types.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) flat in vec4 v_lightPositionRadius;
layout(location = 2) in vec2 v_uv;
layout(location = 3) flat in int v_sceneRenderItemIndex;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer {
    uvec2 textureSamplers[];
};

readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer {
    Material materials[];
};

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer {
    RenderItem sceneRenderItems[];
};

void main() {
    RenderItem renderItem = sceneRenderItems[v_sceneRenderItemIndex];
    Material material = materials[renderItem.materialIndex];
    float alpha = texture(sampler2D(textureSamplers[material.basecolor]), v_uv).a;
    if (alpha < 0.25) discard;

    gl_FragDepth = length(v_worldPosition - v_lightPositionRadius.xyz) / v_lightPositionRadius.w;
}
