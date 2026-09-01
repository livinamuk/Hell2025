#ifndef COMMON_OCEAN_GLSL
#define COMMON_OCEAN_GLSL

#include "ocean_fft.glsl"

const int OCEAN_FFT_BAND_COUNT = 2;
const int OCEAN_DISPLAY_MODE_COMBINED = 0;
const int OCEAN_DISPLAY_MODE_BAND_0 = 1;
const int OCEAN_DISPLAY_MODE_BAND_1 = 2;

uniform float u_domainSize[OCEAN_FFT_BAND_COUNT];

float OceanDisplacementScale(int bandIndex) {
    return u_domainSize[bandIndex] / float(OCEAN_FFT_RESOLUTION);
}

vec2 OceanUV(vec2 worldXZ, int bandIndex) {
    return worldXZ / u_domainSize[bandIndex];
}

vec2 EstimateOceanUndisplacedXZ(sampler2D displacementTexture, vec2 worldXZ, int bandIndex) {
    vec3 displacement = texture(displacementTexture, OceanUV(worldXZ, bandIndex)).xyz;
    return worldXZ - displacement.xz * OceanDisplacementScale(bandIndex);
}

vec3 SampleOceanDisplacement(sampler2D displacementTexture, vec2 worldXZ, int bandIndex) {
    return textureLod(displacementTexture, OceanUV(worldXZ, bandIndex), 0.0).xyz * OceanDisplacementScale(bandIndex);
}

vec3 SampleEstimatedOceanDisplacement(sampler2D displacementTexture, vec2 worldXZ, int bandIndex) {
    vec2 estimatedWorldXZ = EstimateOceanUndisplacedXZ(displacementTexture, worldXZ, bandIndex);
    return texture(displacementTexture, OceanUV(estimatedWorldXZ, bandIndex)).xyz * OceanDisplacementScale(bandIndex);
}

vec2 SampleEstimatedOceanNormalXZ(sampler2D displacementTexture, sampler2D slopeTexture, vec2 worldXZ, int bandIndex, float lod) {
    vec2 estimatedWorldXZ = EstimateOceanUndisplacedXZ(displacementTexture, worldXZ, bandIndex);
    return textureLod(slopeTexture, OceanUV(estimatedWorldXZ, bandIndex), lod).xy * OceanDisplacementScale(bandIndex);
}

vec3 SelectOceanBands(vec3 band0, vec3 band1, int displayMode) {
    if (displayMode == OCEAN_DISPLAY_MODE_BAND_0) return band0;
    if (displayMode == OCEAN_DISPLAY_MODE_BAND_1) return band1;
    return band0 + band1;
}

vec2 SelectOceanBands(vec2 band0, vec2 band1, int displayMode) {
    if (displayMode == OCEAN_DISPLAY_MODE_BAND_0) return band0;
    if (displayMode == OCEAN_DISPLAY_MODE_BAND_1) return band1;
    return band0 + band1;
}

vec3 SampleCombinedOceanDisplacement(sampler2D displacementBand0, sampler2D displacementBand1, vec2 worldXZ, int displayMode) {
    vec3 band0 = SampleOceanDisplacement(displacementBand0, worldXZ, 0);
    vec3 band1 = SampleOceanDisplacement(displacementBand1, worldXZ, 1);
    return SelectOceanBands(band0, band1, displayMode);
}

vec3 SampleCombinedEstimatedOceanDisplacement(sampler2D displacementBand0, sampler2D displacementBand1, vec2 worldXZ, int displayMode) {
    vec3 band0 = SampleEstimatedOceanDisplacement(displacementBand0, worldXZ, 0);
    vec3 band1 = SampleEstimatedOceanDisplacement(displacementBand1, worldXZ, 1);
    return SelectOceanBands(band0, band1, displayMode);
}

vec2 SampleCombinedEstimatedOceanNormalXZ(sampler2D displacementBand0, sampler2D slopeBand0, sampler2D displacementBand1, sampler2D slopeBand1, vec2 worldXZ, float lod0, float lod1, int displayMode) {
    vec2 band0 = SampleEstimatedOceanNormalXZ(displacementBand0, slopeBand0, worldXZ, 0, lod0);
    vec2 band1 = SampleEstimatedOceanNormalXZ(displacementBand1, slopeBand1, worldXZ, 1, lod1);
    return SelectOceanBands(band0, band1, displayMode);
}

float OceanNormalLod(vec2 worldDdx, vec2 worldDdy, int bandIndex) {
    float footprintSq = max(dot(worldDdx, worldDdx), dot(worldDdy, worldDdy));
    float log2FootprintHalf = 0.5 * log2(max(footprintSq, 0.00000001));
    return max(0.0, log2FootprintHalf - log2(OceanDisplacementScale(bandIndex)));
}

#endif
