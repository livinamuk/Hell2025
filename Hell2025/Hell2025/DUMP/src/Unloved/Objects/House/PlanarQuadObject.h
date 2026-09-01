#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/PlanarQuad.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct PlanarQuadObject {
    PlanarQuadObject() = default;
    PlanarQuadObject(uint64_t id, const PlanarQuadObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    PlanarQuadObject(const PlanarQuadObject&) = delete;
    PlanarQuadObject& operator=(const PlanarQuadObject&) = delete;
    PlanarQuadObject(PlanarQuadObject&&) noexcept = default;
    PlanarQuadObject& operator=(PlanarQuadObject&&) noexcept = default;
    ~PlanarQuadObject() = default;

    void CleanUp();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    bool SetPointPosition(uint32_t pointIndex, const glm::vec3& position);
    void SetEditorName(const std::string& editorName);
    void SetDeckingBoardsMaterial(const std::string& materialName);
    void SetCustomBool(uint32_t index, bool value);
    void SetCustomFloat(uint32_t index, float value);
    void SubmitRenderItems() const;

    const glm::vec3& GetPosition() const                           { return m_planarQuad.GetPosition(); }
    const glm::vec3& GetRotation() const                           { return m_planarQuad.GetRotation(); }
    const glm::vec3& GetPositionP0() const                         { return m_planarQuad.GetPositionP0(); }
    const glm::vec3& GetPositionP1() const                         { return m_planarQuad.GetPositionP1(); }
    const glm::vec3& GetPositionP2() const                         { return m_planarQuad.GetPositionP2(); }
    const glm::vec3& GetPositionP3() const                         { return m_planarQuad.GetPositionP3(); }
    const std::string& GetEditorName() const                       { return m_createInfo.editorName; }
    const PlanarQuadObjectCreateInfo& GetCreateInfo() const        { return m_createInfo; }
    const std::vector<RenderItem>& GetRenderItems() const          { return m_renderItems; }
    PlanarQuadObjectType GetType() const                           { return m_createInfo.type; }
    uint64_t GetObjectId() const                                  { return m_objectId; }
    uint32_t GetPointCount() const                                { return 4; }

private:
    void Reset();
    void Rebuild();

    void RebuildDeckingBoards();
    void RebuildRoofingIron();

    void SyncCreateInfoFromPlanarQuad();

    uint64_t m_objectId = 0;
    PlanarQuad m_planarQuad;
    PlanarQuadObjectCreateInfo m_createInfo;
    std::vector<uint32_t> m_meshIds;
    std::vector<uint64_t> m_physicsIds;
    std::vector<RenderItem> m_renderItems;
};
}
