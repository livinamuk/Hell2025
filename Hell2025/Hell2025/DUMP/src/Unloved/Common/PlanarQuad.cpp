#include "PlanarQuad.h"

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>

namespace Unloved {
    namespace {
        constexpr float MINIMUM_SIZE = 0.01f;

        glm::vec3 CalculateRotation(const std::array<glm::vec3, 4>& points) {
            const glm::vec3 normal = glm::normalize(glm::cross(points[1] - points[0], points[3] - points[0]));
            const glm::vec3 right = glm::normalize(points[3] - points[0]);
            const glm::vec3 forward = glm::normalize(glm::cross(right, normal));
            return glm::eulerAngles(glm::normalize(glm::quat_cast(glm::mat3(right, normal, forward))));
        }

        void MakeRectangle(std::array<glm::vec3, 4>& points) {
            const float left = points[0].x;
            const float right = points[3].x;
            const float back = points[0].z;
            const float front = points[1].z;
            points[0] = glm::vec3(left, 0.0f, back);
            points[1] = glm::vec3(left, 0.0f, front);
            points[2] = glm::vec3(right, 0.0f, front);
            points[3] = glm::vec3(right, 0.0f, back);
        }

        void CenterPoints(PlanarQuadCreateInfo& createInfo) {
            const glm::vec3 center = (createInfo.points[0] + createInfo.points[1] + createInfo.points[2] + createInfo.points[3]) * 0.25f;
            createInfo.position += glm::quat(createInfo.rotation) * center;
            for (glm::vec3& point : createInfo.points) point -= center;
        }
    }

    PlanarQuad::PlanarQuad(const std::array<glm::vec3, 4>& points) {
        SetPoints(points);
    }

    PlanarQuad::PlanarQuad(const PlanarQuadCreateInfo& createInfo) {
        m_createInfo = createInfo;
        MakeRectangle(m_createInfo.points);
        CenterPoints(m_createInfo);
        UpdateWorldPoints();
        m_valid = true;
    }

    bool PlanarQuad::SetPoints(const std::array<glm::vec3, 4>& points) {
        m_createInfo.position = (points[0] + points[1] + points[2] + points[3]) * 0.25f;
        m_createInfo.rotation = CalculateRotation(points);
        const glm::quat inverseRotation = glm::inverse(glm::quat(m_createInfo.rotation));
        for (uint32_t i = 0; i < points.size(); i++) {
            m_createInfo.points[i] = inverseRotation * (points[i] - m_createInfo.position);
            m_createInfo.points[i].y = 0.0f;
        }
        MakeRectangle(m_createInfo.points);
        CenterPoints(m_createInfo);

        UpdateWorldPoints();
        m_valid = true;
        return true;
    }

    bool PlanarQuad::SetPointPosition(uint32_t pointIndex, const glm::vec3& position) {
        if (!m_valid || pointIndex >= m_worldPoints.size()) return false;

        PlanarQuadCreateInfo createInfo = m_createInfo;
        const glm::quat rotation = glm::quat(createInfo.rotation);
        glm::vec3 localPosition = glm::inverse(rotation) * (position - createInfo.position);
        createInfo.position += rotation * glm::vec3(0.0f, localPosition.y, 0.0f);

        if (pointIndex == 0) {
            localPosition.x = std::min(localPosition.x, createInfo.points[2].x - MINIMUM_SIZE);
            localPosition.z = std::min(localPosition.z, createInfo.points[1].z - MINIMUM_SIZE);
            createInfo.points[0].x = localPosition.x;
            createInfo.points[1].x = localPosition.x;
            createInfo.points[0].z = localPosition.z;
            createInfo.points[3].z = localPosition.z;
        }
        else if (pointIndex == 1) {
            localPosition.x = std::min(localPosition.x, createInfo.points[2].x - MINIMUM_SIZE);
            localPosition.z = std::max(localPosition.z, createInfo.points[0].z + MINIMUM_SIZE);
            createInfo.points[0].x = localPosition.x;
            createInfo.points[1].x = localPosition.x;
            createInfo.points[1].z = localPosition.z;
            createInfo.points[2].z = localPosition.z;
        }
        else if (pointIndex == 2) {
            localPosition.x = std::max(localPosition.x, createInfo.points[0].x + MINIMUM_SIZE);
            localPosition.z = std::max(localPosition.z, createInfo.points[0].z + MINIMUM_SIZE);
            createInfo.points[2].x = localPosition.x;
            createInfo.points[3].x = localPosition.x;
            createInfo.points[1].z = localPosition.z;
            createInfo.points[2].z = localPosition.z;
        }
        else if (pointIndex == 3) {
            localPosition.x = std::max(localPosition.x, createInfo.points[0].x + MINIMUM_SIZE);
            localPosition.z = std::min(localPosition.z, createInfo.points[1].z - MINIMUM_SIZE);
            createInfo.points[2].x = localPosition.x;
            createInfo.points[3].x = localPosition.x;
            createInfo.points[0].z = localPosition.z;
            createInfo.points[3].z = localPosition.z;
        }
        CenterPoints(createInfo);

        std::array<glm::vec3, 4> worldPoints;
        for (uint32_t i = 0; i < worldPoints.size(); i++) worldPoints[i] = createInfo.position + rotation * createInfo.points[i];

        m_createInfo = createInfo;
        m_worldPoints = worldPoints;
        UpdateWorldMatrices();
        return true;
    }

