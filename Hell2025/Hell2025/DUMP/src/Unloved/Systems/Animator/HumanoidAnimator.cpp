#include "HumanoidAnimator.h"

#include "Hell/Logging.h"
#include "Unloved/Systems/Animator/AnimatorInstance.h"

#include <algorithm>

namespace Unloved {

    bool LocomotionBlendAnimationIndices::IsValid() const {
        return layerIndex != UINT32_MAX && standingIdle != UINT32_MAX && standingMoving != UINT32_MAX && crouchingIdle != UINT32_MAX && crouchingMoving != UINT32_MAX;
    }

    bool HumanoidAnimatorState::IsValid() const {
        return locomotion.IsValid() && upperBodyLayerIndex != UINT32_MAX;
    }
}

namespace Unloved::HumanoidAnimator {

    HumanoidAnimatorState Configure(AnimatorInstance& animatorInstance, const HumanoidAnimatorConfig& config) {
        HumanoidAnimatorState state;

        if (config.skinnedModelNames.empty()) {
            Logging::Error() << "HumanoidAnimator::Configure(..) received no skinned model names\n";
            return state;
        }
        const bool hasLinkedSkinnedModelName = !config.linkedSkinnedModelName.empty();
        const bool hasLinkedModelSourceNodeName = !config.linkedModelSourceNodeName.empty();
        if (hasLinkedSkinnedModelName != hasLinkedModelSourceNodeName) {
            Logging::Error() << "HumanoidAnimator::Configure(..) requires both linked model fields or neither\n";
            return state;
        }

        animatorInstance.RegisterSkinnedModels(config.skinnedModelNames);
        if (hasLinkedSkinnedModelName) animatorInstance.SetSkinnedModelMotionSource(config.linkedSkinnedModelName, config.linkedModelSourceNodeName);

        state.locomotion.layerIndex = animatorInstance.CreateAnimationLayer();
        state.locomotion.standingIdle = animatorInstance.AddBlendAnimation(state.locomotion.layerIndex, config.standingIdleAnimationName, 1.0f);
        state.locomotion.standingMoving = animatorInstance.AddBlendAnimation(state.locomotion.layerIndex, config.standingMovingAnimationName, 1.0f);
        state.locomotion.crouchingIdle = animatorInstance.AddBlendAnimation(state.locomotion.layerIndex, config.crouchingIdleAnimationName, 1.0f);
        state.locomotion.crouchingMoving = animatorInstance.AddBlendAnimation(state.locomotion.layerIndex, config.crouchingMovingAnimationName, 1.0f);
        animatorInstance.SetAnimationLayerBoneMasks(state.locomotion.layerIndex, config.locomotionBoneMasks);

        if (!state.locomotion.IsValid()) {
            Logging::Error() << "HumanoidAnimator::Configure(..) failed to create the locomotion blend\n";
            return state;
        }

        state.upperBodyLayerIndex = animatorInstance.CreateAnimationLayer();
        if (hasLinkedSkinnedModelName) animatorInstance.SetAnimationLayerSkinnedModelWeightSource(state.upperBodyLayerIndex, config.linkedSkinnedModelName, config.linkedModelSourceNodeName);
        animatorInstance.SetAnimationLayerBoneMasks(state.upperBodyLayerIndex, config.upperBodyBoneMasks);
        animatorInstance.SetAnimationLayerGlobalRotationBlend(state.upperBodyLayerIndex, true);
        animatorInstance.SetMorphSourceLayer(state.upperBodyLayerIndex);

        return state;
    }

    void ApplyLocomotionBlend(AnimatorInstance& animatorInstance, const LocomotionBlendAnimationIndices& animationIndices, float crouchBlend, float movementBlend) {
        if (!animationIndices.IsValid()) return;

        crouchBlend = std::clamp(crouchBlend, 0.0f, 1.0f);
        movementBlend = std::clamp(movementBlend, 0.0f, 1.0f);

        const float standingWeight = 1.0f - crouchBlend;
        const float crouchingWeight = crouchBlend;
        animatorInstance.SetAnimationWeight(animationIndices.layerIndex, animationIndices.standingIdle, (1.0f - movementBlend) * standingWeight);
        animatorInstance.SetAnimationWeight(animationIndices.layerIndex, animationIndices.standingMoving, movementBlend * standingWeight);
        animatorInstance.SetAnimationWeight(animationIndices.layerIndex, animationIndices.crouchingIdle, (1.0f - movementBlend) * crouchingWeight);
        animatorInstance.SetAnimationWeight(animationIndices.layerIndex, animationIndices.crouchingMoving, movementBlend * crouchingWeight);
    }
}
