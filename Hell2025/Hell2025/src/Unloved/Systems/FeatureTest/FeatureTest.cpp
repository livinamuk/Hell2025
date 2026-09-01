#include "FeatureTest.h"

#include "Hell/Input.h"
#include "Hell/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/World/World.h"

namespace Unloved::FeatureTest {

    static uint64_t trapKingSkinnedGameObjectId = 0;
    static uint64_t manikinRagdollId0 = 0;
    static uint64_t manikinRagdollId1 = 0;

    void UpdateManikinTest();
    void UpdateRagdollTest();
    void UpdateRatKing();
    void UpdateTrapKing();

    void Update() {
        UpdateManikinTest();
        UpdateRagdollTest();
        // UpdateTrapKing();
    }

    void CleanUp() {
        World::RemoveObjectById(trapKingSkinnedGameObjectId);
        trapKingSkinnedGameObjectId = 0;

        Hell::Physics::MarkRagdollForRemoval(manikinRagdollId0);
        manikinRagdollId0 = 0;

        Hell::Physics::MarkRagdollForRemoval(manikinRagdollId1);
        manikinRagdollId1 = 0;
    }

    void UpdateManikinTest() {
        static bool attemptedNativeSpawn = false;
        if (manikinRagdollId0 == 0 && !attemptedNativeSpawn) {
            attemptedNativeSpawn = true;
            manikinRagdollId0 = Hell::Physics::SpawnRagdoll(glm::vec3(36, 31, 36), glm::vec3(0.0f,  0.2f, 0.0f), "manikin", GetNextObjectId(ObjectType::RAGDOLL_STANDALONE));
            manikinRagdollId1 = Hell::Physics::SpawnRagdoll(glm::vec3(37, 31, 36), glm::vec3(0.0f, -0.4f, 0.0f), "manikin", GetNextObjectId(ObjectType::RAGDOLL_STANDALONE));
        }
    }

    void UpdateRagdollTest() {
        auto& ragdolls = Hell::Physics::GetRagdolls();
        for (auto it = ragdolls.begin(); it != ragdolls.end(); ) {
            Ragdoll& ragdoll = it->second;

            // Disable ragdoll simulation  and reset to initial pose
            if (Hell::Input::KeyPressed(HELL_KEY_Y)) {
                ragdoll.SetToInitialPose();
                ragdoll.DisableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }

            // Enable ragdoll simulation and reset to initial pose
            if (Hell::Input::KeyPressed(HELL_KEY_O)) {
                ragdoll.EnableSimulation();

                for (Light& light : Unloved::World::GetLights()) {
                    AABB aabb = ragdoll.GetWorldSpaceAABB();
                    if (aabb.IntersectsSphere(light.GetPosition(), light.GetRadius())) {
                        light.ForceDirty();
                    }
                }
            }
            ++it;
        }
    }

    void UpdateTrapKing() {
        // Create if non-existent
        if (trapKingSkinnedGameObjectId == 0) {
            World::RemoveObjectById(trapKingSkinnedGameObjectId);

            trapKingSkinnedGameObjectId = World::CreateSkinnedGameObject();
            SkinnedGameObject* object = World::GetSkinnedGameObjectByObjectId(trapKingSkinnedGameObjectId);
            if (!object) return;

            Bible::ConfigureSkinnedModel(*object, Bible::SkinnedModelPreset::TRAP_KING);
            object->SetPosition(glm::vec3(36.0f, 31.0f, 36.23f));
            object->SetAnimationModeToBindPose();
        }
    }
}
