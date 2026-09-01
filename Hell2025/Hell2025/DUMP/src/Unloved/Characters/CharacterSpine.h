#pragma once

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct SkinnedModel;

namespace Unloved {

    struct CharacterSpine {
        bool Init(const SkinnedModel& skinnedModel, const std::vector<std::string>& boneNames, uint32_t anchorIndex, float scale);
        void Reset(const glm::vec3& leadPosition, const glm::vec3& forward);
        void MoveLead(const glm::vec3& displacement);
        void Straighten(const glm::vec3& displacement);

        const glm::vec3& GetPosition(uint32_t index) const;
        const glm::vec3& GetLeadPosition() const;
        const glm::vec3& GetAnchorPosition() const;
        const std::string& GetBoneName(uint32_t index) const;
        uint32_t GetSegmentCount() const;
        uint32_t GetAnchorIndex() const;

    private:
        std::vector<std::string> m_boneNames;
        std::vector<glm::vec3> m_positions;
        std::vector<float> m_segmentLengths;
        uint32_t m_anchorIndex = 0;
    };
}
