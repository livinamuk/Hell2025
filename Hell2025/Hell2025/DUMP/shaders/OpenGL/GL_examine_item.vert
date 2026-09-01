#version 460 core
#include "../common/OpenGL/GL_binding_indices.glsl"

#include "../common/util.glsl"
#include "../common/types.glsl"
#include "../common/constants.glsl"

layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec2 vUV;
layout (location = 3) in vec3 vTangent;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

out vec2 TexCoord;
out vec4 WorldPos;
out vec3 Normal;
out vec3 Tangent;
out vec3 BiTangent;
out vec3 EmissiveColor;

uniform int u_viewportIndex;
uniform mat4 u_model;
uniform mat4 u_viewMatrix;
uniform mat4 u_inverseModel;

void main() {
    mat4 modelMatrix = u_model;
    mat4 inverseModelMatrix = u_inverseModel;  
	mat4 projection = viewportData[u_viewportIndex].projectionReverseZ; 
	mat4 projectionView = viewportData[u_viewportIndex].projectionViewReverseZ;            
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    EmissiveColor = vec3(0,0,0);

	TexCoord = vUV;
    WorldPos = modelMatrix * vec4(vPosition, 1.0);
	gl_Position = projection * u_viewMatrix * WorldPos;
    //gl_Position.y += 0.25;
}