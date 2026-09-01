#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

layout(location = 0) in vec2 a_position;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec4 a_color;

out vec4 v_color;
out vec2 v_uv;
out flat uint v_filterMode;
out flat uint v_textureIndex;

uniform float u_renderTargetWidth;
uniform float u_renderTargetHeight;
uniform int u_flipY;

struct RenderItemUI {
    uint baseVertex;
    uint baseIndex;
    uint indexCount;
    uint textureIndex;

    int clipMinX;
    int clipMinY;
    int clipMaxX;
    int clipMaxY;

    uint filterMode; // 0 for linear, 1 for nearest
    int padding0;
    int padding1;
    int padding2;
};

layout(std430, binding = SSBO_IDX_UI_INSTANCE_DATA) readonly buffer RenderItems {
    RenderItemUI renderItems[];
};

void main() {
    RenderItemUI renderItem = renderItems[gl_BaseInstance];

    v_color = a_color;
    v_filterMode = renderItem.filterMode;
    v_textureIndex = renderItem.textureIndex;
    v_uv = a_uv;

    vec4 position = vec4(a_position, 0.0, 1.0);
    if (u_flipY != 0) position.y = -position.y;

    float ndcLeft   = (renderItem.clipMinX / u_renderTargetWidth)  * 2.0 - 1.0;
    float ndcRight  = (renderItem.clipMaxX / u_renderTargetWidth)  * 2.0 - 1.0;
    float ndcTop    = 1.0 - (renderItem.clipMinY / u_renderTargetHeight) * 2.0;
    float ndcBottom = 1.0 - (renderItem.clipMaxY / u_renderTargetHeight) * 2.0;

    if (u_flipY != 0) {
        float flippedTop = -ndcBottom;
        ndcBottom = -ndcTop;
        ndcTop = flippedTop;
    }

    vec2 ndc = position.xy / position.w;

    gl_ClipDistance[0] = ndc.x - ndcLeft;    // left
    gl_ClipDistance[1] = ndcRight - ndc.x;   // right
    gl_ClipDistance[2] = ndc.y - ndcBottom;  // bottom
    gl_ClipDistance[3] = ndcTop - ndc.y;     // top

    gl_Position = position;
}
