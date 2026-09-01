#pragma once
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace Unloved {

    struct Jetty {
        Jetty() = default;
        Jetty(uint64_t id, JettyCreateInfo& createInfo, SpawnOffset& spawnOffset);
        Jetty(const Jetty&) = delete;
        Jetty& operator=(const Jetty&) = delete;
        Jetty(Jetty&&) noexcept = default;
        Jetty& operator=(Jetty&&) noexcept = default;
        ~Jetty() = default;

        void RecreateAll();
        void CleanUp();
        void Update();

        void SetPosition(const glm::vec3& position);
        void SetRotation(const glm::vec3& rotation);
        void SetScale(const glm::vec3& scale);
        void SetBoardCount(uint32_t boardCount);

        const std::vector<RenderItem>& const GetRenderItems() { return m_renderItems; }
        const uint64_t GetObjectId() const                    { return m_objectId; }
        const JettyCreateInfo& GetCreateInfo() const          { return m_createInfo; }
        const glm::vec3& GetPosition() const                  { return m_createInfo.position; }
        const glm::vec3& GetRotation() const                  { return m_createInfo.rotation; }
        const glm::vec3& GetScale() const                     { return m_createInfo.scale; }
        uint32_t GetBoardCount() const                        { return m_createInfo.boardCount; }

        const glm::vec3& GetWorldSpaceCenter() const          { return m_worldSpaceCenter; }

    private:
        void CreateBoardRenderItems();
        void CreatePoleRenderItems();
        void CreatePhysicsbjects();

        uint64_t m_objectId = 0;
        JettyCreateInfo m_createInfo;

        glm::mat4 m_modelMatrtix;
        std::vector<RenderItem> m_renderItems;

        uint64_t m_rigidStaticId = 0;

        glm::vec3 m_worldSpaceCenter = glm::vec3(0.0f);
    };
}
