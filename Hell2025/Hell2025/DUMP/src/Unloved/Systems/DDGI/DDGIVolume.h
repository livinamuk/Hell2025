#pragma once

#include "Hell/BVH/Types.h"

#include "Unloved/Common/Types.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Systems/DDGI/DDGITypes.h"
#include "Unloved/Systems/DDGI/PointCloud.h"

#include <cstdint>
#include <string>

namespace Unloved {

namespace CoarseWorldBVH {
    struct HouseGeometry;
}

struct DDGIVolume {
    DDGIVolume() = default;
    DDGIVolume(uint64_t id, DDGIVolumeCreateInfo& createInfo, SpawnOffset& spawnOffset);
    DDGIVolume(const DDGIVolume&) = delete;
    DDGIVolume& operator=(const DDGIVolume&) = delete;
    DDGIVolume(DDGIVolume&&) noexcept = default;
    DDGIVolume& operator=(DDGIVolume&&) noexcept = default;
    ~DDGIVolume() = default;

    void Init(const glm::vec3& aabbMin, const glm::vec3& aabbMax, float probeSpacing);
    void Update();
    void CleanUp();
    
    void SetEditorName(const std::string& name);
    void SetPosition(const glm::vec3& position);
    void SetOrigin(const glm::vec3& origin);
    void SetRotation(const glm::vec3& rotation);
    void SetExtents(const glm::vec3& extents);
    void SetProbeSpacing(float spacing);
    void SetPointCloudSpacing(float spacing);
    void DebugDraw();
    void RebuildDDGIGeometry();
    void UpdateDDGISceneBvh();

    void MarkPointCloudAsUploaded() { m_pointCloudNeedsGpuUpload = false; }
    void MarkProbesUpdated()        { m_framesSinceLastProbeUpdate = 0; }
    void SetProbeOffset(uint32_t offset) { m_probeOffset = offset; }

    uint32_t GetTotalProbeCount() const;
    DDGIVolumeGPU GetGPUData() const;
    const std::vector<BvhNode>& GetSceneNodes();

