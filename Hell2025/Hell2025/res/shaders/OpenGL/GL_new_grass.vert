#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

uniform mat4 u_projectionView;
uniform mat4 u_prevProjectionView;
uniform mat4 u_rasterProjectionView;
uniform vec3 u_viewPosition;
uniform int u_verticesPerBlade;

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

void main() {
    const vec4 visiblePoint = visibleGrassPositions[uint(gl_InstanceID)];
    const vec3 basePosition = visiblePoint.xyz;
    const uint packedBladeData = floatBitsToUint(visiblePoint.w);
    const uint bladeVariant = packedBladeData & 0x1ffu;
    const float densityFade = float((packedBladeData >> 9u) & 0xffu) * (1.0 / 255.0);

    const uint baseVertex = bladeVariant * uint(u_verticesPerBlade);
    const uint localVertex = uint(gl_VertexID);
    const uint vertexIndex = baseVertex + localVertex;
    const Vertex vertex = inputVertices[vertexIndex];
    const vec3 localPosition = vec3(vertex.posX, vertex.posY, vertex.posZ);
    const uint pairedVertexIndex = baseVertex + (localVertex ^ 1u);
    const Vertex pairedVertex = inputVertices[pairedVertexIndex];
    const vec3 pairedPosition = vec3(pairedVertex.posX, pairedVertex.posY, pairedVertex.posZ);
    const vec3 bladeSideDirection = (localVertex & 1u) == 0u
        ? pairedPosition - localPosition
        : localPosition - pairedPosition;

    Normal = vec3(vertex.normX, vertex.normY, vertex.normZ);
    BladeTangent = normalize(cross(Normal, bladeSideDirection));
    BladeSide = (localVertex & 1u) == 0u ? -1.0 : 1.0;
    WorldPos = localPosition * densityFade + basePosition;

    vec4 worldPosition = vec4(WorldPos, 1.0);
    v_currPos = u_projectionView * worldPosition;
    v_prevPos = u_prevProjectionView * worldPosition;
    gl_Position = u_rasterProjectionView * worldPosition;
}
