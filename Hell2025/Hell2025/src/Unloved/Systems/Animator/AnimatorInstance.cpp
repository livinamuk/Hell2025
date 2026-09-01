#include "AnimatorInstance.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Systems/Animator/SkeletonCache.h"

#include <glm/gtc/quaternion.hpp>

namespace Unloved {

    void AnimatorInstance::RegisterSkinnedModels(const std::vector<std::string>& skinnedModelNames) {
        const Skeleton& newSkeleton = SkeletonCache::GetOrCreateSkeleton(skinnedModelNames);
        if (newSkeleton.GetSkinnedModelIds().empty()) return;

        const std::map<uint32_t, std::string> motionSourceNodeNames = m_skinnedModelMotionSourceNodeNames;
        m_skeleton = newSkeleton;
        m_skinnedModelBoneSkeletonNodeIndices.clear();
        m_skinnedModelMotionSourceNodeNames.clear();
        m_boneSkinningMatrices.clear();
        m_morphTargetWeights.clear();

        const std::vector<SkeletonNode>& nodes = m_skeleton.GetNodes();
        m_localPose.resize(nodes.size());
        m_globalPose.resize(nodes.size());
        m_globalRotations.resize(nodes.size());
        m_nodeTranslationOffsets.assign(nodes.size(), glm::vec3(0.0f));
        m_nodeRotationOffsets.assign(nodes.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
        m_motionSourceNodeIndicesByTargetNodeIndex.assign(nodes.size(), -1);
        m_motionSourceBindPosesByTargetNodeIndex.resize(nodes.size());

        // Preserve every animation layer across the new skeleton
        for (AnimationLayer& animationLayer : m_animationLayers) {
            animationLayer.RebindSkeleton(m_skeleton);
        }

        BuildSkinnedModelMappings();

        // Restore motion sources belonging to models still registered
        for (const auto& motionSourceEntry : motionSourceNodeNames) {
            if (!m_skeleton.ContainsSkinnedModel(motionSourceEntry.first)) continue;

            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(motionSourceEntry.first);
            if (!skinnedModel) return;
            SetSkinnedModelMotionSource(skinnedModel->GetName(), motionSourceEntry.second);
        }

        // Recalculate masks against the new skeleton
        for (uint32_t layerIndex = 0; layerIndex < m_animationLayers.size(); layerIndex++) {
            const std::vector<std::string> boneMaskNames = m_animationLayers[layerIndex].GetBoneMaskNames();
            if (!boneMaskNames.empty()) SetAnimationLayerBoneMasks(layerIndex, boneMaskNames);
        }

        Update(0.0f);
        m_poseChangePending = true;
    }

    void AnimatorInstance::ReplaceSkinnedModel(const std::string& oldSkinnedModelName, const std::string& newSkinnedModelName) {
        SkinnedModel* oldSkinnedModel = Hell::ResourceManager::GetSkinnedModelByName(oldSkinnedModelName);
        SkinnedModel* newSkinnedModel = Hell::ResourceManager::GetSkinnedModelByName(newSkinnedModelName);
        if (!oldSkinnedModel) {
            Logging::Error() << "AnimatorInstance::ReplaceSkinnedModel(..) failed to find skinned model '" << oldSkinnedModelName << "'\n";
            return;
        }
        if (!newSkinnedModel) {
            Logging::Error() << "AnimatorInstance::ReplaceSkinnedModel(..) failed to find skinned model '" << newSkinnedModelName << "'\n";
            return;
        }
        if (oldSkinnedModel == newSkinnedModel) return;

        const uint32_t oldSkinnedModelId = oldSkinnedModel->GetSkinnedModelId();
        const uint32_t newSkinnedModelId = newSkinnedModel->GetSkinnedModelId();
        bool oldSkinnedModelFound = false;
        std::vector<std::string> skinnedModelNames;
        skinnedModelNames.reserve(m_skeleton.GetSkinnedModelIds().size());

        for (uint32_t skinnedModelId : m_skeleton.GetSkinnedModelIds()) {
            if (skinnedModelId == newSkinnedModelId) {
                Logging::Error() << "AnimatorInstance::ReplaceSkinnedModel(..) received an already registered skinned model '" << newSkinnedModelName << "'\n";
                return;
            }

            if (skinnedModelId == oldSkinnedModelId) {
                skinnedModelNames.push_back(newSkinnedModelName);
                oldSkinnedModelFound = true;
                continue;
            }

            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModel) return;
            skinnedModelNames.push_back(skinnedModel->GetName());
        }

        if (!oldSkinnedModelFound) {
            Logging::Error() << "AnimatorInstance::ReplaceSkinnedModel(..) received unregistered skinned model '" << oldSkinnedModelName << "'\n";
            return;
        }

        const Skeleton& newSkeleton = SkeletonCache::GetOrCreateSkeleton(skinnedModelNames);
        if (newSkeleton.GetSkinnedModelIds().empty()) return;

        for (AnimationLayer& animationLayer : m_animationLayers) {
            animationLayer.ReplaceSkinnedModelWeightSource(oldSkinnedModelId, newSkinnedModelId);
        }

        auto motionSourceIt = m_skinnedModelMotionSourceNodeNames.find(oldSkinnedModelId);
        if (motionSourceIt != m_skinnedModelMotionSourceNodeNames.end()) {
            const std::string nodeName = motionSourceIt->second;
            m_skinnedModelMotionSourceNodeNames.erase(motionSourceIt);
            m_skinnedModelMotionSourceNodeNames[newSkinnedModelId] = nodeName;
        }

        RegisterSkinnedModels(skinnedModelNames);
    }

