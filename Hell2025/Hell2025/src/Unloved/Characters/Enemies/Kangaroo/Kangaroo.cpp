#include "Kangaroo.h"

#include "Hell/Audio.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"

// get me out of here
#include "World/LegacyWorld.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"
#include "Timer.hpp"
//

#include <glm/gtc/quaternion.hpp>

namespace Audio = Hell::Audio;

namespace {
    glm::vec3 YawOnlyRotation(const glm::vec3& rotation) {
        return glm::vec3(0.0f, rotation.y, 0.0f);
    }

    glm::vec3 ForwardFromRotation(const glm::vec3& rotation) {
        return glm::normalize(glm::quat(rotation) * glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

namespace Unloved {

    Kangaroo::Kangaroo(uint64_t id, KangarooCreateInfo createInfo, SpawnOffset spawnOffset) {
        Init(id, createInfo, spawnOffset);
    }

    void Kangaroo::Init(uint64_t id, KangarooCreateInfo createInfo, SpawnOffset spawnOffset) {
        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;
        m_createInfo.rotation.y += spawnOffset.yRotation;
        m_createInfo.rotation = YawOnlyRotation(m_createInfo.rotation);
        m_objectId = id;

        Respawn();
        m_aStar.InitGrid();

        if (m_skinnedGameObjectId == 0) {
            m_animatorInstanceId = Animator::CreateAnimatorInstance();
            m_skinnedGameObjectId = World::CreateSkinnedGameObject();

            AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
            Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();

            if (!animatorInstance || !skinnedGameObject) {
                Logging::Error() << "Kangaroo::Init() failed to create core objects\n";
                __debugbreak();
                return;
            }

            animatorInstance->RegisterSkinnedModels({ "Kangaroo" });
            m_animationLayerIndex = animatorInstance->CreateAnimationLayer();
            skinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);

            skinnedGameObject->SetOwnerObjectId(m_objectId);
            skinnedGameObject->SetSkinnedModel("Kangaroo");
            skinnedGameObject->SetPosition(m_position);
            skinnedGameObject->SetRotationX(m_rotation.x);
            skinnedGameObject->SetRotationY(m_rotation.y);
            skinnedGameObject->SetRotationZ(m_rotation.z);
            skinnedGameObject->SetAnimationModeToBindPose();
            skinnedGameObject->SetName("Roo");

            skinnedGameObject->CreateRagdoll("Kangaroo");
            skinnedGameObject->SetAllMeshMaterials("Kangaroo");
            skinnedGameObject->SetMeshMaterialByMeshName("LeftEye_Iris", "KangarooIris");
            skinnedGameObject->SetMeshMaterialByMeshName("RightEye_Iris", "KangarooIris");

            skinnedGameObject->SetBlendingModeByMeshName("LeftEye_Sclera", BlendingMode::DO_NOT_RENDER);
            skinnedGameObject->SetBlendingModeByMeshName("RightEye_Sclera", BlendingMode::DO_NOT_RENDER);

            PlayAndLoopAnimation("Kangaroo_Idle", 1.0f);

            int32_t woundMaskIndex = Renderer::GetNextFreeWoundMaskIndexAndMarkItTaken();

            skinnedGameObject->SetMeshWoundMaskArrayIndex("Body", woundMaskIndex);
            skinnedGameObject->SetMeshWoundMaterialByMeshName("Body", "KangarooBlood");
           
            //Logging::Debug() << "Assigned a Kangaroo a 'Body' mesh wound mask index of " << woundMaskIndex;

            CreateCharacterController(m_createInfo.position);
        }
    }

    void Kangaroo::Respawn() {
        m_position = m_createInfo.position;
        m_rotation = m_createInfo.rotation;
        m_forward = ForwardFromRotation(m_rotation);
        m_alive = true;
        m_health = m_maxHealth;
        m_yVelocity = 0;

        m_agroState = KanagarooAgroState::CHILLING;
        m_movementState = KanagarooMovementState::IDLE;
        m_animationState = KanagarooAnimationState::IDLE;
        m_woundTextureNeedsClearing = true;

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        if (skinnedGameObject) {
            skinnedGameObject->SetPosition(m_position);
            skinnedGameObject->SetRotationX(m_rotation.x);
            skinnedGameObject->SetRotationY(m_rotation.y);
            skinnedGameObject->SetRotationZ(m_rotation.z);

            if (Ragdoll* ragdoll = GetRagdoll()) {
                ragdoll->SetToInitialPose();
                ragdoll->DisableSimulation();
            }
            skinnedGameObject->SetAnimationModeToAnimated();
            PlayAndLoopAnimation("Kangaroo_Idle", 1.0f);
        }

        CharacterController* characterController = GetCharacterController();
        if (characterController) {
            characterController->SetPosition(m_createInfo.position);
        }
    }

    void Kangaroo::SetPosition(const glm::vec3& position) {
        m_createInfo.position = position;
        m_position = position;

        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
            skinnedGameObject->SetPosition(position);
        }
        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetSpawnPosition(position);
        }
        if (CharacterController* characterController = GetCharacterController()) {
            characterController->SetPosition(position);
        }
    }

