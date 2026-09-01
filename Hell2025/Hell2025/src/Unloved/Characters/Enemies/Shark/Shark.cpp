#include "Shark.h"

#include "Hell/Audio.h"
#include "Hell/Logging.h"

#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/World/World.h"

#include <cmath>
#include <iostream>

namespace Unloved {
    namespace {
        constexpr uint32_t kSharkSpineAnchorIndex = 3;

        const std::vector<std::string> kSharkSpineBoneNames = {
            "BN_Head_00",
            "BN_Neck_01",
            "BN_Neck_00",
            "Spine_00",
            "BN_Spine_01",
            "BN_Spine_02",
            "BN_Spine_03",
            "BN_Spine_04",
            "BN_Spine_05",
            "BN_Spine_06",
            "BN_Spine_07"
        };

        std::vector<glm::vec3> GetCirclePoints(const glm::vec3& center, int segments, float radius) {
            std::vector<glm::vec3> points;
            points.reserve(segments);

            const float pi = 3.14159265358979323846f;
            const float deltaTheta = 2.0f * pi / segments;

            for (int i = 0; i < segments; ++i) {
                float theta = i * deltaTheta;
                points.emplace_back(
                    center.x + std::cos(theta) * radius,
                    center.y,
                    center.z + std::sin(theta) * radius
                );
            }
            return points;
        }
    }

    Shark::Shark(uint64_t id, const SharkCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
        m_objectId = id;
        m_createInfo = createInfo;
        m_createInfo.position += spawnOffset.translation;

        Init();
    }

    void Shark::Init() {
        const glm::vec3& initialPosition = m_createInfo.position;
        m_yHeight = initialPosition.y;

        // Create core objects
        m_animatorInstanceId = Animator::CreateAnimatorInstance();
        m_skinnedGameObjectId = World::CreateSkinnedGameObject();

        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = GetAnimatorInstance();

        if (!skinnedGameObject || !animatorInstance) {
            Logging::Error() << "Shark::Init() some core shit fucked up\n";
            __debugbreak();
        }

        skinnedGameObject->SetOwnerObjectId(m_objectId);

        // Configure skinned object
        skinnedGameObject->SetSkinnedModel("Shark");
        skinnedGameObject->SetName("GreatestGreatWhiteShark");
        skinnedGameObject->SetAllMeshMaterials("Shark");
        skinnedGameObject->SetScale(0.01);
        skinnedGameObject->SetPosition(glm::vec3(0, 0, 0));
        skinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);

        // Create ragdoll from the configured skinned object transform
        skinnedGameObject->CreateRagdoll("Shark");

        // Initialize procedural spine from the model bind pose
        SkinnedModel* skinnedModel = skinnedGameObject->GetSkinnedModel();
        if (!m_spine.Init(*skinnedModel, kSharkSpineBoneNames, kSharkSpineAnchorIndex, 0.01f)) {
            Logging::Error() << "Shark::Init() failed to initialize spine\n";
        }

        // Register skinned model with animator
        animatorInstance->RegisterSkinnedModels({ "Shark" });

        // Create animation layer index
        m_animationLayerIndex = animatorInstance->CreateAnimationLayer();

        // Begin swimming
        PlayAndLoopAnimation(Bible::AnimationSlot::SHARK_SWIM, 1.0f);

        // Hack in a path
        glm::vec3 center = initialPosition;
        float radius = 10;
        int segments = 9;
        m_path = GetCirclePoints(center, segments, radius);
        ResetSpine(initialPosition);

        m_alive = true;
    }

    void Shark::CleanUp() {
        Animator::RemoveAnimatorInstance(m_animatorInstanceId);
        World::RemoveObjectById(m_skinnedGameObjectId);

        m_animatorInstanceId = 0;
        m_skinnedGameObjectId = 0;
    }

    void Shark::GiveDamage(uint64_t playerId, int damageAmount) {
        m_health -= damageAmount;
        std::cout << "Shark health: " << m_health << "\n";
        HuntPlayer(playerId);
    }

    void Shark::HuntPlayer(uint64_t playerId) {
        m_huntedPlayerId = playerId;
        m_movementState = SharkMovementState::HUNT_PLAYER;
        m_huntingState = SharkHuntingState::CHARGE_PLAYER;
    }

    void Shark::SetPositionToBeginningOfPath() {
        if (m_path.empty()) {
            ResetSpine(glm::vec3(0, 30.0f, 0));
        }
        else {
            glm::vec3 position = m_path[0];
            ResetSpine(position);
            m_nextPathPointIndex = 1;
        }
    }

    void Shark::Respawn() {
        SetPositionToBeginningOfPath();
        m_movementState = SharkMovementState::FOLLOWING_PATH;
        m_health = kSharkHealthMax;
        m_alive = true;
        m_hasBitPlayer = false;
        if (Ragdoll* ragdoll = GetRagdoll()) {
            ragdoll->SetToInitialPose();
            ragdoll->DisableSimulation();
        }
        if (Unloved::SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject()) {
            skinnedGameObject->SetAnimationModeToAnimated();
        }
        PlayAndLoopAnimation(Bible::AnimationSlot::SHARK_SWIM, 1.0f);
    }

    void Shark::Kill() {
        m_health = 0;
        m_alive = false;
        Hell::Audio::PlayAudio("Shark_Death.wav", 1.0f);
        PlayAnimation(Bible::AnimationSlot::SHARK_DEATH, 1.0f);
    }

    void Shark::SetMovementState(SharkMovementState movementState) {
        m_movementState = movementState;
    }

    void Shark::SetPosition(const glm::vec3& position) {
        SetPatrolCenter(position);
    }

    void Shark::ResetSpine(const glm::vec3& position) {
        m_spine.Reset(position, glm::vec3(0.0f, 0.0f, 1.0f));
        m_forward = glm::vec3(0, 0, 1);
    }

    void Shark::SetPatrolCenter(const glm::vec3& position) {
        const glm::vec3 offset = position - m_createInfo.position;
        m_createInfo.position = position;
        m_yHeight = position.y;

        for (glm::vec3& pathPoint : m_path) {
            pathPoint += offset;
        }

        ResetSpine(position);
    }

    Unloved::SkinnedGameObject* Shark::GetSkinnedGameObject() {
        return World::GetSkinnedGameObjectByObjectId(m_skinnedGameObjectId);
    }

    AnimatorInstance* Shark::GetAnimatorInstance() {
        return Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    }

    Ragdoll* Shark::GetRagdoll() {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        return skinnedGameObject ? skinnedGameObject->GetRagdoll() : nullptr;
    }
}
