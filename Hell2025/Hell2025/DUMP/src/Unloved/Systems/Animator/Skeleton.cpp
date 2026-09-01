#include "Skeleton.h"

#include "Hell/Logging.h"
#include "Hell/Math/Math.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include <glm/mat4x4.hpp>

namespace Unloved {

    void Skeleton::Build(const std::vector<uint32_t>& skinnedModelIds) {
        m_nodes.clear();
        m_nodeMapping.clear();
        m_skinnedModelIds.clear();
        std::map<std::string, glm::mat4> boneOffsetsByName;

        // Merge each skinned model into the skeleton
        for (uint32_t skinnedModelId : skinnedModelIds) {
            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModel) {
                Logging::Error() << "Skeleton::Build(..) failed to find skinned model " << skinnedModelId << "\n";
                return;
            }

            std::vector<uint32_t> skeletonNodeIndices;
            skeletonNodeIndices.resize(skinnedModel->m_nodes.size());

            // Merge model nodes and build the node index remap
            for (uint32_t modelNodeIndex = 0; modelNodeIndex < skinnedModel->m_nodes.size(); modelNodeIndex++) {
                const Node& modelNode = skinnedModel->m_nodes[modelNodeIndex];
                const Hell::QuatTransform modelBindPose(modelNode.localBindTransform);
                int32_t skeletonParentIndex = -1;
                if (modelNode.parentIndex >= 0) {
                    skeletonParentIndex = static_cast<int32_t>(skeletonNodeIndices[modelNode.parentIndex]);
                }

                int32_t skeletonNodeIndex = GetNodeIndex(modelNode.name);

                if (skeletonNodeIndex == -1) {
                    skeletonNodeIndex = static_cast<int32_t>(m_nodes.size());
                    SkeletonNode& skeletonNode = m_nodes.emplace_back();
                    skeletonNode.name = modelNode.name;
                    skeletonNode.parentIndex = skeletonParentIndex;
                    skeletonNode.localBindPose = modelBindPose;
                    m_nodeMapping[skeletonNode.name] = static_cast<uint32_t>(skeletonNodeIndex);
                }
                else {
                    const SkeletonNode& skeletonNode = m_nodes[skeletonNodeIndex];
                    if (skeletonNode.parentIndex != skeletonParentIndex) {
                        Logging::Error() << "Skeleton::Build(..) rejected skinned model '" << skinnedModel->GetName() << "' because node '" << modelNode.name << "' has a different parent\n";
                        return;
                    }
                    if (!Hell::Math::NearlyEqual(skeletonNode.localBindPose.ToMat4(), modelBindPose.ToMat4())) {
                        Logging::Error() << "Skeleton::Build(..) rejected skinned model '" << skinnedModel->GetName() << "' because node '" << modelNode.name << "' has a different bind transform\n";
                        return;
                    }
                }

                skeletonNodeIndices[modelNodeIndex] = static_cast<uint32_t>(skeletonNodeIndex);
            }

            // Validate inverse bind matrices shared by the models
            for (const auto& boneEntry : skinnedModel->m_boneMapping) {
                const std::string& boneName = boneEntry.first;
                const uint32_t modelBoneIndex = boneEntry.second;

                if (!boneOffsetsByName.count(boneName)) {
                    boneOffsetsByName[boneName] = skinnedModel->m_boneOffsets[modelBoneIndex];
                }
                else if (!Hell::Math::NearlyEqual(boneOffsetsByName[boneName], skinnedModel->m_boneOffsets[modelBoneIndex])) {
                    Logging::Error() << "Skeleton::Build(..) rejected skinned model '" << skinnedModel->GetName() << "' because bone '" << boneName << "' has a different inverse bind matrix\n";
                    return;
                }
            }
        }

        m_skinnedModelIds = skinnedModelIds;
    }

    bool Skeleton::ContainsSkinnedModel(uint32_t skinnedModelId) const {
        for (uint32_t registeredSkinnedModelId : m_skinnedModelIds) {
            if (registeredSkinnedModelId == skinnedModelId) return true;
        }
        return false;
    }

    int32_t Skeleton::GetNodeIndex(const std::string& nodeName) const {
        auto nodeIt = m_nodeMapping.find(nodeName);
        if (nodeIt == m_nodeMapping.end()) return -1;
        return static_cast<int32_t>(nodeIt->second);
    }
}
