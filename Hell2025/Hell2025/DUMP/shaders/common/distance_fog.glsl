
vec3 DistanceFog(vec3 inputColor, float fragDistance) {
    float fogStartDist = 5;
    float fogEndDist = 25;
    float fogCurve = 5;
    float fogStrength = 1.0;
    vec3 fogColor = vec3(0,0,0);
    float distanceFogFactor = pow(clamp((fragDistance - fogStartDist) / max(fogEndDist - fogStartDist, 1e-6), 0.0, 1.0), max(fogCurve, 1e-6));
    float fogAlpha = distanceFogFactor * fogStrength;
    return mix(inputColor, fogColor, fogAlpha);
}