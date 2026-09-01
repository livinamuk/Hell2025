#include "GenericAnimatedObject.h"

#include "Hell/Input.h"
#include "Hell/Physics/PhysicsTypes.h"
#include "Unloved/Session/Session.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/World/World.h"


namespace Unloved {

GenericAnimatedObject::GenericAnimatedObject(uint64_t id, const GenericAnimatedObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    CreateSkinnedGameObjects();
}

void GenericAnimatedObject::Update() {
    //if (SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId)) {
    //    if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_0)) {
    //        skinnedGameObject->SetIgnoredViewportIndex(0);
    //    }
    //    if (Hell::Input::KeyPressed(HELL_KEY_NUMPAD_1)) {
    //        skinnedGameObject->SetIgnoredViewportIndex(-1);
    //    }
    //
    //
    //    Player* player = Session::GetLocalPlayerByViewportIndex(0);
    //    Camera& camera = player->GetCamera();
    //    skinnedGameObject->SetPosition(player->GetFootPosition());
    //    skinnedGameObject->SetRotationY(camera.GetEulerRotation().y + HELL_PI);
    //}

    if (m_debugDraw) DebugDraw();
    if (m_debugDrawEjectionPort) {
        SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);
        if (!weaponSkinnedGameObject) return;
        DebugDraw::DrawPoint(weaponSkinnedGameObject->GetNodeWorldPosition("EjectionPort"), RED);
    }
}

void GenericAnimatedObject::DebugDraw() {
    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    if (!animatorInstance || !characterSkinnedGameObject) return;

    const std::vector<SkeletonNode>& nodes = animatorInstance->GetSkeleton().GetNodes();
    const std::vector<glm::mat4>& globalPose = animatorInstance->GetGlobalPose();
    if (nodes.size() != globalPose.size()) return;

    const glm::mat4 modelMatrix = characterSkinnedGameObject->GetModelMatrix();

    // Draw the evaluated skeleton in world space
    for (uint32_t i = 0; i < nodes.size(); i++) {
        const glm::vec3 position = modelMatrix * globalPose[i] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        DebugDraw::DrawPoint(position, YELLOW);

        const int32_t parentIndex = nodes[i].parentIndex;
        if (parentIndex == -1) continue;

        const glm::vec3 parentPosition = modelMatrix * globalPose[parentIndex] * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        DebugDraw::DrawLine(position, parentPosition, WHITE);
    }
}

void GenericAnimatedObject::CleanUp() {
    if (m_characterSkinnedGameObjectId != 0) {
        World::RemoveObjectById(m_characterSkinnedGameObjectId);
        m_characterSkinnedGameObjectId = 0;
    }
    if (m_weaponSkinnedGameObjectId != 0) {
        World::RemoveObjectById(m_weaponSkinnedGameObjectId);
        m_weaponSkinnedGameObjectId = 0;
    }
    if (m_animatorInstanceId != 0) {
        Animator::RemoveAnimatorInstance(m_animatorInstanceId);
        m_animatorInstanceId = 0;
    }

    m_humanoidAnimatorState = {};
}

void GenericAnimatedObject::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;

    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);

    if (characterSkinnedGameObject) characterSkinnedGameObject->SetPosition(position);
    if (weaponSkinnedGameObject)    weaponSkinnedGameObject->SetPosition(position);
}

void GenericAnimatedObject::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;

    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);

    if (characterSkinnedGameObject) {
        characterSkinnedGameObject->SetRotationX(rotation.x);
        characterSkinnedGameObject->SetRotationY(rotation.y);
        characterSkinnedGameObject->SetRotationZ(rotation.z);
    }
    if (weaponSkinnedGameObject) {
        weaponSkinnedGameObject->SetRotationX(rotation.x);
        weaponSkinnedGameObject->SetRotationY(rotation.y);
        weaponSkinnedGameObject->SetRotationZ(rotation.z);
    }
}

void GenericAnimatedObject::SetScale(float scale) {
    m_createInfo.scale = scale;

    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);

    if (characterSkinnedGameObject) characterSkinnedGameObject->SetScale(scale);
    if (weaponSkinnedGameObject)    weaponSkinnedGameObject->SetScale(scale);
}

void GenericAnimatedObject::SetType(GenericAnimatedObjectType type) {
    if (m_createInfo.type == type) return;

    m_createInfo.type = type;

    CleanUp();
    CreateSkinnedGameObjects();
}

void GenericAnimatedObject::SetCrouchBlend(float crouchBlend) {
    if (crouchBlend < 0.0f) crouchBlend = 0.0f;
    if (crouchBlend > 1.0f) crouchBlend = 1.0f;
    m_crouchBlend = crouchBlend;

    UpdateLocomotionAnimationWeights();
}

void GenericAnimatedObject::SetMovementBlend(float movementBlend) {
    if (movementBlend < 0.0f) movementBlend = 0.0f;
    if (movementBlend > 1.0f) movementBlend = 1.0f;
    m_movementBlend = movementBlend;

    UpdateLocomotionAnimationWeights();
}

