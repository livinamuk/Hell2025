#version 460 core
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/types.glsl"

layout(std430, binding = SSBO_IDX_DEBUG_POINT_CLOUD_POINTS) readonly buffer PointCloudBuffer { CloudPoint points[]; };

uniform int u_viewportIndex;
uniform mat4 u_projectionView;
out vec4 v_normal;
out vec4 v_directLighting;
out vec2 v_uv;
out vec3 v_baseColor;
out vec3 v_worldPos;
flat out uint v_vertexId;

void main() {
    CloudPoint point = points[gl_VertexID];
    vec3 position = point.position.xyz;

    // Offset along normal to get it out of the wall
    position += point.normal.xyz * 0.01;

    gl_Position = u_projectionView * vec4(position, 1.0);

    v_worldPos = point.position.xyz;
    v_normal = point.normal;
    v_directLighting = point.directLightingRGB_dirty;
    v_baseColor = point.baseColor.rgb;
    v_vertexId = uint(gl_VertexID);

    v_uv = vec2(point.baseColor.x, point.baseColor.y); // They're temporarily baked in here
}
