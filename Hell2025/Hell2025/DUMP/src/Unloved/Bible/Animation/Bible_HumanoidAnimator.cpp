#include "../Bible.h"

#include "Hell/Logging.h"
#include "Unloved/Systems/Animator/HumanoidAnimator.h"

namespace Bible {
    using namespace Unloved;

    HumanoidAnimatorState ConfigureHumanoidAnimator(AnimatorInstance& animatorInstance, AnimationProfile animationProfile) {
        HumanoidAnimatorConfig config;

        switch (animationProfile) {
            case AnimationProfile::RAT_KING_GLOCK:
                config.skinnedModelNames = { "RatKing", "CharacterGlock" };
                config.locomotionBoneMasks = { "RatKing_LowerBody", "RatKing_Chest" };
                config.upperBodyBoneMasks = { "RatKing_ArmsHead" };
                config.linkedSkinnedModelName = "CharacterGlock";
                config.linkedModelSourceNodeName = "hand_R";
                break;
            default:
                Logging::Error() << "Bible::ConfigureHumanoidAnimator(..) received an unsupported animation profile\n";
                return {};
        }

        config.standingIdleAnimationName = GetAnimation(animationProfile, AnimationSlot::IDLE);
        config.standingMovingAnimationName = GetAnimation(animationProfile, AnimationSlot::WALK);
        config.crouchingIdleAnimationName = GetAnimation(animationProfile, AnimationSlot::IDLE_CROUCHING);
        config.crouchingMovingAnimationName = GetAnimation(animationProfile, AnimationSlot::WALK_CROUCHING);
        return HumanoidAnimator::Configure(animatorInstance, config);
    }
}
