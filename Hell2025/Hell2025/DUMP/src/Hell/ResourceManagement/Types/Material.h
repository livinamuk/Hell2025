#pragma once
#include <cstdint>

struct Material {
    int32_t m_basecolor = -1;
    int32_t m_normal = -1;
    int32_t m_rma = -1;
    int32_t m_emissive = -1;

    int32_t m_opacity = -1;
    int32_t m_hairMaps = -1;
    int32_t m_displacement = 0;
    int32_t m_padding1 = 0;

    // Terrain3D TextureAsset displacement controls: offset first, scale second.
    float m_displacementOffset = 0.0f;
    float m_displacementScale = 1.0f;
    int32_t m_padding2 = 0;
    int32_t m_padding3 = 0;
};

static_assert(sizeof(Material) == 48);
