#pragma once

#include <cstdint>

namespace VulkanDeletionQueue {
    void SetFrameIndex(uint32_t frameIndex);
    void Flush(uint32_t frameIndex);
    void FlushAll();

    void Retire(uint64_t id);
}
