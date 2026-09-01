#include "ChristmasLights.h"
#include "Hell/Common/Bit.h"
#include "Hell/Common/Random.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererUtil.h"
#include <array>

#include "Hell/Logging.h"

#include "Unloved/Objects/Exterior/Wire.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/World/World.h"

namespace Unloved {

ChristmasLightSet::ChristmasLightSet(uint64_t id, const ChristmasLightsCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_objectId = id;
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.sprialTopCenter += spawnOffset.translation;

    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
        sequencePoint.position += spawnOffset.translation;
    }

    m_position = m_createInfo.position;
    UpdateSequencePoints(m_createInfo.sequencePoints);
}

void ChristmasLightSet::UpdateSequencePoints(const std::vector<SequencePoint>& sequencePoints) {
    // Clear old runtime data
    CleanUp();
    m_renderItems.clear();
    m_GPUChristmasLights.clear();

    // Update create info
    m_createInfo.sequencePoints = sequencePoints;

    if (m_createInfo.sequencePoints.empty()) {
        m_position = m_createInfo.position;
        return;
    }

    m_createInfo.position = m_createInfo.sequencePoints.front().position;
    m_position = m_createInfo.position;

    if (m_createInfo.sequencePoints.size() >= 2) {
        WireCreateInfo wireCreateInfo;
        wireCreateInfo.sequencePoints = m_createInfo.sequencePoints;
        wireCreateInfo.radius = m_createInfo.wireRadius;
        wireCreateInfo.spacing = m_createInfo.spacing;
        wireCreateInfo.parentObjectId = m_objectId;
        m_wireIds.push_back(World::AddWire(wireCreateInfo));
    }

    RecreateLightRenderItems();
}

void ChristmasLightSet::SetPosition(const glm::vec3& position) {
    const glm::vec3 translation = position - m_position;
    m_createInfo.position = position;
    m_createInfo.sprialTopCenter += translation;

    for (SequencePoint& sequencePoint : m_createInfo.sequencePoints) {
        sequencePoint.position += translation;
    }

    UpdateSequencePoints(m_createInfo.sequencePoints);
}

void ChristmasLightSet::SetSpacing(float spacing) {
    m_createInfo.spacing = spacing;
    UpdateSequencePoints(m_createInfo.sequencePoints);
}

void ChristmasLightSet::SetWireRadius(float wireRadius) {
    m_createInfo.wireRadius = wireRadius;
    UpdateSequencePoints(m_createInfo.sequencePoints);
}

