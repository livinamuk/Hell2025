#include "DirtyTracker.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"
#include "Hell/Math/AABB.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/World/World.h"

#include <unordered_map>

namespace Unloved::DirtyTracker {

    void CalculateDirtyDoorAABBs();
    bool IntersectAABB(const RenderItem& renderItemA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const RenderItem& renderItemB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB);
    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB);
    void UpdateLightRayTracingDirtyFlags();
    void UpdateDirtyLightIds();

    void DebugDrawLightIds();
    void DebugDrawDirtyBounds();
    void DebugPrintDirtyLightInfo(const Light& light, uint64_t intersectingObjectId);
    void DebugMessageOfAllDirtyLights();

    std::vector<uint64_t> g_dirtyDoorIds;
    std::vector<uint64_t> g_staticDirtyLightIds;
    std::vector<uint64_t> g_compositeDirtyLightIds;
    std::vector<DirtyBounds> g_dirtyBoundsSet;

    const std::vector<uint64_t>& GetDirtyDoorIds()           { return g_dirtyDoorIds; }
    const std::vector<uint64_t>& GetStaticDirtyLightIds()    { return g_staticDirtyLightIds; }
    const std::vector<uint64_t>& GetCompositeDirtyLightIds() { return g_compositeDirtyLightIds; }

    std::vector<GPUAABB> g_dirtyDoorAABBs;
    std::unordered_map<uint64_t, AABB> g_previousDoorAABBs;

    void BeginFrame() {
        g_dirtyDoorIds.clear();
        g_staticDirtyLightIds.clear();
        g_compositeDirtyLightIds.clear();
        g_dirtyBoundsSet.clear();
    }

    void Update() {
        CalculateDirtyDoorAABBs();
        UpdateLightRayTracingDirtyFlags();

        // Doors
        for (const DirtyBounds& dirtyBounds : g_dirtyBoundsSet) {

            // Skip non doors
            if (Unloved::GetObjectIdType(dirtyBounds.objectId) != ObjectType::DOOR) {
                continue;
            }

            bool found = false;

            // Check whether the ID is already in there
            for (uint64_t dirtyId : g_dirtyDoorIds) {
                if (dirtyId == dirtyBounds.objectId) {
                    found = true;
                    break;
                }
            }

            // Add it if it isn't
            if (!found) {
                g_dirtyDoorIds.push_back(dirtyBounds.objectId);
            }
        }

        UpdateDirtyLightIds();

        //DebugDrawDirtyBounds();
        //DebugDrawLightIds();
        //DebugMessageOfAllDirtyLights();
    }

    void UpdateDirtyLightIds() {
        for (Light& light : World::GetLights()) {
            bool staticDirty = false;
            bool compositeDirty = false;

            if (light.IsForcedDirty()) {
                light.ConsumeForcedDirtyFlag();
                staticDirty = true;
                compositeDirty = true;
            }
            else {
                for (const DirtyBounds& dirtyBounds : g_dirtyBoundsSet) {
                    if (!dirtyBounds.castShadows) {
                        continue;
                    }

                    if (IntersectAABB(light.GetWorldBoundsMin(), light.GetWorldBoundsMax(), dirtyBounds.boundsMin, dirtyBounds.boundsMax)) {
                        compositeDirty = true;

                        if (dirtyBounds.type == DirtyBoundsType::STATIC) {
                            staticDirty = true;
                            break;
                        }
                    }
                }
            }

            if (staticDirty) {
                g_staticDirtyLightIds.push_back(light.GetObjectId());
            }
            if (compositeDirty) {
                g_compositeDirtyLightIds.push_back(light.GetObjectId());
            }
        }
    }

    void AddDirtyBounds(const DirtyBounds& dirtyBounds) {
        // Bail if invalid AABB was passed in
        if (dirtyBounds.boundsMin.x > dirtyBounds.boundsMax.x ||
            dirtyBounds.boundsMin.y > dirtyBounds.boundsMax.y ||
            dirtyBounds.boundsMin.z > dirtyBounds.boundsMax.z) {
            return;
        }

        g_dirtyBoundsSet.push_back(dirtyBounds);
    }

    const std::vector<GPUAABB>& GetDirtyDoorAABBs() {
        return g_dirtyDoorAABBs;
    }

    void CalculateDirtyDoorAABBs() {
        g_dirtyDoorAABBs.clear();

        for (Door& door : Unloved::World::GetDoors()) {
            const uint64_t doorId = door.GetObjectId();
            const AABB& currentAABB = door.GetPhsyicsAABB();

            if (door.IsDirty()) {
                AABB dirtyAABB = currentAABB;

                auto previousAABB = g_previousDoorAABBs.find(doorId);
                if (previousAABB != g_previousDoorAABBs.end()) {
                    dirtyAABB.Grow(previousAABB->second);
                }

                GPUAABB aabb;
                aabb.boundsMin = glm::vec4(dirtyAABB.GetBoundsMin(), 0.0f);
                aabb.boundsMax = glm::vec4(dirtyAABB.GetBoundsMax(), 0.0f);
                g_dirtyDoorAABBs.push_back(aabb);

                //DebugDraw::DrawAABB(dirtyAABB, YELLOW);
            }

            g_previousDoorAABBs[doorId] = currentAABB;
        }
    }

    void UpdateLightRayTracingDirtyFlags() {
        for (Light& light : Unloved::World::GetLights()) {

            // Begin false
            light.SetRaytracingDirtyFlag(false);

            if (light.IsForcedDirty()) {
                light.SetRaytracingDirtyFlag(true);
                continue;
            }

            // Was the light a fireplace??? These don't save to file <------------------ VERY HACKY
            if (!light.GetCreateInfo().saveToFile) {
                // Do nothing
            }
            else {
                for (const GPUAABB& gpuAabb : g_dirtyDoorAABBs) {
                    AABB doorAABB(glm::vec3(gpuAabb.boundsMin), glm::vec3(gpuAabb.boundsMax));
                    AABB lightCullingAABB(light.GetWorldBoundsMin(), light.GetWorldBoundsMax());

                    if (doorAABB.IntersectsAABB(lightCullingAABB)) {
                        light.SetRaytracingDirtyFlag(true);
                        break;
                    }
                }
            }
        }
    }

    void DebugDrawDirtyBounds() {
        for (DirtyBounds& dirtyBounds : g_dirtyBoundsSet) {
            AABB aabb(dirtyBounds.boundsMin, dirtyBounds.boundsMax);
            DebugDraw::DrawAABB(aabb, YELLOW);
        }
    }

    void DebugDrawLightIds() {
        for (uint64_t id : g_compositeDirtyLightIds) {
            if (Light* light = World::GetLightByObjectId(id)) {
                AABB aabb(light->GetWorldBoundsMin(), light->GetWorldBoundsMax());
                DebugDraw::DrawAABB(aabb, GREEN);
            }
        }
    }

    void DebugMessageOfAllDirtyLights() {
        std::string message;

        for (uint64_t id : g_compositeDirtyLightIds) {
            message += std::to_string(id) + "\n";
        }

        Debug::BlitQuickDebugMessage(message);
    }

    void DebugPrintDirtyLightInfo(const Light& light, uint64_t intersectingObjectId) {
        Logging::Debug() << "LIGHT " << light.GetObjectId() << " triggered dirty by " << Hell::Enum::ToString(Unloved::GetObjectIdType(intersectingObjectId)) << " " << intersectingObjectId << "\n";
    }

    bool IntersectAABB(const RenderItem& renderItemA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return IntersectAABB(renderItemA.aabbMin, renderItemA.aabbMax, boundsMinB, boundsMaxB);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const RenderItem& renderItemB) {
        return IntersectAABB(boundsMinA, boundsMaxA, renderItemB.aabbMin, renderItemB.aabbMax);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }

    bool IntersectAABB(const glm::vec3& boundsMinA, const glm::vec3& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }

    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec3& boundsMinB, const glm::vec3& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }

    bool IntersectAABB(const glm::vec4& boundsMinA, const glm::vec4& boundsMaxA, const glm::vec4& boundsMinB, const glm::vec4& boundsMaxB) {
        return (boundsMinA.x <= boundsMaxB.x && boundsMaxA.x >= boundsMinB.x) && (boundsMinA.y <= boundsMaxB.y && boundsMaxA.y >= boundsMinB.y) && (boundsMinA.z <= boundsMaxB.z && boundsMaxA.z >= boundsMinB.z);
    }
}
