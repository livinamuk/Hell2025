#version 460
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/util.glsl"
#include "../../common/types.glsl"
#include "../../common/constants.glsl"

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inTangent;

layout (binding = 0) uniform sampler2D u_PosTex;
layout (binding = 1) uniform sampler2D u_NormTex;

out vec4 WorldPos;
out vec3 Normal;
out vec3 v_ViewDir;
out vec3 v_fragPos;

uniform int u_viewportIndex;
uniform mat4 u_modelMatrix;
uniform mat4 u_inverseModelMatrix;
uniform float u_time;

readonly restrict layout(std430, binding = SSBO_IDX_VIEWPORT_DATA) buffer viewportDataBuffer {
	ViewportData viewportData[];
};

void main() {

	mat4 projectionView = viewportData[u_viewportIndex].jitteredProjectionViewReverseZ;
	mat4 inverseView = viewportData[u_viewportIndex].inverseView;

    mat4 modelMatrix = u_modelMatrix;
    mat4 inverseModelMatrix = u_inverseModelMatrix;
    mat4 normalMatrix = transpose(inverseModelMatrix);

    int u_NumOfFrames = 81;
    int u_Speed = 35;
    vec3 u_HeightOffset = vec3(-45.4, -26.17, 12.7);

    u_HeightOffset = vec3(0, 0, 0);

    float currentSpeed = 1.0f / (u_NumOfFrames / u_Speed);
    float timeInFrames = ((ceil(fract(-u_time * currentSpeed) * u_NumOfFrames)) / u_NumOfFrames) + (1.0 / u_NumOfFrames);

    vec3 v = inPosition;
    vec2 uv = inUV;

    timeInFrames = 0.0;
	timeInFrames = u_time;

	vec2 TexCoord = vec2(uv.x, (timeInFrames + uv.y));
    TexCoord = clamp(TexCoord, 0, 1);

	vec4 texturePos = textureLod(u_PosTex, TexCoord, 0);
	vec4 textureNorm = textureLod(u_NormTex, TexCoord, 0);

    Normal = textureNorm.xzy * 2.0 - 1.0;
	Normal = normalize((normalMatrix * vec4(Normal, 0)).xyz);

    WorldPos = modelMatrix * vec4(texturePos.xzy, 1.0);

    gl_Position =  projectionView * WorldPos;

    // Fix this!!!
    if (u_time > 0.7) {
        gl_Position =  vec4(0);
    }

}
