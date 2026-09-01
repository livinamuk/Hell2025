#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

struct PictureFrame {
    PictureFrame() = default;
    PictureFrame(uint64_t id, PictureFrameCreateInfo& createInfo, SpawnOffset& spawnOffset);
    PictureFrame(const PictureFrame&) = delete;
    PictureFrame& operator=(const PictureFrame&) = delete;
    PictureFrame(PictureFrame&&) noexcept = default;
    PictureFrame& operator=(PictureFrame&&) noexcept = default;
    ~PictureFrame() = default;

    void Update();
    void CleanUp();

    void SelectRandomPicture();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetScale(const glm::vec3& scale);
    void SetType(PictureFrameType type);
    void SetUseRandom(bool useRandom);
    void SetMaterialName(const std::string& materialName);
    void UpdateRenderItems();

    MeshNodes& GetMeshNodes()                               { return m_meshNodes; }

    const uint64_t& GetObjectId() const                     { return m_objectId; }
    const glm::vec3& GetPosition() const                    { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                    { return m_createInfo.rotation; }
    const glm::vec3& GetScale() const                       { return m_createInfo.scale; }
    const PictureFrameCreateInfo& GetCreateInfo() const     { return m_createInfo; }
    const PictureFrameType& GetType() const                 { return m_createInfo.type; }
    const std::vector<RenderItem>& GetRenderItems() const   { return m_meshNodes.GetRenderItems(); }

private:
    MeshNodes m_meshNodes;
    uint64_t m_objectId = 0;
    PictureFrameCreateInfo m_createInfo;
    std::vector<RenderItem> m_renderItems;
};
}
