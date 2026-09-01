#include "SpotLight.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace Unloved {

    SpotLight::SpotLight(uint64_t objectId, uint64_t ownerObjectId, int32_t ownerViewportIndex) {
        m_objectId = objectId;
        m_ownerObjectId = ownerObjectId;
        m_ownerViewportIndex = ownerViewportIndex;
        SetData(m_data);
    }

    void SpotLight::SetData(const SpotLightData& data) {
        m_data = data;

        if (glm::dot(m_data.direction, m_data.direction) < 0.000001f) {
            m_data.direction = glm::vec3(0.0f, 0.0f, -1.0f);
        }
        else {
            m_data.direction = glm::normalize(m_data.direction);
        }

        m_data.range = std::max(m_data.range, 0.0f);
        m_data.shadowHalfAngleDegrees = std::clamp(m_data.shadowHalfAngleDegrees, 1.0f, 89.0f);

        m_frustum.Update(m_data.projectionView);
        UpdateBounds();
    }

    void SpotLight::UpdateBounds() {
        const glm::vec3 direction = m_data.direction;
        const glm::vec3 baseCenter = m_data.position + direction * m_data.range;
        const float baseRadius = std::tan(glm::radians(m_data.shadowHalfAngleDegrees)) * m_data.range;

        // Maximum projection of the cone's base disc onto each world axis.
        const glm::vec3 discExtent = baseRadius * glm::sqrt(glm::max(glm::vec3(1.0f) - direction * direction, glm::vec3(0.0f)));
        m_worldBoundsMin = glm::min(m_data.position, baseCenter - discExtent);
        m_worldBoundsMax = glm::max(m_data.position, baseCenter + discExtent);
    }
}
