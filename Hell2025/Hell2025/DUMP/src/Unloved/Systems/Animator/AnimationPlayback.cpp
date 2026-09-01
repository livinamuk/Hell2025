#include "AnimationPlayback.h"

#include "Unloved/Systems/Animator/Skeleton.h"

#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace Unloved {

    void AnimationPlayback::Play(const Animation& animation, const Skeleton& skeleton, float speed) {
        m_animation = &animation;
        m_time = 0.0f;
        m_speed = speed;

        RebindSkeleton(skeleton);
    }

    void AnimationPlayback::RebindSkeleton(const Skeleton& skeleton) {
        if (!m_animation) return;

        m_animationChannelIndexBySkeletonNode.assign(skeleton.GetNodes().size(), -1);
        m_localPose.resize(skeleton.GetNodes().size());

        // Match animation channels to skeleton nodes once
        for (uint32_t channelIndex = 0; channelIndex < m_animation->m_channels.size(); channelIndex++) {
            const int32_t nodeIndex = skeleton.GetNodeIndex(m_animation->m_channels[channelIndex].nodeName);
            if (nodeIndex == -1) continue;
            if (m_animationChannelIndexBySkeletonNode[nodeIndex] != -1) continue;
            m_animationChannelIndexBySkeletonNode[nodeIndex] = static_cast<int32_t>(channelIndex);
        }
    }

    void AnimationPlayback::SetSpeed(float speed) {
        m_speed = speed;
    }

    void AnimationPlayback::CalculateLocalPose(const Skeleton& skeleton) {
        if (!m_animation) return;
        const std::vector<SkeletonNode>& nodes = skeleton.GetNodes();

        // Sample every skeleton node from its bind pose
        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            m_localPose[nodeIndex] = nodes[nodeIndex].localBindPose;

            const int32_t channelIndex = m_animationChannelIndexBySkeletonNode[nodeIndex];
            if (channelIndex == -1) continue;

            const AnimationChannel& channel = m_animation->m_channels[channelIndex];
            if (!channel.translationKeys.empty()) m_localPose[nodeIndex].translation = SampleVectorKeys(channel.translationKeys);
            if (!channel.scaleKeys.empty()) m_localPose[nodeIndex].scale = SampleVectorKeys(channel.scaleKeys);

            if (!channel.rotationKeys.empty()) {
                if (channel.rotationKeys.size() == 1 || m_time <= channel.rotationKeys.front().time) {
                    m_localPose[nodeIndex].rotation = channel.rotationKeys.front().value;
                }
                else if (m_time >= channel.rotationKeys.back().time) {
                    m_localPose[nodeIndex].rotation = channel.rotationKeys.back().value;
                }
                else {
                    uint32_t left = 1;
                    uint32_t right = static_cast<uint32_t>(channel.rotationKeys.size() - 1);

                    while (left < right) {
                        const uint32_t middle = left + (right - left) / 2;

                        if (m_time <= channel.rotationKeys[middle].time) right = middle;
                        else left = middle + 1;
                    }

                    const AnimationQuaternionKey& previousKey = channel.rotationKeys[left - 1];
                    const AnimationQuaternionKey& nextKey = channel.rotationKeys[left];
                    const float keyDuration = nextKey.time - previousKey.time;
                    const float blendFactor = keyDuration > 0.0f ? (m_time - previousKey.time) / keyDuration : 0.0f;
                    glm::quat nextRotation = nextKey.value;
                    if (glm::dot(previousKey.value, nextRotation) < 0.0f) nextRotation = -nextRotation;
                    m_localPose[nodeIndex].rotation = glm::normalize(glm::slerp(previousKey.value, nextRotation, blendFactor));
                }
            }
        }
    }

    void AnimationPlayback::SetNormalizedTime(float normalizedTime, const Skeleton& skeleton) {
        if (!m_animation) return;

        m_time = normalizedTime * m_animation->m_duration;
        CalculateLocalPose(skeleton);
        CalculateMorphTargetWeights(normalizedTime);
    }

    float AnimationPlayback::GetCycleRate() const {
        if (!m_animation || m_animation->m_duration <= 0.0f) return 0.0f;
        return m_speed / m_animation->m_duration;
    }

    uint32_t AnimationPlayback::GetAnimationFrameNumber() const {
        if (!m_animation) return 0;
        return static_cast<uint32_t>(m_time * m_animation->m_ticksPerSecond);
    }

    bool AnimationPlayback::IsPlaying(const Animation& animation) const {
        return m_animation == &animation;
    }

    const AnimationChannel* AnimationPlayback::GetAnimationChannel(uint32_t skeletonNodeIndex) const {
        if (!m_animation) return nullptr;
        if (skeletonNodeIndex >= m_animationChannelIndexBySkeletonNode.size()) return nullptr;

        const int32_t animationChannelIndex = m_animationChannelIndexBySkeletonNode[skeletonNodeIndex];
        if (animationChannelIndex == -1) return nullptr;
        return &m_animation->m_channels[animationChannelIndex];
    }

    const std::vector<Hell::QuatTransform>& AnimationPlayback::GetLocalPose() const {
        return m_localPose;
    }

    const std::map<std::string, float>& AnimationPlayback::GetMorphTargetWeights() const {
        return m_morphTargetWeights;
    }

    void AnimationPlayback::CalculateMorphTargetWeights(float normalizedTime) {
        m_morphTargetWeights.clear();
        if (!m_animation) return;

        for (const ShapeAnimationChannel& channel : m_animation->m_shapeAnimation.channels) {
            if (channel.samples.empty()) continue;

            const float samplePosition = normalizedTime * static_cast<float>(channel.samples.size() - 1);
            const uint32_t previousSampleIndex = static_cast<uint32_t>(samplePosition);
            const uint32_t nextSampleIndex = std::min(previousSampleIndex + 1, static_cast<uint32_t>(channel.samples.size() - 1));
            const float blendFactor = samplePosition - static_cast<float>(previousSampleIndex);
            const float previousValue = channel.samples[previousSampleIndex];
            const float nextValue = channel.samples[nextSampleIndex];
            m_morphTargetWeights[channel.targetName] = previousValue + (nextValue - previousValue) * blendFactor;
        }
    }

    glm::vec3 AnimationPlayback::SampleVectorKeys(const std::vector<AnimationVectorKey>& keys) const {
        if (keys.size() == 1 || m_time <= keys.front().time) return keys.front().value;
        if (m_time >= keys.back().time) return keys.back().value;

        uint32_t left = 1;
        uint32_t right = static_cast<uint32_t>(keys.size() - 1);

        while (left < right) {
            const uint32_t middle = left + (right - left) / 2;

            if (m_time <= keys[middle].time) right = middle;
            else left = middle + 1;
        }

        const AnimationVectorKey& previousKey = keys[left - 1];
        const AnimationVectorKey& nextKey = keys[left];
        const float keyDuration = nextKey.time - previousKey.time;
        const float blendFactor = keyDuration > 0.0f ? (m_time - previousKey.time) / keyDuration : 0.0f;

        return previousKey.value + (nextKey.value - previousKey.value) * blendFactor;
    }
}
