#version 460 core
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"
#include "../../common/ddgi.glsl"

layout (location = 0) out vec4 FragOut;

in vec3 v_worldPos;
in vec4 v_normal;
in vec4 v_directLighting;
in vec2 v_uv;
in vec3 v_baseColor;
flat in uint v_vertexId;

layout(std430, binding = SSBO_IDX_DEBUG_POINT_CLOUD_LIGHTS) readonly buffer lightsBuffer { Light lights[]; };
layout(std430, binding = SSBO_IDX_DEBUG_POINT_CLOUD_GRID_OFFSETS) readonly buffer GridOffsetsBuffer { uint cellOffsets[]; };
layout(std430, binding = SSBO_IDX_DEBUG_POINT_CLOUD_GRID_COUNTS) readonly buffer GridCountsBuffer { uint cellCounts[]; };
layout(std430, binding = SSBO_IDX_DEBUG_POINT_CLOUD_DIRTY_FLAGS) buffer PointCloudDirtyFlagsBuffer { uint pointCloudDirtyFlags[]; };

uniform ivec3 u_pointCloudGridDimensions;
uniform float u_pointCloudCellSize;
uniform vec3 u_volumeMinBounds;

//void main() {
//    gl_FragDepth = 0.0;
//    
//    // Test to clear all cell dirty flags
//    int xSize = u_pointCloudGridDimensions.x;
//    int ySize = u_pointCloudGridDimensions.y;
//    int zSize = u_pointCloudGridDimensions.z;
//
//    for (int x = 0; x < xSize; x++) {
//        for (int y = 0; y < xSize; y++) {
//            for (int z = 0; z < zSize; z++) {
//                int idx = GetPointCloudCellIndex(ivec3(x, y, z), u_pointCloudGridDimensions);
//                //cellDirtyFlags[idx] = 0;
//            }
//        }
//    }
//    
//    // Test to mark any cells that overlap that aabb as dirty
//    vec3 testAABBMin = vec3(0, 0, 0);
//    vec3 testAABBMax = vec3(1, 1, 4.5);
//    
//    ivec3 startCell = ivec3((testAABBMin - u_volumeMinBounds) / u_pointCloudCellSize);
//    ivec3 endCell = ivec3((testAABBMax - u_volumeMinBounds) / u_pointCloudCellSize);
//    startCell = clamp(startCell, ivec3(0), u_pointCloudGridDimensions - 1);
//    endCell = clamp(endCell,   ivec3(0), u_pointCloudGridDimensions - 1);
//    
//    for (int z = startCell.z; z <= endCell.z; z++) {
//        for (int y = startCell.y; y <= endCell.y; y++) {
//            for (int x = startCell.x; x <= endCell.x; x++) {
//            
//                ivec3 cellCoords = ivec3(x, y, z);
//                int idx = GetPointCloudCellIndex(cellCoords, u_pointCloudGridDimensions);
//    
//                cellDirtyFlags[idx] = 1; 
//            }
//        }
//    }
//
//    // Test to render all points within dirty cells as RED and GREEN if not
//    vec3 relativePos = v_worldPos - u_volumeMinBounds;
//    ivec3 myCell = ivec3(relativePos / u_pointCloudCellSize);
//    
//    myCell = clamp(myCell, ivec3(0), u_pointCloudGridDimensions - 1); // prevent reading outside the buffer
//    
//    int myIdx = GetPointCloudCellIndex(myCell, u_pointCloudGridDimensions);
//    
//    if (cellDirtyFlags[myIdx] == 1) {
//        FragOut = vec4(1.0, 0.0, 0.0, 1.0); // red for dirty
//    } 
//    else {
//        FragOut = vec4(0.0, 1.0, 0.0, 1.0); // green for clean
//    }
//}

void main() {
    //if (v_directLighting.rgb == vec3(0,0,0)) {
    //    discard;
    //}
    //else {
        FragOut.rgba = vec4(v_directLighting.rgb, 1.0);
    //}

    // Dirty points
    if (pointCloudDirtyFlags[v_vertexId] == 1u) {
        FragOut.r += 0.5;
    }
}

void main3() {
    vec2 wrappedUV = fract(v_uv);
    FragOut = vec4(wrappedUV, 0.0, 1.0);
}

void m55ain() {
    FragOut.rgb = v_baseColor;
	FragOut.a = 1.0;
}
