#include "../viewport.glsl"

vec3 SampleSurfaceGuidedBilateralUpscale_VK(texture2D lightingTexture, sampler lightingSampler, texture2D surfaceTexture, sampler surfaceSampler, vec2 screenUV, vec3 surfaceNormal, float surfaceViewDistance, ivec2 fullSize, ivec4 viewportRect, float normalPower, float minDepthSigma, float depthSigmaScale) {
    ivec2 lowSize = textureSize(sampler2D(lightingTexture, lightingSampler), 0);
    if (lowSize.x <= 0 || lowSize.y <= 0) {
        return vec3(0.0);
    }

    vec2 lowScale = vec2(lowSize) / vec2(fullSize);
    ivec2 viewportMin = VulkanViewportRectFromScreenRect(viewportRect, fullSize).xy;
    ivec2 viewportMax = viewportMin + viewportRect.zw;
    ivec2 lowViewportMin = ivec2(floor(vec2(viewportMin) * lowScale));
    ivec2 lowViewportMax = ivec2(ceil(vec2(viewportMax) * lowScale)) - ivec2(1);

    lowViewportMin = clamp(lowViewportMin, ivec2(0), lowSize - ivec2(1));
    lowViewportMax = clamp(max(lowViewportMax, lowViewportMin), ivec2(0), lowSize - ivec2(1));

    vec2 lowPos = screenUV * vec2(lowSize) - vec2(0.5);
    ivec2 lowBase = ivec2(floor(lowPos));
    vec2 f = fract(lowPos);

    vec3 normal = normalize(surfaceNormal);
    float depthSigma = max(minDepthSigma, surfaceViewDistance * depthSigmaScale);

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    for (int y = 0; y <= 1; y++) {
        for (int x = 0; x <= 1; x++) {
            ivec2 lowPx = clamp(lowBase + ivec2(x, y), lowViewportMin, lowViewportMax);

            vec4 lowSurface = texelFetch(sampler2D(surfaceTexture, surfaceSampler), lowPx, 0);
            vec3 lowNormal = lowSurface.xyz;
            if (dot(lowNormal, lowNormal) < 0.0001) {
                continue;
            }

            float spatialWeight = ((x == 0) ? 1.0 - f.x : f.x) * ((y == 0) ? 1.0 - f.y : f.y);
            float normalWeight = pow(max(dot(normal, normalize(lowNormal)), 0.0), normalPower);
            float depthWeight = exp2(-abs(surfaceViewDistance - lowSurface.w) / depthSigma);
            float weight = spatialWeight * normalWeight * depthWeight;

            result += texelFetch(sampler2D(lightingTexture, lightingSampler), lowPx, 0).rgb * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0001) {
        ivec2 nearestPx = clamp(ivec2(floor(screenUV * vec2(lowSize))), lowViewportMin, lowViewportMax);
        return texelFetch(sampler2D(lightingTexture, lightingSampler), nearestPx, 0).rgb;
    }

    return result / totalWeight;
}
