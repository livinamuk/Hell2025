#include "ChristmasTree.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/World/World.h"

namespace Unloved {

ChristmasTree::ChristmasTree(uint64_t id, const ChristmasTreeCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;
    m_position = m_createInfo.position;
    m_rotation = m_createInfo.rotation;

    Transform transform;
    transform.position = m_position;
    transform.rotation = m_rotation;
    m_modelMatrix = transform.to_mat4();

    CreateRenderItems();

    ChristmasLightsCreateInfo christmasLightsCreateInfo;
    christmasLightsCreateInfo.sprialTopCenter = createInfo.position + glm::vec3(-0.08f, 1.7f, -0.03f);
    christmasLightsCreateInfo.spiral = true;

    Unloved::World::AddChristmasLights(christmasLightsCreateInfo, spawnOffset);
}

void ChristmasTree::CreateRenderItems() {
    //m_renderItems.clear();
    //
    //m_model = Hell::ResourceManager::GetModelByName("ChristmasTree");
    //if (!m_model) {
    //    std::cout << "Could not get ChristmasTree model\n";
    //    return;
    //}
    //
    //m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName("ChristmasTree");
    //Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);
    //if (!material) {
    //    std::cout << "Could not get ChristmasTree material\n";
    //    return;
    //}
    //
    //for (uint32_t meshIndex : m_model->GetMeshIndices()) {
    //    Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshIndex);
    //    if (!mesh) continue;
    //
    //    RenderItem& renderItem = m_renderItems.emplace_back();
    //    renderItem.objectType = (int)ObjectType::GAME_OBJECT;
    //    renderItem.modelMatrix = m_modelMatrix;
    //    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
    //    renderItem.meshId = meshIndex;
    //    renderItem.castShadows = false;
    //    renderItem.baseVertex = mesh->baseVertex;
    //    renderItem.baseIndex = mesh->baseIndex;
    //
    //    renderItem.materialIndex = materialIndex;
    //
    //    RendererUtil::UpdateRenderItemAABB(renderItem);
    //}
}

void ChristmasTree::Update(float deltaTime) {
    // Nothing as of yet
}

void ChristmasTree::CleanUp() {
    // Nothing as of yet
}
}
