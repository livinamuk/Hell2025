#pragma once

#include "AssetData.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string>

namespace Hell::AssetFormats {

    inline constexpr uint32_t SKINNED_MODEL_VERSION = 4;

    struct ModelMetadata {
        uint32_t version = 0;
        uint32_t meshCount = 0;
        uint32_t armatureCount = 0;
        uint64_t timestamp = 0;
        glm::vec3 aabbMin = glm::vec3(0.0f);
        glm::vec3 aabbMax = glm::vec3(0.0f);
    };

    struct ModelBvhMetadata {
        uint64_t version = 0;
        uint64_t meshCount = 0;
        uint64_t timestamp = 0;
    };

    struct SkinnedModelMetadata {
        uint32_t version = 0;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t meshCount = 0;
        uint32_t nodeCount = 0;
        uint32_t boneCount = 0;
        uint64_t timestamp = 0;
    };

    bool LoadModel(const std::string& path, ModelData& outModel);
    bool SaveModel(const std::string& path, const ModelData& model);
    bool ReadModelMetadata(const std::string& path, ModelMetadata& outMetadata);

    bool LoadModelBvh(const std::string& path, ModelBvhData& outBvh);
    bool SaveModelBvh(const std::string& path, const ModelBvhData& bvh);
    bool ReadModelBvhMetadata(const std::string& path, ModelBvhMetadata& outMetadata);

    bool LoadSkinnedModel(const std::string& path, SkinnedModelData& outModel);
    bool SaveSkinnedModel(const std::string& path, const SkinnedModelData& model);
    bool ReadSkinnedModelMetadata(const std::string& path, SkinnedModelMetadata& outMetadata);

    bool LoadHeightMap(const std::string& path, HeightMapData& outHeightMap);
    bool SaveHeightMap(const std::string& path, const HeightMapData& heightMap);
}
