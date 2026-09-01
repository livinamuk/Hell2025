#version 460

#include "../../common/constants.glsl"
#include "../../common/types.glsl"
#include "../../common/OpenGL/GL_binding_indices.glsl"

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 tc_controlPointPosition[];
layout(location = 1) flat in int tc_sceneRenderItemIndex[];

layout(location = 0) flat out int v_sceneRenderItemIndex;
layout(location = 1) out vec2 v_baseUv;
layout(location = 2) out vec3 v_worldPosition;

layout(binding = 5) uniform sampler2D u_HeightMapTexture;
layout(binding = 6) uniform sampler2DArray u_DisplacementBuffer;

readonly restrict layout(std430, binding = SSBO_IDX_SCENE_RENDER_ITEMS) buffer sceneRenderItemsBuffer { RenderItem sceneRenderItems[]; };
readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };

uniform int u_viewportIndex;

vec3 SampleTerrainDisplacement(vec2 controlPosition, float density) {
#if TERRAIN_DISPLACEMENT_ENABLED
    // Direct GLSL port of Terrain3D's get_displacement coordinate transform.
    float level = floor(log2(density));
    float sampleScale = exp2(level) * 0.5;
    vec2 target = viewportDataArr[u_viewportIndex].viewPos.xz / HEIGHTMAP_SCALE_XZ;
    vec2 displacementUv =
        (sampleScale * controlPosition - round(target * sampleScale)) /
        (TERRAIN_DISPLACEMENT_MESH_SIZE * 2.0) + 0.5;
    displacementUv.x += level - 1.0;
    displacementUv.x /= float(TERRAIN_DISPLACEMENT_TESSELLATION_LEVEL);

    if (all(greaterThanEqual(displacementUv, vec2(0.0))) && all(lessThanEqual(displacementUv, vec2(1.0)))) {
        return (textureLod(u_DisplacementBuffer, vec3(displacementUv, float(u_viewportIndex)), 0.0).rgb * 2.0 - 1.0) * TERRAIN_DISPLACEMENT_SCALE;
    }
#endif
    return vec3(0.0);
}

vec3 GetTerrainDisplacement(vec2 controlPosition) {
#if TERRAIN_DISPLACEMENT_ENABLED
    vec2 target = viewportDataArr[u_viewportIndex].viewPos.xz / HEIGHTMAP_SCALE_XZ;
    float distanceToTarget = max(abs(controlPosition.x - target.x), abs(controlPosition.y - target.y));
    float density = 1.0;
    if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 8.0) density = 16.0;
    else if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 4.0) density = 8.0;
    else if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE / 2.0) density = 4.0;
    else if (distanceToTarget <= TERRAIN_DISPLACEMENT_MESH_SIZE) density = 2.0;
    if (density <= 1.0) return vec3(0.0);

    // Terrain3D geomorphs each clipmap segment into the next coarser segment.
    float vertexLerp = smoothstep(
        0.0,
        1.0,
        (distanceToTarget * density - TERRAIN_DISPLACEMENT_MESH_SIZE - 4.0) /
        (TERRAIN_DISPLACEMENT_MESH_SIZE - 4.0));
    return mix(
        SampleTerrainDisplacement(controlPosition, density),
        SampleTerrainDisplacement(controlPosition, density * 0.5),
        vertexLerp);
#else
    return vec3(0.0);
#endif
}

void main() {
    v_sceneRenderItemIndex = tc_sceneRenderItemIndex[0];
    RenderItem renderItem = sceneRenderItems[v_sceneRenderItemIndex];

    vec2 controlPosition =
        tc_controlPointPosition[0].xz * gl_TessCoord.x +
        tc_controlPointPosition[1].xz * gl_TessCoord.y +
        tc_controlPointPosition[2].xz * gl_TessCoord.z;
    ivec2 heightMapMaximum = textureSize(u_HeightMapTexture, 0) - 1;
    v_baseUv = controlPosition / vec2(max(heightMapMaximum, ivec2(1)));

    float terrainHeight = textureLod(u_HeightMapTexture, v_baseUv, 0.0).r;
    vec3 localPosition = vec3(controlPosition.x, terrainHeight, controlPosition.y);
    v_worldPosition = (renderItem.modelMatrix * vec4(localPosition, 1.0)).xyz;
    v_worldPosition += GetTerrainDisplacement(controlPosition);

    gl_Position = viewportDataArr[u_viewportIndex].jitteredProjectionViewReverseZ * vec4(v_worldPosition, 1.0);
}
