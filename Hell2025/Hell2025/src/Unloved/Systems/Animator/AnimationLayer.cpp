#include "AnimationLayer.h"

#include "Hell/Logging.h"
#include "Unloved/Systems/Animator/Skeleton.h"

#include <glm/gtc/quaternion.hpp>

namespace Unloved {

    void AnimationLayer::RebindSkeleton(const Skeleton& skeleton) {
        const std::vector<SkeletonNode>& nodes = skeleton.GetNodes();
        const float defaultNodeWeight = m_boneMaskNames.empty() ? 1.0f : 0.0f;

        m_nodeWeights.assign(nodes.size(), defaultNodeWeight);
        m_animatedTranslations.assign(nodes.size(), 0);
        m_animatedRotations.assign(nodes.size(), 0);
        m_animatedScales.assign(nodes.size(), 0);
        m_localPose.resize(nodes.size());
        m_globalPose.resize(nodes.size());
        m_globalRotations.resize(nodes.size());

        // Remove weight sources belonging to models no longer registered
        std::map<uint32_t, std::string> retainedWeightSources;
        for (const auto& weightSourceEntry : m_skinnedModelWeightSourceNodeNames) {
            if (!skeleton.ContainsSkinnedModel(weightSourceEntry.first)) continue;
            retainedWeightSources[weightSourceEntry.first] = weightSourceEntry.second;
        }
        m_skinnedModelWeightSourceNodeNames = retainedWeightSources;

        for (AnimationPlayback& animationPlayback : m_animationPlaybacks) {
            animationPlayback.RebindSkeleton(skeleton);
        }
    }

    uint32_t AnimationLayer::AddBlendAnimation(const Animation& animation, const Skeleton& skeleton, float speed) {
        return AddAnimationPlayback(animation, skeleton, speed, 0.0f);
    }

    uint32_t AnimationLayer::AddAnimationPlayback(const Animation& animation, const Skeleton& skeleton, float speed, float initialWeight) {
        AnimationPlayback& animationPlayback = m_animationPlaybacks.emplace_back();
        animationPlayback.Play(animation, skeleton, speed);
        m_animationWeights.push_back(initialWeight);
        return static_cast<uint32_t>(m_animationPlaybacks.size() - 1);
    }

    void AnimationLayer::PlayAnimation(const Animation& animation, const Skeleton& skeleton, float speed) {
        m_normalizedTime = 0.0f;
        m_looping = false;
        m_animationPlaybacks.clear();
        m_animationWeights.clear();
        AddAnimationPlayback(animation, skeleton, speed, 1.0f);
    }

    void AnimationLayer::PlayAndLoopAnimation(const Animation& animation, const Skeleton& skeleton, float speed) {
        if (m_looping && m_animationPlaybacks.size() == 1 && m_animationPlaybacks[0].IsPlaying(animation)) {
            m_animationPlaybacks[0].SetSpeed(speed);
            m_animationWeights[0] = 1.0f;
            return;
        }

        m_normalizedTime = 0.0f;
        m_looping = true;
        m_animationPlaybacks.clear();
        m_animationWeights.clear();
        AddAnimationPlayback(animation, skeleton, speed, 1.0f);
    }

    void AnimationLayer::Restart() {
        m_normalizedTime = 0.0f;
    }

    void AnimationLayer::SetAnimationWeight(uint32_t animationIndex, float weight) {
        if (animationIndex >= m_animationWeights.size()) {
            Logging::Error() << "AnimationLayer::SetAnimationWeight(..) received invalid animation index " << animationIndex << "\n";
            return;
        }

        if (weight < 0.0f) weight = 0.0f;
        m_animationWeights[animationIndex] = weight;
    }

    void AnimationLayer::SetWeight(float weight) {
        if (weight < 0.0f) weight = 0.0f;
        if (weight > 1.0f) weight = 1.0f;
        m_weight = weight;
    }

    void AnimationLayer::SetBoneMasks(const std::vector<std::string>& boneMaskNames, const std::vector<float>& nodeWeights) {
        if (nodeWeights.size() != m_nodeWeights.size()) {
            Logging::Error() << "AnimationLayer::SetBoneMasks(..) received " << nodeWeights.size() << " weights for " << m_nodeWeights.size() << " nodes\n";
            return;
        }
        m_boneMaskNames = boneMaskNames;
        m_nodeWeights = nodeWeights;
    }

    void AnimationLayer::ClearBoneMasks() {
        m_boneMaskNames.clear();
        m_nodeWeights.assign(m_nodeWeights.size(), 1.0f);
    }

