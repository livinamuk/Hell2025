#pragma once
#include "Unloved/Common/Types.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"

namespace Unloved {

enum struct DeadLockType {
    BOLT,
    UNDEFINED,
};

struct DeadLock {
    void Init(uint64_t parentDoorId, const glm::vec3& localOffset, DeadLockType type);
    void Update(float deltaTime);
    void CleanUp();

    glm::vec3 m_localOffset = glm::vec3(0.0f);
    DeadLockType m_type = DeadLockType::BOLT;
    MeshNodes m_meshNodes;
    uint64_t m_parentDoorId = 0;

    glm::mat4 m_doorTransform;
    glm::mat4 m_doorFrameTransform;
};
}
