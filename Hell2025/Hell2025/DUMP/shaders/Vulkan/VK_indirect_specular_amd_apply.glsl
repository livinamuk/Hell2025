#include "VK_indirect_specular_amd_config.glsl"

vec3 GetAMDIndirectSpecularPrimaryResponse(vec3 linearBaseColor, float metallic, float perceptualRoughness, vec3 normal, vec3 viewDirToCamera, int brdfLutTextureIndex) {
    if (brdfLutTextureIndex < 0) {
        return vec3(0.0);
    }

    float normalDotView = clamp(dot(normal, viewDirToCamera), 0.0, 1.0);
    
    vec2 brdfSamplePoint = clamp(
        vec2(normalDotView, 1.0 - perceptualRoughness),
        vec2(0.0),
        vec2(1.0));

    uint brdfTextureIndex = uint(brdfLutTextureIndex);
    vec2 brdf = textureLod(sampler2D(textures[nonuniformEXT(brdfTextureIndex)], textureSamplers[nonuniformEXT(brdfTextureIndex)]), brdfSamplePoint, 0.0).rg;
    vec3 specularColor = mix(vec3(0.04), linearBaseColor, metallic);

    return specularColor * brdf.x + brdf.y;
}

vec3 ApplyAMDIndirectSpecularBRDF(vec3 incidentRadiance, vec3 primaryResponse) {
    return incidentRadiance * primaryResponse;
}
