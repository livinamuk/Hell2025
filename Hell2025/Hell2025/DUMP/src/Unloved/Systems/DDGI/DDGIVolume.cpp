#include "DDGIVolume.h"

#include "Hell/BVH/BVH.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVHGeometry.h"

#include <limits>

namespace Unloved {

DDGIVolume::DDGIVolume(uint64_t id, DDGIVolumeCreateInfo& createInfo, SpawnOffset& spawnOffset) {
    m_id = id;
    m_createInfo = createInfo;
    m_createInfo.origin += spawnOffset.translation;
    m_createInfo.rotation += glm::vec3(0.0f, spawnOffset.yRotation, 0.0f);

    CreateGpuResourceNames();
    CreateProbeTextureArrays();
    UpdateMembers();
}

void DDGIVolume::Update() {
    if (m_framesSinceLastProbeUpdate != std::numeric_limits<uint32_t>::max()) {
        m_framesSinceLastProbeUpdate++;
    }

    if (m_ddgiGeometryDirty) {
        RebuildDDGIGeometry();
        m_ddgiGeometryDirty = false;
    }

    m_pointCloud.Update();
}

void DDGIVolume::CleanUp() {
    CleanUpDDGIGeometry();
    CleanUpProbeTextureArrays();
    CleanUpSSBOs();
}

void DDGIVolume::CreateGpuResourceNames() {
    const std::string baseName = "DDGI_" + std::to_string(m_id) + "_";

    m_probeDistanceTextureArrayName = baseName + "ProbeDistance";
    m_probeIrradianceTextureArrayName = baseName + "ProbeIrradiance";
    m_pointCloudSSBOName = baseName + "PointCloud";
    m_pointCloudDirtyFlagsSSBOName = baseName + "PointCloudDirtyFlags";
    m_pointCloudTextureInfoSSBOName = baseName + "PointCloudTextureInfo";
    m_pointCloudGridOffsetsSSBOName = baseName + "PointCloudGridOffsets";
    m_pointCloudGridCountsSSBOName = baseName + "PointCloudGridCounts";
    m_probePointIndicesSSBOName = baseName + "ProbePointIndices";
    m_probePointOffsetsSSBOName = baseName + "ProbePointOffsets";
    m_probePointCountsSSBOName = baseName + "ProbePointCounts";
    m_sceneBvhSSBOName = baseName + "SceneBvh";
    m_meshesBvhSSBOName = baseName + "MeshesBvh";
    m_triangleDataSSBOName = baseName + "TriangleData";
    m_entityInstancesSSBOName = baseName + "EntityInstances";
    m_rayQueryDescriptorSetName = baseName + "RayQueryDescriptorSet";
}

void DDGIVolume::CreateProbeTextureArrays() {
    Hell::ResourceManager::CreateTextureArray(m_probeDistanceTextureArrayName);
    Hell::ResourceManager::CreateTextureArray(m_probeIrradianceTextureArrayName);
}

void DDGIVolume::CleanUpProbeTextureArrays() {
    if (!m_probeDistanceTextureArrayName.empty()) {
        Hell::ResourceManager::RemoveTextureArrayByName(m_probeDistanceTextureArrayName);
        m_probeDistanceTextureArrayName.clear();
    }

    if (!m_probeIrradianceTextureArrayName.empty()) {
        Hell::ResourceManager::RemoveTextureArrayByName(m_probeIrradianceTextureArrayName);
        m_probeIrradianceTextureArrayName.clear();
    }
}

void DDGIVolume::CleanUpSSBOs() {
    OpenGL::ResourceManager::RemoveSSBOByName(m_pointCloudSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_pointCloudDirtyFlagsSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_pointCloudTextureInfoSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_pointCloudGridOffsetsSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_pointCloudGridCountsSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_probePointIndicesSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_probePointOffsetsSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_probePointCountsSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_sceneBvhSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_meshesBvhSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_triangleDataSSBOName);
    OpenGL::ResourceManager::RemoveSSBOByName(m_entityInstancesSSBOName);

    VulkanResourceManager::RemoveBuffer(m_pointCloudSSBOName);
    VulkanResourceManager::RemoveBuffer(m_pointCloudDirtyFlagsSSBOName);
    VulkanResourceManager::RemoveBuffer(m_pointCloudTextureInfoSSBOName);
    VulkanResourceManager::RemoveBuffer(m_pointCloudGridOffsetsSSBOName);
    VulkanResourceManager::RemoveBuffer(m_pointCloudGridCountsSSBOName);
    VulkanResourceManager::RemoveBuffer(m_probePointIndicesSSBOName);
    VulkanResourceManager::RemoveBuffer(m_probePointOffsetsSSBOName);
    VulkanResourceManager::RemoveBuffer(m_probePointCountsSSBOName);
    VulkanResourceManager::RemoveDescriptorSet(m_rayQueryDescriptorSetName);
    VulkanRenderer::DestroyDDGIRayQueryScene(m_id);
}

void DDGIVolume::CleanUpDDGIGeometry() {
    Hell::Bvh::DestroyMeshBvh(m_houseBvhId);
    Hell::Bvh::DestroySceneBvh(m_sceneBvhId);

    m_houseBvhId = 0;
    m_sceneBvhId = 0;
    m_probePointIndexPoolSize = 0;

    m_pointCloudSeedTriangles.clear();
    m_pointCloud.CleanUp();

}

void DDGIVolume::RebuildDDGIGeometry() {
    CleanUpDDGIGeometry();

    CoarseWorldBVH::HouseGeometry houseGeometry = CoarseWorldBVH::BuildHouseGeometry(m_boundsMin, m_boundsMax);
    RebuildPointCloudSeedTriangles(houseGeometry);
    RebuildDDGIHouseBvh(houseGeometry);

    m_sceneBvhId = Hell::Bvh::CreateSceneBvh();

    if (SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId)) {
        std::vector<SceneBvhMeshInput> meshBvhs;

        if (MeshBvh* houseMeshBvh = Hell::Bvh::GetMeshBvhById(m_houseBvhId)) {
            meshBvhs.push_back({ m_houseBvhId, houseMeshBvh });
        }

        const uint64_t doorProxyBvhId = CoarseWorldBVH::GetDoorProxyBvhId();
        if (MeshBvh* doorMeshBvh = Hell::Bvh::GetMeshBvhById(doorProxyBvhId)) {
            meshBvhs.push_back({ doorProxyBvhId, doorMeshBvh });
        }

        sceneBvh->AddMeshBvhs(meshBvhs);
    }

