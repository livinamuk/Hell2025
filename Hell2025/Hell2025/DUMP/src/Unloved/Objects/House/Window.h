#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Systems/House/ClippingVolume.h"

namespace Unloved {

struct Window {
    Window() = default;
    Window(uint64_t id, const WindowCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) noexcept = default;
    Window& operator=(Window&&) noexcept = default;
    ~Window() = default;

    void Update(float deltaTime);
    void CleanUp();
    void SetPosition(const glm::vec3& position); 
    void SetRotation(const glm::vec3& rotation);
    void SetRotationY(float value);
    
    const uint64_t GetObjectId() const                          { return m_objectId; }
    const glm::vec3& GetPosition() const                        { return m_transform.position; }
    const glm::vec3& GetRotation() const                        { return m_transform.rotation; }
    const std::vector<RenderItem>& GetRenderItems() const       { return m_meshNodes.GetRenderItems(); }
    const WindowCreateInfo& GetCreateInfo() const               { return m_createInfo; }
    const ClippingVolume& GetClippingVolume() const             { return m_clippingVolume; }
    MeshNodes& GetMeshNodes()                                   { return m_meshNodes; }

private:
    void UpdateClippingVolume();

    uint64_t m_objectId = 0;
    uint64_t m_physicsId = 0;
    MeshNodes m_meshNodes;
    Hell::Transform m_transform;
    ClippingVolume m_clippingVolume;
    WindowCreateInfo m_createInfo;
};
}