    void AnimatorInstance::BuildSkinnedModelMappings() {
        // Map each model bone to its node in the combined skeleton
        for (uint32_t skinnedModelId : m_skeleton.GetSkinnedModelIds()) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModel) {
                Logging::Error() << "AnimatorInstance::BuildSkinnedModelMappings() failed to find skinned model " << skinnedModelId << "\n";
                return;
            }

            std::vector<uint32_t>& skeletonNodeIndices = m_skinnedModelBoneSkeletonNodeIndices[skinnedModelId];
            skeletonNodeIndices.resize(skinnedModel->GetBoneCount());
            m_boneSkinningMatrices[skinnedModelId].resize(skinnedModel->GetBoneCount());

            for (const auto& boneEntry : skinnedModel->m_boneMapping) {
                const std::string& boneName = boneEntry.first;
                const uint32_t modelBoneIndex = boneEntry.second;
                const int32_t skeletonNodeIndex = m_skeleton.GetNodeIndex(boneName);
                if (skeletonNodeIndex == -1) {
                    Logging::Error() << "AnimatorInstance::BuildSkinnedModelMappings() failed to find bone '" << boneName << "' in the combined skeleton\n";
                    return;
                }
                skeletonNodeIndices[modelBoneIndex] = static_cast<uint32_t>(skeletonNodeIndex);
            }
        }
    }

    void AnimatorInstance::SetAnimationLayerSkinnedModelWeightSource(uint32_t layerIndex, const std::string& skinnedModelName, const std::string& nodeName) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerSkinnedModelWeightSource(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(skinnedModelName);
        if (!skinnedModel) return;

        if (!m_skeleton.ContainsSkinnedModel(skinnedModel->GetSkinnedModelId())) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerSkinnedModelWeightSource(..) received unregistered skinned model '" << skinnedModelName << "'\n";
            return;
        }

        if (m_skeleton.GetNodeIndex(nodeName) == -1) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerSkinnedModelWeightSource(..) failed to find node '" << nodeName << "'\n";
            return;
        }

        AnimationLayer& animationLayer = m_animationLayers[layerIndex];
        animationLayer.SetSkinnedModelWeightSource(skinnedModel->GetSkinnedModelId(), nodeName);
        const std::vector<std::string> boneMaskNames = animationLayer.GetBoneMaskNames();
        if (!boneMaskNames.empty()) SetAnimationLayerBoneMasks(layerIndex, boneMaskNames);
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetSkinnedModelMotionSource(const std::string& skinnedModelName, const std::string& nodeName) {
        SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(skinnedModelName);
        if (!skinnedModel) return;

        if (!m_skeleton.ContainsSkinnedModel(skinnedModel->GetSkinnedModelId())) {
            Logging::Error() << "AnimatorInstance::SetSkinnedModelMotionSource(..) received unregistered skinned model '" << skinnedModelName << "'\n";
            return;
        }

        const std::vector<SkeletonNode>& nodes = m_skeleton.GetNodes();
        const int32_t sourceNodeIndexResult = m_skeleton.GetNodeIndex(nodeName);
        if (sourceNodeIndexResult == -1) {
            Logging::Error() << "AnimatorInstance::SetSkinnedModelMotionSource(..) failed to find node '" << nodeName << "'\n";
            return;
        }
        const uint32_t sourceNodeIndex = static_cast<uint32_t>(sourceNodeIndexResult);

        std::vector<glm::mat4> globalBindPose(nodes.size());
        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            const glm::mat4 localBindPose = nodes[nodeIndex].localBindPose.ToMat4();
            if (nodes[nodeIndex].parentIndex == -1) globalBindPose[nodeIndex] = localBindPose;
            else globalBindPose[nodeIndex] = globalBindPose[nodes[nodeIndex].parentIndex] * localBindPose;
        }

        bool branchRootFound = false;
        for (const Node& modelNode : skinnedModel->m_nodes) {
            bool nodeShared = false;
            bool parentShared = modelNode.parentIndex == -1;

            for (uint32_t otherSkinnedModelId : m_skeleton.GetSkinnedModelIds()) {
                if (otherSkinnedModelId == skinnedModel->GetSkinnedModelId()) continue;
                SkinnedModel* otherSkinnedModel = Hell::ResourceManager::GetSkinnedModelById(otherSkinnedModelId);
                if (!otherSkinnedModel) return;
                if (otherSkinnedModel->m_nodeMapping.count(modelNode.name)) nodeShared = true;
                if (modelNode.parentIndex >= 0 && otherSkinnedModel->m_nodeMapping.count(skinnedModel->m_nodes[modelNode.parentIndex].name)) parentShared = true;
            }

            if (nodeShared || !parentShared) continue;

            const int32_t targetNodeIndexResult = m_skeleton.GetNodeIndex(modelNode.name);
            if (targetNodeIndexResult == -1) {
                Logging::Error() << "AnimatorInstance::SetSkinnedModelMotionSource(..) failed to find branch root '" << modelNode.name << "'\n";
                return;
            }
            const uint32_t targetNodeIndex = static_cast<uint32_t>(targetNodeIndexResult);
            if (sourceNodeIndex >= targetNodeIndex) {
                Logging::Error() << "AnimatorInstance::SetSkinnedModelMotionSource(..) requires the source node to precede branch root '" << modelNode.name << "'\n";
                return;
            }

            m_motionSourceNodeIndicesByTargetNodeIndex[targetNodeIndex] = static_cast<int32_t>(sourceNodeIndex);
            m_motionSourceBindPosesByTargetNodeIndex[targetNodeIndex] = Hell::QuatTransform(glm::inverse(globalBindPose[sourceNodeIndex]) * globalBindPose[targetNodeIndex]);
            branchRootFound = true;
        }

        if (!branchRootFound) {
            Logging::Error() << "AnimatorInstance::SetSkinnedModelMotionSource(..) failed to find a unique branch for skinned model '" << skinnedModelName << "'\n";
            return;
        }
        m_skinnedModelMotionSourceNodeNames[skinnedModel->GetSkinnedModelId()] = nodeName;
        m_poseChangePending = true;
    }

    uint32_t AnimatorInstance::CreateAnimationLayer() {
        AnimationLayer& animationLayer = m_animationLayers.emplace_back();
        animationLayer.RebindSkeleton(m_skeleton);
        m_poseChangePending = true;
        return static_cast<uint32_t>(m_animationLayers.size() - 1);
    }

    uint32_t AnimatorInstance::AddBlendAnimation(uint32_t layerIndex, const std::string& animationName, float speed) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::AddBlendAnimation(..) received invalid animation layer index " << layerIndex << "\n";
            return UINT32_MAX;
        }

        const Animation* animation = Hell::ResourceManager::GetAnimationPtr(animationName);
        if (!animation) return UINT32_MAX;
        const uint32_t animationIndex = m_animationLayers[layerIndex].AddBlendAnimation(*animation, m_skeleton, speed);
        m_poseChangePending = true;
        return animationIndex;
    }

    void AnimatorInstance::SetAnimationWeight(uint32_t layerIndex, uint32_t animationIndex, float weight) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetAnimationWeight(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }
        m_animationLayers[layerIndex].SetAnimationWeight(animationIndex, weight);
        m_poseChangePending = true;
    }

    void AnimatorInstance::PlayAnimation(uint32_t layerIndex, const std::string& animationName, float speed) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::PlayAnimation(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        const Animation* animation = Hell::ResourceManager::GetAnimationPtr(animationName);
        if (!animation) return;

        m_animationLayers[layerIndex].PlayAnimation(*animation, m_skeleton, speed);
        m_poseChangePending = true;
    }

    void AnimatorInstance::PlayAndLoopAnimation(uint32_t layerIndex, const std::string& animationName, float speed) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::PlayAndLoopAnimation(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        const Animation* animation = Hell::ResourceManager::GetAnimationPtr(animationName);
        if (!animation) return;

        m_animationLayers[layerIndex].PlayAndLoopAnimation(*animation, m_skeleton, speed);
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetAnimationLayerWeight(uint32_t layerIndex, float weight) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerWeight(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        m_animationLayers[layerIndex].SetWeight(weight);
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetMorphSourceLayer(uint32_t layerIndex) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetMorphSourceLayer(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        m_morphSourceLayerIndex = layerIndex;
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetAnimationLayerBoneMasks(uint32_t layerIndex, const std::vector<std::string>& boneMaskNames) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }
        if (boneMaskNames.empty()) {
            ClearLayerMask(layerIndex);
            return;
        }

        AnimationLayer& animationLayer = m_animationLayers[layerIndex];
        const std::vector<SkeletonNode>& nodes = m_skeleton.GetNodes();
        std::vector<float> nodeWeights(nodes.size(), 0.0f);

        for (const std::string& boneMaskName : boneMaskNames) {
            BoneMask* boneMask = Hell::ResourceManager::GetBoneMaskPtr(boneMaskName);
            if (!boneMask) return;

            SkinnedModel* boneMaskSkinnedModel = Hell::ResourceManager::GetSkinnedModelByName(boneMask->skinnedModelName);
            if (!boneMaskSkinnedModel) {
                Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) mask '" << boneMaskName << "' was authored for missing skinned model '" << boneMask->skinnedModelName << "'\n";
                return;
            }

            if (!m_skeleton.ContainsSkinnedModel(boneMaskSkinnedModel->GetSkinnedModelId())) {
                Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) mask '" << boneMaskName << "' was authored for unregistered skinned model '" << boneMask->skinnedModelName << "'\n";
                return;
            }

            // Keep the strongest weight when masks overlap
            for (const auto& weightEntry : boneMask->weights) {
                const int32_t nodeIndex = m_skeleton.GetNodeIndex(weightEntry.first);
                if (nodeIndex == -1) {
                    Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) failed to find bone '" << weightEntry.first << "' from mask '" << boneMask->name << "'\n";
                    return;
                }
                if (weightEntry.second > nodeWeights[nodeIndex]) nodeWeights[nodeIndex] = weightEntry.second;
            }
        }

        // Propagate source weights through linked model branches
        for (const auto& weightSourceEntry : animationLayer.GetSkinnedModelWeightSources()) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(weightSourceEntry.first);
            if (!skinnedModel) return;
            const int32_t sourceNodeIndex = m_skeleton.GetNodeIndex(weightSourceEntry.second);
            if (sourceNodeIndex == -1) {
                Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) failed to find weight source node '" << weightSourceEntry.second << "'\n";
                return;
            }
            const float weight = nodeWeights[sourceNodeIndex];

            for (const Node& modelNode : skinnedModel->m_nodes) {
                bool sharedNode = false;
                for (uint32_t otherSkinnedModelId : m_skeleton.GetSkinnedModelIds()) {
                    if (otherSkinnedModelId == weightSourceEntry.first) continue;
                    SkinnedModel* otherSkinnedModel = Hell::ResourceManager::GetSkinnedModelById(otherSkinnedModelId);
                    if (!otherSkinnedModel) return;
                    if (otherSkinnedModel->m_nodeMapping.count(modelNode.name)) sharedNode = true;
                    if (sharedNode) break;
                }
                if (sharedNode) continue;

                const int32_t nodeIndex = m_skeleton.GetNodeIndex(modelNode.name);
                if (nodeIndex == -1) {
                    Logging::Error() << "AnimatorInstance::SetAnimationLayerBoneMasks(..) failed to find linked node '" << modelNode.name << "'\n";
                    return;
                }
                nodeWeights[nodeIndex] = weight;
            }
        }

        animationLayer.SetBoneMasks(boneMaskNames, nodeWeights);
        m_poseChangePending = true;
    }

    void AnimatorInstance::ClearLayerMask(uint32_t layerIndex) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::ClearLayerMask(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        m_animationLayers[layerIndex].ClearBoneMasks();
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetAnimationLayerGlobalRotationBlend(uint32_t layerIndex, bool globalRotationBlend) {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::SetAnimationLayerGlobalRotationBlend(..) received invalid animation layer index " << layerIndex << "\n";
            return;
        }

        m_animationLayers[layerIndex].SetGlobalRotationBlend(globalRotationBlend);
        m_poseChangePending = true;
    }

    void AnimatorInstance::SetNodeTranslationOffset(const std::string& nodeName, const glm::vec3& translation) {
        const int32_t nodeIndex = m_skeleton.GetNodeIndex(nodeName);
        if (nodeIndex == -1) {
            Logging::Error() << "AnimatorInstance::SetNodeTranslationOffset(..) failed to find node '" << nodeName << "'\n";
            return;
        }

        m_nodeTranslationOffsets[nodeIndex] = translation;
        m_nodeOffsetsPending = true;
    }

    void AnimatorInstance::SetNodeRotationOffset(const std::string& nodeName, const glm::quat& rotation) {
        const int32_t nodeIndex = m_skeleton.GetNodeIndex(nodeName);
        if (nodeIndex == -1) {
            Logging::Error() << "AnimatorInstance::SetNodeRotationOffset(..) failed to find node '" << nodeName << "'\n";
            return;
        }

        m_nodeRotationOffsets[nodeIndex] = glm::normalize(rotation);
        m_nodeOffsetsPending = true;
    }

    void AnimatorInstance::RestartAnimation() {
        for (AnimationLayer& animationLayer : m_animationLayers) {
            animationLayer.Restart();
        }
        Update(0.0f);
        m_poseChangePending = true;
    }

    void AnimatorInstance::Update(float deltaTime) {
        const std::vector<SkeletonNode>& nodes = m_skeleton.GetNodes();
        m_poseChangedThisFrame = m_poseChangePending || m_nodeOffsetsPending || m_nodeOffsetsAppliedLastFrame;
        m_poseChangePending = false;
        m_nodeOffsetsAppliedLastFrame = m_nodeOffsetsPending;
        m_nodeOffsetsPending = false;

        // Reset the final pose before blending layer output
        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            if (m_motionSourceNodeIndicesByTargetNodeIndex[nodeIndex] == -1) m_localPose[nodeIndex] = nodes[nodeIndex].localBindPose;
            else m_localPose[nodeIndex] = m_motionSourceBindPosesByTargetNodeIndex[nodeIndex];
        }

        bool calculateVisibleLayerGlobalPoses = !m_skinnedModelMotionSourceNodeNames.empty();
        if (!calculateVisibleLayerGlobalPoses) {
            for (const AnimationLayer& animationLayer : m_animationLayers) {
                if (animationLayer.GetWeight() <= 0.0f) continue;
                if (!animationLayer.UsesGlobalRotationBlend()) continue;
                calculateVisibleLayerGlobalPoses = true;
                break;
            }
        }

        for (AnimationLayer& animationLayer : m_animationLayers) {
            animationLayer.Update(deltaTime, m_skeleton, calculateVisibleLayerGlobalPoses && animationLayer.GetWeight() > 0.0f);
            if (animationLayer.GetWeight() > 0.0f && animationLayer.AnimationAdvancedThisFrame()) m_poseChangedThisFrame = true;
        }

        CalculateMorphTargetWeights();

        // Normalize layer contributions for each transform component
        for (uint32_t nodeIndex = 0; nodeIndex < nodes.size(); nodeIndex++) {
            Hell::QuatTransform& localTransform = m_localPose[nodeIndex];
            glm::vec3 translation(0.0f);
            glm::vec3 scale(0.0f);
            float translationWeight = 0.0f;
            float rotationWeight = 0.0f;
            float scaleWeight = 0.0f;
            bool globalRotationBlend = false;

            // Use one rotation space for every layer contributing to this node
            for (const AnimationLayer& animationLayer : m_animationLayers) {
                const float weight = animationLayer.GetWeight() * animationLayer.GetNodeWeight(nodeIndex);
                if (weight <= 0.0f) continue;
                if (!animationLayer.HasRotation(nodeIndex)) continue;
                if (animationLayer.UsesGlobalRotationBlend()) globalRotationBlend = true;
            }

            for (const AnimationLayer& animationLayer : m_animationLayers) {
                const float weight = animationLayer.GetWeight() * animationLayer.GetNodeWeight(nodeIndex);
                if (weight <= 0.0f) continue;

                Hell::QuatTransform layerTransform = animationLayer.GetLocalPose()[nodeIndex];
                const int32_t motionSourceNodeIndex = m_motionSourceNodeIndicesByTargetNodeIndex[nodeIndex];
                if (motionSourceNodeIndex != -1) {
                    const std::vector<glm::mat4>& layerGlobalPose = animationLayer.GetGlobalPose();
                    layerTransform = Hell::QuatTransform(glm::inverse(layerGlobalPose[motionSourceNodeIndex]) * layerGlobalPose[nodeIndex]);
                }
                if (animationLayer.HasTranslation(nodeIndex)) {
                    translation += layerTransform.translation * weight;
                    translationWeight += weight;
                }
                if (animationLayer.HasScale(nodeIndex)) {
                    scale += layerTransform.scale * weight;
                    scaleWeight += weight;
                }
                if (animationLayer.HasRotation(nodeIndex)) {
                    glm::quat layerRotation = layerTransform.rotation;
                    if (globalRotationBlend) layerRotation = animationLayer.GetGlobalRotations()[nodeIndex];
                    if (rotationWeight == 0.0f) {
                        localTransform.rotation = layerRotation;
                        rotationWeight = weight;
                    }
                    else {
                        if (glm::dot(localTransform.rotation, layerRotation) < 0.0f) layerRotation = -layerRotation;
                        localTransform.rotation = glm::normalize(glm::slerp(localTransform.rotation, layerRotation, weight / (rotationWeight + weight)));
                        rotationWeight += weight;
                    }
                }
            }

            if (translationWeight > 0.0f) localTransform.translation = translation / translationWeight;
            if (scaleWeight > 0.0f) localTransform.scale = scale / scaleWeight;

            int32_t parentIndex = nodes[nodeIndex].parentIndex;
            if (m_motionSourceNodeIndicesByTargetNodeIndex[nodeIndex] != -1) parentIndex = m_motionSourceNodeIndicesByTargetNodeIndex[nodeIndex];

            if (globalRotationBlend && rotationWeight > 0.0f && parentIndex != -1) {
                localTransform.rotation = glm::normalize(glm::inverse(m_globalRotations[parentIndex]) * localTransform.rotation);
            }

            if (parentIndex == -1) m_globalRotations[nodeIndex] = localTransform.rotation;
            else m_globalRotations[nodeIndex] = glm::normalize(m_globalRotations[parentIndex] * localTransform.rotation);
        }

        CalculateGlobalPose();
        CalculateBoneSkinningMatrices();
    }

    const Skeleton& AnimatorInstance::GetSkeleton() const {
        return m_skeleton;
    }

    uint32_t AnimatorInstance::GetAnimationFrameNumber(uint32_t layerIndex) const {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::GetAnimationFrameNumber(..) received invalid animation layer index " << layerIndex << "\n";
            return 0;
        }
        return m_animationLayers[layerIndex].GetAnimationFrameNumber();
    }

    bool AnimatorInstance::IsAnimationComplete(uint32_t layerIndex) const {
        if (layerIndex >= m_animationLayers.size()) {
            Logging::Error() << "AnimatorInstance::IsAnimationComplete(..) received invalid animation layer index " << layerIndex << "\n";
            return false;
        }
        return m_animationLayers[layerIndex].IsAnimationComplete();
    }

    bool AnimatorInstance::PoseChangedThisFrame() const {
        return m_poseChangedThisFrame || m_poseChangePending || m_nodeOffsetsPending;
    }

    const std::vector<Hell::QuatTransform>& AnimatorInstance::GetLocalPose() const {
        return m_localPose;
    }

    const std::vector<glm::mat4>& AnimatorInstance::GetGlobalPose() const {
        return m_globalPose;
    }

    const std::vector<glm::mat4>& AnimatorInstance::GetBoneSkinningMatrices(uint32_t skinnedModelId) const {
        auto matrixIt = m_boneSkinningMatrices.find(skinnedModelId);
        if (matrixIt != m_boneSkinningMatrices.end()) {
            return matrixIt->second;
        }

        Logging::Error() << "AnimatorInstance::GetBoneSkinningMatrices(..) failed to find skinned model " << skinnedModelId << "\n";
        static const std::vector<glm::mat4> emptyMatrices;
        return emptyMatrices;
    }

    const std::map<std::string, float>& AnimatorInstance::GetMorphTargetWeights(uint32_t skinnedModelId) const {
        const auto weightsIt = m_morphTargetWeights.find(skinnedModelId);
        if (weightsIt != m_morphTargetWeights.end()) return weightsIt->second;

        static const std::map<std::string, float> emptyWeights;
        return emptyWeights;
    }

    void AnimatorInstance::CalculateMorphTargetWeights() {
        m_morphTargetWeights.clear();
        if (m_morphSourceLayerIndex >= m_animationLayers.size()) return;

        const AnimationLayer& morphSourceLayer = m_animationLayers[m_morphSourceLayerIndex];
        if (morphSourceLayer.GetWeight() <= 0.0f) return;

        const std::map<std::string, float>& sourceWeights = morphSourceLayer.GetMorphTargetWeights();
        for (uint32_t skinnedModelId : m_skeleton.GetSkinnedModelIds()) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModel) continue;

            std::map<std::string, float>& modelWeights = m_morphTargetWeights[skinnedModelId];
            for (const auto& [targetName, targetValue] : sourceWeights) {
                if (!skinnedModel->MorphTargetExists(targetName)) continue;
                modelWeights[targetName] = targetValue;
            }
        }
    }

    void AnimatorInstance::CalculateGlobalPose() {
        const std::vector<SkeletonNode>& nodes = m_skeleton.GetNodes();

        // Apply procedural offsets then calculate each node relative to the skeleton root
        for (uint32_t i = 0; i < nodes.size(); i++) {
            m_localPose[i].translation += m_nodeTranslationOffsets[i];
            m_localPose[i].rotation = glm::normalize(m_nodeRotationOffsets[i] * m_localPose[i].rotation);
            m_nodeTranslationOffsets[i] = glm::vec3(0.0f);
            m_nodeRotationOffsets[i] = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

            const glm::mat4 localTransform = m_localPose[i].ToMat4();
            int32_t parentIndex = nodes[i].parentIndex;
            if (m_motionSourceNodeIndicesByTargetNodeIndex[i] != -1) parentIndex = m_motionSourceNodeIndicesByTargetNodeIndex[i];

            if (parentIndex == -1) m_globalPose[i] = localTransform;
            else m_globalPose[i] = m_globalPose[parentIndex] * localTransform;

        }
    }

    void AnimatorInstance::CalculateBoneSkinningMatrices() {
        // Build matrices in the bone index order expected by each skinned model
        for (uint32_t skinnedModelId : m_skeleton.GetSkinnedModelIds()) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModel) {
                Logging::Error() << "AnimatorInstance::CalculateBoneSkinningMatrices() failed to find skinned model " << skinnedModelId << "\n";
                return;
            }

            const std::vector<uint32_t>& skeletonNodeIndices = m_skinnedModelBoneSkeletonNodeIndices[skinnedModelId];
            std::vector<glm::mat4>& boneSkinningMatrices = m_boneSkinningMatrices[skinnedModelId];

            for (uint32_t boneIndex = 0; boneIndex < boneSkinningMatrices.size(); boneIndex++) {
                boneSkinningMatrices[boneIndex] = m_globalPose[skeletonNodeIndices[boneIndex]] * skinnedModel->m_boneOffsets[boneIndex];
            }
        }
    }
}
