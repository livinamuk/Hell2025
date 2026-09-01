#version 450 core
#include "../../common/OpenGL/GL_binding_indices.glsl"
#extension GL_ARB_bindless_texture : enable
#include "../../common/ocean.glsl"
#include "../../common/types.glsl"

layout(binding = 0) uniform sampler2D DisplacementTexture_band0;
layout(binding = 2) uniform sampler2D DisplacementTexture_band1;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer { ViewportData viewportDataArr[]; };
readonly restrict layout(std430, binding = SSBO_IDX_RENDERER_DATA) buffer rendererDataBuffer { RendererData rendererData; };

uniform int u_viewportIndex;

out vec3 v_worldPos;

struct OceanMeshConfig {
    int gridSize;
    int lodLevelCount;
    float baseSpacing;
    float lodScale;
    int holeMargin;
    float lodDepthBias;
};

uniform OceanMeshConfig u_mesh;

const int VERTICES_PER_QUAD = 6;

void main() {
    vec3 viewPos = viewportDataArr[u_viewportIndex].viewPos.xyz;
    mat4 projectionView = viewportDataArr[u_viewportIndex].jitteredProjectionViewReverseZ;

    int totalQuadsPerLOD = u_mesh.gridSize * u_mesh.gridSize;
    int quadID = gl_VertexID / VERTICES_PER_QUAD;

    int lodLevel = quadID / totalQuadsPerLOD;
    int localQuadID = quadID % totalQuadsPerLOD;

    if (lodLevel >= u_mesh.lodLevelCount) {
        gl_Position = vec4(0.0);
        return;
    }

    int vertexID = gl_VertexID % VERTICES_PER_QUAD;
    int quadX = localQuadID % u_mesh.gridSize;
    int quadY = localQuadID / u_mesh.gridSize;

    // hollowing out the center for the higher detail inner lods
    if (lodLevel > 0) {
        // leaving a slight overlap to hide snapping seams
        int holeStart = (u_mesh.gridSize / 4) + u_mesh.holeMargin;
        int holeEnd = (u_mesh.gridSize * 3 / 4) - u_mesh.holeMargin;

        if (quadX >= holeStart && quadX < holeEnd && quadY >= holeStart && quadY < holeEnd) {
            // outputting degenerate triangle to cull the quad
            gl_Position = vec4(0.0);
            return;
        }
    }

    // flipping mapped vertices to reverse the winding order
    int localX = (vertexID == 2 || vertexID == 3 || vertexID == 5) ? 1 : 0;
    int localY = (vertexID == 1 || vertexID == 4 || vertexID == 5) ? 1 : 0;

    float gridX = float(quadX + localX) - float(u_mesh.gridSize) * 0.5;
    float gridY = float(quadY + localY) - float(u_mesh.gridSize) * 0.5;

    float currentSpacing = u_mesh.baseSpacing * pow(u_mesh.lodScale, float(lodLevel));

    vec3 worldPos = vec3(gridX * currentSpacing, rendererData.oceanOriginY, gridY * currentSpacing);

    // snapping camera to the current lod grid
    vec2 snappedCam = floor(viewPos.xz / currentSpacing) * currentSpacing;
    worldPos.x += snappedCam.x;
    worldPos.z += snappedCam.y;

    vec3 displacement = SampleCombinedOceanDisplacement(DisplacementTexture_band0, DisplacementTexture_band1, worldPos.xz, rendererData.oceanDisplayMode);

    // applying the displacement
    worldPos += displacement;

    // dropping lower detail lods slightly after displacement
    // doing this so the higher detail mesh sits on top to resolve z fighting
    worldPos.y -= float(lodLevel) * u_mesh.lodDepthBias;

    v_worldPos = worldPos;
    gl_Position = projectionView * vec4(worldPos, 1.0);
}
