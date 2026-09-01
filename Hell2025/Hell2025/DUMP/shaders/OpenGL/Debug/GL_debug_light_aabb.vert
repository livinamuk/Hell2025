#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"

layout(location = 0) out vec3 v_color;

restrict layout(std430, binding = SSBO_IDX_LIGHT_AABB_DEBUG_LIGHTS) readonly buffer lightsBuffer    { Light lights[]; };
restrict layout(std430, binding = SSBO_IDX_LIGHT_AABB_DEBUG_BOUNDS) buffer          LightAABBBuffer { AABB lightAABBs[]; };

uniform int u_lightIndex;
uniform mat4 u_projectionView;

void main() {

    Light light = lights[u_lightIndex];
    vec3 pos = vec3(light.posX, light.posY, light.posZ);
    float radius = light.radius;

    //lightAABBs[u_lightIndex].boundsMin.xyz = pos - vec3(radius);
    //lightAABBs[u_lightIndex].boundsMax.xyz = pos + vec3(radius);

    
    //lightAABBs[u_lightIndex].boundsMin.xyz = vec3(0);
    //lightAABBs[u_lightIndex].boundsMax.xyz = vec3(0);

    AABB aabb = lightAABBs[u_lightIndex];

    vec3 aabbMin = aabb.boundsMin.xyz;
    vec3 aabbMax = aabb.boundsMax.xyz;

    vec3 vertices[8] = vec3[](
        vec3(aabbMin.x, aabbMin.y, aabbMin.z),
        vec3(aabbMax.x, aabbMin.y, aabbMin.z),
        vec3(aabbMax.x, aabbMax.y, aabbMin.z),
        vec3(aabbMin.x, aabbMax.y, aabbMin.z),
        vec3(aabbMin.x, aabbMin.y, aabbMax.z),
        vec3(aabbMax.x, aabbMin.y, aabbMax.z),
        vec3(aabbMax.x, aabbMax.y, aabbMax.z),
        vec3(aabbMin.x, aabbMax.y, aabbMax.z)
    );

    int indices[16] = int[](0, 1, 2, 3, 0, 4, 5, 6, 7, 4, 5, 1, 2, 6, 7, 3);

    gl_Position = u_projectionView * vec4(vertices[indices[gl_VertexID]], 1.0);
    v_color = vec3(1.0, 0.5, 0.0);
}