    RebuildPointCloud();
    CalculateProbePointIndexPoolSize();
}

void DDGIVolume::RebuildPointCloudSeedTriangles(const CoarseWorldBVH::HouseGeometry& houseGeometry) {
    m_pointCloudSeedTriangles.clear();
    m_pointCloudSeedTriangles.reserve(houseGeometry.surfaceTriangles.size());

    for (const CoarseWorldBVH::SurfaceTriangle& sourceTriangle : houseGeometry.surfaceTriangles) {
        Triangle& triangle = m_pointCloudSeedTriangles.emplace_back();
        triangle.v0 = sourceTriangle.v0;
        triangle.v1 = sourceTriangle.v1;
        triangle.v2 = sourceTriangle.v2;
        triangle.uv0 = sourceTriangle.uv0;
        triangle.uv1 = sourceTriangle.uv1;
        triangle.uv2 = sourceTriangle.uv2;
        triangle.normal = sourceTriangle.normal;
        triangle.baseColorTextureIndex = sourceTriangle.baseColorTextureIndex;
        triangle.rmaTextureIndex = sourceTriangle.rmaTextureIndex;
    }

}

void DDGIVolume::RebuildDDGIHouseBvh(const CoarseWorldBVH::HouseGeometry& houseGeometry) {
    if (m_houseBvhId != 0) {
        Hell::Bvh::DestroyMeshBvh(m_houseBvhId);
        m_houseBvhId = 0;
    }

    if (houseGeometry.vertices.empty() || houseGeometry.indices.empty()) {
        return;
    }

    m_houseBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(houseGeometry.vertices, houseGeometry.indices);
}

void DDGIVolume::RebuildPointCloud() {
    m_pointCloud.Create(m_pointCloudSeedTriangles, GetBoundsMin(), GetBoundsMax(), GetPointCloudSpacing(), 3.0f);
    m_pointCloudNeedsGpuUpload = true;
}

