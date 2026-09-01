#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"

uniform mat4 u_view;
uniform mat4 u_projectionView;

const float SCALE = 0.005;

out vec2 v_uv;
out vec3 v_worldPos;

out float v_lifetime;

restrict layout(std430, binding = SSBO_IDX_PARTICLE_DRAW_POOL) readonly buffer Buffer6 { Particle particlePool[]; };
restrict layout(std430, binding = SSBO_IDX_PARTICLE_DRAW_ACTIVE_INDICES) readonly buffer Buffer7 { uint particleActiveIndices[]; };

//uniform int u_particleIndex;

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

    uint particleIndex = particleActiveIndices[gl_InstanceID];
    vec3 origin = particlePool[particleIndex].position.xyz;

    v_worldPos = origin + cameraRight * quadPos.x * SCALE + cameraUp * quadPos.y * SCALE;
    v_lifetime = particlePool[particleIndex].lifeTime;

    gl_Position = u_projectionView * vec4(v_worldPos, 1.0);
}
