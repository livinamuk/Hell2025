#include "Trim.h"
#include "Hell/Common/Bit.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/RendererUtil.h"
#include "Unloved/ObjectId.h"

#include <iostream> // TODO clean up logging

namespace Unloved {

void Trim::Init(Transform transform, const std::string& modelName, const std::string& materialName) {
    m_transform = transform;
    m_objectId = Unloved::GetNextObjectId(ObjectType::TRIM);

    Model* model = Hell::ResourceManager::GetModelByName(modelName);
    m_materialIndex = Hell::ResourceManager::GetMaterialIndexByName(materialName);
    Material* material = Hell::ResourceManager::GetMaterialByIndex(m_materialIndex);

    if (!model || !material) {
        std::cout << "Trim::Init() failed: model name '" << modelName << "' not found\n";
        return;
    }

    m_renderItem.modelMatrix = transform.to_mat4();
    m_renderItem.inverseModelMatrix = glm::inverse(m_renderItem.modelMatrix);
    m_renderItem.meshId = model->GetMeshIndices()[0];
    m_renderItem.materialIndex = m_materialIndex;
    RendererUtil::UpdateRenderItemAABB(m_renderItem);
    Hell::Bit::PackUint64(m_objectId, m_renderItem.objectIdLowerBit, m_renderItem.objectIdUpperBit);
}

void Trim::SubmitRenderItem() {
    RenderDataManager::SubmitRenderItem(m_renderItem);
}
}
