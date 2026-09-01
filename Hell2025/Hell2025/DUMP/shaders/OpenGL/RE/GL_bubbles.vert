#version 450
#include "../../common/OpenGL/GL_binding_indices.glsl"

uniform mat4 u_view;
uniform mat4 u_projectionView;

const vec3 ORIGIN = vec3(36.5, 32.5, 35.0);
const float SCALE = 0.1;

out vec2 v_uv;

layout(std430, binding = SSBO_IDX_BUBBLE_DRAW_POSITIONS) readonly buffer BubblePositionsBuffer { vec4 bubblePositions[]; };

void main() {
    // Vertex positions
    const vec2 positions[6] = vec2[6](
        vec2(-1.0, -1.0),
        vec2( 1.0, -1.0),
        vec2(-1.0,  1.0),
        vec2(-1.0,  1.0),
        vec2( 1.0, -1.0),
        vec2( 1.0,  1.0)
    );

    // isolate the current vertex within the current quad
    vec2 quadPos = positions[gl_VertexID % 6];

    // UVs
    v_uv = quadPos * 0.5 + 0.5;

    // Make it face the camera
    vec3 cameraRight = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
    vec3 cameraUp = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
    //vec3 worldPos = ORIGIN + cameraRight * quadPos.x * SCALE + cameraUp * quadPos.y * SCALE;

    vec3 origin = bubblePositions[gl_InstanceID].xyz;
    vec3 worldPos = origin + cameraRight * quadPos.x * SCALE + cameraUp * quadPos.y * SCALE;

    gl_Position = u_projectionView * vec4(worldPos, 1.0);
}
