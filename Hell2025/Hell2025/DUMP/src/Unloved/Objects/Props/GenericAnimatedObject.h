#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Systems/Animator/HumanoidAnimator.h"

namespace Unloved {

struct SkinnedGameObject;

struct GenericAnimatedObject {
    GenericAnimatedObject() = default;
    GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    GenericAnimatedObject(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject& operator=(const GenericAnimatedObject&) = delete;
    GenericAnimatedObject(GenericAnimatedObject&&) noexcept = default;
    GenericAnimatedObject& operator=(GenericAnimatedObject&&) noexcept = default;
    ~GenericAnimatedObject() = default;

    void Update();
    void DebugDraw();
    void CleanUp();

    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(float scale);
    void SetType(GenericAnimatedObjectType type);
    void SetCrouchBlend(float crouchBlend);
    void SetMovementBlend(float movementBlend);
    void SetWeaponAnimationName(const std::string& animationName);
    void SetHumanoidAnimatorState(const HumanoidAnimatorState& state);
    void SetDebugDraw(bool debugDraw);
    void SetDebugDrawEjectionPort(bool debugDrawEjectionPort);

    uint64_t GetObjectId() const                                        { return m_objectId; }
    uint64_t GetCharacterSkinnedGameObjectId() const                   { return m_characterSkinnedGameObjectId; }
    uint64_t GetWeaponSkinnedGameObjectId() const                      { return m_weaponSkinnedGameObjectId; }
    uint64_t GetAnimatorInstanceId() const                              { return m_animatorInstanceId; }
    uint32_t GetWeaponAnimationLayerIndex() const                       { return m_humanoidAnimatorState.upperBodyLayerIndex; }
    float GetCrouchBlend() const                                       { return m_crouchBlend; }
    float GetMovementBlend() const                                     { return m_movementBlend; }
    const std::string& GetWeaponAnimationName() const                   { return m_weaponAnimationName; }
    bool GetDebugDraw() const                                          { return m_debugDraw; }
    bool GetDebugDrawEjectionPort() const                              { return m_debugDrawEjectionPort; }
    const glm::vec3& GetPosition() const                                { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                                { return m_createInfo.rotation; }
    float GetScale() const                                              { return m_createInfo.scale; }
    GenericAnimatedObjectType GetType() const                           { return m_createInfo.type; }
    const std::string& GetEditorName() const                            { return m_createInfo.editorName; }
    const GenericAnimatedObjectCreateInfo& GetCreateInfo() const        { return m_createInfo; }

private:
    void CreateSkinnedGameObjects();
    void ApplyTransform();
    void RestartAnimation();
    void UpdateLocomotionAnimationWeights();
    void UpdateWeaponAnimation();

    GenericAnimatedObjectCreateInfo m_createInfo;
    uint64_t m_objectId = 0;
    uint64_t m_characterSkinnedGameObjectId = 0;
    uint64_t m_weaponSkinnedGameObjectId = 0;
    uint64_t m_animatorInstanceId = 0;
    HumanoidAnimatorState m_humanoidAnimatorState;
    float m_crouchBlend = 0.0f;
    float m_movementBlend = 1.0f;
    std::string m_weaponAnimationName;
    bool m_debugDraw = false;
    bool m_debugDrawEjectionPort = false;
};
}
