#include "PlanarQuadObject.h"

#include "Hell/BVH/BVH.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Systems/House/HouseGeometryBuilder.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

#include <glm/mat3x3.hpp>

namespace Unloved {

    PlanarQuadObject::PlanarQuadObject(uint64_t id, const PlanarQuadObjectCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
        m_objectId = id;
        m_createInfo = createInfo;
        m_createInfo.planarQuad.position += spawnOffset.translation;
        m_planarQuad = PlanarQuad(m_createInfo.planarQuad);
        SyncCreateInfoFromPlanarQuad();
        Rebuild();
    }

    void PlanarQuadObject::Reset() {
        for (uint64_t physicsId : m_physicsIds) Hell::Physics::MarkRigidStaticForRemoval(physicsId);

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");
        for (uint32_t meshId : m_meshIds) {
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (mesh) Hell::Bvh::DestroyMeshBvh(mesh->meshBvhId);
            meshBuffer.RemoveMesh(meshId);
        }

        m_physicsIds.clear(); 
        m_meshIds.clear();
        m_renderItems.clear();
    }

    void PlanarQuadObject::CleanUp() {
        Reset();
        m_objectId = 0;
        m_planarQuad = {};
        m_createInfo = {};
        WorldBVH::MarkStaticSceneBvhDirty();
    }

    void PlanarQuadObject::SetPosition(const glm::vec3& position) {
        m_planarQuad.SetPosition(position);
        SyncCreateInfoFromPlanarQuad();
        Rebuild();
    }

    void PlanarQuadObject::SetRotation(const glm::vec3& rotation) {
        if (!m_planarQuad.SetRotation(rotation)) return;
        SyncCreateInfoFromPlanarQuad();
        Rebuild();
    }

    bool PlanarQuadObject::SetPointPosition(uint32_t pointIndex, const glm::vec3& position) {
        if (pointIndex >= 4 || !m_planarQuad.SetPointPosition(pointIndex, position)) return false;
        SyncCreateInfoFromPlanarQuad();
        Rebuild();
        return true;
    }

    void PlanarQuadObject::SetEditorName(const std::string& editorName) {
        m_createInfo.editorName = editorName;
    }

    void PlanarQuadObject::SetDeckingBoardsMaterial(const std::string& materialName) {
        m_createInfo.materialNames[0] = materialName;
        Rebuild();
    }

    void PlanarQuadObject::SetCustomBool(uint32_t index, bool value) {
        if (index >= m_createInfo.customBools.size()) return;
        m_createInfo.customBools[index] = value;
        Rebuild();
    }

    void PlanarQuadObject::SetCustomFloat(uint32_t index, float value) {
        if (index >= m_createInfo.customFloats.size()) return;
        m_createInfo.customFloats[index] = value;
        Rebuild();
    }

    void PlanarQuadObject::SubmitRenderItems() const {
        for (const RenderItem& renderItem : m_renderItems) RenderDataManager::SubmitRenderItemProcedural(renderItem);
    }

    void PlanarQuadObject::SyncCreateInfoFromPlanarQuad() {
        m_createInfo.planarQuad = m_planarQuad.GetCreateInfo();
    }
}