void DDGIVolume::CalculateProbePointIndexPoolSize() {
    const PointCloud& pointCloud = GetPointClound();
    const glm::ivec3 gridDims = pointCloud.GetGridDimensions();
    const float gridCellSize = pointCloud.GetGridCellSize();
    const std::vector<uint32_t>& gridCellCounts = pointCloud.GetGridCellCounts();

    // Calculate how many probes originate in a single point-grid cell
    float probesPerAxis = gridCellSize / GetProbeSpacing();
    uint32_t probesPerCell = static_cast<uint32_t>(std::ceil(probesPerAxis * probesPerAxis * probesPerAxis));

    m_probePointIndexPoolSize = 0;

    // Map wide density scan
    for (int z = 0; z < gridDims.z; ++z) {
        for (int y = 0; y < gridDims.y; ++y) {
            for (int x = 0; x < gridDims.x; ++x) {

                uint32_t pointsIn27Cells = 0;

                // Sum all points in the 3x3x3 neighborhood of this cell
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dy = -1; dy <= 1; ++dy) {
                        for (int dx = -1; dx <= 1; ++dx) {
                            int nx = x + dx;
                            int ny = y + dy;
                            int nz = z + dz;

                            if (nx >= 0 && nx < gridDims.x &&
                                ny >= 0 && ny < gridDims.y &&
                                nz >= 0 && nz < gridDims.z) {

                                // Flatten the 3D coords to get the point count for this specific cell
                                int cellIdx = nx + ny * gridDims.x + nz * gridDims.x * gridDims.y;
                                pointsIn27Cells += gridCellCounts[cellIdx];
                            }
                        }
                    }
                }

                // Every probe that could possibly "start" in this cell is allocated the full point-count of its neighborhood
                m_probePointIndexPoolSize += (pointsIn27Cells * probesPerCell);
            }
        }
    }
}

void DDGIVolume::UpdateDDGISceneBvh() {
    std::vector<PrimitiveInstance> instances;

    MeshBvh* houseMeshBvh = Hell::Bvh::GetMeshBvhById(m_houseBvhId);
    if (!houseMeshBvh || houseMeshBvh->m_nodes.empty()) {
        return;
    }

    // Add the house
    PrimitiveInstance& instance = instances.emplace_back();
    instance.worldAabbBoundsMin = houseMeshBvh->m_nodes[0].boundsMin; // This works because the house mesh never rotates
    instance.worldAabbBoundsMax = houseMeshBvh->m_nodes[0].boundsMax; // This works because the house mesh never rotates
    instance.objectId = 0;
    instance.worldTransform = glm::mat4(1.0f);
    instance.inverseWorldTransform = glm::inverse(instance.worldTransform);
    instance.meshBvhId = m_houseBvhId;
    instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;

    const uint64_t doorProxyBvhId = CoarseWorldBVH::GetDoorProxyBvhId();
    if (doorProxyBvhId != 0) {
        const std::vector<CoarseWorldBVH::DoorProxyInstance> doorProxyInstances = CoarseWorldBVH::CollectDoorProxyInstances(m_boundsMin, m_boundsMax);
        for (const CoarseWorldBVH::DoorProxyInstance& doorProxyInstance : doorProxyInstances) {

            PrimitiveInstance& instance = instances.emplace_back();
            instance.worldAabbBoundsMin = doorProxyInstance.worldAabb.GetBoundsMin();
            instance.worldAabbBoundsMax = doorProxyInstance.worldAabb.GetBoundsMax();
            instance.objectId = doorProxyInstance.objectId;
            instance.worldTransform = doorProxyInstance.worldTransform;
            instance.inverseWorldTransform = glm::inverse(instance.worldTransform);
            instance.meshBvhId = doorProxyBvhId;
            instance.worldAabbCenter = (instance.worldAabbBoundsMin + instance.worldAabbBoundsMax) * 0.5f;
        }
    }

    SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId);
    if (!sceneBvh) return;

    if (!Hell::Bvh::AddInstanceMeshBvhsToSceneBvh(m_sceneBvhId, instances)) return;
    sceneBvh->UpdateInstances(instances);
}

void DDGIVolume::DebugDraw() {
    AABB aabb = AABB(GetBoundsMin(), GetBoundsMax());
    DebugDraw::DrawAABB(aabb, YELLOW);

    for (uint32_t x = 0; x < m_probeCountX; x++) {
        for (uint32_t y = 0; y < m_probeCountY; y++) {
            for (uint32_t z = 0; z < m_probeCountZ; z++) {
                glm::vec3 probePosition = GetProbeBaseWorldPosition(glm::ivec3(x, y, z));
                DebugDraw::DrawPoint(probePosition, RED);
            }
        }
    }
}

