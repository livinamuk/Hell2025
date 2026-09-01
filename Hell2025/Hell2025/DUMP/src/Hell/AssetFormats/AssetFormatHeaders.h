#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>

namespace Hell::AssetFormats {

    inline constexpr char MODEL_SIGNATURE[] = "HELL_MODEL";
    inline constexpr char MESH_SIGNATURE[] = "HELL_MESH";
    inline constexpr char MODEL_BVH_SIGNATURE[] = "HELL_MODEL_BVH";
    inline constexpr char MESH_BVH_SIGNATURE[] = "HELL_MESH_BVH";
    inline constexpr char HEIGHT_MAP_SIGNATURE[] = "HELL_HEIGHT_MAP";
    inline constexpr char ANIMATION_SIGNATURE[] = "HELL_ANIMATION";

    inline constexpr size_t SIGNATURE_BUFFER_SIZE = 32;
    inline constexpr size_t NAME_BUFFER_SIZE = 256;

#pragma pack(push, 1)

    struct ModelHeaderV2 {
        char signature[SIGNATURE_BUFFER_SIZE];
        uint32_t version;
        uint32_t meshCount;
        uint64_t timestamp;
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
    };

    struct ModelHeaderV3 {
        char signature[SIGNATURE_BUFFER_SIZE];
        uint32_t version;
        uint32_t meshCount;
        uint32_t armatureCount;
        uint64_t timestamp;
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
    };

    struct MeshHeaderV2 {
        char signature[SIGNATURE_BUFFER_SIZE];
        char name[NAME_BUFFER_SIZE];
        uint32_t vertexCount;
        uint32_t indexCount;
        int32_t parentIndex;
        glm::vec3 aabbMin;
        glm::vec3 aabbMax;
        glm::mat4 localTransform;
        glm::mat4 inverseBindTransform;
    };

    struct ArmatureHeader {
        char signature[SIGNATURE_BUFFER_SIZE];
        char name[NAME_BUFFER_SIZE];
        uint32_t boneCount;
    };

    struct ModelBvhHeader {
        char signature[SIGNATURE_BUFFER_SIZE];
        uint64_t version;
        uint64_t meshCount;
        uint64_t timestamp;
    };

    struct MeshBvhHeader {
        char signature[SIGNATURE_BUFFER_SIZE];
        uint64_t floatCount;
        uint64_t nodeCount;
    };

    struct SkinnedModelHeader {
        char signature[18];
        uint32_t version;
        uint32_t nameLength;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t meshCount;
        uint32_t nodeCount;
        uint32_t boneCount;
        uint64_t timestamp;
    };

    struct SkinnedModelMorphSectionHeader {
        char signature[16];
        uint32_t meshCount;
        uint32_t targetCount;
        uint64_t positionDeltaCount;
        uint64_t normalDeltaCount;
        uint64_t tangentDeltaCount;
    };

    struct AnimationHeader {
        char signature[SIGNATURE_BUFFER_SIZE];
        uint32_t version;
        uint32_t channelCount;
        float duration;
        uint64_t timestamp;
    };

    struct HeightMapHeader {
        char signature[SIGNATURE_BUFFER_SIZE];
        char name[NAME_BUFFER_SIZE];
        uint32_t width;
        uint32_t height;
    };

#pragma pack(pop)
}
