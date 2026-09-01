#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Render/RendererTypes.h"

#include <glm/mat4x4.hpp>

namespace Unloved {

struct PointPairObject {
    PointPairObject() = default;
    PointPairObject(uint64_t id, const PointPairCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    PointPairObject(const PointPairObject&) = delete;
    PointPairObject& operator=(const PointPairObject&) = delete;
    PointPairObject(PointPairObject&&) noexcept = default;
    PointPairObject& operator=(PointPairObject&&) noexcept = default;
    ~PointPairObject() = default;

    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    bool SetPointPosition(uint32_t pointIndex, const glm::vec3& position);
    void SetEditorName(const std::string& editorName);
    void SetCustomBool(uint32_t index, bool value);
    void SetCustomFloat(uint32_t index, float value);
    void SubmitRenderItems() const;

    const glm::vec3& GetPosition() const                      { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                      { return m_createInfo.rotation; }
    const glm::vec3& GetPositionP0() const                    { return m_worldP0; }
    const glm::vec3& GetPositionP1() const                    { return m_worldP1; }
    const glm::mat4& GetWorldMatrix() const                   { return m_worldMatrix; }
    const glm::mat4& GetWorldMatrixP0() const                 { return m_worldMatrixP0; }
    const glm::mat4& GetWorldMatrixP1() const                 { return m_worldMatrixP1; }
    glm::vec3 GetRight() const;
    glm::vec3 GetUp() const;
    glm::vec3 GetForward() const;
    float GetLength() const;
    const std::string& GetEditorName() const                  { return m_createInfo.editorName; }
    const PointPairCreateInfo& GetCreateInfo() const          { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const     { return m_renderItems; }
    PointPairObjectType GetType() const                       { return m_createInfo.type; }
    uint64_t GetObjectId() const                              { return m_objectId; }
    uint32_t GetPointCount() const                            { return 2; }

private:
    void Reset();
    void Rebuild();

    void RebuildDeckingPost();
    void RebuildGutter();
    void RebuildDeckingBearer();
    void RebuildRidgeCapping();

    void UpdateWorldPoints();

    uint64_t m_objectId = 0;
    PointPairCreateInfo m_createInfo;
    glm::vec3 m_worldP0 = glm::vec3(0.0f);
    glm::vec3 m_worldP1 = glm::vec3(0.0f);
    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    glm::mat4 m_worldMatrixP0 = glm::mat4(1.0f);
    glm::mat4 m_worldMatrixP1 = glm::mat4(1.0f);
    std::vector<uint32_t> m_meshIds;
    std::vector<uint64_t> m_physicsIds;
    std::vector<RenderItem> m_renderItems;
};
}
