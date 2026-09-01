#pragma once

#include "Hell/Containers/SlotMap.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"

#include <cstdint>
#include <vector>

namespace Unloved::DDGIManager {
    Hell::SlotMap<DDGIVolume>& GetVolumes();
    DDGIVolume* GetVolumeByObjectId(uint64_t objectId);
    const std::vector<uint64_t>& GetProbeUpdateVolumeIds();
    uint32_t GetTotalProbeCount();
    uint64_t GetProbeResetVersion();

    uint64_t AddVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset = SpawnOffset());
    bool RemoveVolume(uint64_t objectId);

    void Update();
    void CleanUp();

    void ResetProbes();
    bool ConsumeProbeResetRequest();
}
