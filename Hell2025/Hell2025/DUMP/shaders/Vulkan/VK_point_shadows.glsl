#ifndef VULKAN_POINT_SHADOWS_GLSL
#define VULKAN_POINT_SHADOWS_GLSL

const uint POINT_SHADOW_MODE_SHADOW_MAP = 0u;
const uint POINT_SHADOW_MODE_RAY_QUERY = 1u;

layout(set = 0, binding = DESC_IDX_TEXTURE_CUBE_ARRAYS_DEPTH) uniform textureCubeArray pointShadowMaps[];
layout(set = 0, binding = DESC_IDX_SHADOW_SAMPLERS) uniform samplerShadow pointShadowSamplers[];

float SamplePointShadowReceiverPlaneBindless(int lightIndex, vec3 lightToFrag, vec3 receiverNormal, float currentDepth, float lightRadius, float bias, vec3 sampleDir, float maxPlaneDepthDelta, uint shadowResourceIndex) {
    vec3 sampleRay = normalize(sampleDir);

    float receiverDepth = currentDepth;
    float denominator = dot(receiverNormal, sampleRay);

    if (denominator < -0.03) {
        receiverDepth = dot(receiverNormal, lightToFrag) / denominator;
        receiverDepth = clamp(receiverDepth, currentDepth - maxPlaneDepthDelta, currentDepth + maxPlaneDepthDelta);
    }

    float compareDepth = clamp((receiverDepth - bias) / lightRadius, 0.0, 1.0);
    return texture(samplerCubeArrayShadow(pointShadowMaps[nonuniformEXT(shadowResourceIndex)], pointShadowSamplers[nonuniformEXT(shadowResourceIndex)]), vec4(sampleDir, float(lightIndex)), compareDepth);
}

float ShadowCalculationBindless(int lightIndex, vec3 lightPosition, float lightRadius, vec3 worldPosition, vec3 viewPosition, vec3 normal, uint shadowResourceIndex) {
    vec3 lightToFrag = worldPosition - lightPosition;
    float currentDepth = length(lightToFrag);
    vec3 rayDir = lightToFrag / currentDepth;
    vec3 lightDirection = -rayDir;

    vec3 receiverNormal = normalize(normal);
    if (dot(receiverNormal, lightDirection) < 0.0) {
        receiverNormal = -receiverNormal;
    }

    float cosTheta = clamp(dot(receiverNormal, lightDirection), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);
    float shadowMapSize = float(textureSize(samplerCubeArrayShadow(pointShadowMaps[nonuniformEXT(shadowResourceIndex)], pointShadowSamplers[nonuniformEXT(shadowResourceIndex)]), 0).x);
    float texelWorldSize = currentDepth * 2.0 / shadowMapSize;
    float softness = 2.25;
    float grazingScale = smoothstep(0.08, 0.45, cosTheta);
    float diskRadius = texelWorldSize * mix(0.75, softness, grazingScale);
    float maxPlaneDepthDelta = max(diskRadius * 8.0, 0.02);

    vec3 basisSeed = abs(rayDir.y) < 0.99 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(basisSeed, rayDir));
    vec3 up = cross(rayDir, right);

    vec2 poissonDisk[8] = vec2[](
        vec2( 0.527,  0.085),
        vec2(-0.406,  0.331),
        vec2( 0.226, -0.543),
        vec2(-0.589, -0.205),
        vec2( 0.703, -0.391),
        vec2(-0.168,  0.743),
        vec2(-0.812,  0.125),
        vec2( 0.311,  0.379)
    );

    float visibility = SamplePointShadowReceiverPlaneBindless(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, lightToFrag, maxPlaneDepthDelta, shadowResourceIndex) * 2.0;

    for (int i = 0; i < 8; ++i) {
        vec2 disk = poissonDisk[i] * diskRadius;
        vec3 sampleDir = lightToFrag + right * disk.x + up * disk.y;
        visibility += SamplePointShadowReceiverPlaneBindless(lightIndex, lightToFrag, receiverNormal, currentDepth, lightRadius, bias, sampleDir, maxPlaneDepthDelta, shadowResourceIndex);
    }

    return visibility * 0.1;
}

