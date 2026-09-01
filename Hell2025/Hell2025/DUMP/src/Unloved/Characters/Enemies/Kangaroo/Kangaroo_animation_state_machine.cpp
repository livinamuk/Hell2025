 #include "Kangaroo.h"
#include "Unloved/Session/Session.h"

#include <iostream> // TODO clean up logging


namespace Unloved {

    void Kangaroo::UpdateAnimationStateMachine() {
        if (m_animationState == KanagarooAnimationState::IDLE_TO_HOP) {
            if (AnimationIsComplete()) {
                m_animationState = KanagarooAnimationState::HOP;
                PlayAnimation("Kangaroo_Hop", 1.0f);
            }
        }

        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
        float distanceToPlayer = glm::distance(player->GetFootPosition(), m_position);
        float biteRange = 10.0f;

        // Hack to stop him killing you
       //if (distanceToPlayer < 1.0f) {
       //    m_animationState = KanagarooAnimationState::IDLE;
       //}

        // If at end of hop, 
        if (m_animationState == KanagarooAnimationState::HOP) {
            if (AnimationIsComplete()) {

                // Bite if within range
                if (distanceToPlayer < biteRange) {
                    m_animationState = KanagarooAnimationState::BITE;
                    PlayAnimation("Kangaroo_Bite", 1.0f);
                    PlayBiteSound();
                    std::cout << "Trigger bite\n";
                    m_timeSinceBiteBegan = 0;

                }
                // Otherwise keep hopping
                else {
                    m_animationState = KanagarooAnimationState::HOP;
                    PlayAnimation("Kangaroo_Hop", 1.0f);

                }
            }
        }

        // If bite done, then resume idle 
        if (m_animationState == KanagarooAnimationState::BITE) {
            if (AnimationIsComplete()) {
                m_animationState = KanagarooAnimationState::IDLE;
                PlayAnimation("Kangaroo_Idle", 1.0f);
            }
        }

        // If been in idle long enough, attack player, assuming roo is angry
        float coolDownDuration = 1.0f;
        if (m_animationState == KanagarooAnimationState::IDLE &&
            m_agroState == KanagarooAgroState::ANGRY &&
            m_timeSinceIdleBegan > coolDownDuration) {

            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
            glm::vec3 playerPosition = player->GetCameraPosition();
            GoToTarget(playerPosition);
        }


        // Did you kill the player
        // aka is bite playing and player within death range
        uint32_t animationFrameNumber = GetAnimationFrameNumber();

        if (m_animationState == KanagarooAnimationState::BITE &&
            animationFrameNumber > 10 &&
            animationFrameNumber < 20 &&
            distanceToPlayer < 4.0f) {
            player->Kill(false);
        }
    }
}
