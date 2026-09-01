#version 460
#extension GL_EXT_buffer_reference2 : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_ARB_shader_draw_parameters : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : require

#include "../common/constants.glsl"
#include "../common/types.glsl"
#include "../common/Vulkan/VK_push_constants.glsl"

layout(push_constant, scalar) uniform PushConstants {
    PushConstantsSpriteSheet data;
} pc;

layout(location = 0) in vec3 a_position;
layout(location = 2) in vec2 a_uv;

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec2 v_texCoordNext;
layout(location = 2) flat out int v_textureIndex;
layout(location = 3) flat out float v_mixFactor;

void main() {
    ViewportDataBuffer viewportData = pc.data.frame.viewportDataBuffer;
    SpriteSheetRenderItemBuffer spriteSheetRenderItems = pc.data.frame.spriteSheetRenderItemBuffer;

    uint viewportIndex = pc.data.viewportIndex;
    uint globalInstanceIndex = uint(gl_InstanceIndex);

    SpriteSheetRenderItem renderItem = spriteSheetRenderItems.spriteSheetRenderItems[globalInstanceIndex];

    vec2 uv = a_uv;
    uv.y = 1.0 - uv.y;

    v_texCoord = renderItem.uvFrame.xy + uv * renderItem.uvFrame.zw;
    v_texCoordNext = renderItem.uvFrameNext.xy + uv * renderItem.uvFrameNext.zw;
    v_textureIndex = renderItem.textureIndex;
    v_mixFactor = renderItem.mixFactor;

    mat4 projectionView = viewportData.viewportData[viewportIndex].projectionViewReverseZ;
    mat4 inverseView = viewportData.viewportData[viewportIndex].inverseView;
    mat4 modelMatrix = renderItem.modelMatrix;

    if (renderItem.isBillboard != 0) {
        vec3 worldPosition = modelMatrix[3].xyz;
        mat4 localMatrix = modelMatrix;
        localMatrix[3] = vec4(0.0, 0.0, 0.0, 1.0);

        vec3 cameraRight = normalize(inverseView[0].xyz);
        vec3 cameraUp = normalize(inverseView[1].xyz);
        vec3 cameraForward = normalize(-inverseView[2].xyz);

        mat4 billboardMatrix = mat4(1.0);
        billboardMatrix[0] = vec4(cameraRight, 0.0);
        billboardMatrix[1] = vec4(cameraUp, 0.0);
        billboardMatrix[2] = vec4(cameraForward, 0.0);
        billboardMatrix[3] = vec4(worldPosition, 1.0);

        modelMatrix = billboardMatrix * localMatrix;
    }

    vec3 localPosition = a_position + renderItem.localOffset.xyz;
    vec4 worldPos = modelMatrix * vec4(localPosition, 1.0);
    gl_Position = projectionView * worldPos;
}
