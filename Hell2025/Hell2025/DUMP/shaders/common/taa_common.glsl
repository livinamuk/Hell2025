#ifndef TAA_COMMON_GLSL
#define TAA_COMMON_GLSL

#define TAA_RADIUS 1
#define TAA_GROUP_SIZE 16
#define TAA_TILE_DIM (2 * TAA_RADIUS + TAA_GROUP_SIZE)
#define TAA_TILE_SAMPLE_COUNT (TAA_TILE_DIM * TAA_TILE_DIM)

// Source: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1

vec3 TAAReinhard(vec3 hdr) {
    return hdr / (hdr + 1.0);
}

vec3 TAAReinhardInverse(vec3 sdr) {
    return sdr / max(vec3(1.0) - sdr, vec3(1e-3));
}

vec3 TAASampleHistoryCatmullRom(sampler2D historyBuffer, vec2 uv, vec2 texelSize, vec2 viewportUvMin, vec2 viewportUvMax) {
    vec2 samplePos = uv / texelSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / w12;

    vec2 texPos0 = (texPos1 - 1.0) * texelSize;
    vec2 texPos3 = (texPos1 + 2.0) * texelSize;
    vec2 texPos12 = (texPos1 + offset12) * texelSize;

    vec2 uv00 = clamp(vec2(texPos0.x, texPos0.y), viewportUvMin, viewportUvMax);
    vec2 uv10 = clamp(vec2(texPos12.x, texPos0.y), viewportUvMin, viewportUvMax);
    vec2 uv20 = clamp(vec2(texPos3.x, texPos0.y), viewportUvMin, viewportUvMax);
    vec2 uv01 = clamp(vec2(texPos0.x, texPos12.y), viewportUvMin, viewportUvMax);
    vec2 uv11 = clamp(vec2(texPos12.x, texPos12.y), viewportUvMin, viewportUvMax);
    vec2 uv21 = clamp(vec2(texPos3.x, texPos12.y), viewportUvMin, viewportUvMax);
    vec2 uv02 = clamp(vec2(texPos0.x, texPos3.y), viewportUvMin, viewportUvMax);
    vec2 uv12 = clamp(vec2(texPos12.x, texPos3.y), viewportUvMin, viewportUvMax);
    vec2 uv22 = clamp(vec2(texPos3.x, texPos3.y), viewportUvMin, viewportUvMax);

    vec3 result = vec3(0.0);
    result += textureLod(historyBuffer, uv00, 0.0).rgb * w0.x * w0.y;
    result += textureLod(historyBuffer, uv10, 0.0).rgb * w12.x * w0.y;
    result += textureLod(historyBuffer, uv20, 0.0).rgb * w3.x * w0.y;

    result += textureLod(historyBuffer, uv01, 0.0).rgb * w0.x * w12.y;
    result += textureLod(historyBuffer, uv11, 0.0).rgb * w12.x * w12.y;
    result += textureLod(historyBuffer, uv21, 0.0).rgb * w3.x * w12.y;

    result += textureLod(historyBuffer, uv02, 0.0).rgb * w0.x * w3.y;
    result += textureLod(historyBuffer, uv12, 0.0).rgb * w12.x * w3.y;
    result += textureLod(historyBuffer, uv22, 0.0).rgb * w3.x * w3.y;

    return max(result, vec3(0.0));
}

#endif
