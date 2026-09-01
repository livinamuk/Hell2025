#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable
#include "../../common/normal_encoding.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 EmissiveOut;

readonly restrict layout(std430, binding = SSBO_IDX_SAMPLERS) buffer textureSamplersBuffer { uvec2 textureSamplers[]; };
readonly restrict layout(std430, binding = SSBO_IDX_MATERIALS) buffer materialsBuffer { Material materials[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer renderItemsBuffer { RenderItem renderItems[]; };

in flat int v_globalInstanceIndex;

in vec2 v_uv;

void main() {
    RenderItem renderItem = renderItems[v_globalInstanceIndex];
    Material material = materials[renderItem.materialIndex];
    vec3 emissiveColor = vec3(renderItem.emissiveR, renderItem.emissiveG, renderItem.emissiveB);

    if (material.emissive != -1) {
        emissiveColor *= texture(sampler2D(textureSamplers[material.emissive]), v_uv).rgb;
    }
   
    EmissiveOut = vec4(emissiveColor, 1.0);
}
