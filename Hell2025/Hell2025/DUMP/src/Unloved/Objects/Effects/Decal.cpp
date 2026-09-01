#include "Decal.h"

#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"
#include "Hell/Math/Rotation.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/World/World.h"

namespace Unloved {

Decal::Decal(uint64_t id, const DecalCreateInfo& createInfo) {
    m_objectId = id;
    m_createInfo = createInfo;

    m_localPosition = glm::vec3(glm::inverse(GetParentWorldMatrix()) * glm::vec4(createInfo.surfaceHitPosition, 1.0f));
    m_localNormal = glm::vec3(glm::inverse(GetParentWorldMatrix()) * glm::vec4(createInfo.surfaceHitNormal, 0.0f));
      
    // Re-normalize because it got fucked up somewhere further up the chain
    m_localNormal = glm::normalize(m_localNormal);

    // Offset along local normal to avoid z fighting
    m_localPosition += m_localNormal * 0.0025f;

    // Determine type
    if (MeshNode* meshNode = World::GetMeshNodeByObjectIdAndLocalNodeIndex(m_createInfo.parentObjectId, m_createInfo.localMeshNodeIndex)) {
        m_type = meshNode->decalType;
    }
    else {
        m_type = DecalType::PLASTER;
    }

    float scale = 0.1f;

    if (m_type == DecalType::GLASS) {
        m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName("BulletHole_Glass");
        scale = 0.035f * 0.825f;
    }
    else if (m_type == DecalType::PLASTER) {
        m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName("BulletHole_Plaster");
        scale = 0.02f * 0.5f;
    }

    // Compute the local matrix once because it never changes
    float randomRotation = Hell::Random::Float(0.0f, HELL_PI * 2.0f);
    m_localMatrix = glm::translate(glm::mat4(1.0f), m_localPosition);
    m_localMatrix *= Hell::Math::RotationMatrixFromForward(m_localNormal, glm::vec3(0, 0, 1), glm::vec3(0, 1, 0));
    m_localMatrix *= glm::rotate(glm::mat4(1.0f), randomRotation, glm::vec3(0, 0, 1));
    m_localMatrix *= glm::scale(glm::mat4(1.0f), glm::vec3(scale));

    Model* primitives = Hell::ResourceManager::GetModelByName("Primitives");
    if (!primitives || primitives->GetMeshIndices().empty()) {
        Logging::Fatal() << "Decal::Decal(..) failed to get primitive quad mesh id\n";
        return;
    }

    if (primitives->GetMeshCount() == 0) return;
        
    uint32_t meshId = primitives->GetMeshIndices()[0];
  
    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
    if (!mesh) {
        Logging::Fatal() << "Decal::Decal(..) failed to get quad mesh index\n";
        return;
    }

    Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
    if (!material) return;

    // Set persistent RenderItem values
    m_renderItem.meshId = meshId;
    m_renderItem.materialIndex = m_materialIndex;
    m_renderItem.baseVertex = mesh->baseVertex;
    m_renderItem.baseIndex = mesh->baseIndex;
    m_renderItem.vertexCount = mesh->vertexCount;
    m_renderItem.indexCount = mesh->indexCount;
    m_renderItem.shadowFlags = SHADOW_FLAG_NONE;
    m_renderItem.blendingMode = static_cast<int32_t>(BlendingMode::ALPHA_DISCARD);
}

void Decal::CleanUp() {

}

void Decal::Update() {
    glm::vec3 position = GetParentWorldMatrix() * glm::vec4(m_localPosition, 1.0f);

    m_worldNormal = GetParentWorldMatrix() * glm::vec4(m_localNormal, 0.0f);
    m_worldMatrix = GetParentWorldMatrix() * m_localMatrix;

    glm::vec3 boundsMin = glm::vec3(-0.5f);
    glm::vec3 boundsMax = glm::vec3(0.5f);

    AABB m_localAABB(boundsMin, boundsMax);

    m_renderItem.prevModelMatrix = m_renderItem.modelMatrix;
    m_renderItem.modelMatrix = m_worldMatrix;
    m_renderItem.inverseModelMatrix = glm::inverse(m_renderItem.modelMatrix);
    m_renderItem.aabbMin = glm::vec4(GetPosition() - m_localAABB.GetBoundsMin(), 1.0);
    m_renderItem.aabbMax = glm::vec4(GetPosition() + m_localAABB.GetBoundsMax(), 1.0);

    RendererUtil::UpdateRenderItemAABB(m_renderItem);

    RenderDataManager::SubmitRenderItem({ m_renderItem });
}


const glm::mat4& Decal::GetParentWorldMatrix() {
    static glm::mat4 identity = glm::mat4(1.0f);

    if (MeshNode* meshNode = World::GetMeshNodeByObjectIdAndLocalNodeIndex(m_createInfo.parentObjectId, m_createInfo.localMeshNodeIndex)) {
        return meshNode->worldMatrix;
    }
    
    return identity;
}
}
