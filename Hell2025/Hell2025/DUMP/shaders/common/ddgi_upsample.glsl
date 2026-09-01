const float DDGI_UPSAMPLE_NORMAL_POWER = 32.0;
const float DDGI_UPSAMPLE_MIN_DEPTH_SIGMA = 0.25;
const float DDGI_UPSAMPLE_DEPTH_SIGMA_SCALE = 0.03;

vec3 SampleDDGIIndirectDiffuseBilateral(sampler2D irradianceTexture, sampler2D surfaceTexture, vec2 screenUV, vec3 normal, float viewDistance, ivec2 fullSize, ivec4 viewportRect) {
    ivec2 lowSize = textureSize(irradianceTexture, 0);
    if (lowSize.x <= 0 || lowSize.y <= 0) {
        return vec3(0.0);
    }

    vec2 lowScale = vec2(lowSize) / vec2(fullSize);
    ivec2 lowViewportMin = ivec2(floor(vec2(viewportRect.xy) * lowScale));
    ivec2 lowViewportMax = ivec2(ceil(vec2(viewportRect.xy + viewportRect.zw) * lowScale)) - ivec2(1);

    lowViewportMin = clamp(lowViewportMin, ivec2(0), lowSize - ivec2(1));
    lowViewportMax = clamp(max(lowViewportMax, lowViewportMin), ivec2(0), lowSize - ivec2(1));

    vec2 lowPos = screenUV * vec2(lowSize) - vec2(0.5);
    ivec2 lowBase = ivec2(floor(lowPos));
    vec2 f = fract(lowPos);

    vec3 n = normalize(normal);
    float depthSigma = max(DDGI_UPSAMPLE_MIN_DEPTH_SIGMA, viewDistance * DDGI_UPSAMPLE_DEPTH_SIGMA_SCALE);

    vec3 result = vec3(0.0);
    float totalWeight = 0.0;

    for (int y = 0; y <= 1; y++) {
        for (int x = 0; x <= 1; x++) {
            ivec2 lowPx = clamp(lowBase + ivec2(x, y), lowViewportMin, lowViewportMax);

            vec4 surface = texelFetch(surfaceTexture, lowPx, 0);
            vec3 sampleNormal = surface.xyz;
            if (dot(sampleNormal, sampleNormal) < 0.0001) {
                continue;
            }

            float spatialWeight = ((x == 0) ? 1.0 - f.x : f.x) * ((y == 0) ? 1.0 - f.y : f.y);
            float normalWeight = pow(max(dot(n, normalize(sampleNormal)), 0.0), DDGI_UPSAMPLE_NORMAL_POWER);
            float depthWeight = exp2(-abs(viewDistance - surface.w) / depthSigma);
            float weight = spatialWeight * normalWeight * depthWeight;

            result += texelFetch(irradianceTexture, lowPx, 0).rgb * weight;
            totalWeight += weight;
        }
    }

    if (totalWeight <= 0.0001) {
        ivec2 nearestPx = clamp(ivec2(floor(screenUV * vec2(lowSize))), lowViewportMin, lowViewportMax);
        return texelFetch(irradianceTexture, nearestPx, 0).rgb;
    }

    return result / totalWeight;
}
