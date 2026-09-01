/**/#include "Snake.h"

#include "Hell/Audio.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"

#include <glm/gtc/quaternion.hpp>

// GET ME OUT OF HERE
#include "Unloved/Session/Session.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;
// GET ME OUT OF HERE

namespace {
    glm::vec3 YawOnlyRotation(const glm::vec3& rotation) {
        return glm::vec3(0.0f, rotation.y, 0.0f);
    }

    glm::vec3 ForwardFromRotation(const glm::vec3& rotation) {
        return glm::normalize(glm::quat(rotation) * glm::vec3(0.0f, 0.0f, 1.0f));
    }
}

namespace Unloved {

    Snake::Snake(uint64_t id, SnakeCreateInfo createInfo, SpawnOffset spawnOffset) {
        Init(id, createInfo, spawnOffset);
    }

    void Snake::Init(uint64_t id, SnakeCreateInfo createInfo, SpawnOffset spawnOffset) {
        m_objectId = id;

        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;
        m_createInfo.rotation.y += spawnOffset.yRotation;
        m_createInfo.rotation = YawOnlyRotation(m_createInfo.rotation);
        m_initalForward = ForwardFromRotation(m_createInfo.rotation);

        m_forward = m_initalForward;

        // Create core objects
        m_animatorInstanceId = Animator::CreateAnimatorInstance();
        m_skinnedGameObjectId = World::CreateSkinnedGameObject();

        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = GetAnimatorInstance();

        if (!skinnedGameObject || !animatorInstance) {
            Logging::Error() << "Shark::Init() some core shit fucked up\n";
            __debugbreak();
        }
        else {
            // Register skinned model with animator
            animatorInstance->RegisterSkinnedModels({ "Snake" });

            // Create animation layer index
            m_animationLayerIndex = animatorInstance->CreateAnimationLayer();
        }

        skinnedGameObject->SetOwnerObjectId(m_objectId);
        skinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);
        skinnedGameObject->SetSkinnedModel("Snake");
        skinnedGameObject->SetPosition(m_createInfo.position);
        skinnedGameObject->SetRotationX(m_createInfo.rotation.x);
        skinnedGameObject->SetRotationY(m_createInfo.rotation.y);
        skinnedGameObject->SetRotationZ(m_createInfo.rotation.z);
        skinnedGameObject->SetName("Snake " + std::to_string(m_objectId));
        skinnedGameObject->SetMeshMaterialByMeshName("Body", "SnakeMouthBlood");
        skinnedGameObject->SetMeshMaterialByMeshName("Jaw", "SnakeMouthBlood");
        skinnedGameObject->SetMeshMaterialByMeshName("Tongue", "SnakeMouthBlood");
        skinnedGameObject->SetMeshMaterialByMeshName("Iris", "SnakeIris");
        skinnedGameObject->CreateRagdoll("Snake");

        int32_t woundMaskIndex = Renderer::GetNextFreeWoundMaskIndexAndMarkItTaken();
        skinnedGameObject->SetMeshWoundMaskArrayIndex("Body", woundMaskIndex);
        skinnedGameObject->SetMeshWoundMaterialByMeshName("Body", "SnakeFullBlood");


        m_bloodPoolState.Configure(skinnedGameObject->GetRagdollId(), "rMarker_head");

        ResetToInitialState();

        CreateCharacterController(GetPosition());

