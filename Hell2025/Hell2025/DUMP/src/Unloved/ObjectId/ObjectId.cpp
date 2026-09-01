#include "Unloved/ObjectId/ObjectId.h"

#include <atomic>

namespace {
    std::atomic<uint64_t> g_globalObjectId { 1 };
    std::atomic<uint32_t> g_globalCustomId { 1 };

    uint64_t MakeId(ObjectType type, uint64_t local) {
        return (uint64_t(static_cast<uint16_t>(type)) << kTypeShift) | (local & kLocalMask);
    }
}

namespace Unloved {
    uint64_t GetNextObjectId(ObjectType type) {
        return MakeId(type, g_globalObjectId++);
    }

    uint32_t GetNextCustomId() {
        return g_globalCustomId++;
    }
}
