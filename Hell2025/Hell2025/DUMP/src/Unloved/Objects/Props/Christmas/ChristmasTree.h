#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Hell/ResourceManagement/Types/Model.h"

// TODO: remove me
#include "Hell/ResourceManagement/Types/Material.h"

#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct ChristmasTree {
    ChristmasTree() = default;
    ChristmasTree(uint64_t id, const ChristmasTreeCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    void Update(float deltaTime);
    void CleanUp();
    void CreateRenderItems();

    const glm::vec3& GetPosition() const                    { return m_position; }
    const glm::mat4& GetModelMatrix() const                 { return m_modelMatrix; }
    uint64_t GetObjectId() const                            { return m_objectId; }
    const std::vector<RenderItem>& GetRenderItems() const   { return m_renderItems; }
    const ChristmasTreeCreateInfo& GetCreateInfo() const    { return m_createInfo; }
    const std::string& GetEditorName() const                { return m_createInfo.editorName; }

private:
    uint64_t m_objectId = 0;
    ChristmasTreeCreateInfo m_createInfo;
    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_rotation = glm::vec3(0.0f);
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    Model* m_model = nullptr;
    int32_t m_materialIndex = -1;
    std::vector<RenderItem> m_renderItems;
};
}
