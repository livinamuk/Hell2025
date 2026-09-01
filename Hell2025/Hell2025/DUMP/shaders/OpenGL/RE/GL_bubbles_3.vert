#version 450

uniform mat4 u_view;
uniform mat4 u_projectionView;

//const vec3 ORIGIN = vec3(36.5, 32.5, 37.0);
//const float SCALE = 0.1;

const vec3 ORIGIN = vec3(26.0, 28.5, 36.9);
const float SCALE = 0.01;

out vec3 v_worldPos;
out vec2 v_uv;

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
    v_worldPos = ORIGIN + cameraRight * quadPos.x * SCALE + cameraUp * quadPos.y * SCALE;

    gl_Position = u_projectionView * vec4(v_worldPos, 1.0);
}