void DDGIVolume::UpdateMembers() {
    m_boundsMin = m_createInfo.origin - m_createInfo.extents * 0.5f;
    m_boundsMax = m_createInfo.origin + m_createInfo.extents * 0.5f;

    m_worldSpaceWidth = m_boundsMax.x - m_boundsMin.x;
    m_worldSpaceHeight = m_boundsMax.y - m_boundsMin.y;
    m_worldSpaceDepth = m_boundsMax.z - m_boundsMin.z;

    m_probeCountX = (int)std::ceil(m_worldSpaceWidth / m_createInfo.probeSpacing) + 1;
    m_probeCountY = (int)std::ceil(m_worldSpaceHeight / m_createInfo.probeSpacing) + 1;
    m_probeCountZ = (int)std::ceil(m_worldSpaceDepth / m_createInfo.probeSpacing) + 1;

    m_ddgiGeometryDirty = true;
}

void DDGIVolume::Init(const glm::vec3& aabbMin, const glm::vec3& aabbMax, float probeSpacing) {
    glm::vec3 inflatedAabbMin = aabbMin - glm::vec3(1.0f);
    glm::vec3 inflatedAabbMax = aabbMax + glm::vec3(1.0f);

    m_createInfo.origin = (inflatedAabbMin + inflatedAabbMax) * 0.5f;

    m_worldSpaceWidth = inflatedAabbMax.x - inflatedAabbMin.x;
    m_worldSpaceHeight = inflatedAabbMax.y - inflatedAabbMin.y;
    m_worldSpaceDepth = inflatedAabbMax.z - inflatedAabbMin.z;

    m_createInfo.probeSpacing = probeSpacing;
    m_probeCountX = (int)std::ceil(m_worldSpaceWidth / GetProbeSpacing()) + 1;
    m_probeCountY = (int)std::ceil(m_worldSpaceHeight / GetProbeSpacing()) + 1;
    m_probeCountZ = (int)std::ceil(m_worldSpaceDepth / GetProbeSpacing()) + 1;

}

uint32_t DDGIVolume::GetTotalProbeCount() const {
    return m_probeCountX * m_probeCountY * m_probeCountZ;
}

DDGIVolumeGPU DDGIVolume::GetGPUData() const {
    glm::vec3 halfExtents = glm::vec3(m_worldSpaceWidth, m_worldSpaceHeight, m_worldSpaceDepth) * 0.5f;

    DDGIVolumeGPU volume;
    volume.origin = GetOrigin();
    volume.probeSpacing = GetProbeSpacing();
    volume.probeCounts = glm::ivec3(m_probeCountX, m_probeCountY, m_probeCountZ);
    volume.numProbes = GetTotalProbeCount(); // sort this out, uint vs int
    volume.worldBoundsMin = GetOrigin() - halfExtents;
    volume.padding0 = 0;
    volume.worldBoundsMax = GetOrigin() + halfExtents;
    volume.padding1 = 0;
    volume.probeOffset = m_probeOffset;

    return volume;
}

void DDGIVolume::SetEditorName(const std::string& name) {
    m_createInfo.editorName = name;
}

void DDGIVolume::SetPosition(const glm::vec3& position) {
    SetOrigin(position);
}

void DDGIVolume::SetOrigin(const glm::vec3& origin) {
    m_createInfo.origin = origin;
    UpdateMembers();
}

void DDGIVolume::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    UpdateMembers();
}

void DDGIVolume::SetExtents(const glm::vec3& extents) {
    m_createInfo.extents = extents;
    UpdateMembers();
}

void DDGIVolume::SetProbeSpacing(float spacing) {
    m_createInfo.probeSpacing = spacing;
    UpdateMembers();
}

void DDGIVolume::SetPointCloudSpacing(float spacing) {
    m_createInfo.pointCloudSpacing = spacing;
    UpdateMembers();
}

glm::vec3 DDGIVolume::GetProbeBaseWorldPosition(const glm::ivec3& probeCoords) const {
    const glm::vec3 counts = glm::vec3(m_probeCountX, m_probeCountY, m_probeCountZ);
    const glm::vec3 coords = glm::vec3(probeCoords);
    return GetOrigin() + (coords - (counts - 1.0f) * 0.5f) * GetProbeSpacing();
}

const std::vector<BvhNode>& DDGIVolume::GetSceneNodes() {
    static std::vector<BvhNode> empty;
    if (m_sceneBvhId == 0) {
        return empty;
    }

    SceneBvh* sceneBvh = Hell::Bvh::GetSceneBvhById(m_sceneBvhId);
    if (!sceneBvh) return empty;

    return sceneBvh->m_nodes;
}


} // namespace Unloved
