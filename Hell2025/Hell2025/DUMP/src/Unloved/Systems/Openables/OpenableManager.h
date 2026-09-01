#pragma once
#include "Unloved/Systems/Openables/Openable.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

namespace Unloved::OpenableManager {
    uint32_t CreateOpenable(const OpenableCreateInfo& createInfo, uint64_t parentObjectId);
    void RemoveOpenable(uint32_t openableId);
    void Update(float deltaTime);
    std::string TriggerInteract(uint32_t openableId, const glm::vec3& cameraPosition, const glm::vec3& cameraForward);
    void LockOpenablebyId(uint32_t openableId);
    void UnlockOpenablebyId(uint32_t openableId);

    bool OpenableExists(uint32_t openableId);
    bool IsInteractable(uint32_t openableId, const glm::vec3& viewPos);
    Openable* GetOpenableByOpenableId(uint32_t openableId);
}
