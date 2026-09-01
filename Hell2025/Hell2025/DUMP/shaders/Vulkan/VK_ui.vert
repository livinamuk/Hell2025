#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/Vulkan/VK_types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsUI data;
} pc;

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

layout(location = 0) out vec2 v_uv;
layout(location = 1) out vec4 v_color;
layout(location = 2) flat out uint v_textureIndex;
layout(location = 3) flat out uint v_filterMode;

out gl_PerVertex {
    vec4 gl_Position;
    float gl_ClipDistance[4];
};

void main() {
    RenderItemUIBuffer renderItems = pc.data.frame.uiRenderItemBuffer;
    RenderItemUI renderItem = renderItems.uiRenderItems[gl_InstanceIndex];

    v_uv = a_uv;
    v_color = a_color;
    v_textureIndex = renderItem.textureIndex;
    v_filterMode = renderItem.filterMode;

    vec4 position = vec4(a_position, 0.0, 1.0);

    float ndcLeft = (renderItem.clipMinX / pc.data.renderTargetWidth) * 2.0 - 1.0;
    float ndcRight = (renderItem.clipMaxX / pc.data.renderTargetWidth) * 2.0 - 1.0;
    float ndcTop = 1.0 - (renderItem.clipMinY / pc.data.renderTargetHeight) * 2.0;
    float ndcBottom = 1.0 - (renderItem.clipMaxY / pc.data.renderTargetHeight) * 2.0;

    vec2 ndc = position.xy / position.w;

    gl_ClipDistance[0] = ndc.x - ndcLeft;
    gl_ClipDistance[1] = ndcRight - ndc.x;
    gl_ClipDistance[2] = ndc.y - ndcBottom;
    gl_ClipDistance[3] = ndcTop - ndc.y;

    gl_Position = position;
}