    void AnimationLayer::SetSkinnedModelWeightSource(uint32_t skinnedModelId, const std::string& nodeName) {
        m_skinnedModelWeightSourceNodeNames[skinnedModelId] = nodeName;
    }

    void AnimationLayer::ReplaceSkinnedModelWeightSource(uint32_t oldSkinnedModelId, uint32_t newSkinnedModelId) {
        auto weightSourceIt = m_skinnedModelWeightSourceNodeNames.find(oldSkinnedModelId);
        if (weightSourceIt == m_skinnedModelWeightSourceNodeNames.end()) return;

        const std::string nodeName = weightSourceIt->second;
        m_skinnedModelWeightSourceNodeNames.erase(weightSourceIt);
        m_skinnedModelWeightSourceNodeNames[newSkinnedModelId] = nodeName;
    }

    void AnimationLayer::SetGlobalRotationBlend(bool globalRotationBlend) {
        m_globalRotationBlend = globalRotationBlend;
    }

    void AnimationLayer::Update(float deltaTime, const Skeleton& skeleton, bool calculateGlobalPose) {
        const std::vector<SkeletonNode>& nodes = skeleton.GetNodes();
        const float previousNormalizedTime = m_normalizedTime;
        m_animationAdvancedThisFrame = false;
        m_morphTargetWeights.clear();

        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            m_localPose[nodeIndex] = nodes[nodeIndex].localBindPose;
            m_animatedTranslations[nodeIndex] = 0;
            m_animatedRotations[nodeIndex] = 0;
            m_animatedScales[nodeIndex] = 0;
        }

        float totalAnimationWeight = 0.0f;
        float cycleRate = 0.0f;

        // Calculate the shared phase speed from the active animations
        for (uint32_t animationIndex = 0; animationIndex < m_animationPlaybacks.size(); animationIndex++) {
            const float weight = m_animationWeights[animationIndex];
            if (weight <= 0.0f) continue;
            totalAnimationWeight += weight;
            cycleRate += m_animationPlaybacks[animationIndex].GetCycleRate() * weight;
        }
        if (totalAnimationWeight <= 0.0f) {
            if (calculateGlobalPose) CalculateGlobalPose(skeleton);
            return;
        }

        if (!IsAnimationComplete()) {
            m_normalizedTime += deltaTime * cycleRate / totalAnimationWeight;
            if (m_looping) {
                while (m_normalizedTime >= 1.0f) m_normalizedTime -= 1.0f;
                while (m_normalizedTime < 0.0f) m_normalizedTime += 1.0f;
            }
            else {
                if (m_normalizedTime >= 1.0f) m_normalizedTime = 1.0f;
                if (m_normalizedTime < 0.0f) m_normalizedTime = 0.0f;
            }
        }
        m_animationAdvancedThisFrame = m_normalizedTime != previousNormalizedTime;

        // Sample each animation at the layer phase
        for (uint32_t animationIndex = 0; animationIndex < m_animationPlaybacks.size(); animationIndex++) {
            if (m_animationWeights[animationIndex] <= 0.0f) continue;
            m_animationPlaybacks[animationIndex].SetNormalizedTime(m_normalizedTime, skeleton);
        }

        // A missing morph channel represents the basis value of zero, so the
        // denominator includes every active animation in the layer.
        for (uint32_t animationIndex = 0; animationIndex < m_animationPlaybacks.size(); animationIndex++) {
            const float weight = m_animationWeights[animationIndex];
            if (weight <= 0.0f) continue;

            for (const auto& [targetName, targetWeight] : m_animationPlaybacks[animationIndex].GetMorphTargetWeights()) {
                m_morphTargetWeights[targetName] += targetWeight * weight;
            }
        }
        for (auto& [targetName, targetWeight] : m_morphTargetWeights) {
            targetWeight /= totalAnimationWeight;
        }

        // Normalize only the animations that provide each transform component
        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            Hell::QuatTransform& localTransform = m_localPose[nodeIndex];
            glm::vec3 translation(0.0f);
            glm::vec3 scale(0.0f);
            float translationWeight = 0.0f;
            float rotationWeight = 0.0f;
            float scaleWeight = 0.0f;

