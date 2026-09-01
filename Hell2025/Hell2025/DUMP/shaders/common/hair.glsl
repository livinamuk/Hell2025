#define HAIR_DARK_AREA_STRENGTH 0.5

vec3 DarkenHairBaseColor(vec3 linearColor) {
    float luminance = dot(linearColor, vec3(0.2126, 0.7152, 0.0722));
    float darkArea = 1.0 - smoothstep(0.0, 0.25, luminance);
    return linearColor * (1.0 - darkArea * HAIR_DARK_AREA_STRENGTH);
}

float ComputeHairMipLevel(vec2 uv, vec2 textureSizePixels) {
    vec2 uvTexels = uv * textureSizePixels;
    vec2 dx = dFdx(uvTexels);
    vec2 dy = dFdy(uvTexels);
    return max(0.0, 0.5 * log2(max(dot(dx, dx), dot(dy, dy))));
}

float ComputeHairCoverageAlphaFromMip(float alpha, float mipLevel) {
    return clamp((alpha - 0.025) * max(1.0, mipLevel * 0.75) + 0.025, 0.0, 1.0);
}

float ComputeHairCoverageAlpha(float alpha, vec2 uv, vec2 textureSizePixels) {
    float mipLevel = ComputeHairMipLevel(uv, textureSizePixels);
    return ComputeHairCoverageAlphaFromMip(alpha, mipLevel);
}
