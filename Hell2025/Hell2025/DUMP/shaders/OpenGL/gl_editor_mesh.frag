#version 460 core
layout (location = 0) out vec4 BaseColorOut;
layout (location = 1) out vec4 NormalOut;
layout (location = 2) out vec4 RMAOut;
layout (location = 3) out vec4 EmissiveOut;

layout (binding = 0) uniform sampler2D baseColorTexture;
layout (binding = 1) uniform sampler2D normalTexture;
layout (binding = 2) uniform sampler2D rmaTexture;

in vec2 TexCoord;
in vec3 Normal;
in vec3 Tangent;
in vec3 BiTangent;
in vec3 WorldPos;

void main() {
    vec4 baseColor = texture2D(baseColorTexture, TexCoord);
    vec3 normalMap = texture2D(normalTexture, TexCoord).rgb;
    vec3 rma = texture2D(rmaTexture, TexCoord).rgb;
    
    BaseColorOut = baseColor;
    RMAOut = vec4(rma, 1);
    NormalOut.rgb = normalize(Normal);
    EmissiveOut = vec4(0,0,0,1);
}
