#version 460
layout(location = 0) out vec3 v_color;

uniform mat4 u_projectionView;

void main() {
    vec3 aabbMin = vec3(-1.0, -1.0, -1.0);
    vec3 aabbMax = vec3(1.0, 1.0, 1.0);

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