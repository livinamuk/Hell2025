#include "BloodSystemOLD.h"

#include "Hell/Physics/Physics.h"

#include <cstddef>

namespace Unloved::BloodSystemOLD {
    std::vector<BloodScreenSpaceDecal> g_bloodScreenSpaceDecals;
    std::vector<BloodVAT> g_bloodVAT;

    void AddBloodVAT(const glm::vec3& position, const glm::vec3& direction) {
        size_t maxAllowed = 4;
        if (g_bloodVAT.size() < maxAllowed) {
            g_bloodVAT.push_back(BloodVAT(position, direction));
        }

        glm::vec3 rayOrigin = position;
        glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
        float rayLength = 100;
        PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(rayOrigin, rayDirection, rayLength);

        if (rayResult.hitFound) {
            BloodScreenSpaceDecalCreateInfo decalCreateInfo;
            decalCreateInfo.position = rayResult.hitPosition;
            decalCreateInfo.direction = direction;
            AddBloodScreenSpaceDecal(decalCreateInfo);
        }
    }

    void AddBloodScreenSpaceDecal(BloodScreenSpaceDecalCreateInfo createInfo) {
        BloodScreenSpaceDecal& bloodScreenSpaceDecal = g_bloodScreenSpaceDecals.emplace_back();
        bloodScreenSpaceDecal.Init(createInfo);
    }

    std::vector<BloodScreenSpaceDecal>& GetBloodScreenSpaceDecals() {
        return g_bloodScreenSpaceDecals;
    }

    std::vector<BloodVAT>& GetBloodVAT() {
        return g_bloodVAT;
    }

    void Update(float deltaTime) {
        for (size_t i = 0; i < g_bloodVAT.size();) {
            BloodVAT& bloodVAT = g_bloodVAT[i];

            if (bloodVAT.GetLifeTime() < 0.9f) {
                bloodVAT.Update(deltaTime);
                i++;
            }
            else {
                g_bloodVAT.erase(g_bloodVAT.begin() + i);
            }
        }
    }

    void CleanUp() {
        g_bloodScreenSpaceDecals.clear();
        g_bloodVAT.clear();
    }
}
