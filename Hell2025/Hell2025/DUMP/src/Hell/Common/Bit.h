#pragma once

#include <cstdint>

namespace Hell::Bit {

    inline void SetState(uint32_t& bitmask, uint32_t bit, bool state) {
        bitmask = (bitmask & ~bit) | (state ? bit : 0);
    }

    inline bool Contains(uint32_t bitmask, uint32_t bit) {
        return (bitmask & bit) != 0u;
    }

    inline void PackUint64(uint64_t value, uint32_t& xOut, uint32_t& yOut) {
        xOut = static_cast<uint32_t>((value & 0xffffffff00000000ull) >> 32);
        yOut = static_cast<uint32_t>(value & 0xffffffffull);
    }

    inline void UnpackUint64(uint32_t xValue, uint32_t yValue, uint64_t& out) {
        out = (static_cast<uint64_t>(xValue) << 32) | yValue;
    }
}