float GetPointShadowMapVisibility(Light light, vec3 worldPosition, vec3 normal, vec3 viewPosition) {
    vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);

    if (light.hiResShadowMapIndex >= 0) {
        return ShadowCalculationBindless(light.hiResShadowMapIndex, lightPosition, light.radius, worldPosition, viewPosition, normal, VULKAN_POINT_SHADOW_IDX_HIGH_RES);
    }
    if (light.lowResShadowMapIndex >= 0) {
        return ShadowCalculationBindless(light.lowResShadowMapIndex, lightPosition, light.radius, worldPosition, viewPosition, normal, VULKAN_POINT_SHADOW_IDX_LOW_RES);
    }
    return 1.0;
}

float ShadowCalculationSkinBindless(int lightIndex, vec3 lightPosition, float lightRadius, vec3 worldPosition, vec3 viewPosition, vec3 normal, uint shadowResourceIndex) {
    vec3 lightToFrag = worldPosition - lightPosition;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float farPlane = lightRadius;

    float cosTheta = clamp(dot(normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 20;
    float viewDistance = length(viewPosition - worldPosition);
    float diskRadius = (1.0 + (viewDistance / farPlane)) / 200.0;
    float compareDepth = clamp((currentDepth - bias) / farPlane, 0.0, 1.0);

    float visibility = 0.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        visibility += texture(samplerCubeArrayShadow(pointShadowMaps[nonuniformEXT(shadowResourceIndex)], pointShadowSamplers[nonuniformEXT(shadowResourceIndex)]), vec4(sampleDir, float(lightIndex)), compareDepth);
    }

    return visibility / float(samples);
}

float GetPointShadowMapVisibilitySkin(Light light, vec3 worldPosition, vec3 normal, vec3 viewPosition) {
    vec3 lightPosition = vec3(light.posX, light.posY, light.posZ);

    if (light.hiResShadowMapIndex >= 0) {
        return ShadowCalculationSkinBindless(light.hiResShadowMapIndex, lightPosition, light.radius, worldPosition, viewPosition, normal, VULKAN_POINT_SHADOW_IDX_HIGH_RES);
    }
    if (light.lowResShadowMapIndex >= 0) {
        return ShadowCalculationSkinBindless(light.lowResShadowMapIndex, lightPosition, light.radius, worldPosition, viewPosition, normal, VULKAN_POINT_SHADOW_IDX_LOW_RES);
    }
    return 1.0;
}

float ShadowCalculationMediumBindless(int lightIndex, vec3 lightPosition, float lightRadius, vec3 worldPosition, vec3 viewPosition, vec3 normal, uint shadowResourceIndex) {
    vec3 lightToFrag = worldPosition - lightPosition;
    vec3 L = normalize(-lightToFrag);
    float currentDepth = length(lightToFrag);
    float far_plane = lightRadius;
    float shadow = 0.0;

    float cosTheta = clamp(dot(normal, L), 0.0, 1.0);
    float bias = max(0.05 * (1.0 - cosTheta), 0.005);

    int samples = 8;
    float viewDistance = length(viewPosition - worldPosition);
    float diskRadius = (1.0 + (viewDistance / far_plane)) / 200.0;

    for (int i = 0; i < samples; ++i) {
        vec3 sampleDir = lightToFrag + gridSamplingDisk[i] * diskRadius;
        float compareDepth = (currentDepth - bias) / far_plane;
        float visibility = texture(samplerCubeArrayShadow(pointShadowMaps[nonuniformEXT(shadowResourceIndex)], pointShadowSamplers[nonuniformEXT(shadowResourceIndex)]), vec4(sampleDir, float(lightIndex)), compareDepth);
        shadow += 1.0 - visibility;
    }

    shadow /= float(samples);
    return 1.0 - shadow;
}

#endif
