#pragma once

#include "Hell/Physics/Ragdoll/Ragdoll.h"

#include "Unloved/Characters/CharacterSpine.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Bible/Bible_enums.h"
#include "Unloved/Systems/Animator/AnimatorInstance.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"

#include <cstdint>
#include <string>
#include <vector>

enum class SharkMovementState {
    STOPPED,
    FOLLOWING_PATH,
    FOLLOWING_PATH_ANGRY,
    ARROW_KEYS,
    HUNT_PLAYER,
    UNDEFINED
};

enum class SharkHuntingState {
    CHARGE_PLAYER,
    BITING_PLAYER,
    UNDEFINED
};

namespace Unloved {

    inline constexpr int kSharkCollisionSphereRadius = 1;
    inline constexpr int kSharkHealthMax = 1000;

    struct Shark {
        Shark() = default;
        Shark(uint64_t id, const SharkCreateInfo& createInfo, const SpawnOffset& spawnOffset);
        Shark(const Shark&) = delete;
        Shark& operator=(const Shark&) = delete;
        Shark(Shark&&) noexcept = default;
        Shark& operator=(Shark&&) noexcept = default;
        ~Shark() = default;

        void Init();
        void UpdateMovement(float deltaTime);
        void Update(float deltaTime);
        void SetPosition(const glm::vec3& position);
        void SetPatrolCenter(const glm::vec3& position);
        void CleanUp();
        void DrawSpinePoints();
        void HuntPlayer(uint64_t playerId);
        void GiveDamage(uint64_t playerId, int damageAmount);
        void Kill();
        void Respawn();
        void SetPositionToBeginningOfPath();
        void PlayAnimation(Bible::AnimationSlot animationSlot, float speed);
        void PlayAndLoopAnimation(Bible::AnimationSlot animationSlot, float speed);
        void SetMovementState(SharkMovementState state);
        void StraightenSpine(float deltaTime, float straightSpeed);

        std::string GetDebugInfoAsString();
        void DrawDebug();

        AnimatorInstance* GetAnimatorInstance();
        Ragdoll* GetRagdoll();
        SkinnedGameObject* GetSkinnedGameObject();

        SharkHuntingState GetHuntingState()          { return m_huntingState; }
        SharkMovementState GetMovementState()        { return m_movementState; }
        const bool IsDead() const                    { return !m_alive; }
        const bool IsAlive() const                   { return m_alive; }
        const uint64_t& GetObjectId() const          { return m_objectId; };
        const glm::vec3& GetPosition() const         { return m_spine.GetLeadPosition(); }
        const SharkCreateInfo& GetCreateInfo() const { return m_createInfo; }
        const std::string& GetEditorName() const     { return m_createInfo.editorName; }

    private:
        void CalculateTargetFromPath();
        void CalculateForwardVectorFromTarget(float deltaTime);
        void CalculateForwardVectorFromArrowKeys(float deltaTime);
        void CalculateTargetFromPlayer();
        void MoveShark(float deltaTime);
        void ResetSpine(const glm::vec3& position);

        void UpdateHuntingLogic(float deltaTime);

        // Animation
        int GetAnimationFrameNumber();
        void UpdateSpinePose();

        // Movement queries
        float GetDistanceMouthToTarget3D();
        float GetDistanceToTarget2D();
        float GetTurningRadius() const;
        bool TargetIsOnLeft(glm::vec3 targetPosition);
        bool IsBehindEvadePoint(glm::vec3 position);
        glm::vec3 GetMouthPosition3D();
        glm::vec3 GetForwardVector();
        glm::vec3 GetTargetPosition2D();
        glm::vec3 GetHeadPosition2D();
        glm::vec3 GetMouthPosition2D();
        glm::vec3 GetCollisionLineEnd();
        glm::vec3 GetCollisionSphereFrontPosition();
        glm::vec3 GetEvadePoint3D();
        glm::vec3 GetEvadePoint2D();

        uint64_t m_objectId = 0;
        uint64_t m_animatorInstanceId = 0;
        uint64_t m_skinnedGameObjectId = 0;
        uint64_t m_huntedPlayerId = 0;
        uint32_t m_animationLayerIndex = 0;

        int m_health = kSharkHealthMax;
        int m_logicSubStepCount = 8;
        float m_swimSpeed = 8.0f;
        float m_rotationSpeed = 2.5f;
        glm::vec3 m_forward = glm::vec3(0);
        glm::vec3 m_right = glm::vec3(0);
        glm::vec3 m_left = glm::vec3(0);
        bool m_hasBitPlayer = false;
        bool m_alive = false;
        float m_yHeight = 0.0f;

        SharkHuntingState m_huntingState = SharkHuntingState::UNDEFINED;
        SharkMovementState m_movementState = SharkMovementState::FOLLOWING_PATH;

        int32_t m_nextPathPointIndex = 0;
        glm::vec3 m_targetPosition = glm::vec3(0);
        std::vector<glm::vec3> m_path;
        CharacterSpine m_spine;
        SharkCreateInfo m_createInfo;
    };
}
