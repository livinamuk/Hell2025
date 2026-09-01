#pragma once

#include "Hell/Physics/PhysicsTypes.h"

#include <cstdint>

namespace Hell::Physics {

    enum struct PhysicsObjectType : uint16_t {
        NONE = 0,
        RIGID_DYNAMIC,
        RIGID_STATIC,
        HEIGHT_FIELD,
        GROUND_PLANE,
        CHARACTER_CONTROLLER,
        D6_JOINT,
        RAGDOLL
    };

    inline constexpr uint32_t PHYSICS_ID_TYPE_BITS = 16;
    inline constexpr uint64_t PHYSICS_ID_TYPE_SHIFT = 64 - PHYSICS_ID_TYPE_BITS;
    inline constexpr uint64_t PHYSICS_ID_TYPE_MASK = ((1ull << PHYSICS_ID_TYPE_BITS) - 1) << PHYSICS_ID_TYPE_SHIFT;
    inline constexpr uint64_t PHYSICS_ID_LOCAL_MASK = ~PHYSICS_ID_TYPE_MASK;

    uint64_t CreatePhysicsId(PhysicsObjectType type);
    PhysicsObjectType GetPhysicsObjectType(uint64_t physicsId);
    uint64_t GetPhysicsObjectIndex(uint64_t physicsId);
    bool PhysicsIdHasType(uint64_t physicsId, PhysicsObjectType type);

    PhysicsType ToPhysicsType(PhysicsObjectType type);
    PhysicsObjectType ToPhysicsObjectType(PhysicsType type);
}
