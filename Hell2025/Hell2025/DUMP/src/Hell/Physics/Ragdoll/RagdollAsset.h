#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

using RagdollMarkerId = uint32_t;

inline constexpr RagdollMarkerId INVALID_RAGDOLL_MARKER_ID = 0;

enum class RagdollShapeType : uint8_t {
    BOX,
    SPHERE,
    CAPSULE,
    CONVEX_HULL
};

enum class RagdollAxisMotion : uint8_t {
    LOCKED,
    LIMITED,
    FREE
};

enum class RagdollCombineMode : uint8_t {
    AVERAGE,
    MINIMUM,
    MULTIPLY,
    MAXIMUM
};

enum class RagdollMassMode : uint8_t {
    AUTOMATIC,
    OVERRIDE
};

struct RagdollAxisLimit {
    RagdollAxisMotion motion = RagdollAxisMotion::LOCKED;
    float limit = 0.0f;
};

struct RagdollSolverAsset {
    float linearLimitStiffness = 1'000'000.0f;
    float linearLimitDamping = 10'000.0f;
    float angularLimitStiffness = 1'000'000.0f;
    float angularLimitDamping = 10'000.0f;
};

struct RagdollShape {
    RagdollShapeType type = RagdollShapeType::SPHERE;
    glm::vec3 extents = glm::vec3(1.0f);
    glm::vec3 offset = glm::vec3(0.0f);
    glm::vec3 rotationRadians = glm::vec3(0.0f);
    float length = 1.0f;
    float radius = 1.0f;
    std::vector<glm::vec3> convexVertices;
    std::vector<uint32_t> convexIndices;
};

struct RagdollRigidBodyAsset {
    bool isKinematic = false;
    bool enableCCD = false;
    RagdollMassMode massMode = RagdollMassMode::AUTOMATIC;
    float mass = 1.0f;
    float linearDamping = 0.5f;
    float angularDamping = 1.0f;
    float friction = 0.5f;
    float restitution = 0.1f;
    RagdollCombineMode restitutionCombineMode = RagdollCombineMode::MULTIPLY;
    RagdollCombineMode frictionCombineMode = RagdollCombineMode::MULTIPLY;
    float thickness = 0.02f;
    uint32_t positionIterations = 8;
    uint32_t velocityIterations = 1;
    float maxDepenetrationVelocity = -1.0f;
};

struct RagdollMarkerAsset {
    RagdollMarkerId id = INVALID_RAGDOLL_MARKER_ID;
    std::string name;
    std::string bonePath;
    std::string boneName;
    glm::mat4 inputTransform = glm::mat4(1.0f);
    glm::mat4 restTransform = glm::mat4(1.0f);
    glm::mat4 bodyTransform = glm::mat4(1.0f);
    glm::vec4 color = glm::vec4(1.0f);
    RagdollShape shape;
    RagdollRigidBodyAsset rigidBody;
};

struct RagdollJointAsset {
    std::string name;
    RagdollMarkerId parentMarkerId = INVALID_RAGDOLL_MARKER_ID;
    RagdollMarkerId childMarkerId = INVALID_RAGDOLL_MARKER_ID;
    glm::mat4 parentFrame = glm::mat4(1.0f);
    glm::mat4 childFrame = glm::mat4(1.0f);
    std::array<RagdollAxisLimit, 3> linearLimits;
    std::array<RagdollAxisLimit, 3> angularLimits;
    bool limitEnabled = true;
    float linearLimitStiffness = 0.0f;
    float linearLimitDamping = 0.0f;
    float angularLimitStiffness = 0.0f;
    float angularLimitDamping = 0.0f;
};

struct RagdollAsset {
    uint32_t version = 4;
    std::string name;
    std::string skinnedModelName;
    float skinnedModelScale = 1.0f;
    std::string testAnimationName;
    std::string testMaterialPresetName;
    uint64_t skeletonSignature = 0;
    float targetMass = 75.0f;
    RagdollSolverAsset solver;
    std::vector<RagdollMarkerAsset> markers;
    std::vector<RagdollJointAsset> joints;
};
