#version 460
#extension GL_EXT_nonuniform_qualifier : require

#include "../common/Vulkan/VK_binding_indices.glsl"

layout(set = 0, binding = DESC_IDX_SAMPLERS) uniform sampler samplers[];
layout(set = 0, binding = DESC_IDX_TEXTURES) uniform texture2D textures[];
layout(set = 0, binding = DESC_IDX_TEXTURE_SAMPLERS) uniform sampler textureSamplers[];

layout(location = 0) in vec3 v_worldPosition;
layout(location = 1) flat in vec4 v_lightPositionRadius;
layout(location = 2) in vec2 v_uv;
layout(location = 3) flat in int v_baseColorTextureIndex;

void main() {
    if (v_baseColorTextureIndex >= 0) {
        uint textureIndex = uint(v_baseColorTextureIndex);
        float alpha = texture(sampler2D(textures[nonuniformEXT(textureIndex)], textureSamplers[nonuniformEXT(textureIndex)]), v_uv).a;
        if (alpha < 0.25) discard;
    }

    vec3 lightPosition = v_lightPositionRadius.xyz;
    float lightRadius = v_lightPositionRadius.w;
    gl_FragDepth = length(v_worldPosition - lightPosition) / lightRadius;
}
