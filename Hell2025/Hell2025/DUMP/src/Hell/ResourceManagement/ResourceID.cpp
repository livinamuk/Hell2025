#include "ResourceID.h"

#include <atomic>

namespace {
    std::atomic<uint64_t> g_nextResourceId { 1 };
}

namespace Hell::ResourceManagement {

uint64_t GetNextID(ResourceType type) {
    const uint64_t local = g_nextResourceId++;
    return (uint64_t(static_cast<uint16_t>(type)) << kResourceTypeShift) | (local & kResourceLocalMask);
}

}
