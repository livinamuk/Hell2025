#pragma once

#include "Hell/Transform.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Unloved {

    struct SkeletonNode {
        std::string name;
        int32_t parentIndex = -1;
        Hell::QuatTransform localBindPose;
    };

    struct Skeleton {
        void Build(const std::vector<uint32_t>& skinnedModelIds);

        bool ContainsSkinnedModel(uint32_t skinnedModelId) const;
        int32_t GetNodeIndex(const std::string& nodeName) const;
        const std::vector<SkeletonNode>& GetNodes() const { return m_nodes; }
        const std::vector<uint32_t>& GetSkinnedModelIds() const { return m_skinnedModelIds; }

    private:
        std::vector<SkeletonNode> m_nodes;
        std::map<std::string, uint32_t> m_nodeMapping;
        std::vector<uint32_t> m_skinnedModelIds;
    };
}
