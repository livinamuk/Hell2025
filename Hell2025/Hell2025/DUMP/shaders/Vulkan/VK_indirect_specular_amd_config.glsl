#ifndef VK_INDIRECT_SPECULAR_AMD_CONFIG_GLSL
#define VK_INDIRECT_SPECULAR_AMD_CONFIG_GLSL

#define VK_INDIRECT_SPECULAR_AMD_ROUGHNESS_THRESHOLD 0.5

// FidelityFX SDK 1.1.4 SSSR IsMirrorReflection threshold.
// Mirror rays are never deactivated by the per-quad scheduler
#define VK_INDIRECT_SPECULAR_AMD_MIRROR_ROUGHNESS_THRESHOLD 0.0001

// Internal values used only between the sparse ray pass and its resolve pass
// Valid reflection-ray lengths are non-negative; -1 remains the ray shader's invalid-direction result.
#define VK_INDIRECT_SPECULAR_AMD_COPY_HORIZONTAL (-2.0)
#define VK_INDIRECT_SPECULAR_AMD_COPY_VERTICAL (-3.0)
#define VK_INDIRECT_SPECULAR_AMD_COPY_DIAGONAL (-4.0)

// FidelityFX SDK 1.1.4 SSSR and Hybrid Reflections sample default
#define VK_INDIRECT_SPECULAR_AMD_TEMPORAL_STABILITY_FACTOR 0.7

#endif
