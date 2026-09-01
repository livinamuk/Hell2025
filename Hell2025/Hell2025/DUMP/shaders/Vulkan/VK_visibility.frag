#version 460

layout(location = 0) flat in uint v_sceneRenderItemIndex;
layout(location = 0) out uvec2 out_visibility;

void main() {
    out_visibility = uvec2(v_sceneRenderItemIndex, uint(gl_PrimitiveID));
}
