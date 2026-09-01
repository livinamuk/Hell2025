#include "OpenableManager.h"

#include <atomic>
#include <cstdint>
#include <unordered_map>

namespace Unloved::OpenableManager {
    std::unordered_map<uint32_t, Openable> g_openables;
    std::atomic<uint32_t> g_global = 1;
    
    uint32_t GetNextID() {
        return g_global++;
    }

    uint32_t CreateOpenable(const OpenableCreateInfo& createInfo, uint64_t parentObjectId) {
        uint64_t openableId = GetNextID();
        Openable& openable = g_openables[openableId];
        openable.Init(createInfo, parentObjectId);

        return openableId;
    }

    void RemoveOpenable(uint32_t openableId) {
        g_openables.erase(openableId);
    }

    void Update(float deltaTime) {
        for (auto& pair : g_openables) {
            Openable& openable = pair.second;
            openable.Update(deltaTime);
        }
    }

    std::string TriggerInteract(uint32_t openableId, const glm::vec3& cameraPosition, const glm::vec3& cameraForward) {
        if (!OpenableExists(openableId)) return "";

        Openable* openable = GetOpenableByOpenableId(openableId);
        return openable->Interact(cameraPosition, cameraForward);
    }

    void LockOpenablebyId(uint32_t openableId) {
        if (!OpenableExists(openableId)) return;

        Openable* openable = GetOpenableByOpenableId(openableId);
        openable->m_locked = true;
    }

    void UnlockOpenablebyId(uint32_t openableId) {
        if (!OpenableExists(openableId)) return;

        Openable* openable = GetOpenableByOpenableId(openableId);
        openable->m_locked = false;
    }

    bool OpenableExists(uint32_t openableId) {
        return (g_openables.find(openableId) != g_openables.end());
    }

    bool IsInteractable(uint32_t openableId, const glm::vec3& viewPos) {
        if (Openable* openable = GetOpenableByOpenableId(openableId)) {
            return openable->IsInteractable(viewPos);
        }
        return false;
    }

    Openable* GetOpenableByOpenableId(uint32_t openableId) {
        if (OpenableExists(openableId)) {
            return &g_openables[openableId];
        }
        return nullptr;
    }
}