        m_health = 1.0f;
    }

    void Snake::CleanUp() {
        Animator::RemoveAnimatorInstance(m_animatorInstanceId);
        World::RemoveObjectById(m_skinnedGameObjectId);
        Hell::Physics::MarkCharacterControllerForRemoval(m_characterControllerId);

        m_animatorInstanceId = 0;
        m_skinnedGameObjectId = 0;
        m_characterControllerId = 0;
    }

    void Snake::TakeDamage(uint32_t damage) {
        const bool wasAlive = IsAlive();
        m_health -= damage;

        if (wasAlive && IsDead()) {
            if (SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
                skinnedGameObject->SetAnimationModeToRagdoll();
            }

            Hell::Audio::PlayAudio("Snake_Death.wav", 1.0f);
        }
    }

    void Snake::SetPosition(const glm::vec3& position) {
        m_createInfo.position = position;

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        skinnedGameObject->SetPosition(position);

        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetSpawnPosition(position);
        }

        //if (CharacterController* characterController = GetCharacterController()) {
        //    characterController->SetPosition(position);
        //}
    }

    void Snake::SetRotation(const glm::vec3& rotation) {
        m_createInfo.rotation = YawOnlyRotation(rotation);
        m_initalForward = ForwardFromRotation(m_createInfo.rotation);
        m_forward = m_initalForward;

        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetSpawnRotation(m_createInfo.rotation);
        }

        UpdateSkinnedGameObjectRotation();
    }

    void Snake::ResetToInitialState() {
        m_target = glm::vec3(0.0f);
        m_path.clear();
        //m_state = SnakeState::LAY;
        m_forward = m_initalForward;
        m_health = m_initalHealth;

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        if (!skinnedGameObject) return;

        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetToInitialPose();
            ragdoll->DisableSimulation();
        }

        skinnedGameObject->SetAnimationModeToBindPose();
        skinnedGameObject->SetPosition(m_createInfo.position);

        //if (CharacterController* characterController = GetCharacterController()) {
        //    characterController->SetPosition(m_createInfo.position);
        //}

        UpdateSkinnedGameObjectRotation();
        PlayAndLoopAnimation(Bible::AnimationSlot::SNAKE_SLITHER, 1.0f);

        m_bloodPoolState.Reset();
    }


    void Snake::UpdateSkinnedGameObjectRotation() {
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        float rotY = Hell::Math::YawBetweenPoints(GetPosition(), GetPosition() + GetForward()) + (HELL_PI * 0.5f);
        skinnedGameObject->SetRotationY(rotY);
    }

    void Snake::DebugDraw() {
        // Forward
        glm::vec3 p1 = GetPosition();
        glm::vec3 p2 = GetPosition() + GetForward() * 0.25f;
        DebugDraw::DrawPoint(p1, GREEN);
        DebugDraw::DrawPoint(p2, GREEN);
        DebugDraw::DrawLine(p1, p2, GREEN);

        // Path
        Unloved::NavMeshManager::DrawPath(m_path, WHITE);
    }

    void Snake::UpdateMovement(float deltaTime) {
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        Ragdoll* ragdoll = GetRagdoll();

        //DebugDraw();

        if (!ragdoll) return;
        if (!skinnedGameObject) return;

        if (Input::KeyPressed(HELL_KEY_Y)) {
            ResetToInitialState();
        }

        if (IsDead()) return;

        if (Input::KeyPressed(HELL_KEY_T)) {
            if (Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {

                m_target = player->GetInteractHitPosition();

                //if (m_state == SnakeState::LAY) {
                //    m_state = SnakeState::GET_UP_FROM_LAY;
                //    PlayAnimation(Bible::AnimationSlot::Snake_LAY_TO_WALK, 1.0f);
                //}
            }
        }

        //if (m_state == SnakeState::GET_UP_FROM_LAY && IsAnimationComplete()) {
        //    m_state = SnakeState::WALK_TO_TARGET;
        //    PlayAndLoopAnimation(Bible::AnimationSlot::Snake_WALK, 1.0f);
        //}
        //else if (m_state == SnakeState::SIT_FROM_LAY && IsAnimationComplete()) {
        //    m_state = SnakeState::LAY;
        //    PlayAndLoopAnimation(Bible::AnimationSlot::Snake_LAY, 1.0f);
        //}
        //
        //// WALK
        //if (m_state == SnakeState::WALK_TO_TARGET) {
        //    float speed = 1.0f;
        //
        //    m_path = Unloved::NavMeshManager::FindPath(GetPosition(), m_target);
        //
        //
        //    if (m_path.size() >= 2) {
        //
        //        // Compute and calculate a new forward vector based on the next path point
        //        const glm::vec3 normalizedPosition = GetPosition() * glm::vec3(1.0f, 0.0f, 1.0f);
        //        const glm::vec3 normalizedNextPathPosition = m_path[1] * glm::vec3(1.0f, 0.0f, 1.0f);
        //        const glm::vec3 normalizedTarget = m_target * glm::vec3(1.0f, 0.0f, 1.0f);
        //        glm::vec3 targetForward = glm::normalize(normalizedNextPathPosition - normalizedPosition);
        //        float turnSpeed = 5.5f;
        //        float alpha = glm::clamp(turnSpeed * deltaTime, 0.0f, 1.0f);
        //        m_forward = glm::normalize(m_forward * (1.0f - alpha) + targetForward * alpha);
        //
        //        glm::vec3 displacement = m_forward * speed * deltaTime;
        //
        //        Hell::Physics::MoveCharacterController(m_characterControllerId, displacement);
        //
        //        if (CharacterController* characterController = GetCharacterController()) {
        //            skinnedGameObject->SetPosition(characterController->GetFootPosition());
        //        }
        //
        //        // Did you reach the target
        //        float distanceToTarget = glm::distance(normalizedPosition, normalizedTarget);
        //        if (distanceToTarget < 0.2f) {
        //            m_state = SnakeState::SIT_FROM_LAY;
        //            PlayAnimation(Bible::AnimationSlot::Snake_STRETCH_TO_LAY, 1.0f);
        //        }
        //    }
        //}
    }

    void Snake::Update(float deltaTime) {
        (void)deltaTime;

        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        Ragdoll* ragdoll = GetRagdoll();

        if (!ragdoll) return;
        if (!skinnedGameObject) return;

        UpdateSkinnedGameObjectRotation();

        if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
            skinnedGameObject->DisableRendering();
        }
        else {
            skinnedGameObject->EnableRendering();
        }

        m_bloodPoolState.Update();
    }

    Unloved::SkinnedGameObject* Snake::GetSkinnedGameObject() {
        return Unloved::World::GetSkinnedGameObjectByObjectId(m_skinnedGameObjectId);
    }

    Ragdoll* Snake::GetRagdoll() {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        return skinnedGameObject ? skinnedGameObject->GetRagdoll() : nullptr;
    }

    glm::vec3 Snake::GetPosition() {
        Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        if (!skinnedGameObject) return glm::vec3(0.0f);

        return skinnedGameObject->GetModelMatrix()[3];

    }

    void Snake::CreateCharacterController(const glm::vec3& position) {
        float capsuleHeight = 0.2f;
        float capsuleRadius = 0.15;

        PhysicsFilterData physicsFilterData;
        physicsFilterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        physicsFilterData.collisionGroup = CollisionGroup::CHARACTER_CONTROLLER;
        //physicsFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER);
        physicsFilterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE);

        m_characterControllerId = Hell::Physics::CreateCharacterController(m_objectId, position, capsuleHeight, capsuleRadius, physicsFilterData);
    }

    AnimatorInstance* Snake::GetAnimatorInstance() {
        return Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    }

    //CharacterController* Snake::GetCharacterController() {
    //    return Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    //}

    void Snake::PlayAnimation(Bible::AnimationSlot animationSlot, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAnimation(m_animationLayerIndex, Bible::GetAnimation(Bible::AnimationProfile::SNAKE, animationSlot), speed);
    }

    void Snake::PlayAndLoopAnimation(Bible::AnimationSlot animationSlot, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAndLoopAnimation(m_animationLayerIndex, Bible::GetAnimation(Bible::AnimationProfile::SNAKE, animationSlot), speed);
    }

    bool Snake::IsAnimationComplete() {
        AnimatorInstance* animatorInstance = GetAnimatorInstance();
        return animatorInstance && animatorInstance->IsAnimationComplete(m_animationLayerIndex);
    }
}
