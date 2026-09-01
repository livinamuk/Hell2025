#include "CharacterSpine.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Common/Types.h"

namespace Unloved {

    bool CharacterSpine::Init(const SkinnedModel& skinnedModel, const std::vector<std::string>& boneNames, uint32_t anchorIndex, float scale) {
        m_boneNames.clear();
        m_positions.clear();
        m_segmentLengths.clear();
        m_anchorIndex = 0;

        if (boneNames.size() < 2) {
            Logging::Error() << "CharacterSpine::Init() requires at least two bones\n";
            return false;
        }
        if (anchorIndex >= boneNames.size()) {
            Logging::Error() << "CharacterSpine::Init() anchor index is out of range\n";
            return false;
        }

        std::vector<glm::mat4> globalBindTransforms(skinnedModel.m_nodes.size());
        for (uint32_t i = 0; i < skinnedModel.m_nodes.size(); i++) {
            const Node& node = skinnedModel.m_nodes[i];
            glm::mat4 parentTransform = node.parentIndex == -1 ? glm::mat4(1.0f) : globalBindTransforms[node.parentIndex];
            globalBindTransforms[i] = AnimatedTransform(parentTransform * node.localBindTransform).to_mat4();
        }

        m_boneNames = boneNames;
        m_positions.resize(boneNames.size());
        m_segmentLengths.resize(boneNames.size() - 1);
        m_anchorIndex = anchorIndex;

        for (uint32_t i = 0; i < boneNames.size(); i++) {
            auto nodeIt = skinnedModel.m_nodeMapping.find(boneNames[i]);
            if (nodeIt == skinnedModel.m_nodeMapping.end()) {
                Logging::Error() << "CharacterSpine::Init() could not find bone '" << boneNames[i] << "'\n";
                m_boneNames.clear();
                m_positions.clear();
                m_segmentLengths.clear();
                m_anchorIndex = 0;
                return false;
            }

            m_positions[i] = glm::vec3(globalBindTransforms[nodeIt->second][3]) * scale;
            m_positions[i].y = 0.0f;
        }

        for (uint32_t i = 0; i < m_segmentLengths.size(); i++) {
            m_segmentLengths[i] = glm::distance(m_positions[i], m_positions[i + 1]);
        }

        return true;
    }

    void CharacterSpine::Reset(const glm::vec3& leadPosition, const glm::vec3& forward) {
        if (m_positions.empty()) return;

        glm::vec3 planarForward = forward;
        planarForward.y = 0.0f;
        if (glm::length(planarForward) == 0.0f) {
            planarForward = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        else {
            planarForward = glm::normalize(planarForward);
        }

        m_positions[0] = leadPosition;
        for (uint32_t i = 1; i < m_positions.size(); i++) {
            m_positions[i] = m_positions[i - 1] - planarForward * m_segmentLengths[i - 1];
        }
    }

    void CharacterSpine::MoveLead(const glm::vec3& displacement) {
        if (m_positions.empty()) return;

        m_positions[0] += displacement;
        for (uint32_t i = 1; i < m_positions.size(); i++) {
            glm::vec3 direction = glm::normalize(m_positions[i - 1] - m_positions[i]);
            m_positions[i] = m_positions[i - 1] - direction * m_segmentLengths[i - 1];
        }
    }

    void CharacterSpine::Straighten(const glm::vec3& displacement) {
        if (m_positions.empty()) return;

        const glm::vec3 originalLeadPosition = m_positions[0];
        m_positions[0] += displacement;

        for (uint32_t i = 1; i < m_positions.size(); i++) {
            glm::vec3 direction = m_positions[i - 1] - m_positions[i];
            float currentDistance = glm::length(direction);
            if (currentDistance > m_segmentLengths[i - 1]) {
                glm::vec3 correction = glm::normalize(direction) * (currentDistance - m_segmentLengths[i - 1]);
                m_positions[i] += correction;
            }
        }

        const glm::vec3 correction = m_positions[0] - originalLeadPosition;
        for (glm::vec3& position : m_positions) {
            position -= correction;
        }
    }

    const glm::vec3& CharacterSpine::GetPosition(uint32_t index) const {
        static const glm::vec3 invalid = glm::vec3(0.0f);
        return index < m_positions.size() ? m_positions[index] : invalid;
    }

    const glm::vec3& CharacterSpine::GetLeadPosition() const {
        return GetPosition(0);
    }

    const glm::vec3& CharacterSpine::GetAnchorPosition() const {
        return GetPosition(m_anchorIndex);
    }

    const std::string& CharacterSpine::GetBoneName(uint32_t index) const {
        static const std::string invalid;
        return index < m_boneNames.size() ? m_boneNames[index] : invalid;
    }

    uint32_t CharacterSpine::GetSegmentCount() const {
        return static_cast<uint32_t>(m_positions.size());
    }

    uint32_t CharacterSpine::GetAnchorIndex() const {
        return m_anchorIndex;
    }
}
