uint GetHairSampleMask4x(vec2 uv, vec2 textureSizePixels, float alpha) {
    vec2 uvTexels = uv * textureSizePixels;

    vec2 dx = dFdx(uvTexels);
    vec2 dy = dFdy(uvTexels);
    float mipLevel = 0.5 * log2(max(dot(dx, dx), dot(dy, dy)));

    float alphaPivot = 0.025;
    float alphaSharpness = 0.75;
    float alphaBaseBoost = 3.0;

    float boost = max(alphaBaseBoost, mipLevel * alphaSharpness);
    alpha = (alpha - alphaPivot) * boost + alphaPivot;
    alpha = clamp(alpha, 0.0, 1.0);

    uint mask = 0u;
    if (alpha > 0.10) mask |= 1u;
    if (alpha > 0.35) mask |= 2u;
    if (alpha > 0.65) mask |= 4u;
    if (alpha > 0.90) mask |= 8u;

    return mask;
}