void GenericAnimatedObject::SetWeaponAnimationName(const std::string& animationName) {
    if (m_weaponAnimationName == animationName) return;
    m_weaponAnimationName = animationName;

    UpdateWeaponAnimation();
}

void GenericAnimatedObject::SetHumanoidAnimatorState(const HumanoidAnimatorState& state) {
    m_humanoidAnimatorState = state;
    UpdateLocomotionAnimationWeights();
    UpdateWeaponAnimation();
}

void GenericAnimatedObject::SetDebugDraw(bool debugDraw) {
    m_debugDraw = debugDraw;
}

void GenericAnimatedObject::SetDebugDrawEjectionPort(bool debugDrawEjectionPort) {
    m_debugDrawEjectionPort = debugDrawEjectionPort;
}

void GenericAnimatedObject::UpdateLocomotionAnimationWeights() {
    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    if (!animatorInstance) return;
    HumanoidAnimator::ApplyLocomotionBlend(*animatorInstance, m_humanoidAnimatorState.locomotion, m_crouchBlend, m_movementBlend);
}

void GenericAnimatedObject::UpdateWeaponAnimation() {
    const uint32_t upperBodyLayerIndex = m_humanoidAnimatorState.upperBodyLayerIndex;
    if (upperBodyLayerIndex == UINT32_MAX) return;

    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    if (!animatorInstance) return;

    if (m_weaponAnimationName.empty()) {
        animatorInstance->SetAnimationLayerWeight(upperBodyLayerIndex, 0.0f);
        return;
    }

    animatorInstance->SetAnimationLayerWeight(upperBodyLayerIndex, 1.0f);
    animatorInstance->PlayAndLoopAnimation(upperBodyLayerIndex, m_weaponAnimationName, 1.0f);
}

void GenericAnimatedObject::CreateSkinnedGameObjects() {
    m_animatorInstanceId = Animator::CreateAnimatorInstance();
    m_characterSkinnedGameObjectId = World::CreateSkinnedGameObject();
    m_weaponSkinnedGameObjectId = World::CreateSkinnedGameObject();

    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);

    if (!characterSkinnedGameObject || !weaponSkinnedGameObject || !animatorInstance) {
        CleanUp();
        return;
    }

    characterSkinnedGameObject->SetOwnerObjectId(m_objectId);
    weaponSkinnedGameObject->SetOwnerObjectId(m_objectId);

    ApplyTransform();

    if (m_createInfo.type == GenericAnimatedObjectType::RAT_KING) {
        const AnimationProfile animationProfile = AnimationProfile::RAT_KING_GLOCK;
        const HumanoidAnimatorState humanoidAnimatorState = Bible::ConfigureHumanoidAnimator(*animatorInstance, animationProfile);
        if (!humanoidAnimatorState.IsValid()) {
            CleanUp();
            return;
        }

        SetHumanoidAnimatorState(humanoidAnimatorState);
        SetWeaponAnimationName(Bible::GetAnimation(animationProfile, AnimationSlot::IDLE));

        characterSkinnedGameObject->SetSkinnedModel("RatKing", SkinnedModelPreset::RATKING);
        weaponSkinnedGameObject->SetSkinnedModel("CharacterGlock");
        characterSkinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);
        weaponSkinnedGameObject->SetAnimatorInstanceId(m_animatorInstanceId);
        characterSkinnedGameObject->SetAnimationModeToAnimated();
        weaponSkinnedGameObject->SetAnimationModeToAnimated();

        PhysicsFilterData filterData;
        filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
        filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
        filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);
        characterSkinnedGameObject->CreateRagdoll("RatKing", filterData);
    }

    RestartAnimation();
}

void GenericAnimatedObject::ApplyTransform() {
    SkinnedGameObject* characterSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_characterSkinnedGameObjectId);
    SkinnedGameObject* weaponSkinnedGameObject = World::GetSkinnedGameObjectByObjectId(m_weaponSkinnedGameObjectId);

    if (characterSkinnedGameObject) {
        characterSkinnedGameObject->SetPosition(m_createInfo.position);
        characterSkinnedGameObject->SetRotationX(m_createInfo.rotation.x);
        characterSkinnedGameObject->SetRotationY(m_createInfo.rotation.y);
        characterSkinnedGameObject->SetRotationZ(m_createInfo.rotation.z);
        characterSkinnedGameObject->SetScale(m_createInfo.scale);
    }
    if (weaponSkinnedGameObject) {
        weaponSkinnedGameObject->SetPosition(m_createInfo.position);
        weaponSkinnedGameObject->SetRotationX(m_createInfo.rotation.x);
        weaponSkinnedGameObject->SetRotationY(m_createInfo.rotation.y);
        weaponSkinnedGameObject->SetRotationZ(m_createInfo.rotation.z);
        weaponSkinnedGameObject->SetScale(m_createInfo.scale);
    }
}

void GenericAnimatedObject::RestartAnimation() {
    AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
    if (!animatorInstance) return;
    animatorInstance->RestartAnimation();
}
}