    void Kangaroo::SetRotation(const glm::vec3& rotation) {
        m_createInfo.rotation = YawOnlyRotation(rotation);
        m_rotation = m_createInfo.rotation;
        m_forward = ForwardFromRotation(m_rotation);

        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetSpawnRotation(m_rotation);
        }
        if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
            skinnedGameObject->SetRotationX(m_rotation.x);
            skinnedGameObject->SetRotationY(m_rotation.y);
            skinnedGameObject->SetRotationZ(m_rotation.z);
        }
    }

    Unloved::SkinnedGameObject* Kangaroo::GetSkinnedGameObject(){
        return Unloved::World::GetSkinnedGameObjectByObjectId(m_skinnedGameObjectId);
    }

    Ragdoll* Kangaroo::GetRagdoll() {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        return skinnedGameObject ? skinnedGameObject->GetRagdoll() : nullptr;
    }

    void Kangaroo::Kill() {
        if (m_alive) {
            Audio::PlayAudio("Kangaroo_Death.wav", 1.0f);

            Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
            if (skinnedGameObject) {
                skinnedGameObject->SetAnimationModeToRagdoll();
            }
            m_health = 0;
            m_alive = false;
            m_agroState = KanagarooAgroState::KANGAROO_DEAD;
            m_animationState = KanagarooAnimationState::RAGDOLL;
            m_movementState = KanagarooMovementState::KANGAROO_DEAD;
            std::cout << "Killed kangaroo\n";
        }
    }

    void Kangaroo::GiveDamage(int damage) {
        m_health -= damage;
        if (m_health <= 0) {
            Kill();
            return;
        }

        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
        glm::vec3 playerPosition = player->GetCameraPosition();
        GoToTarget(playerPosition);
        PlayFleshAudio();
        return;
        m_agroState = KanagarooAgroState::ANGRY;
    }

    void Kangaroo::CleanUp() {
        Animator::RemoveAnimatorInstance(m_animatorInstanceId);
        World::RemoveObjectById(m_skinnedGameObjectId);
        Hell::Physics::MarkCharacterControllerForRemoval(m_characterControllerId);

        m_animatorInstanceId = 0;
        m_skinnedGameObjectId = 0;
        m_characterControllerId = 0;
    }

    void Kangaroo::SetAgroState(KanagarooAgroState state) {
        m_agroState = state;
    }

    void Kangaroo::SetMovementState(KanagarooMovementState state) {
        m_movementState = state;
    }

    void Kangaroo::PlayAnimation(const std::string& animationName, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAnimation(m_animationLayerIndex, animationName, speed);
    }

    void Kangaroo::PlayAndLoopAnimation(const std::string& animationName, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAndLoopAnimation(m_animationLayerIndex, animationName, speed);
    }

    bool Kangaroo::AnimationIsComplete() {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        return animatorInstance && animatorInstance->IsAnimationComplete(m_animationLayerIndex);
    }

    uint32_t Kangaroo::GetAnimationFrameNumber() {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!animatorInstance) return 0;

        return animatorInstance->GetAnimationFrameNumber(m_animationLayerIndex);
    }

    CharacterController* Kangaroo::GetCharacterController() {
        return Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    }

    glm::vec2 Kangaroo::GetGridPosition() {
        return AStarMap::GetCellCoordsFromWorldSpacePosition(m_position);
    }

    std::vector<glm::ivec2> Kangaroo::GetPath() {
        return m_aStar.GetPath();
    }

    void Kangaroo::MarkWoundTextureAsCleared() {
        m_woundTextureNeedsClearing = false;
    }
}
