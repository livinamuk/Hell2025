#pragma once

#include "ResourceType.h"

#include <cstdint>

namespace Hell::ResourceManagement {

inline constexpr uint32_t kResourceTypeBits = 16;
inline constexpr uint64_t kResourceTypeShift = 64 - kResourceTypeBits;
inline constexpr uint64_t kResourceTypeMask = ((1ull << kResourceTypeBits) - 1) << kResourceTypeShift;
inline constexpr uint64_t kResourceLocalMask = ~kResourceTypeMask;

inline ResourceType GetType(uint64_t id) {
    return static_cast<ResourceType>((id >> kResourceTypeShift) & ((1ull << kResourceTypeBits) - 1));
}

inline uint64_t GetLocal(uint64_t id) {
    return id & kResourceLocalMask;
}

uint64_t GetNextID(ResourceType type);

}
