#include "../common/constants.glsl"

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    return nom / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    //float r = 0.5 + 0.5 * roughness; // Disney remapping
    float k = (r*r) / 8.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 microfacetBRDF(in vec3 L, in vec3 V, vec3 N, in vec3 baseColor, in float metallic, in float fresnelReflect, in float roughness) {
    vec3 H = normalize(V + L);
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);
    vec3 F0 = vec3(0.04 * fresnelReflect);
    F0 = mix(F0, baseColor, metallic);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(N, L), 0.0);

  //  return specular;

    return (kD * baseColor / PI + specular);
}


vec3 microfacetBRDFSpecularOnlyOLD(in vec3 L, in vec3 V, vec3 N, in vec3 baseColor, in float metallic, in float fresnelReflect, in float roughness) {
    vec3 H = normalize(V + L);
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);
    vec3 F0 = vec3(0.04 * fresnelReflect);
    F0 = mix(F0, baseColor, metallic);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;
    float NdotL = max(dot(N, L), 0.0);

    vec3 nonSpecularPart = (kD * baseColor / PI) * 0.1;

    return nonSpecularPart + specular;
}





vec3 microfacetBRDFSpecularOnly(in vec3 L, in vec3 V, vec3 N, in vec3 baseColor, in float metallic, in float fresnelReflect, in float roughness) {
    vec3 H = normalize(V + L);
    float NoV = clamp(dot(N, V), 0.0, 1.0);
    float NoL = clamp(dot(N, L), 0.0, 1.0);
    float NoH = clamp(dot(N, H), 0.0, 1.0);
    float VoH = clamp(dot(V, H), 0.0, 1.0);

    // F0 is 0.04 for plastic (dielectrics), tinted by baseColor if metallic
    vec3 F0 = vec3(0.04 * fresnelReflect);
    F0 = mix(F0, baseColor, metallic);

    // Standard Cook-Torrance terms
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3 F    = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * NoV * NoL + 0.0001;
    vec3 specular = numerator / denominator;

    // Return only the reflected energy
    // The main shader handles (1.0 - F) for the refraction pass
    return specular;
}







