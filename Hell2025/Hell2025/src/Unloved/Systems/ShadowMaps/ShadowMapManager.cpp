#include "ShadowMapManager.h"

#include "Hell/Math/Math.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Unloved::ShadowMapManager {

    std::vector<ShadowMapInfo> g_hiResShadowMaps;
    std::vector<ShadowMapInfo> g_lowResShadowMaps;

    std::vector<ShadowMapInfo> g_dynamicDirtyHiResShadowMaps;
    std::vector<ShadowMapInfo> g_dynamicDirtyLowResShadowMaps;
    std::vector<ShadowMapInfo> g_staticDirtyHiResShadowMaps;
    std::vector<ShadowMapInfo> g_staticDirtyLowResShadowMaps;


    void CheckForRemovedLights();
    void AssignShadowMapIndices();
    void GatherDirtyShadowMaps();
    void BlitDebugInfo();

    void RemoveHiResShadowMap(uint64_t lightId);
    void RemoveLowResShadowMap(uint64_t lightId);
    void AssignNextFreeHiResShadowMapIndex(uint64_t lightId);
    void AssignNextFreeLowResShadowMapIndex(uint64_t lightId);

    bool HasHiResShadowMap(uint64_t lightId);
    bool HasLowResShadowMap(uint64_t lightId);

    void BeginFrame() {
         g_dynamicDirtyHiResShadowMaps.clear();
         g_dynamicDirtyLowResShadowMaps.clear();
         g_staticDirtyHiResShadowMaps.clear();
         g_staticDirtyLowResShadowMaps.clear();
    }

    void Update() {
        CheckForRemovedLights();
        AssignShadowMapIndices();
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

        // Static

        for (uint64_t lightId : DirtyTracker::GetStaticDirtyLightIds()) {

            // Add hi res
            for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    bool found = false;

                    for (const ShadowMapInfo& dirtyShadowMapInfo : g_staticDirtyHiResShadowMaps) {
                        if (dirtyShadowMapInfo.lightId == lightId) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        g_staticDirtyHiResShadowMaps.push_back(shadowMapInfo);
                    }

                    break;
                }
            }

            // Add low res
            for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    bool found = false;

                    for (const ShadowMapInfo& dirtyShadowMapInfo : g_staticDirtyLowResShadowMaps) {
                        if (dirtyShadowMapInfo.lightId == lightId) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        g_staticDirtyLowResShadowMaps.push_back(shadowMapInfo);
                    }

                    break;
                }
            }
        }


        // Dynamic

        for (uint64_t lightId : DirtyTracker::GetDynamicDirtyLightIds()) {

            // Add hi res
            for (const ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    bool found = false;

                    for (const ShadowMapInfo& dirtyShadowMapInfo : g_dynamicDirtyHiResShadowMaps) {
                        if (dirtyShadowMapInfo.lightId == lightId) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        g_dynamicDirtyHiResShadowMaps.push_back(shadowMapInfo);
                    }

                    break;
                }
            }

            // Add low res
            for (const ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
                if (shadowMapInfo.lightId == lightId) {
                    bool found = false;

                    for (const ShadowMapInfo& dirtyShadowMapInfo : g_dynamicDirtyLowResShadowMaps) {
                        if (dirtyShadowMapInfo.lightId == lightId) {
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        g_dynamicDirtyLowResShadowMaps.push_back(shadowMapInfo);
                    }

                    break;
                }
            }
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

                // Init both static and dynamic shadow maps as dirty
                g_dynamicDirtyHiResShadowMaps.push_back(shadowMapInfo);
                g_staticDirtyHiResShadowMaps.push_back(shadowMapInfo);
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

                // Init both static and dynamic shadow maps as dirty
                g_dynamicDirtyLowResShadowMaps.push_back(shadowMapInfo);
                g_staticDirtyLowResShadowMaps.push_back(shadowMapInfo);
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

    const std::vector<ShadowMapInfo>& GetStaticDirtyHiResShadowMaps() {
        return g_staticDirtyHiResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetStaticDirtyLowResShadowMaps() {
        return g_staticDirtyLowResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetDynamicDirtyHiResShadowMaps() {
        return g_dynamicDirtyHiResShadowMaps;
    }

    const std::vector<ShadowMapInfo>& GetDynamicDirtyLowResShadowMaps() {
        return g_dynamicDirtyLowResShadowMaps;
    }

    void BlitDebugInfo() {
        //std::string message;
        //
        //message += "DIRTY HI RES\n";
        //for (ShadowMapInfo& shadowMapInfo : g_dirtyHiResShadowMaps) {
        //    message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        //}
        //message += "\n";
        //
        //message += "DIRTY LOW RES\n";
        //for (ShadowMapInfo& shadowMapInfo : g_dirtyLowResShadowMaps) {
        //    message += "Light " + std::to_string(shadowMapInfo.lightId) + "\n";
        //}
        //message += "\n";
        //
        //message += "HI RES\n";
        //for (ShadowMapInfo& shadowMapInfo : g_hiResShadowMaps) {
        //    message += "Light " + std::to_string(shadowMapInfo.lightId) + " idx " + std::to_string(shadowMapInfo.shadowMapIndex) + "\n";
        //}
        //message += "\n";
        //
        //message += "LOW RES\n";
        //for (ShadowMapInfo& shadowMapInfo : g_lowResShadowMaps) {
        //    message += "Light " + std::to_string(shadowMapInfo.lightId) + " idx " + std::to_string(shadowMapInfo.shadowMapIndex) + "\n";
        //}
        //
        //Debug::BlitQuickDebugMessage(message);
    }
}
