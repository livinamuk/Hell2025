#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Unloved {

    struct AnimatorInstance;

    struct LocomotionBlendAnimationIndices {
        uint32_t layerIndex = UINT32_MAX;
        uint32_t standingIdle = UINT32_MAX;
        uint32_t standingMoving = UINT32_MAX;
        uint32_t crouchingIdle = UINT32_MAX;
        uint32_t crouchingMoving = UINT32_MAX;

        bool IsValid() const;
    };

    struct HumanoidAnimatorConfig {
        std::vector<std::string> skinnedModelNames;
        std::string standingIdleAnimationName;
        std::string standingMovingAnimationName;
        std::string crouchingIdleAnimationName;
        std::string crouchingMovingAnimationName;
        std::vector<std::string> locomotionBoneMasks;
        std::vector<std::string> upperBodyBoneMasks;
        std::string linkedSkinnedModelName;
        std::string linkedModelSourceNodeName;
    };

    struct HumanoidAnimatorState {
        LocomotionBlendAnimationIndices locomotion;
        uint32_t upperBodyLayerIndex = UINT32_MAX;

        bool IsValid() const;
    };
}

namespace Unloved::HumanoidAnimator {

    HumanoidAnimatorState Configure(AnimatorInstance& animatorInstance, const HumanoidAnimatorConfig& config);
    void ApplyLocomotionBlend(AnimatorInstance& animatorInstance, const LocomotionBlendAnimationIndices& animationIndices, float crouchBlend, float movementBlend);
}
