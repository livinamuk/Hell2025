#pragma once
#include "HellTypes.h"
#include <vector>
#include <glad/glad.h>

struct CloudPoint {
    glm::vec4 position = glm::vec4(0);
    glm::vec4 normal = glm::vec4(0);
    glm::vec4 directLighting = glm::vec4(0);
};

struct LightVolume {
    float m_worldSpaceWidth = 0.0f;
    float m_worldSpaceHeight = 0.0f;
    float m_worldSpaceDepth = 0.0f;
    int m_textureWidth = 0.0f;
    int m_textureHeight = 0.0f;
    int m_textureDepth = 0.0f;
    glm::vec3 m_offset = glm::vec3(0.0f);

    // OpenGL only (Figure out a way to make this more Vulkan friendly later!)
    GLuint m_lightVolumeTextures[2];
    GLuint m_lightVolumeA = 0;
    GLuint m_lightVolumeB = 0;
    GLuint m_lightVolumeMaskTexture = 0;
    int m_pingPongReadIndex = 0;
    int m_pingPongWriteIndex = 1;

    void Init(const std::vector<Vertex>& vertices, const glm::vec3& aabbMin, const glm::vec3& aabbMax);
    void CleanUp();
    GLuint GetLightingTextureHandle();
    GLuint GetMaskTextureHandle();
};

struct PointCloudOctrant {
    unsigned int m_cloudPointCount;
    unsigned int m_offset;
};

namespace GlobalIllumination {
    void Update();
    void SetGlobalIlluminationStructuresDirtyState(bool state);
    void SetPointCloudNeedsGpuUpdateState(bool state);

    bool GlobalIlluminationStructuresAreDirty();
    bool PointCloudNeedsGpuUpdate();

    float GetProbeSpacing();

    uint64_t GetSceneBvhId();
    const std::vector<BvhNode>& GetSceneNodes();
    std::vector<CloudPoint>& GetPointCloud();
    std::vector<LightVolume>& GetLightVolumes();
    std::vector<PointCloudOctrant>& GetPointCloudOctrants();
    std::vector<unsigned int>& GetPointIndices();
    glm::uvec3 GetPointCloudGridDimensions();
    glm::vec3 GetPointGridWorldMin();
    glm::vec3 GetPointGridWorldMax();
}