    bool PlanarQuad::SetRotation(const glm::vec3& rotation) {
        if (!m_valid) return false;

        m_createInfo.rotation = rotation;
        UpdateWorldPoints();
        return true;
    }

    void PlanarQuad::SetPosition(const glm::vec3& position) {
        if (!m_valid) return;
        m_createInfo.position = position;
        UpdateWorldPoints();
    }

    void PlanarQuad::Translate(const glm::vec3& offset) {
        if (!m_valid) return;
        SetPosition(m_createInfo.position + offset);
    }

    glm::vec3 PlanarQuad::GetCenter() const {
        return m_createInfo.position;
    }

    glm::vec3 PlanarQuad::GetLeft() const {
        return -GetRight();
    }

    glm::vec3 PlanarQuad::GetRight() const {
        if (!m_valid) return glm::vec3(1.0f, 0.0f, 0.0f);
        return glm::quat(m_createInfo.rotation) * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    glm::vec3 PlanarQuad::GetUp() const {
        if (!m_valid) return glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::quat(m_createInfo.rotation) * glm::vec3(0.0f, 1.0f, 0.0f);
    }

    glm::vec3 PlanarQuad::GetForward() const {
        if (!m_valid) return glm::vec3(0.0f, 0.0f, 1.0f);
        return glm::quat(m_createInfo.rotation) * glm::vec3(0.0f, 0.0f, 1.0f);
    }

    glm::vec3 PlanarQuad::GetNormal() const {
        return GetUp();
    }

    float PlanarQuad::GetWidth() const {
        return glm::distance(m_worldPoints[0], m_worldPoints[3]);
    }

    float PlanarQuad::GetDepth() const {
        return glm::distance(m_worldPoints[0], m_worldPoints[1]);
    }

    void PlanarQuad::UpdateWorldPoints() {
        const glm::quat rotation = glm::quat(m_createInfo.rotation);
        for (uint32_t i = 0; i < m_worldPoints.size(); i++) m_worldPoints[i] = m_createInfo.position + rotation * m_createInfo.points[i];
        UpdateWorldMatrices();
    }

    void PlanarQuad::UpdateWorldMatrices() {
        const glm::mat4 rotationMatrix = glm::mat4_cast(glm::quat(m_createInfo.rotation));

        m_worldMatrixP0 = rotationMatrix;
        m_worldMatrixP1 = rotationMatrix;
        m_worldMatrixP2 = rotationMatrix;
        m_worldMatrixP3 = rotationMatrix;

        m_worldMatrixP0[3] = glm::vec4(m_worldPoints[0], 1.0f);
        m_worldMatrixP1[3] = glm::vec4(m_worldPoints[1], 1.0f);
        m_worldMatrixP2[3] = glm::vec4(m_worldPoints[2], 1.0f);
        m_worldMatrixP3[3] = glm::vec4(m_worldPoints[3], 1.0f);
    }
}
