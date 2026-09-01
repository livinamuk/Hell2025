#pragma once

#include "Hell/Physics/Physics.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"

namespace Unloved {

struct DoorChain {
    DoorChain() = default;
    DoorChain(uint64_t id, DoorChainCreateInfo& createInfo, SpawnOffset& spawnOffset);
    DoorChain(const DoorChain&) = delete;
    DoorChain& operator=(const DoorChain&) = delete;
    DoorChain(DoorChain&&) noexcept = default;
    DoorChain& operator=(DoorChain&&) noexcept = default;
    ~DoorChain() = default;

    void SubmitRenderItems();
    void Update(float deltaTime);
    void CleanUp();

private:
    uint64_t m_kinematicOriginPhysicsId = 0;
    std::vector<uint64_t> m_chainLinkPhysicsIds;

    uint32_t m_chainLinkMeshId = 0;
    uint32_t m_chainLinkEndMeshId = 0;
    
    bool m_animateFinalLink = false;
    glm::vec3 m_finalLinkPosition = glm::vec3(0.0f);
    PxQuat m_finalLinkRotation = PxQuat(PxIdentity);
};
}
