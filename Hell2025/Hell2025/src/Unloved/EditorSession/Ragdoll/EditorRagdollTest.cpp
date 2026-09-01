#include "EditorRagdollTest.h"

#include "Hell/Common/Enum.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Physics/Ragdoll/Ragdoll.h"
#include "Hell/Physics/Ragdoll/RagdollAsset.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/World/World.h"

namespace Unloved::EditorSession::RagdollTest {
    namespace {
        uint64_t g_standaloneRagdollId = 0;
        uint64_t g_skinnedGameObjectId = 0;

        constexpr const char* RAGDOLL_TEST_HOUSE_NAME = "RagdollTestScene";

        SkinnedGameObject* GetSkinnedGameObject() {
            if (g_skinnedGameObjectId == 0) return nullptr;
            return World::GetSkinnedGameObjectByObjectId(g_skinnedGameObjectId);
        }

        Ragdoll* GetRagdoll() {
            if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
                return skinnedGameObject->GetRagdoll();
            }
            return Hell::Physics::GetRagdollById(g_standaloneRagdollId);
        }
    }

    bool IsActive() {
        return GetRagdoll() != nullptr;
    }

    void Start(const RagdollAsset& asset) {
        Stop();
        EditorSession::Close();
        World::LoadSingleHouse(RAGDOLL_TEST_HOUSE_NAME);
        Session::RespawnPlayers();

        const uint64_t ownerObjectId = GetNextObjectId(ObjectType::RAGDOLL_STANDALONE);
        if (asset.skinnedModelPresetName.empty()) {
            g_standaloneRagdollId = Hell::Physics::SpawnRagdoll(glm::vec3(0.0f), glm::vec3(0.0f), asset, ownerObjectId);
            if (g_standaloneRagdollId == 0) {
                Logging::Error() << "RagdollTest::Start() failed to create the physics ragdoll";
                return;
            }
            Debug::BlitQuickDebugMessage("Testing ragdoll '" + asset.name + "' (physics shapes only)");
            return;
        }

        const uint64_t skinnedGameObjectId = World::CreateSkinnedGameObject();
        const uint64_t animatorInstanceId = Animator::CreateAnimatorInstance();
        SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(skinnedGameObjectId);
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(animatorInstanceId);
        if (!skinnedGameObject || !animatorInstance) {
            if (skinnedGameObjectId != 0) World::RemoveObjectById(skinnedGameObjectId);
            if (animatorInstanceId != 0) Animator::RemoveAnimatorInstance(animatorInstanceId);
            Logging::Error() << "RagdollTest::Start() failed to create the skinned game object or animator instance";
            return;
        }

        skinnedGameObject->SetOwnerObjectId(ownerObjectId);
        skinnedGameObject->SetName("Ragdoll Test");
        Bible::ConfigureSkinnedModel(*skinnedGameObject, Hell::Enum::FromString(asset.skinnedModelPresetName, Bible::SkinnedModelPreset::UNDEFINED));

        SkinnedModel* skinnedModel = skinnedGameObject->GetSkinnedModel();
        if (!skinnedModel) {
            World::RemoveObjectById(skinnedGameObjectId);
            Animator::RemoveAnimatorInstance(animatorInstanceId);
            Logging::Error() << "RagdollTest::Start() failed to configure the skinned model";
            return;
        }

        animatorInstance->RegisterSkinnedModels({ skinnedModel->GetName() });
        const uint32_t animationLayerIndex = animatorInstance->CreateAnimationLayer();
        skinnedGameObject->SetAnimatorInstanceId(animatorInstanceId);
        skinnedGameObject->SetPosition(glm::vec3(0.0f));
        skinnedGameObject->SetRotationX(0.0f);
        skinnedGameObject->SetRotationY(0.0f);
        skinnedGameObject->SetRotationZ(0.0f);
        skinnedGameObject->SetScale(asset.skinnedModelScale);
        if (asset.testAnimationName.empty()) {
            skinnedGameObject->SetAnimationModeToBindPose();
        }
        else {
            skinnedGameObject->SetAnimationModeToAnimated();
            animatorInstance->PlayAndLoopAnimation(animationLayerIndex, asset.testAnimationName, 1.0f);
            animatorInstance->RestartAnimation();
        }

        if (skinnedGameObject->CreateRagdoll(asset) == 0) {
            World::RemoveObjectById(skinnedGameObjectId);
            Animator::RemoveAnimatorInstance(animatorInstanceId);
            Logging::Error() << "RagdollTest::Start() failed to create the physics ragdoll";
            return;
        }

        g_skinnedGameObjectId = skinnedGameObjectId;
        Debug::BlitQuickDebugMessage("Testing ragdoll '" + asset.name + "'");
    }

    void Stop() {
        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
            const uint64_t animatorInstanceId = skinnedGameObject->GetAnimatorInstanceId();
            World::RemoveObjectById(g_skinnedGameObjectId);
            if (animatorInstanceId != 0) Animator::RemoveAnimatorInstance(animatorInstanceId);
        }
        else if (g_standaloneRagdollId != 0) {
            Hell::Physics::MarkRagdollForRemoval(g_standaloneRagdollId);
        }

        g_standaloneRagdollId = 0;
        g_skinnedGameObjectId = 0;
    }

    void Simulate() {
        Ragdoll* ragdoll = GetRagdoll();
        if (!ragdoll) return;

        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) skinnedGameObject->SetAnimationModeToRagdoll();
        else ragdoll->EnableSimulation();
    }

    void SetToBindPose() {
        Ragdoll* ragdoll = GetRagdoll();
        if (!ragdoll) return;

        ragdoll->DisableSimulation();
        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) skinnedGameObject->SetAnimationModeToBindPose();
        else ragdoll->SetToInitialPose();
    }

    void SetToTestAnimation() {
        Ragdoll* ragdoll = GetRagdoll();
        if (!ragdoll) return;

        ragdoll->DisableSimulation();
        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
            skinnedGameObject->SetAnimationModeToAnimated();
            AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(skinnedGameObject->GetAnimatorInstanceId());
            if (animatorInstance) animatorInstance->RestartAnimation();
        }
        else {
            ragdoll->SetToInitialPose();
        }
    }

    void Elevate() {
        Ragdoll* ragdoll = GetRagdoll();
        if (!ragdoll) return;

        const glm::vec3 position(0.0f, 0.5f, 0.0f);
        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) skinnedGameObject->SetPosition(position);
        else ragdoll->SetSpawnPosition(position);
    }
}
