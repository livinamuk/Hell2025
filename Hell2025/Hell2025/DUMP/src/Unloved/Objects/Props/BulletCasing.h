#pragma once
#include "Hell/Physics/Physics.h"

#include "Unloved/Common/Types.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct BulletCasing {
    BulletCasing() = default;
    BulletCasing(uint64_t id, BulletCasingCreateInfo createInfo);
    BulletCasing(const BulletCasing&) = delete;
    BulletCasing& operator=(const BulletCasing&) = delete;
    BulletCasing(BulletCasing&&) noexcept = default;
    BulletCasing& operator=(BulletCasing&&) noexcept = default;
    ~BulletCasing() = default;

    float m_audioDelay = 0.0f;
    float m_lifeTime = 0.0f;
    bool m_collisionsEnabled = false;

    void Update(float deltaTime);
    void CleanUp();
    void SubmitRenderItem();
    //void CollisionResponse();
    const glm::mat4& GetModelMatrix();

    uint64_t GetObjectId()                              { return m_objectId; }
    uint64_t GetrigidDynamicId()                        { return m_rigidDynamicId; }
    uint32_t GetMeshId()                                { return m_meshId; }
    uint32_t GetMaterialIndex()                         { return m_materialIndex; }
    const BulletCasingCreateInfo& GetCreateInfo() const { return m_createInfo; }
    const std::string& GetEditorName() const            { return m_createInfo.editorName; }

private:
    BulletCasingCreateInfo m_createInfo;
    RenderItem m_renderItem;
    uint64_t m_objectId = 0;
    uint64_t m_rigidDynamicId = 0;
    uint32_t m_materialIndex = 0;
    uint32_t m_meshId = 0;
    glm::mat4 m_modelMatrix = glm::mat4(1);
};
}