void ChristmasLightSet::RecreateLightRenderItems() {
    // Bit of a hack, but reset time here, that way when update runs, it'll detect this 0 and pick a random start time
    m_time = 0;

    // TODO but something like this...
    static Model* model = Hell::ResourceManager::GetModelByName("ChristmasLight");
    static int whiteMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("ChristmasLightWhite");
    static int blackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Black");

    std::vector<glm::mat4> modelMatrices;
    m_renderItems.clear();

    // Wire
    for (uint64_t wireId : m_wireIds) {
        Wire* wire = World::GetWireByObjectId(wireId);
        if (!wire) continue;

        for (const glm::vec3& position : wire->GetSegmentPoints()) {
            Transform transform;
            transform.position = position;
            transform.rotation = glm::vec3(Hell::Random::Float(-1.0f, 1.0f), Hell::Random::Float(-1.0f, 1.0f), Hell::Random::Float(-1.0f, 1.0f));
            transform.rotation.x += HELL_PI * -0.5f;
            transform.scale = glm::vec3(0.325f);
            modelMatrices.push_back(transform.to_mat4());
        }
    }

    // Light
    for (const glm::mat4& modelMatrix : modelMatrices) {
        Material* material = Hell::ResourceManager::GetMaterialByIndex(whiteMaterialIndex);
        RenderItem renderItem;
        renderItem.modelMatrix = modelMatrix;
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.meshId = model->GetMeshIndices()[1];;
        renderItem.materialIndex = whiteMaterialIndex;
        //renderItem.useEmissiveMask = 1.0f;                            // CHECK IM NOT IMPORTANT
        renderItem.shadowFlags = SHADOW_FLAG_NONE;
        renderItem.emissiveR = 1.0f;
        renderItem.emissiveG = 0.0f;
        renderItem.emissiveB = 0.0f;
        Hell::Bit::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);
        RendererUtil::UpdateRenderItemAABB(renderItem);
        m_renderItems.push_back(renderItem);
    }

    // Plastic
    for (const glm::mat4& modelMatrix : modelMatrices) {
        Material* material = Hell::ResourceManager::GetMaterialByIndex(blackMaterialIndex);
        RenderItem renderItem;
        renderItem.modelMatrix = modelMatrix;
        renderItem.meshId = model->GetMeshIndices()[0];
        renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);
        renderItem.materialIndex = blackMaterialIndex;
        renderItem.shadowFlags = SHADOW_FLAG_NONE;
        Hell::Bit::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);
        RendererUtil::UpdateRenderItemAABB(renderItem);
        m_renderItems.push_back(renderItem);
    }

    // Fill previous model matrices. These never move
    for (RenderItem& renderItem : m_renderItems) {
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
        if (!mesh) continue;

        renderItem.baseVertex = mesh->baseVertex;
        renderItem.baseIndex = mesh->baseIndex;
        renderItem.vertexCount = mesh->vertexCount;
        renderItem.indexCount = mesh->indexCount;

        renderItem.prevModelMatrix = renderItem.modelMatrix;
    }
}

void ChristmasLightSet::Update(float deltaTime) {
    // Define the patterns
    std::vector<std::array<bool, 4>> patterns = {
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},
        {1, 1, 1, 1},
        {0, 0, 0, 0},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},

        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {0, 0, 0, 1},
    };
    glm::vec3 colors[] = {
        /* red    */ glm::vec3(1.0f, 0.0f, 0.0f),
        /* blue   */ glm::vec3(0.0f, 0.025f, 1.0f),
        /* yellow */ glm::vec3(1.0f, 0.5f, 0.0f),
        /* green  */ glm::vec3(0.05f, 0.9f, 0.05f)
    };

    if (m_time == 0) {
        m_time = Hell::Random::Float(0.0f, 5.0f);
    }

    m_time += deltaTime;
    float flashSpeed = 0.09f;
    int currentPatternIndex = static_cast<int>(m_time / flashSpeed) % patterns.size();
    const auto& currentPattern = patterns[currentPatternIndex];

    //for (auto& p : m_createInfo.sequencePoints) {
    //    DebugDraw::DrawPoint(p, RED);
    //}

    m_GPUChristmasLights.clear();

    for (size_t i = 0; i < m_renderItems.size() / 2; i++) {
        int colorIndex = i % 4;
        glm::vec3 color = currentPattern[colorIndex] ? colors[colorIndex] : BLACK;
        m_renderItems[i].emissiveR = color.r;
        m_renderItems[i].emissiveG = color.g;
        m_renderItems[i].emissiveB = color.b;

        // If the light is on, add it to the gpu list
        if (color != glm::vec3(0.0f)) {
            GPUChristmasLight& light = m_GPUChristmasLights.emplace_back();
            light.position.r = m_renderItems[i].modelMatrix[3].x;
            light.position.g = m_renderItems[i].modelMatrix[3].y;
            light.position.b = m_renderItems[i].modelMatrix[3].z;
            light.color.r = color.r;
            light.color.g = color.g;
            light.color.b = color.b;
            light.color.a = 1.0f;
        }

        //DebugDraw::DrawPoint(m_renderItems[i].modelMatrix[3], glm::vec4(color, 1.0f));
    }
}

void ChristmasLightSet::CleanUp() {
    for (uint64_t wireId : m_wireIds) {
        World::RemoveObjectById(wireId);
    }

    m_wireIds.clear();
}
}
