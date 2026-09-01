
#define MISC_FLAG_DYNAMIC_OBJECT (1u << 0)
#define MISC_FLAG_RESEVERED      (1u << 1)
#define MISC_FLAG_MIRROR_SURFACE MISC_FLAG_RESEVERED

#define SHADOW_FLAG_NONE        0u
#define SHADOW_FLAG_POINT_LIGHT (1u << 0)
#define SHADOW_FLAG_CSM         (1u << 1)

#ifdef __cplusplus
#pragma once

static float EncodeMiscFlags(unsigned int flags) {
    return float(flags & 3u) / 3.0f;
}

static unsigned int DecodeMiscFlags(float value) {
    float clamped = value < 0.0f ? 0.0f : value > 1.0f ? 1.0f : value;
    return static_cast<unsigned int>(clamped * 3.0f + 0.5f);
}

#else

float EncodeMiscFlags(uint flags) {
    return float(flags & 3u) / 3.0;
}

uint DecodeMiscFlags(float value) {
    float clamped = value < 0.0 ? 0.0 : value > 1.0 ? 1.0 : value;
    return uint(clamped * 3.0 + 0.5);
}

#endif
