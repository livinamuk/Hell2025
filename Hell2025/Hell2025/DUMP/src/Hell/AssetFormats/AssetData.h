#pragma once

#include "Hell/BVH/Types.h"
#include "Hell/Common.h"
#include "Hell/Render/VertexAttributes.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <vector>

struct MeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    int32_t parentIndex = -1;
    glm::mat4 localTransform = glm::mat4(1.0f);
    glm::mat4 inverseBindTransform = glm::mat4(1.0f);
};

struct Bone {
    char name[64];
    glm::mat4 localRestPose = glm::mat4(1.0f);
    glm::mat4 inverseBindPose = glm::mat4(1.0f);
    int32_t parentIndex = -1;
    int32_t deformFlag = 0;
};

struct ArmatureData {
    std::string name = UNDEFINED_STRING;
    uint32_t boneCount = 0;
    std::vector<Bone> bones;
};

struct ModelData {
    std::string name;
    uint32_t meshCount = 0;
    uint32_t armatureCount = 0;
    uint64_t timestamp = 0;
    std::vector<MeshData> meshes;
    std::vector<ArmatureData> armatures;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
};

struct Node {
    std::string name;
    int parentIndex = -1;
    glm::mat4 localBindTransform = glm::mat4(1.0f);
};

struct MorphTargetVertexDelta {
    uint32_t vertexIndex = 0;
    glm::vec3 delta = glm::vec3(0.0f);
};

static_assert(sizeof(MorphTargetVertexDelta) == 16, "MorphTargetVertexDelta must match the OpenGL std430 MorphDelta layout");
static_assert(offsetof(MorphTargetVertexDelta, delta) == 4, "MorphTargetVertexDelta fields must match the OpenGL std430 MorphDelta layout");

struct MorphTargetData {
    std::string name;
    std::vector<MorphTargetVertexDelta> positionDeltas;
    std::vector<MorphTargetVertexDelta> normalDeltas;
    std::vector<MorphTargetVertexDelta> tangentDeltas;
};

struct SkinnedMeshData {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<VertexWeight> vertexWeights;
    std::vector<uint32_t> indices;
    std::vector<MorphTargetData> morphTargets;
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(-std::numeric_limits<float>::max());
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    bool requiresSkinning = false;
    int32_t nonDeformingBoneIndex = -1;
};

struct SkinnedModelData {
    std::string name;
    std::vector<SkinnedMeshData> meshes;
    std::vector<glm::mat4> boneOffsets;
    std::vector<Node> nodes;
    std::map<std::string, unsigned int> boneMapping;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint64_t timestamp = 0;

    uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshes.size()); }
    uint32_t GetNodeCount() const { return static_cast<uint32_t>(nodes.size()); }
    uint32_t GetBoneCount() const { return static_cast<uint32_t>(boneOffsets.size()); }
};

struct ModelBvhData {
    uint64_t timestamp = 0;
    std::vector<MeshBvh> bvhs;
};

struct HeightMapData {
    uint32_t textureWidth = 0;
    uint32_t textureHeight = 0;
    std::vector<float> data;
};
