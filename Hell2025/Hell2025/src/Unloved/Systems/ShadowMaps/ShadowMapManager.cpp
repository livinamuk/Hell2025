#include "ShadowMapManager.h"

#include "Hell/Math/Math.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Unloved::ShadowMapManager {

    std::vector<ShadowMapInfo> g_hiResShadowMaps;
    std::vector<ShadowMapInfo> g_lowResShadowMaps;
    std::vector<ShadowMapInfo> g_staticDirtyHiResShadowMaps;
    std::vector<ShadowMapInfo> g_staticDirtyLowResShadowMaps;
    std::vector<ShadowMapInfo> g_compositeDirtyHiResShadowMaps;
    std::vector<ShadowMapInfo> g_compositeDirtyLowResShadowMaps;
    bool g_staticCacheEnabled = true;
    bool g_staticCacheSettingInitialized = false;

    void CheckForRemovedLights();
    void AssignShadowMapIndices();
    void GatherDirtyShadowMaps();
    void InvalidateAllShadowMaps();
    void BlitDebugInfo();

    void RemoveHiResShadowMap(uint64_t lightId);
    void RemoveLowResShadowMap(uint64_t lightId);
    void AssignNextFreeHiResShadowMapIndex(uint64_t lightId);
    void AssignNextFreeLowResShadowMapIndex(uint64_t lightId);

    bool HasHiResShadowMap(uint64_t lightId);
    bool HasLowResShadowMap(uint64_t lightId);
    void AddUniqueShadowMap(std::vector<ShadowMapInfo>& shadowMaps, const ShadowMapInfo& shadowMapInfo);
    void InvalidateShadowMap(const ShadowMapInfo& shadowMapInfo, bool staticGeometryDirty, std::vector<ShadowMapInfo>& staticDirtyShadowMaps, std::vector<ShadowMapInfo>& compositeDirtyShadowMaps, uint8_t faceMask = 0x3f);

    void BeginFrame() {
        g_staticDirtyHiResShadowMaps.clear();
        g_staticDirtyLowResShadowMaps.clear();
        g_compositeDirtyHiResShadowMaps.clear();
        g_compositeDirtyLowResShadowMaps.clear();
    }

    void Update() {
        const bool staticCacheEnabled = Renderer::GetCurrentRendererSettings().enableStaticShadowMapCaching;
        const bool staticCacheModeChanged = !g_staticCacheSettingInitialized || staticCacheEnabled != g_staticCacheEnabled;
        g_staticCacheEnabled = staticCacheEnabled;
        g_staticCacheSettingInitialized = true;

        CheckForRemovedLights();
        AssignShadowMapIndices();
        if (staticCacheModeChanged) {
            InvalidateAllShadowMaps();
        }
        GatherDirtyShadowMaps();
    }

    void CheckForRemovedLights() {
        // Hi res
        for (int i = (int)g_hiResShadowMaps.size() - 1; i >= 0; i--) {
            if (!World::GetLightByObjectId(g_hiResShadowMaps[i].lightId)) {
                g_hiResShadowMaps.erase(g_hiResShadowMaps.begin() + i);
            }
        }

        // Low res
        for (int i = (int)g_lowResShadowMaps.size() - 1; i >= 0; i--) {
            if (!World::GetLightByObjectId(g_lowResShadowMaps[i].lightId)) {
                g_lowResShadowMaps.erase(g_lowResShadowMaps.begin() + i);
            }
        }
    }

    void AssignShadowMapIndices() {
        // Get player one camera
        Camera* camera = Session::GetLocalPlayerCameraByViewportIndex(0);
        if (!camera) return;

        const glm::vec3 cameraPosition = camera->GetPosition();

        // Gather all Light pointers
        std::vector<Light*> lights;

        for (Light& light : World::GetLights()) {
            lights.push_back(&light);
        }

        // Sort lights by distance to camera
        std::sort(lights.begin(), lights.end(), [&](Light* a, Light* b) {
            const float distA = Hell::Math::DistSquared(a->GetPosition(), cameraPosition);
            const float distB = Hell::Math::DistSquared(b->GetPosition(), cameraPosition);
            return distA < distB;
            });

        // Remove shadow map info from the wrong lists
        for (size_t i = 0; i < lights.size(); i++) {
            Light* light = lights[i];
            uint64_t lightId = light->GetObjectId();

            // Hi res
            if (i < SHADOW_MAP_HI_RES_MAX_COUNT) {
                RemoveLowResShadowMap(lightId);
                continue;
            }

            // Low res
            if (i < SHADOW_MAP_HI_RES_MAX_COUNT + SHADOW_MAP_LOW_RES_MAX_COUNT) {
                RemoveHiResShadowMap(lightId);
                continue;
            }

            // Remove any now unused shadow map info
            RemoveHiResShadowMap(lightId);
            RemoveLowResShadowMap(lightId);
        }

        // Assign shadow map indices
        for (size_t i = 0; i < lights.size(); i++) {
            Light* light = lights[i];
            uint64_t lightId = light->GetObjectId();

            // Hi res
            if (i < SHADOW_MAP_HI_RES_MAX_COUNT) {
                if (!HasHiResShadowMap(lightId)) {
                    AssignNextFreeHiResShadowMapIndex(lightId);
                }

                continue;
            }

            // Low res
            if (i < SHADOW_MAP_HI_RES_MAX_COUNT + SHADOW_MAP_LOW_RES_MAX_COUNT) {
                if (!HasLowResShadowMap(lightId)) {
                    AssignNextFreeLowResShadowMapIndex(lightId);
                }

                continue;
            }
        }
    }

    void GatherDirtyShadowMaps() {
        for (uint64_t lightId : DirtyTracker::GetStaticDirtyLightIds()) {
            for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyHiResShadowMaps, g_compositeDirtyHiResShadowMaps, DirtyTracker::GetStaticDirtyLightFaceMask(lightId));
                    break;
                }
            }

            for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyLowResShadowMaps, g_compositeDirtyLowResShadowMaps, DirtyTracker::GetStaticDirtyLightFaceMask(lightId));
                    break;
                }
            }
        }

        for (uint64_t lightId : DirtyTracker::GetCompositeDirtyLightIds()) {
            for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    InvalidateShadowMap(shadowMapInfo, false, g_staticDirtyHiResShadowMaps, g_compositeDirtyHiResShadowMaps, DirtyTracker::GetCompositeDirtyLightFaceMask(lightId));
                    break;
                }
            }

            for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    InvalidateShadowMap(shadowMapInfo, false, g_staticDirtyLowResShadowMaps, g_compositeDirtyLowResShadowMaps, DirtyTracker::GetCompositeDirtyLightFaceMask(lightId));
                    break;
                }
            }
        }
    }

    void AddUniqueShadowMap(std::vector<ShadowMapInfo>& shadowMaps, const ShadowMapInfo& shadowMapInfo) {
        for (ShadowMapInfo& existing : shadowMaps) {
            if (existing.lightId == shadowMapInfo.lightId) {
                existing.faceMask |= shadowMapInfo.faceMask;
                return;
            }
        }

        shadowMaps.push_back(shadowMapInfo);
    }

    void InvalidateShadowMap(const ShadowMapInfo& shadowMapInfo, bool staticGeometryDirty, std::vector<ShadowMapInfo>& staticDirtyShadowMaps, std::vector<ShadowMapInfo>& compositeDirtyShadowMaps, uint8_t faceMask) {
        ShadowMapInfo invalidatedShadowMap = shadowMapInfo;
        invalidatedShadowMap.faceMask = faceMask & uint8_t(0x3f);
        if (invalidatedShadowMap.faceMask == 0) return;

        // Static invalidation always requires a new composite. Uncached mode suppresses only the separate static work.
        if (g_staticCacheEnabled && staticGeometryDirty) {
            AddUniqueShadowMap(staticDirtyShadowMaps, invalidatedShadowMap);
        }

        AddUniqueShadowMap(compositeDirtyShadowMaps, invalidatedShadowMap);
    }

    void InvalidateAllShadowMaps() {
        for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
            InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyHiResShadowMaps, g_compositeDirtyHiResShadowMaps);
        }

        for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
            InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyLowResShadowMaps, g_compositeDirtyLowResShadowMaps);
        }
    }

    bool HasHiResShadowMap(uint64_t lightId) {
        for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
            if (shadowMapInfo.lightId == lightId) {
                return true;
            }
        }

        return false;
    }

    bool HasLowResShadowMap(uint64_t lightId) {
        for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
            if (shadowMapInfo.lightId == lightId) {
                return true;
            }
        }

        return false;
    }

    void RemoveHiResShadowMap(uint64_t lightId) {
        // Walk backwards, remove if found
        for (int i = (int)g_hiResShadowMaps.size() - 1; i >= 0; i--) {
            if (g_hiResShadowMaps[i].lightId == lightId) {
                g_hiResShadowMaps.erase(g_hiResShadowMaps.begin() + i);
            }
        }
    }

    void RemoveLowResShadowMap(uint64_t lightId) {
        // Walk backwards, remove if found
        for (int i = (int)g_lowResShadowMaps.size() - 1; i >= 0; i--) {
            if (g_lowResShadowMaps[i].lightId == lightId) {
                g_lowResShadowMaps.erase(g_lowResShadowMaps.begin() + i);
            }
        }
    }

    void AssignNextFreeHiResShadowMapIndex(uint64_t lightId) {
        // Loop over every possible hi res shadow map index
        for (int32_t shadowMapIndex = 0; shadowMapIndex < (int32_t)SHADOW_MAP_HI_RES_MAX_COUNT; shadowMapIndex++) {
            bool indexUsed = false;

            // Check whether this index is already used
            for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
                if (shadowMapInfo.shadowMapIndex == shadowMapIndex) {
                    indexUsed = true;
                    break;
                }
            }

            // Assign the first free index
            if (!indexUsed) {
                ShadowMapInfo shadowMapInfo = { lightId, shadowMapIndex };
                g_hiResShadowMaps.push_back(shadowMapInfo);
                InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyHiResShadowMaps, g_compositeDirtyHiResShadowMaps);
                return;
            }
        }
    }

    void AssignNextFreeLowResShadowMapIndex(uint64_t lightId) {
        // Loop over every possible low res shadow map index
        for (int32_t shadowMapIndex = 0; shadowMapIndex < (int32_t)SHADOW_MAP_LOW_RES_MAX_COUNT; shadowMapIndex++) {
            bool indexUsed = false;

            // Check whether this index is already used
            for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
                if (shadowMapInfo.shadowMapIndex == shadowMapIndex) {
                    indexUsed = true;
                    break;
                }
            }

            // Assign the first free index
            if (!indexUsed) {
                ShadowMapInfo shadowMapInfo = { lightId, shadowMapIndex };
                g_lowResShadowMaps.push_back(shadowMapInfo);
                InvalidateShadowMap(shadowMapInfo, true, g_staticDirtyLowResShadowMaps, g_compositeDirtyLowResShadowMaps);
                return;
            }
        }
    }

    int32_t GetHiResShadowMapIndex(uint64_t lightId) {
        for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
            if (shadowMapInfo.lightId == lightId) {
                return shadowMapInfo.shadowMapIndex;
            }
        }

        return -1;
    }

    int32_t GetLowResShadowMapIndex(uint64_t lightId) {
        for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
            if (shadowMapInfo.lightId == lightId) {
                return shadowMapInfo.shadowMapIndex;
            }
        }

        return -1;
    }

    bool StaticCacheEnabled() {
        return g_staticCacheEnabled;
    }

    const std::vector<ShadowMapInfo>& GetStaticDirtyHiResShadowMaps() {
        return g_staticDirtyHiResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetStaticDirtyLowResShadowMaps() {
        return g_staticDirtyLowResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetCompositeDirtyHiResShadowMaps() {
        return g_compositeDirtyHiResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetCompositeDirtyLowResShadowMaps() {
        return g_compositeDirtyLowResShadowMaps;
    }

    void BlitDebugInfo() {
        std::string message;

        message += "STATIC DIRTY HI RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_staticDirtyHiResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        }
        message += "\n";

        message += "STATIC DIRTY LOW RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_staticDirtyLowResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        }
        message += "\n";

        message += "COMPOSITE DIRTY HI RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_compositeDirtyHiResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        }
        message += "\n";

        message += "COMPOSITE DIRTY LOW RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_compositeDirtyLowResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        }
        message += "\n";

        message += "HI RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + " idx " + std::to_string(shadowMapInfo.shadowMapIndex) + "\n";
        }
        message += "\n";

        message += "LOW RES\n";
        for (ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
            message += "Light " + std::to_string(shadowMapInfo.lightId) + " idx " + std::to_string(shadowMapInfo.shadowMapIndex) + "\n";
        }

        Debug::BlitQuickDebugMessage(message);
    }
}
