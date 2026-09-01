
const int MAX_CASCADES = 5;
const int PCF_SAMPLES = 3;
const int SHADOW_CASCADE_COUNT = 5;

vec3 gridSamplingDiskCSM[20] = vec3[](
    vec3(1, 1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1, 1,  1),
    vec3(1, 1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1, 1,  0),
    vec3(1, 0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1, 0, -1),
    vec3(0, 1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0, 1, -1)
);

vec3 ShadowCalculationCSM(vec3 fragPosWorldSpace, vec3 normal, vec3 lightDir, mat4 viewMatrix, uint viewportIndex) {
    int SHADOW_CASCADE_COUNT = 5;

    // select cascade layer
    vec4 fragPosViewSpace = viewMatrix * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = SHADOW_CASCADE_COUNT - 1;
    for (int i = 0; i < SHADOW_CASCADE_COUNT - 1; ++i) {
        if (depthValue < u_cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }

    mat4 lightProjectionView = viewportDataArr[viewportIndex].csmLightProjectionView[layer];

    vec3 normalOffset = normal * 0.05;
    vec4 fragPosLightSpace = lightProjectionView * vec4(fragPosWorldSpace + normalOffset, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // Calculate bias (based on depth map resolution and slope)
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
    const float biasModifier = 10.5;
    float scaledBias = bias;
    if (layer == SHADOW_CASCADE_COUNT - 1) {
        scaledBias *= 1.0 / (u_cascadeFarPlane * biasModifier);
    }
    else {
        scaledBias *= 1.0 / (u_cascadePlaneDistances[layer] * biasModifier);
    }

    // Disk Sampling for PCF
    float shadow = 0.0;
    int samples = 3;
    vec2 texelSize = 1.0 / vec2(textureSize(u_shadowMapCascadeArray, 0));
    float diskRadius = 2.0; // Radius in texture space (adjust as needed)

    uint arrayIndex = uint(layer) + (viewportIndex * SHADOW_CASCADE_COUNT);

    for (int i = 0; i < samples; ++i) {
        vec2 offset = gridSamplingDiskCSM[i].xy * diskRadius * texelSize;
        vec3 shadowUV = vec3(clamp(projCoords.xy + offset, 0.0, 1.0), float(arrayIndex));
        float pcfDepth = textureLod(u_shadowMapCascadeArray, shadowUV, 0.0).r;
        shadow += (currentDepth - scaledBias) > pcfDepth ? 1.0 : 0.0;
    }

    shadow /= float(samples);

    return vec3(1 - shadow);
}
