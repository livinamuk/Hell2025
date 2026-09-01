#pragma once

#include "Unloved/Camera/Frustum.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace Unloved {

    struct SpotLightData {
        glm::mat4 projectionView = glm::mat4(1.0f);
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 direction = glm::vec3(0.0f, 0.0f, -1.0f);

        // CPU-only copies of the shared flashlight profile values needed to
        // maintain conservative culling bounds. They are not packed per light.
        float range = 20.0f;
        float modifier = 0.0f;
        float shadowHalfAngleDegrees = 45.0f;
        bool castsShadows = true;
        bool skipOwnerShadow = false;
        bool useFlashlightViewDistanceScale = true;
    };

    struct SpotLight {
        SpotLight() = default;
        SpotLight(uint64_t objectId, uint64_t ownerObjectId, int32_t ownerViewportIndex);

        void CleanUp() {}
        void SetData(const SpotLightData& data);
        void SetShadowLayer(int32_t shadowLayer) { m_shadowLayer = shadowLayer; }

        uint64_t GetObjectId() const { return m_objectId; }
        uint64_t GetOwnerObjectId() const { return m_ownerObjectId; }
        int32_t GetOwnerViewportIndex() const { return m_ownerViewportIndex; }
        int32_t GetShadowLayer() const { return m_shadowLayer; }
        const SpotLightData& GetData() const { return m_data; }
        const glm::vec3& GetWorldBoundsMin() const { return m_worldBoundsMin; }
        const glm::vec3& GetWorldBoundsMax() const { return m_worldBoundsMax; }
        Unloved::Frustum& GetFrustum() { return m_frustum; }

        bool IsActive() const { return m_data.modifier > 0.05f && m_data.range > 0.0f; }
        bool CastsShadows() const { return m_data.castsShadows; }

    private:
        void UpdateBounds();

        uint64_t m_objectId = 0;
        uint64_t m_ownerObjectId = 0;
        int32_t m_ownerViewportIndex = -1;
        int32_t m_shadowLayer = -1;
        SpotLightData m_data;
        glm::vec3 m_worldBoundsMin = glm::vec3(0.0f);
        glm::vec3 m_worldBoundsMax = glm::vec3(0.0f);
        Unloved::Frustum m_frustum;
    };
}
