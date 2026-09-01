#include "Hell/Physics/PhysicsIds.h"

#include <atomic>

namespace {
    std::atomic<uint64_t> g_nextPhysicsId { 1 };

    uint64_t MakePhysicsId(Hell::Physics::PhysicsObjectType type, uint64_t local) {
        return (uint64_t(static_cast<uint16_t>(type)) << Hell::Physics::PHYSICS_ID_TYPE_SHIFT) |
               (local & Hell::Physics::PHYSICS_ID_LOCAL_MASK);
    }
}

namespace Hell::Physics {

    uint64_t CreatePhysicsId(PhysicsObjectType type) {
        return MakePhysicsId(type, g_nextPhysicsId++);
    }

    PhysicsObjectType GetPhysicsObjectType(uint64_t physicsId) {
        return static_cast<PhysicsObjectType>((physicsId >> PHYSICS_ID_TYPE_SHIFT) & ((1ull << PHYSICS_ID_TYPE_BITS) - 1));
    }

    uint64_t GetPhysicsObjectIndex(uint64_t physicsId) {
        return physicsId & PHYSICS_ID_LOCAL_MASK;
    }

    bool PhysicsIdHasType(uint64_t physicsId, PhysicsObjectType type) {
        return GetPhysicsObjectType(physicsId) == type;
    }

    PhysicsType ToPhysicsType(PhysicsObjectType type) {
        switch (type) {
            case PhysicsObjectType::RIGID_DYNAMIC:        return PhysicsType::RIGID_DYNAMIC;
            case PhysicsObjectType::RIGID_STATIC:         return PhysicsType::RIGID_STATIC;
            case PhysicsObjectType::HEIGHT_FIELD:         return PhysicsType::HEIGHT_FIELD;
            case PhysicsObjectType::GROUND_PLANE:         return PhysicsType::GROUND_PLANE;
            case PhysicsObjectType::CHARACTER_CONTROLLER: return PhysicsType::CHARACTER_CONTROLLER;
            case PhysicsObjectType::NONE:                 return PhysicsType::NONE;
            default:                                      return PhysicsType::UNDEFINED;
        }
    }

    PhysicsObjectType ToPhysicsObjectType(PhysicsType type) {
        switch (type) {
            case PhysicsType::RIGID_DYNAMIC:        return PhysicsObjectType::RIGID_DYNAMIC;
            case PhysicsType::RIGID_STATIC:         return PhysicsObjectType::RIGID_STATIC;
            case PhysicsType::HEIGHT_FIELD:         return PhysicsObjectType::HEIGHT_FIELD;
            case PhysicsType::GROUND_PLANE:         return PhysicsObjectType::GROUND_PLANE;
            case PhysicsType::CHARACTER_CONTROLLER: return PhysicsObjectType::CHARACTER_CONTROLLER;
            case PhysicsType::NONE:                 return PhysicsObjectType::NONE;
            default:                                return PhysicsObjectType::NONE;
        }
    }
}
