#version 460

layout(location = 0) out uvec2 VisBufferOut;

layout(location = 0) flat in int v_sceneRenderItemIndex;
layout(location = 1) in vec2 v_baseUv;
layout(location = 2) in vec3 v_worldPosition;

uniform float u_discardHeight = 0.01;

void main() {
    if (v_worldPosition.y < u_discardHeight) {
        discard;
    }

    // Height-map pixels are resolved by their dedicated stencil-selected pass,
    // so this payload does not need the generic scene-item/primitive pair.
    // Keep both terrain coordinates at full float precision: packing them into
    // one UNORM16x2 word quantizes the derivatives used for textureGrad.
    VisBufferOut = uvec2(floatBitsToUint(v_baseUv.x), floatBitsToUint(v_baseUv.y));
}
