#pragma once

#include "Unloved/Systems/Animator/AnimationPlayback.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Unloved {

    struct Skeleton;

    struct AnimationLayer {
        void RebindSkeleton(const Skeleton& skeleton);
        uint32_t AddBlendAnimation(const Animation& animation, const Skeleton& skeleton, float speed);
        void PlayAnimation(const Animation& animation, const Skeleton& skeleton, float speed);
        void PlayAndLoopAnimation(const Animation& animation, const Skeleton& skeleton, float speed);
        void Restart();
        void SetAnimationWeight(uint32_t animationIndex, float weight);
        void SetWeight(float weight);
        void SetBoneMasks(const std::vector<std::string>& boneMaskNames, const std::vector<float>& nodeWeights);
        void ClearBoneMasks();
        void SetSkinnedModelWeightSource(uint32_t skinnedModelId, const std::string& nodeName);
        void ReplaceSkinnedModelWeightSource(uint32_t oldSkinnedModelId, uint32_t newSkinnedModelId);
        void SetGlobalRotationBlend(bool globalRotationBlend);
        void Update(float deltaTime, const Skeleton& skeleton, bool calculateGlobalPose);

        float GetWeight() const;
        float GetNodeWeight(uint32_t nodeIndex) const;
        bool UsesGlobalRotationBlend() const;
        bool HasTranslation(uint32_t nodeIndex) const;
        bool HasRotation(uint32_t nodeIndex) const;
        bool HasScale(uint32_t nodeIndex) const;
        uint32_t GetAnimationFrameNumber() const;
        bool IsAnimationComplete() const;
        bool AnimationAdvancedThisFrame() const;
        const std::vector<Hell::QuatTransform>& GetLocalPose() const;
        const std::vector<glm::mat4>& GetGlobalPose() const;
        const std::vector<glm::quat>& GetGlobalRotations() const;
        const std::map<std::string, float>& GetMorphTargetWeights() const;
        const std::vector<std::string>& GetBoneMaskNames() const;
        const std::map<uint32_t, std::string>& GetSkinnedModelWeightSources() const;

    private:
        uint32_t AddAnimationPlayback(const Animation& animation, const Skeleton& skeleton, float speed, float initialWeight);
        void CalculateGlobalPose(const Skeleton& skeleton);

        float m_normalizedTime = 0.0f;
        float m_weight = 1.0f;
        bool m_looping = true;
        bool m_globalRotationBlend = false;
        bool m_animationAdvancedThisFrame = false;
        std::vector<AnimationPlayback> m_animationPlaybacks;
        std::vector<float> m_animationWeights;
        std::vector<float> m_nodeWeights;
        std::vector<std::string> m_boneMaskNames;
        std::map<uint32_t, std::string> m_skinnedModelWeightSourceNodeNames;
        std::vector<uint8_t> m_animatedTranslations;
        std::vector<uint8_t> m_animatedRotations;
        std::vector<uint8_t> m_animatedScales;
        std::vector<Hell::QuatTransform> m_localPose;
        std::vector<glm::mat4> m_globalPose;
        std::vector<glm::quat> m_globalRotations;
        std::map<std::string, float> m_morphTargetWeights;
    };
}
