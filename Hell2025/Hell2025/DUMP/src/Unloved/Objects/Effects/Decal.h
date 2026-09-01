#pragma once

#include <cstdint>

#include "Hell/ResourceManagement/Types/Material.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct Decal {
    Decal() = default;
    Decal(uint64_t id, const DecalCreateInfo& createInfo);
    Decal(const Decal&) = delete;
    Decal& operator=(const Decal&) = delete;
    Decal(Decal&&) noexcept = default;
    Decal& operator=(Decal&&) noexcept = default;
    ~Decal() = default;
    void Update();
    void CleanUp();

    uint64_t GetObjectId() const               { return m_objectId; }
    const glm::vec3 GetPosition() const         { return glm::vec3(m_worldMatrix[3]); }
    const glm::vec3 GetWorldNormal() const      { return glm::vec3(m_worldNormal); }
    const RenderItem& GetRenderItem() const     { return m_renderItem; }
    const DecalCreateInfo& GetCreateInfo() const { return m_createInfo; }
    const std::string& GetEditorName() const     { return m_createInfo.editorName; }

private:
    const glm::mat4& GetParentWorldMatrix();

    uint64_t m_objectId = 0;
    DecalType m_type = DecalType::UNDEFINED;
    DecalCreateInfo m_createInfo;
    int32_t m_materialIndex = -1;
    RenderItem m_renderItem;
    glm::vec3 m_localPosition = glm::vec3(0.0f);
    glm::vec3 m_localNormal = glm::vec3(0.0f);
    glm::vec3 m_worldNormal = glm::vec3(0.0f);
    glm::mat4 m_worldMatrix = glm::mat4(1.0f);
    glm::mat4 m_localMatrix = glm::mat4(1.0f);

};
}
