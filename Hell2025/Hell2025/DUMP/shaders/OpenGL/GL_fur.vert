#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"
#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out float LayerFraction;

uniform int u_viewportIndex;
uniform mat4 u_model;
uniform mat3 u_normalMatrix;

//uniform int u_hairLayerCount; 
//uniform float u_furLength = 0.015 * 0.15 * 10;
//uniform float u_curvature = 2.5;
//uniform float u_displacementStrength = 0.0005;
//uniform float u_shellDistanceAttenuation = 2.5 * 0.5;

// bunny
uniform int u_hairLayerCount; 
uniform float u_furLength = 0.015 * 0.2;// * 10;
uniform float u_displacementStrength = 0.0005;
uniform float u_shellDistanceAttenuation = 1.5;


//uniform int u_hairLayerCount; 
////uniform float u_furLength = 0.015 * 0.10 * 5.5;//750; // art station roo
//uniform float u_furLength = 0.015 * 0.10 * 500;
//uniform float u_displacementStrength = 1.5;
//uniform float u_shellDistanceAttenuation = 2050.5;

out float H0;

void main() {
    Normal = normalize(u_normalMatrix * inNormal);
    Tangent = normalize(u_normalMatrix * inTangent);
    BiTangent = normalize(cross(Normal, Tangent));
    TexCoord = inUV * 1;

    // Normalized shell height
    H0 = float(gl_InstanceID + 1) / float(u_hairLayerCount);
    float curvatureH0 = pow(H0, u_shellDistanceAttenuation);

    // Extrude along local normal
    vec3 pos = inPosition + inNormal * ((u_furLength * H0) + (curvatureH0 * u_displacementStrength));

    WorldPos = u_model * vec4(pos, 1.0);
    mat4 jitterMatrix = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ *
                        viewportData[u_viewportIndex].inverseProjectionViewReverseZ;
    gl_Position = jitterMatrix * viewportData[u_viewportIndex].projectionView * WorldPos;


}
