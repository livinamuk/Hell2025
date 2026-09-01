#include "VATInstance.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Transform.h"

#include <algorithm>
#include <cmath>

void VATInstance::Init(const VATInstanceCreateInfo& createInfo) {
    m_createInfo = createInfo;
    m_currentTime = 0.0f;
    m_currentFrameIdx = 0;
    m_fps = 0.0f;
    m_frameCount = 0;
    m_positionTextureIndex = -1;
    m_rotationTextureIndex = -1;
    m_lookupTextureIndex = -1;
    m_animationComplete = false;

    Hell::Vat* vat = Hell::ResourceManager::GetVATPtr(m_createInfo.resourceName);
    if (!vat) {
        Logging::Error() << "VATInstance::Init(..) fucked up VAT resource '" << m_createInfo.resourceName << "' not found\n";
        return;
    };

    m_positionTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_pos", true);
    m_rotationTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_rot", true);
    m_lookupTextureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(m_createInfo.resourceName + "_lookup", true);

    const Hell::VATMetadata& metadata = vat->GetMetadata();
    m_frameCount = std::max(metadata.frameCount, 1);

    const float fps = metadata.fps > 0.0f ? metadata.fps : 24.0f;
    m_fps = std::max(fps * m_createInfo.playbackSpeed, 0.01f);

    m_duration = static_cast<float>(m_frameCount) / m_fps;

    if (!HasValidTextureIndices()) {
        Logging::Error() << "VATInstance::Init() failed to resolve VAT textures for '" << m_createInfo.resourceName << "'\n";
    }

    m_worldBoundsMin = glm::vec4(metadata.boundsMin, 1.0f);
    m_worldBoundsMax = glm::vec4(metadata.boundsMax, 1.0f);

    Model* model = Hell::ResourceManager::GetModelById(vat->GetModelId());
    if (!vat) {
        Logging::Error() << "VATInstance::Init(..) fucked up coz model id '" << vat->GetModelId() << "' not found\n";
        return;
    }

    if (model->GetMeshIndices().empty()) {
        Logging::Error() << "VATInstance::Init(..) fucked up coz altho model was found it had no mesh\n";
        return;
    }

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);

    if (!mesh) {
        Logging::Error() << "VATInstance::Init(..) fucked up coz mesh[0] id returned a nullptr mesh\n";
        return;
    }

    m_baseIndex = mesh->baseIndex;
    m_baseVertex = mesh->baseVertex;
    m_indexCount = mesh->indexCount;
    m_vertexCount = mesh->vertexCount;

    Hell::LocalFrame localFrame(m_createInfo.worldForward);

    m_transform = Hell::QuatTransform(m_createInfo.worldPosition, localFrame);
    m_transform.scale = glm::vec3(m_createInfo.scale);
}

void VATInstance::Update(float deltaTime) {

    m_currentTime += deltaTime;

    Hell::Vat* vat = Hell::ResourceManager::GetVATPtr(m_createInfo.resourceName);
    if (!vat) return;

    const float loopDuration = static_cast<float>(m_frameCount) / m_fps;
    const float stopTime = static_cast<float>(m_frameCount - 1) / m_fps;

    if (m_createInfo.loop) {
        m_currentTime = std::fmod(m_currentTime, loopDuration);
    }
    else {
        if (m_currentTime >= stopTime) {
            m_animationComplete = true;
        }

        m_currentTime = std::min(m_currentTime, stopTime);
    }

    m_currentFrameIdx = std::min(static_cast<int32_t>(m_currentTime * m_fps), m_frameCount - 1);
}

VATRenderItem VATInstance::CreateRenderItem() {
    VATRenderItem renderItem;
    renderItem.modelMatrix = m_transform.ToMat4();
    renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
    renderItem.boundsMin = m_worldBoundsMin;
    renderItem.boundsMax = m_worldBoundsMax;
    renderItem.positionTextureIdx = m_positionTextureIndex;
    renderItem.rotationTextureIdx = m_rotationTextureIndex;
    renderItem.lookupTextureIdx = m_lookupTextureIndex;
    renderItem.fps = m_fps;
    renderItem.frameCount = m_frameCount;
    renderItem.baseIndex = m_baseIndex;
    renderItem.baseVertex = m_baseVertex;
    renderItem.indexCount = m_indexCount;
    renderItem.vertexCount = m_vertexCount;
    renderItem.currentTime = m_currentTime;
    renderItem.mirror = m_createInfo.mirror;

    return renderItem;
}
