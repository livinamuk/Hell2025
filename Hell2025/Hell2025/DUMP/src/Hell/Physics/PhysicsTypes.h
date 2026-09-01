#pragma once

#include "Hell/Common/Constants.h"

#include <cstdint>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <limits>
#include <string>
#include <vector>

enum struct PhysicsType {
    NONE = 0,
    RIGID_DYNAMIC,
    RIGID_STATIC,
    HEIGHT_FIELD,
    GROUND_PLANE,
    CHARACTER_CONTROLLER,
    UNDEFINED
};

enum CollisionGroup : uint64_t {
    NO_COLLISION = 0,
    BULLET_CASING = 1,
    CHARACTER_CONTROLLER = 2,
    ENVIROMENT_OBSTACLE = 4,
    GENERIC_BOUNCEABLE = 8,
    ITEM_PICK_UP = 16,
    RAGDOLL_PLAYER = 32,
    DOG_CHARACTER_CONTROLLER = 64,
    GENERTIC_INTERACTBLE = 128,
    ENVIROMENT_OBSTACLE_NO_DOG = 256,
    SHARK = 512,
    LADDER = 1024,
    RAGDOLL_ENEMY = 2048
};

// Re-evaluate how this works, coz it alway fucks you up,
// and PhysX this group bitmask is used for more than just raycasts, pretty sure
enum RaycastGroup {
    RAYCAST_DISABLED = 0,
    RAYCAST_ENABLED = 1,
    DOBERMAN = 32
};

inline constexpr uint32_t RAGDOLL_SELF_COLLISION_FILTER_TAG = 0xa5000000;
inline constexpr uint32_t RAGDOLL_SELF_COLLISION_FILTER_TAG_MASK = 0xff000000;

struct PhysicsFilterData {
    RaycastGroup raycastGroup = RaycastGroup::RAYCAST_DISABLED;
    CollisionGroup collisionGroup = CollisionGroup::NO_COLLISION;
    CollisionGroup collidesWith = CollisionGroup::ENVIROMENT_OBSTACLE;
};

struct PhysicsUserData {
    uint64_t physicsId = 0;
    uint64_t objectId = 0;
    PhysicsType physicsType = PhysicsType::NONE; // REDUNDANT: this is now bakedc into physicsId. Check nothi8ng reference sme then remove this
};

struct PhysXRayResult {
    PhysicsUserData userData;
    std::string hitObjectName = UNDEFINED_STRING;
    glm::vec3 hitPosition = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
    glm::vec3 rayDirection = glm::vec3(0.0f);
    bool hitFound = false;
    float distanceToHit = std::numeric_limits<float>::max();
};

struct PhysicsContactResult {
    bool hitFound = false;
    glm::vec3 hitPosition = glm::vec3(0.0f);
    glm::vec3 hitNormal = glm::vec3(0.0f);
};

struct PhysXOverlapResult {
    PhysicsUserData userData;
    glm::vec3 objectPosition;
};

struct PhysXOverlapReport {
    std::vector<PhysXOverlapResult> hits;

    bool HitsFound() {
        return hits.size();
    }
};

namespace Hell::Physics {

    enum struct DebugMode {
        NONE,
        ALL,
        RAYCAST_SHAPES,
        COLLISION_SHAPES,
        RAGDOLLS
    };

    struct PhysicsDebugLine {
        glm::vec3 p1 = glm::vec3(0.0f);
        glm::vec3 p2 = glm::vec3(0.0f);
        glm::vec4 color = glm::vec4(1.0f);
    };
}
