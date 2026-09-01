#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

uniform mat4 u_projectionView;
uniform mat4 u_prevProjectionView;
uniform mat4 u_rasterProjectionView;
uniform int u_segmentCount;
uniform int u_verticesPerBlade;
uniform int u_indicesPerBlade;

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

layout(std430, binding = SSBO_IDX_GRASS_POSITION_BLADE_POSITIONS) buffer bladePositions {
    vec4 BladePositions[];
};

layout(std430, binding = SSBO_IDX_GRASS_POSITION_INPUT_VERTICES) buffer inputVertexBuffer {
    Vertex InputVertexBuffer[];
};

layout(std430, binding = SSBO_IDX_GRASS_POSITION_INPUT_INDICES) buffer inputIndexBuffer {
    uint InputIndexBuffer[];
};

vec3 GetVertexPosition(uint vertexIndex) {
    const Vertex vertex = InputVertexBuffer[vertexIndex];
    return vec3(vertex.posX, vertex.posY, vertex.posZ);
}

vec3 GetBladeRowCenter(uint baseVertex, uint row) {
    const uint leftVertex = baseVertex + row * 2u;
    return (GetVertexPosition(leftVertex) + GetVertexPosition(leftVertex + 1u)) * 0.5;
}

uint HashMix(vec2 v) {
    uint x = floatBitsToUint(v.x);
    uint y = floatBitsToUint(v.y);
    x ^= (x >> 17);
    y ^= (y << 13);
    x *= 374761393u;
    y *= 668265263u;
    x ^= (x >> 15);
    y ^= (y << 17);
    return x ^ y;
}

void main() {
    const uint hashMod = 360u;
    const uint indicesPerBlade = uint(u_indicesPerBlade);
    const uint verticesPerBlade = uint(u_verticesPerBlade);
    const uint frontVertexCount = uint((u_segmentCount + 1) * 2);

    const uint basePosIndex = uint(gl_VertexID) / indicesPerBlade;
    const vec4 basePos = BladePositions[basePosIndex];

    const uint hashVal = HashMix(vec2(basePos.z, basePos.x));
    const uint baseVertex = (hashVal % hashMod) * verticesPerBlade;

    const uint baseIndex = (hashVal % hashMod) * indicesPerBlade;
    const uint vertex = InputIndexBuffer[baseIndex + (uint(gl_VertexID) % indicesPerBlade)];
    const Vertex v = InputVertexBuffer[vertex];

    // Back-face vertices duplicate the same configurable row layout.
    const uint localVertex = (vertex - baseVertex) % frontVertexCount;
    const uint bladeRow = localVertex / 2u;
    const uint previousRow = bladeRow > 0u ? bladeRow - 1u : bladeRow;
    const uint nextRow = bladeRow < uint(u_segmentCount) ? bladeRow + 1u : bladeRow;

    const vec3 previousCenter = GetBladeRowCenter(baseVertex, previousRow);
    const vec3 nextCenter = GetBladeRowCenter(baseVertex, nextRow);
    BladeTangent = normalize(nextCenter - previousCenter);
    BladeSide = (localVertex & 1u) == 0u ? -1.0 : 1.0;

    WorldPos = vec3(v.posX, v.posY, v.posZ) + basePos.xyz;
    Normal = vec3(v.normX, v.normY, v.normZ);

    vec4 worldPos = vec4(WorldPos, 1.0);
    v_currPos = u_projectionView * worldPos;
    v_prevPos = u_prevProjectionView * worldPos;

	gl_Position = u_rasterProjectionView * worldPos;
}
