#pragma once

#include "Unloved/Systems/Animator/AnimationLayer.h"
#include "Unloved/Systems/Animator/Skeleton.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Unloved {

    struct AnimatorInstance {
        void RegisterSkinnedModels(const std::vector<std::string>& skinnedModelNames);
        void ReplaceSkinnedModel(const std::string& oldSkinnedModelName, const std::string& newSkinnedModelName);
        void SetAnimationLayerSkinnedModelWeightSource(uint32_t layerIndex, const std::string& skinnedModelName, const std::string& nodeName);
        void SetSkinnedModelMotionSource(const std::string& skinnedModelName, const std::string& nodeName);
        void SetAnimationWeight(uint32_t layerIndex, uint32_t animationIndex, float weight);
        void PlayAnimation(uint32_t layerIndex, const std::string& animationName, float speed);
        void PlayAndLoopAnimation(uint32_t layerIndex, const std::string& animationName, float speed);
        void SetAnimationLayerWeight(uint32_t layerIndex, float weight);
        void SetMorphSourceLayer(uint32_t layerIndex);
        void SetAnimationLayerBoneMasks(uint32_t layerIndex, const std::vector<std::string>& boneMaskNames);
        void ClearLayerMask(uint32_t layerIndex);
        void SetAnimationLayerGlobalRotationBlend(uint32_t layerIndex, bool globalRotationBlend);
        void SetNodeTranslationOffset(const std::string& nodeName, const glm::vec3& translation);
        void SetNodeRotationOffset(const std::string& nodeName, const glm::quat& rotation);
        void RestartAnimation();
        void Update(float deltaTime);

        uint32_t CreateAnimationLayer();
        uint32_t AddBlendAnimation(uint32_t layerIndex, const std::string& animationName, float speed);

        const Skeleton& GetSkeleton() const;
        uint32_t GetAnimationFrameNumber(uint32_t layerIndex) const;
        bool IsAnimationComplete(uint32_t layerIndex) const;
        bool PoseChangedThisFrame() const;
        const std::vector<Hell::QuatTransform>& GetLocalPose() const;
        const std::vector<glm::mat4>& GetGlobalPose() const;
        const std::vector<glm::mat4>& GetBoneSkinningMatrices(uint32_t skinnedModelId) const;
        const std::map<std::string, float>& GetMorphTargetWeights(uint32_t skinnedModelId) const;

    private:
        void BuildSkinnedModelMappings();
        void CalculateMorphTargetWeights();
        void CalculateGlobalPose();
        void CalculateBoneSkinningMatrices();

        Skeleton m_skeleton;
        std::vector<Hell::QuatTransform> m_localPose;
        std::vector<glm::mat4> m_globalPose;
        std::vector<glm::quat> m_globalRotations;
        std::map<uint32_t, std::vector<uint32_t>> m_skinnedModelBoneSkeletonNodeIndices;
        std::map<uint32_t, std::string> m_skinnedModelMotionSourceNodeNames;
        std::vector<int32_t> m_motionSourceNodeIndicesByTargetNodeIndex;
        std::vector<Hell::QuatTransform> m_motionSourceBindPosesByTargetNodeIndex;
        std::map<uint32_t, std::vector<glm::mat4>> m_boneSkinningMatrices;
        std::map<uint32_t, std::map<std::string, float>> m_morphTargetWeights;
        std::vector<AnimationLayer> m_animationLayers;
        uint32_t m_morphSourceLayerIndex = UINT32_MAX;
        std::vector<glm::vec3> m_nodeTranslationOffsets;
        std::vector<glm::quat> m_nodeRotationOffsets;
        bool m_poseChangePending = false;
        bool m_poseChangedThisFrame = false;
        bool m_nodeOffsetsPending = false;
        bool m_nodeOffsetsAppliedLastFrame = false;
    };
}
