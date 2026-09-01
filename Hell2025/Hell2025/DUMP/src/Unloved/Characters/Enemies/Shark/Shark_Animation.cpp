#include "Shark.h"

#include "Hell/Math/Math.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Render/Renderer.h"

#include <glm/gtc/quaternion.hpp>

namespace Unloved {

    void Shark::UpdateSpinePose() {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = GetAnimatorInstance();
        if (!skinnedGameObject || !animatorInstance) return;

        const uint32_t segmentCount = m_spine.GetSegmentCount();
        const uint32_t anchorIndex = m_spine.GetAnchorIndex();
        if (segmentCount < 2 || anchorIndex == 0 || anchorIndex + 1 >= segmentCount) return;

        const float anchorRotation = Hell::Math::YawBetweenPoints(m_spine.GetPosition(anchorIndex), m_spine.GetPosition(anchorIndex - 1)) + HELL_PI * 0.5f;

        if (IsAlive()) {
            glm::vec3 position = m_spine.GetAnchorPosition();
            position.y = m_yHeight;
            skinnedGameObject->SetPosition(position);
        }

        skinnedGameObject->SetRotationY(anchorRotation);

        // Bend from the anchor toward the tail
        float parentRotation = anchorRotation;
        for (uint32_t i = anchorIndex + 1; i < segmentCount; i++) {
            const float rotation = Hell::Math::YawBetweenPoints(m_spine.GetPosition(i), m_spine.GetPosition(i - 1)) + HELL_PI * 0.5f;
            animatorInstance->SetNodeRotationOffset(m_spine.GetBoneName(i), glm::angleAxis(rotation - parentRotation, glm::vec3(0, 1, 0)));
            parentRotation = rotation;
        }

        // Bend from the anchor toward the head
        parentRotation = Hell::Math::YawBetweenPoints(m_spine.GetPosition(anchorIndex + 1), m_spine.GetPosition(anchorIndex)) + HELL_PI * 0.5f;
        for (int32_t i = static_cast<int32_t>(anchorIndex) - 1; i >= 0; i--) {
            const float rotation = Hell::Math::YawBetweenPoints(m_spine.GetPosition(i + 1), m_spine.GetPosition(i)) + HELL_PI * 0.5f;
            animatorInstance->SetNodeRotationOffset(m_spine.GetBoneName(i), glm::angleAxis(rotation - parentRotation, glm::vec3(0, 1, 0)));
            parentRotation = rotation;
        }

        if (Ragdoll* ragdoll = GetRagdoll()) {
            if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
                skinnedGameObject->DisableRendering();
            }
            else {
                skinnedGameObject->EnableRendering();
            }
        }
    }

    void Shark::PlayAnimation(AnimationSlot animationSlot, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAnimation(m_animationLayerIndex, Bible::GetAnimation(AnimationProfile::SHARK, animationSlot), speed);
    }

    void Shark::PlayAndLoopAnimation(AnimationSlot animationSlot, float speed) {
        SkinnedGameObject* skinnedGameObject = GetSkinnedGameObject();
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!skinnedGameObject) return;
        if (!animatorInstance) return;

        skinnedGameObject->SetAnimationModeToAnimated();
        animatorInstance->PlayAndLoopAnimation(m_animationLayerIndex, Bible::GetAnimation(AnimationProfile::SHARK, animationSlot), speed);
    }

    int Shark::GetAnimationFrameNumber() {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (!animatorInstance) return 0;

        return animatorInstance->GetAnimationFrameNumber(m_animationLayerIndex);
    }
}
