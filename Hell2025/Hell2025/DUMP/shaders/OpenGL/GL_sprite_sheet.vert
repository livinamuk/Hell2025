#version 460 core
#include "../common/types.glsl"
#include "../common/constants.glsl"
#include "../common/OpenGL/GL_binding_indices.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vTexCoord;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

readonly restrict layout(std430, binding = SSBO_IDX_SPRITE_SHEET_INSTANCE_DATA) buffer spriteSheetRenderItemsBuffer {
    SpriteSheetRenderItem spriteSheetRenderItems[];
};

out vec2 TexCoord;
out vec2 TexCoordNext;
flat out int TextureIndex;
flat out float MixFactor;

uniform int u_viewportIndex;

void main() {
    int viewportIndex = u_viewportIndex;
    int globalInstanceIndex = gl_BaseInstance + gl_InstanceID;

    SpriteSheetRenderItem renderItem = spriteSheetRenderItems[globalInstanceIndex];

    // correct flipped uvs in quad model
    vec2 uv = vTexCoord;
    uv.y = 1.0 - uv.y;

    TexCoord = renderItem.uvFrame.xy + uv * renderItem.uvFrame.zw;
    TexCoordNext = renderItem.uvFrameNext.xy + uv * renderItem.uvFrameNext.zw;
    TextureIndex = renderItem.textureIndex;
    MixFactor = renderItem.mixFactor;

	mat4 projectionView = viewportData[viewportIndex].jitteredProjectionViewReverseZ;
	mat4 inverseView = viewportData[viewportIndex].inverseView;

    mat4 modelMatrix = renderItem.modelMatrix;

    if (renderItem.isBillboard != 0) {
        vec3 worldPosition = modelMatrix[3].xyz;
        mat4 localMatrix = modelMatrix;
        localMatrix[3] = vec4(0.0, 0.0, 0.0, 1.0);

        // Camera basis vectors
        vec3 cameraRight = normalize(inverseView[0].xyz);
        vec3 cameraUp = normalize(inverseView[1].xyz);
        vec3 cameraForward = normalize(-inverseView[2].xyz);

        // Construct the billboard matrix
        mat4 billboardMatrix = mat4(1.0);
        billboardMatrix[0] = vec4(cameraRight, 0.0);
        billboardMatrix[1] = vec4(cameraUp, 0.0);
        billboardMatrix[2] = vec4(cameraForward, 0.0);
        billboardMatrix[3] = vec4(worldPosition, 1.0);

        modelMatrix = billboardMatrix * localMatrix;
    }

    // Apply offset
    vec3 localPosition = vPosition + renderItem.localOffset.xyz;
    vec4 worldPos = modelMatrix * vec4(localPosition, 1.0);

    // Final position
    gl_Position = projectionView * worldPos;
}
