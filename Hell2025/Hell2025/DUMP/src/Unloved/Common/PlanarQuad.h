#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cstdint>

namespace Unloved {

struct PlanarQuadCreateInfo {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    std::array<glm::vec3, 4> points = {};
};

struct PlanarQuad {
    PlanarQuad() = default;
    PlanarQuad(const std::array<glm::vec3, 4>& points);
    PlanarQuad(const PlanarQuadCreateInfo& createInfo);

    bool SetPoints(const std::array<glm::vec3, 4>& points);
    bool SetPointPosition(uint32_t pointIndex, const glm::vec3& position);
    bool SetRotation(const glm::vec3& rotation);
    void SetPosition(const glm::vec3& position);
    void Translate(const glm::vec3& offset);

    const std::array<glm::vec3, 4>& GetPoints() const { return m_worldPoints; }
    const glm::vec3& GetPositionP0() const { return m_worldPoints[0]; }
    const glm::vec3& GetPositionP1() const { return m_worldPoints[1]; }
    const glm::vec3& GetPositionP2() const { return m_worldPoints[2]; }
    const glm::vec3& GetPositionP3() const { return m_worldPoints[3]; }
    const glm::vec3& GetPosition() const { return m_createInfo.position; }
    const glm::vec3& GetRotation() const { return m_createInfo.rotation; }
    const PlanarQuadCreateInfo& GetCreateInfo() const { return m_createInfo; }
    const glm::mat4& GetWorldMatrixP0() const { return m_worldMatrixP0; }
    const glm::mat4& GetWorldMatrixP1() const { return m_worldMatrixP1; }
    const glm::mat4& GetWorldMatrixP2() const { return m_worldMatrixP2; }
    const glm::mat4& GetWorldMatrixP3() const { return m_worldMatrixP3; }
    glm::vec3 GetCenter() const;
    glm::vec3 GetLeft() const;
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;
    glm::vec3 GetForward() const;
    glm::vec3 GetNormal() const;
    float GetWidth() const;
    float GetDepth() const;
    bool IsValid() const { return m_valid; }

private:
    void UpdateWorldPoints();
    void UpdateWorldMatrices();

    PlanarQuadCreateInfo m_createInfo;
    std::array<glm::vec3, 4> m_worldPoints = {};
    glm::mat4 m_worldMatrixP0 = glm::mat4(1.0f);
    glm::mat4 m_worldMatrixP1 = glm::mat4(1.0f);
    glm::mat4 m_worldMatrixP2 = glm::mat4(1.0f);
    glm::mat4 m_worldMatrixP3 = glm::mat4(1.0f);
    bool m_valid = false;
};
}