            for (uint32_t animationIndex = 0; animationIndex < m_animationPlaybacks.size(); animationIndex++) {
                const float weight = m_animationWeights[animationIndex];
                if (weight <= 0.0f) continue;

                const AnimationPlayback& animationPlayback = m_animationPlaybacks[animationIndex];
                const AnimationChannel* animationChannel = animationPlayback.GetAnimationChannel(nodeIndex);
                if (!animationChannel) continue;

                const Hell::QuatTransform& animationTransform = animationPlayback.GetLocalPose()[nodeIndex];
                if (!animationChannel->translationKeys.empty()) {
                    translation += animationTransform.translation * weight;
                    translationWeight += weight;
                }
                if (!animationChannel->scaleKeys.empty()) {
                    scale += animationTransform.scale * weight;
                    scaleWeight += weight;
                }
                if (!animationChannel->rotationKeys.empty()) {
                    if (rotationWeight == 0.0f) {
                        localTransform.rotation = animationTransform.rotation;
                        rotationWeight = weight;
                    }
                    else {
                        glm::quat animationRotation = animationTransform.rotation;
                        if (glm::dot(localTransform.rotation, animationRotation) < 0.0f) animationRotation = -animationRotation;
                        localTransform.rotation = glm::normalize(glm::slerp(localTransform.rotation, animationRotation, weight / (rotationWeight + weight)));
                        rotationWeight += weight;
                    }
                }
            }

            if (translationWeight > 0.0f) {
                localTransform.translation = translation / translationWeight;
                m_animatedTranslations[nodeIndex] = 1;
            }
            if (rotationWeight > 0.0f) m_animatedRotations[nodeIndex] = 1;
            if (scaleWeight > 0.0f) {
                localTransform.scale = scale / scaleWeight;
                m_animatedScales[nodeIndex] = 1;
            }
        }

        if (calculateGlobalPose) CalculateGlobalPose(skeleton);
    }

    float AnimationLayer::GetWeight() const {
        return m_weight;
    }

    float AnimationLayer::GetNodeWeight(uint32_t nodeIndex) const {
        return nodeIndex < m_nodeWeights.size() ? m_nodeWeights[nodeIndex] : 0.0f;
    }

    bool AnimationLayer::UsesGlobalRotationBlend() const {
        return m_globalRotationBlend;
    }

    bool AnimationLayer::HasTranslation(uint32_t nodeIndex) const {
        return nodeIndex < m_animatedTranslations.size() && m_animatedTranslations[nodeIndex];
    }

    bool AnimationLayer::HasRotation(uint32_t nodeIndex) const {
        return nodeIndex < m_animatedRotations.size() && m_animatedRotations[nodeIndex];
    }

    bool AnimationLayer::HasScale(uint32_t nodeIndex) const {
        return nodeIndex < m_animatedScales.size() && m_animatedScales[nodeIndex];
    }

    uint32_t AnimationLayer::GetAnimationFrameNumber() const {
        if (m_animationPlaybacks.empty()) return 0;
        return m_animationPlaybacks.front().GetAnimationFrameNumber();
    }

    bool AnimationLayer::IsAnimationComplete() const {
        if (m_animationPlaybacks.empty()) return false;
        return !m_looping && m_normalizedTime >= 1.0f;
    }

    bool AnimationLayer::AnimationAdvancedThisFrame() const {
        return m_animationAdvancedThisFrame;
    }

    const std::vector<Hell::QuatTransform>& AnimationLayer::GetLocalPose() const {
        return m_localPose;
    }

    const std::vector<glm::mat4>& AnimationLayer::GetGlobalPose() const {
        return m_globalPose;
    }

    const std::vector<glm::quat>& AnimationLayer::GetGlobalRotations() const {
        return m_globalRotations;
    }

    const std::map<std::string, float>& AnimationLayer::GetMorphTargetWeights() const {
        return m_morphTargetWeights;
    }

    const std::vector<std::string>& AnimationLayer::GetBoneMaskNames() const {
        return m_boneMaskNames;
    }

    const std::map<uint32_t, std::string>& AnimationLayer::GetSkinnedModelWeightSources() const {
        return m_skinnedModelWeightSourceNodeNames;
    }

    void AnimationLayer::CalculateGlobalPose(const Skeleton& skeleton) {
        const std::vector<SkeletonNode>& nodes = skeleton.GetNodes();

        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            const glm::mat4 localTransform = m_localPose[nodeIndex].ToMat4();
            const int32_t parentIndex = nodes[nodeIndex].parentIndex;
            if (parentIndex == -1) {
                m_globalPose[nodeIndex] = localTransform;
                m_globalRotations[nodeIndex] = m_localPose[nodeIndex].rotation;
            }
            else {
                m_globalPose[nodeIndex] = m_globalPose[parentIndex] * localTransform;
                m_globalRotations[nodeIndex] = glm::normalize(m_globalRotations[parentIndex] * m_localPose[nodeIndex].rotation);
            }
        }
    }
}
