#pragma once
#include "Mirror.h"

#include "Hell/Containers/SlotMap.h"

#include <cstdint>

namespace Unloved::MirrorManager {
    void AddMirror(uint64_t parentId, uint32_t meshNodeIndex, uint32_t globalMeshIndex);
    void Update();
    void CleanUp();

    Mirror* GetMirrorByObjectId(uint64_t objectId);
    Hell::SlotMap<Mirror>& GetMirrors();
}
