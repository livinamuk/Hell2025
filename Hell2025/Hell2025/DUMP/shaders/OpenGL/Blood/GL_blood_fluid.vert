#version 460 

layout (location = 0) in vec3 v_position;

uniform mat4 u_projection;
uniform mat4 u_view;
uniform mat4 u_model;

out vec4 v_viewSpacePosition;
out vec3 v_localPosition;
out vec4 v_worldPos;
out vec3 v_sphereCenter;
out float v_sphereScale;

out vec4 v_viewSpaceCenter;
out float v_radius;

void main() {
    // depth shader
    v_viewSpacePosition = u_view * u_model * vec4(v_position, 1.0); 

    // thickness shader
    v_localPosition = v_position;
    v_sphereCenter = vec3(u_model[3]);
    v_sphereScale = length(vec3(u_model[0][0], u_model[0][1], u_model[0][2]));
    v_worldPos = u_model * vec4(v_position, 1.0);

    vec4 worldCenter = u_model * vec4(0.0, 0.0, 0.0, 1.0);
    v_viewSpaceCenter = u_view * worldCenter;
    v_radius = length(vec3(u_model[0][0], u_model[0][1], u_model[0][2]));

    gl_Position = u_projection * v_viewSpacePosition;
}