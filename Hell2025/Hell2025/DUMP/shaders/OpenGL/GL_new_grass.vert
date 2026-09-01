#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

uniform mat4 u_projectionView;
uniform mat4 u_prevProjectionView;
uniform mat4 u_rasterProjectionView;
uniform vec3 u_viewPosition;
uniform int u_segmentCount;
uniform int u_verticesPerBlade;
uniform float u_minCullDistance;
uniform float u_maxCullDistance;
uniform float u_cullExponent;

out vec3 Normal;
out vec3 WorldPos;
out vec3 BladeTangent;
out float BladeSide;
out vec4 v_currPos;
out vec4 v_prevPos;

struct Vertex {
    float posX;
    float posY;
    float posZ;
    float normX;
    float normY;
    float normZ;
};

readonly layout(std430, binding = SSBO_IDX_GRASS_POSITION_BLADE_POSITIONS) buffer VisibleGrassPositions {
    vec4 visibleGrassPositions[];
};

readonly layout(std430, binding = SSBO_IDX_GRASS_POSITION_INPUT_VERTICES) buffer InputVertexBuffer {
    Vertex inputVertices[];
};

vec3 GetVertexPosition(uint vertexIndex) {
    const Vertex vertex = inputVertices[vertexIndex];
    return vec3(vertex.posX, vertex.posY, vertex.posZ);
}

vec3 GetBladeRowCenter(uint baseVertex, uint row) {
    const uint leftVertex = baseVertex + row * 2u;
    return (GetVertexPosition(leftVertex) + GetVertexPosition(leftVertex + 1u)) * 0.5;
}

float GrassKeepProbability(float cameraDistance) {
    const float distanceRange = max(u_maxCullDistance - u_minCullDistance, 0.0001);
    const float distanceAlpha = clamp((cameraDistance - u_minCullDistance) / distanceRange, 0.0, 1.0);
    return pow(1.0 - distanceAlpha, max(u_cullExponent, 0.0001));
}

void main() {
    const uint hashMod = 360u;

    const vec4 visiblePoint = visibleGrassPositions[uint(gl_InstanceID)];
    const vec3 basePosition = visiblePoint.xyz;
    const uint hashVal = floatBitsToUint(visiblePoint.w);
    const float densityRank = float(hashVal >> 8u) * (1.0 / 16777216.0);
    const float keepProbability = GrassKeepProbability(length(basePosition - u_viewPosition));
    const float densityFade = smoothstep(densityRank, min(densityRank + 0.08, 1.0), keepProbability);

    const uint baseVertex = (hashVal % hashMod) * uint(u_verticesPerBlade);
    const uint localVertex = uint(gl_VertexID);
    const uint vertexIndex = baseVertex + localVertex;
    const Vertex vertex = inputVertices[vertexIndex];

    const uint bladeRow = localVertex / 2u;
    const uint previousRow = bladeRow > 0u ? bladeRow - 1u : bladeRow;
    const uint nextRow = bladeRow < uint(u_segmentCount) ? bladeRow + 1u : bladeRow;
    const vec3 previousCenter = GetBladeRowCenter(baseVertex, previousRow);
    const vec3 nextCenter = GetBladeRowCenter(baseVertex, nextRow);

    BladeTangent = normalize(nextCenter - previousCenter);
    BladeSide = (localVertex & 1u) == 0u ? -1.0 : 1.0;
    WorldPos = vec3(vertex.posX, vertex.posY, vertex.posZ) * densityFade + basePosition;
    Normal = vec3(vertex.normX, vertex.normY, vertex.normZ);

    vec4 worldPosition = vec4(WorldPos, 1.0);
    v_currPos = u_projectionView * worldPosition;
    v_prevPos = u_prevProjectionView * worldPosition;
    gl_Position = u_rasterProjectionView * worldPosition;
}
