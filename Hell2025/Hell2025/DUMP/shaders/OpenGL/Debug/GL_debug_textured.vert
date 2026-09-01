#version 460 core
#include "../../common/OpenGL/GL_binding_indices.glsl"

#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

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

// temporarily here
uniform bool u_useMirrorMatrix;
uniform mat4 u_mirrorViewMatrix;
uniform vec4 u_mirrorClipPlane;

mat3 RotationX(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat3(1,0,0, 0,c,-s, 0,s,c);
}

mat3 RotationY(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat3(c,0,s, 0,1,0, -s,0,c);
}

mat3 RotationZ(float angle) {
    float s = sin(angle);
    float c = cos(angle);
    return mat3(c,-s,0, s,c,0, 0,0,1);
}

void main() {
    mat4 modelMatrix = u_model;
    mat4 inverseModelMatrix = inverse(modelMatrix);
	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    Normal = normalize(normalMatrix * vec4(vNormal, 0)).xyz;
    Tangent = normalize(normalMatrix * vec4(vTangent, 0)).xyz;
    BiTangent = normalize(cross(Normal, Tangent));
    EmissiveColor = vec3(0,0,0);
    WorldPos = modelMatrix * vec4(vPosition, 1.0);

    // Planar reflections
    if (u_useMirrorMatrix) {
        mat4 projection = viewportData[u_viewportIndex].projectionReverseZ;
        projection[0][0] *= -1.0;
        projectionView = projection * u_mirrorViewMatrix;
        mat4 jitterMatrix = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ *
                            viewportData[u_viewportIndex].inverseProjectionViewReverseZ;
        projectionView = jitterMatrix * projectionView;
        gl_ClipDistance[0] = dot(WorldPos, u_mirrorClipPlane);

        //vec3 shardRotation = vec3(0.00, 0.00, -0.0005);
        //mat3 shardMatrix = RotationZ(shardRotation.z) * RotationY(shardRotation.y) * RotationX(shardRotation.x);
        //WorldPos.xyz = shardMatrix * WorldPos.xyz;
    }


	TexCoord = vUV;
	gl_Position = projectionView * WorldPos;
}
