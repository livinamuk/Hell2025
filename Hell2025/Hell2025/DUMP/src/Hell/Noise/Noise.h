#pragma once

#include <cstdint>
#include <cmath>

#include <glm/common.hpp>

namespace Hell::Noise {

    inline float Smoothstep01(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    inline uint32_t Hash32(uint32_t x) {
        x ^= x >> 16;
        x *= 0x7feb352dU;
        x ^= x >> 15;
        x *= 0x846ca68bU;
        x ^= x >> 16;
        return x;
    }

    inline float Random01FromInt(int32_t i) {
        uint32_t h = Hash32(static_cast<uint32_t>(i));
        return static_cast<float>(h) * (1.0f / 4294967295.0f);
    }

    inline float ValueNoise1D(float x, int32_t seed) {
        float xf = std::floor(x);
        int32_t x0 = static_cast<int32_t>(xf);
        int32_t x1 = x0 + 1;

        float t = x - xf;
        float s = Smoothstep01(t);

        float v0 = Random01FromInt(x0 + seed * 1013);
        float v1 = Random01FromInt(x1 + seed * 1013);

        return glm::mix(v0, v1, s);
    }

    inline float FractalNoise1D(float x, int32_t seed) {
        float n0 = ValueNoise1D(x * 1.0f, seed + 11);
        float n1 = ValueNoise1D(x * 2.1f, seed + 37);
        float n2 = ValueNoise1D(x * 4.3f, seed + 73);
        float sum = n0 * 0.60f + n1 * 0.30f + n2 * 0.10f;
        return glm::clamp(sum, 0.0f, 1.0f);
    }
}
