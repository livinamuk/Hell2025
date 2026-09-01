#pragma once

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/PlanarQuad.h"
#include "Unloved/Common/Types.h"
#include "Hell/Render/VertexAttributes.h"

#include "Hell/ResourceManagement/Types/Material.h"

namespace Unloved {

struct WorldPlane {
    WorldPlane() = default;
    WorldPlane(uint64_t id, const WorldPlaneCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    WorldPlane(const WorldPlane&) = delete;
    WorldPlane& operator=(const WorldPlane&) = delete;
    WorldPlane(WorldPlane&&) noexcept = default;
    WorldPlane& operator=(WorldPlane&&) noexcept = default;
    ~WorldPlane() = default;

    void UpdateVertexDataFromCreateInfo();
    bool SetPointPosition(uint32_t pointIndex, const glm::vec3& position);
    bool SetRotation(const glm::vec3& rotation);
    void SetPosition(const glm::vec3& position);
    void UpdateWorldSpaceCenter(glm::vec3 worldSpaceCenter);
    void SetMaterial(const std::string& materialName);
    void SetMeshId(uint32_t meshId);
    void SetTextureScale(float value);
    void SetTextureOffsetU(float value);
    void SetTextureOffsetV(float value);
    void SetRotateTexture90(bool value);
    void SetRoughnessFactor(float value);
    void SetMetallicFactor(float value);
    void CleanUp();
    void SubmitRenderItem();
    void DrawEdges(glm::vec4 color);
    void DrawVertices(glm::vec4 color);
	void HideInEditor();
	void UnhideInEditor();

	bool IsHiddenInEditor() const                   { return m_hiddenInEditor; }
    const glm::vec3& GetWorldSpaceCenter() const    { return m_worldSpaceCenter; }
    const glm::vec3& GetRotation() const            { return m_planarQuad.GetRotation(); }
    const std::string& GetEditorName() const        { return m_createInfo.editorName; }
    const uint64_t GetObjectId() const              { return m_objectId; }
    const uint64_t GetParentDoorId() const          { return m_createInfo.parentDoorId; }
    Material* GetMaterial();
    int32_t GetMaterialIndex() const                { return m_materialIndex; }
    const PlanarQuad& GetPlanarQuad() const         { return m_planarQuad; }
    std::vector<Vertex>& GetVertices()              { return m_vertices; }
    std::vector<uint32_t>& GetIndices()             { return m_indices; }
    std::vector<glm::vec2>& GetNavMeshPoly()        { return m_navMeshPoly; }
    WorldPlaneCreateInfo& GetCreateInfo()           { return m_createInfo; }
    WorldPlaneType GetType() const                  { return m_createInfo.type; }
    uint32_t GetMeshId() const                      { return m_meshId; }

private:
    uint64_t m_objectId = 0;
    uint64_t m_parentDoorId = 0;
    uint64_t m_physicsId = 0;
    int32_t m_materialIndex = -1;
    glm::vec3 m_worldSpaceCenter = glm::vec3(0.0f);
    PlanarQuad m_planarQuad;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    std::vector<glm::vec2> m_navMeshPoly;
    WorldPlaneCreateInfo m_createInfo;
    bool m_hiddenInEditor = false;
    uint32_t m_meshId = 0;

    void CreatePhysicsObject();
    void SyncCreateInfoFromPlanarQuad();
};
}