    uint64_t& GetObjectId()                                                    { return m_id; }
    DDGIVolumeCreateInfo& GetCreateInfo()                                      { return m_createInfo; }
    const PointCloud& GetPointClound() const                                   { return m_pointCloud; }
    const std::vector<CloudPoint>& GetPointCloundPoints() const                { return m_pointCloud.GetPoints(); }
    const std::vector<CloudPointTextureInfo>& GetPointCloudTextureInfo() const { return m_pointCloud.GetTextureInfo(); }
    const std::string& GetEditorName() const                                   { return m_createInfo.editorName; }
    const glm::vec3& GetPosition() const                                       { return GetOrigin(); }
    const glm::vec3& GetOrigin() const                                         { return m_createInfo.origin; }
    const glm::vec3& GetRotation() const                                       { return m_createInfo.rotation; }
    const glm::vec3& GetExtents() const                                        { return m_createInfo.extents; }
    const glm::vec3 GetBoundsMin() const                                       { return m_boundsMin; }
    const glm::vec3 GetBoundsMax() const                                       { return m_boundsMax; }
    float GetWorldSpaceWidth() const                                           { return m_worldSpaceWidth; }
    float GetWorldSpaceHeight() const                                          { return m_worldSpaceHeight; }
    float GetWorldSpaceDepth() const                                           { return m_worldSpaceDepth; }
    float GetProbeSpacing() const                                              { return m_createInfo.probeSpacing; }
    float GetPointCloudSpacing() const                                         { return m_createInfo.pointCloudSpacing; }
    int GetProbeCountX() const                                                 { return m_probeCountX; }
    int GetProbeCountY() const                                                 { return m_probeCountY; }
    int GetProbeCountZ() const                                                 { return m_probeCountZ; }
    bool PointCloudNeedsGPUUpload() const                                      { return m_pointCloudNeedsGpuUpload; }
    uint64_t GetSceneBvhId() const                                             { return m_sceneBvhId; }
    uint32_t GetPointCloudCount() const                                        { return m_pointCloud.GetPointCount(); }
    uint32_t GetProbePointIndexPoolSize() const                                { return m_probePointIndexPoolSize; }
    uint32_t GetFramesSinceLastProbeUpdate() const                             { return m_framesSinceLastProbeUpdate; }
    uint32_t GetProbeOffset() const                                            { return m_probeOffset; }
    const std::string& GetProbeDistanceTextureArrayName() const                { return m_probeDistanceTextureArrayName; }
    const std::string& GetProbeIrradianceTextureArrayName() const              { return m_probeIrradianceTextureArrayName; }
    const std::string& GetPointCloudSSBOName() const                           { return m_pointCloudSSBOName; }
    const std::string& GetPointCloudDirtyFlagsSSBOName() const                 { return m_pointCloudDirtyFlagsSSBOName; }
    const std::string& GetPointCloudTextureInfoSSBOName() const                { return m_pointCloudTextureInfoSSBOName; }
    const std::string& GetPointCloudGridOffsetsSSBOName() const                { return m_pointCloudGridOffsetsSSBOName; }
    const std::string& GetPointCloudGridCountsSSBOName() const                 { return m_pointCloudGridCountsSSBOName; }
    const std::string& GetProbePointIndicesSSBOName() const                    { return m_probePointIndicesSSBOName; }
    const std::string& GetProbePointOffsetsSSBOName() const                    { return m_probePointOffsetsSSBOName; }
    const std::string& GetProbePointCountsSSBOName() const                     { return m_probePointCountsSSBOName; }
    const std::string& GetSceneBvhSSBOName() const                             { return m_sceneBvhSSBOName; }
    const std::string& GetMeshesBvhSSBOName() const                            { return m_meshesBvhSSBOName; }
    const std::string& GetTriangleDataSSBOName() const                         { return m_triangleDataSSBOName; }
    const std::string& GetEntityInstancesSSBOName() const                      { return m_entityInstancesSSBOName; }
    const std::string& GetRayQueryDescriptorSetName() const                    { return m_rayQueryDescriptorSetName; }

private:
    void CreateGpuResourceNames();
    void CreateProbeTextureArrays();
    void CleanUpProbeTextureArrays();
    void CleanUpSSBOs();
    void UpdateMembers();
    void CleanUpDDGIGeometry();
    void RebuildPointCloudSeedTriangles(const CoarseWorldBVH::HouseGeometry& houseGeometry);
    void RebuildDDGIHouseBvh(const CoarseWorldBVH::HouseGeometry& houseGeometry);
    void RebuildPointCloud();
    void CalculateProbePointIndexPoolSize();
    glm::vec3 GetProbeBaseWorldPosition(const glm::ivec3& probeCoords) const;

    uint64_t m_id = 0;
    DDGIVolumeCreateInfo m_createInfo;

    glm::vec3 m_boundsMin = glm::vec3(0.0f);
    glm::vec3 m_boundsMax = glm::vec3(0.0f);
    float m_worldSpaceWidth = 0.0f;
    float m_worldSpaceHeight = 0.0f;
    float m_worldSpaceDepth = 0.0f;
    int m_probeCountX = 0;
    int m_probeCountY = 0;
    int m_probeCountZ = 0;
    bool m_pointCloudNeedsGpuUpload = false;
    bool m_ddgiGeometryDirty = false;
    uint32_t m_framesSinceLastProbeUpdate = 0;
    uint32_t m_probeOffset = 0;

    std::vector<Triangle> m_pointCloudSeedTriangles;
    PointCloud m_pointCloud;

    uint64_t m_houseBvhId = 0;
    uint64_t m_sceneBvhId = 0;

    uint32_t m_probePointIndexPoolSize = 0;
    std::string m_probeDistanceTextureArrayName;
    std::string m_probeIrradianceTextureArrayName;
    std::string m_pointCloudSSBOName;
    std::string m_pointCloudDirtyFlagsSSBOName;
    std::string m_pointCloudTextureInfoSSBOName;
    std::string m_pointCloudGridOffsetsSSBOName;
    std::string m_pointCloudGridCountsSSBOName;
    std::string m_probePointIndicesSSBOName;
    std::string m_probePointOffsetsSSBOName;
    std::string m_probePointCountsSSBOName;
    std::string m_sceneBvhSSBOName;
    std::string m_meshesBvhSSBOName;
    std::string m_triangleDataSSBOName;
    std::string m_entityInstancesSSBOName;
    std::string m_rayQueryDescriptorSetName;
};

}
