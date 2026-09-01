#version 460 core
#include "../../common/OpenGL/GL_binding_indices.glsl"
#include "../../common/lighting.glsl"
#include "../../common/post_processing.glsl"
#include "../../common/types.glsl"

layout (location = 0) out vec4 FragOut;

layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec4 WorldPos;
in vec3 ViewPos;

readonly restrict layout(std430, binding = SSBO_IDX_LIGHTS) buffer lightsBuffer {
	Light lights[];
};

void main() {
   vec4 baseColor = texture2D(baseColorTexture, TexCoord);
   vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
   vec3 rma = texture2D(rmaTexture, TexCoord).rgb;

   mat3 tbn = mat3(normalize(Tangent), normalize(BiTangent), normalize(Normal));
   normalMap.rgb = normalMap.rgb * 2.0 - 1.0;
   normalMap = normalize(normalMap);
   vec3 normal = normalize(tbn * (normalMap));

   vec3 gammaBaseColor = pow(baseColor.rgb, vec3(2.2));
   float roughness = rma.r;
   float metallic = rma.g;
    roughness = 0.5;
    metallic = 0.5;

   // Direct light
   vec3 directLighting = vec3(0);
   for (int i = 0; i < 4; i++) {
       Light light = lights[i];
       vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);
       vec3 lightColor =  vec3(light.colorR, light.colorG, light.colorB);
       float lightStrength = light.strength;
       float lightRadius = light.radius;
       directLighting += GetDirectLighting(lightPosition, lightColor, lightRadius, lightStrength, normal.xyz, WorldPos.xyz, gammaBaseColor.rgb, roughness, metallic, ViewPos);
    }

   vec3 finalColor = directLighting;

   FragOut.rgb = vec3(finalColor);
   FragOut.a = baseColor.a;
}
