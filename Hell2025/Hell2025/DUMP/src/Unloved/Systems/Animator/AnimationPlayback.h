#pragma once

#include "Hell/ResourceManagement/Types/Animation.h"
#include "Hell/Transform.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Unloved {

    struct Skeleton;

    struct AnimationPlayback {
        void Play(const Animation& animation, const Skeleton& skeleton, float speed);
        void RebindSkeleton(const Skeleton& skeleton);
        void SetSpeed(float speed);
        void SetNormalizedTime(float normalizedTime, const Skeleton& skeleton);
        float GetCycleRate() const;
        uint32_t GetAnimationFrameNumber() const;
        bool IsPlaying(const Animation& animation) const;
        const AnimationChannel* GetAnimationChannel(uint32_t skeletonNodeIndex) const;
        const std::vector<Hell::QuatTransform>& GetLocalPose() const;
        const std::map<std::string, float>& GetMorphTargetWeights() const;

    private:
        void CalculateLocalPose(const Skeleton& skeleton);
        void CalculateMorphTargetWeights(float normalizedTime);
        glm::vec3 SampleVectorKeys(const std::vector<AnimationVectorKey>& keys) const;

        const Animation* m_animation = nullptr;
        float m_time = 0.0f;
        float m_speed = 1.0f;
        std::vector<int32_t> m_animationChannelIndexBySkeletonNode;
        std::vector<Hell::QuatTransform> m_localPose;
        std::map<std::string, float> m_morphTargetWeights;
    };
}
