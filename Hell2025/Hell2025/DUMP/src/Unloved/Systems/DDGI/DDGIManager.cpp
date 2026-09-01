#include "DDGIManager.h"

#include "Hell/Math/AABB.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/EditorSession/ObjectNames.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Systems/DDGI/DDGITypes.h"
#include "Unloved/Viewport/ViewportManager.h"

#include <algorithm>
#include <glm/geometric.hpp>
#include <limits>

namespace Unloved::DDGIManager {

    Hell::SlotMap<DDGIVolume> g_volumes;
    std::vector<uint64_t> g_probeUpdateVolumeIds;

    bool g_probeResetRequested = false;
    uint64_t g_probeResetVersion = 0;
    uint32_t g_totalProbeCount = 0;
    constexpr uint32_t PROBE_UPDATE_VOLUME_BUDGET = 2;

    void RebuildProbeBufferLayout() {
        uint32_t probeOffset = 0;

        for (DDGIVolume& volume : g_volumes) {
            volume.SetProbeOffset(probeOffset);
            probeOffset += volume.GetTotalProbeCount();
        }

        g_totalProbeCount = probeOffset;
        g_probeResetRequested = true;
        g_probeResetVersion++;
    }

    float DistanceToAABB(const glm::vec3& point, const AABB& aabb) {
        const glm::vec3 nearestPoint = aabb.NearestPointTo(point);
        return glm::length(nearestPoint - point);
    }

    bool CandidateSort(const DDGIProbeUpdateCandidate& a, const DDGIProbeUpdateCandidate& b) {
        if (a.nearestCameraDistance != b.nearestCameraDistance) {
            return a.nearestCameraDistance < b.nearestCameraDistance;
        }

        if (a.framesSinceLastProbeUpdate != b.framesSinceLastProbeUpdate) {
            return a.framesSinceLastProbeUpdate > b.framesSinceLastProbeUpdate;
        }

        return a.volumeId < b.volumeId;
    }

    bool CatchupSort(const DDGIProbeUpdateCandidate& a, const DDGIProbeUpdateCandidate& b) {
        if (a.framesSinceLastProbeUpdate != b.framesSinceLastProbeUpdate) {
            return a.framesSinceLastProbeUpdate > b.framesSinceLastProbeUpdate;
        }

        if (a.nearestCameraDistance != b.nearestCameraDistance) {
            return a.nearestCameraDistance < b.nearestCameraDistance;
        }

        return a.volumeId < b.volumeId;
    }

    void RebuildProbeUpdateVolumeIds() {
        g_probeUpdateVolumeIds.clear();

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        if (viewportData.size() < MAX_VIEWPORT_COUNT) return;

        std::vector<DDGIProbeUpdateCandidate> candidates;

        for (size_t volumeIndex = 0; volumeIndex < g_volumes.size(); volumeIndex++) {
            DDGIVolume& volume = g_volumes[volumeIndex];
            const uint64_t volumeId = g_volumes.id_at(volumeIndex);
            const AABB volumeBounds(volume.GetBoundsMin(), volume.GetBoundsMax());

            bool isVisible = false;
            float nearestCameraDistance = std::numeric_limits<float>::max();

            for (uint32_t viewportIndex = 0; viewportIndex < MAX_VIEWPORT_COUNT; viewportIndex++) {
                Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
                if (!viewport || !viewport->IsVisible()) continue;
                if (!viewport->GetFrustum().IntersectsAABBFast(volumeBounds)) continue;

                isVisible = true;
                nearestCameraDistance = std::min(nearestCameraDistance, DistanceToAABB(glm::vec3(viewportData[viewportIndex].viewPos), volumeBounds));
            }

            if (!isVisible) continue;

            DDGIProbeUpdateCandidate& candidate = candidates.emplace_back();
            candidate.volumeId = volumeId;
            candidate.nearestCameraDistance = nearestCameraDistance;
            candidate.framesSinceLastProbeUpdate = volume.GetFramesSinceLastProbeUpdate();
        }

        if (candidates.empty()) return;

        std::sort(candidates.begin(), candidates.end(), CandidateSort);

        if (candidates.size() > 1) {
            std::sort(candidates.begin() + 1, candidates.end(), CatchupSort);
        }

        for (const DDGIProbeUpdateCandidate& candidate : candidates) {
            if (g_probeUpdateVolumeIds.size() >= PROBE_UPDATE_VOLUME_BUDGET) break;

            g_probeUpdateVolumeIds.push_back(candidate.volumeId);
        }
    }

    Hell::SlotMap<DDGIVolume>& GetVolumes() {
        return g_volumes;
    }

    DDGIVolume* GetVolumeByObjectId(uint64_t objectId) {
        return g_volumes.get(objectId);
    }

    const std::vector<uint64_t>& GetProbeUpdateVolumeIds() {
        return g_probeUpdateVolumeIds;
    }

    uint32_t GetTotalProbeCount() {
        return g_totalProbeCount;
    }

    uint64_t GetProbeResetVersion() {
        return g_probeResetVersion;
    }

    uint64_t AddVolume(DDGIVolumeCreateInfo createInfo, SpawnOffset spawnOffset) {
        EditorSession::AssignEditorName(createInfo, g_volumes);

        const uint64_t id = Unloved::GetNextObjectId(ObjectType::DDGI_VOLUME);
        g_volumes.emplace_with_id(id, id, createInfo, spawnOffset);
        RebuildProbeBufferLayout();
        return id;
    }

    bool RemoveVolume(uint64_t objectId) {
        DDGIVolume* volume = g_volumes.get(objectId);
        if (!volume) return false;

        volume->CleanUp();
        g_volumes.erase(objectId);
        RebuildProbeBufferLayout();
        return true;
    }

    void Update() {
        for (DDGIVolume& volume : g_volumes) {
            volume.Update();
        }

        RebuildProbeUpdateVolumeIds();
    }

    void CleanUp() {
        for (DDGIVolume& volume : g_volumes) {
            volume.CleanUp();
        }

        g_volumes.clear();
        g_probeUpdateVolumeIds.clear();
        g_probeResetRequested = false;
        g_probeResetVersion++;
        g_totalProbeCount = 0;
    }

    void ResetProbes() {
        g_probeResetRequested = true;
        g_probeResetVersion++;
    }

    bool ConsumeProbeResetRequest() {
        const bool requested = g_probeResetRequested;
        g_probeResetRequested = false;
        return requested;
    }
}
