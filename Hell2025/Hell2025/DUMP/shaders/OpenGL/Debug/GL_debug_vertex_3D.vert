#version 460

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vColor;
layout (location = 2) in ivec2 vPixelOffset;
layout (location = 3) in int vDepthEnabled;
layout (location = 4) in int vExclusiveViewportIndex;
layout (location = 5) in int vIgnoredViewportIndex;

uniform int u_viewportIndex;
uniform ivec2 u_viewportSize;
uniform mat4 u_projectionView;
out vec3 Color;

void main() {
    Color = vColor;
	gl_Position = u_projectionView * vec4(vPosition, 1.0);

    // Convert the pixel offset to clip space
    gl_Position.xy += vec2(vPixelOffset) * (2.0 / vec2(u_viewportSize)) * gl_Position.w;
    
    // Set gl_position to something that won't actually render
    if (vExclusiveViewportIndex != -1 && vExclusiveViewportIndex != u_viewportIndex) {
       gl_Position = vec4(0, 0, 0, 0);
    }
    if (vIgnoredViewportIndex != -1 && vIgnoredViewportIndex == u_viewportIndex) {
       gl_Position = vec4(0, 0, 0, 0);
    }
    
    // Manually disable depth testing
    if (vDepthEnabled != 1) {
        gl_Position.z = 0.99999 * gl_Position.w;
    }
}
