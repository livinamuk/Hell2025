#include "World.h"

#include "Unloved/Maps/Map.h"
#include "Unloved/Systems/Ocean/Ocean.h"

#include <glm/glm.hpp>

namespace Unloved::World {
    static std::vector<Map> g_maps;
    glm::vec3 g_moonlightDirection = glm::normalize(glm::vec3(-0.5f, 0.2f, 0.0f));

    std::vector<Map>& GetMaps() {
        return g_maps;
    }

    bool HasLoadedMap() {
        return !g_maps.empty();
    }

    bool HasOcean() {
        return HasLoadedMap();
    }

    void RefreshOceanPhysics() {
        if (HasOcean()) Ocean::CreatePhysicsPlane();
        else            Ocean::DestroyPhysicsPlane();
    }

    void UpdateEnvironment() {
        g_moonlightDirection = glm::normalize(glm::vec3(-0.5f, 0.2f, 0.0f));
    }

    const glm::vec3& GetMoonlightDirection() {
        return g_moonlightDirection;
    }
}
