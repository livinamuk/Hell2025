#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_deletion_queue.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererEnums.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVHGeometry.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <limits>
#include <unordered_map>
#include <vector>

using namespace Unloved;

namespace VulkanRenderer {
namespace {
    constexpr VkBufferUsageFlags DDGI_VERTEX_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

    constexpr VkBufferUsageFlags DDGI_INDEX_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    constexpr VkBufferUsageFlags DDGI_TLAS_INSTANCE_BUFFER_USAGE =
        VK_BUFFER_USAGE_TRANSFER_DST_BIT |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;

    constexpr VmaAllocationCreateFlags DDGI_HOST_BUFFER_VMA_FLAGS = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;

    struct DDGIRayQueryVolumeGeometryCache {
        uint64_t volumeId = 0;
        glm::vec3 boundsMin = glm::vec3(0.0f);
        glm::vec3 boundsMax = glm::vec3(0.0f);

        uint64_t houseVertexBuffer = 0;
        uint64_t houseIndexBuffer = 0;
        uint64_t houseBLAS = 0;
        uint32_t houseVertexCount = 0;
        uint32_t houseIndexCount = 0;

        std::array<uint64_t, FRAME_OVERLAP> tlas = {};
        std::array<uint64_t, FRAME_OVERLAP> instanceBuffer = {};
        std::array<uint32_t, FRAME_OVERLAP> tlasInstanceCapacity = {};
        std::array<VkDeviceSize, FRAME_OVERLAP> tlasScratchSize = {};
    };

    struct DDGIRayQueryDoorGeometryCache {
        uint64_t doorVertexBuffer = 0;
        uint64_t doorIndexBuffer = 0;
        uint64_t doorBLAS = 0;
        uint32_t doorVertexCount = 0;
        uint32_t doorIndexCount = 0;
    };

    std::unordered_map<uint64_t, DDGIRayQueryVolumeGeometryCache> g_ddgiRayQueryVolumeGeometry;
    DDGIRayQueryDoorGeometryCache g_ddgiRayQueryDoorGeometry;

    void RetireResource(uint64_t& id) {
        if (id == 0) return;
        VulkanDeletionQueue::Retire(id);
        id = 0;
    }

    DDGIRayQueryVolumeGeometryCache& GetVolumeGeometryCache(DDGIVolume& ddgiVolume) {
        const uint64_t volumeId = ddgiVolume.GetObjectId();
        auto [it, inserted] = g_ddgiRayQueryVolumeGeometry.try_emplace(volumeId);
        if (inserted) {
            it->second.volumeId = volumeId;
        }
        return it->second;
    }

    DDGIRayQueryVolumeGeometryCache* GetVolumeGeometryCache(uint64_t volumeId) {
        auto it = g_ddgiRayQueryVolumeGeometry.find(volumeId);
        return it != g_ddgiRayQueryVolumeGeometry.end() ? &it->second : nullptr;
    }

    void RetireHouseGeometry(DDGIRayQueryVolumeGeometryCache& geometry) {
        RetireResource(geometry.houseVertexBuffer);
        RetireResource(geometry.houseIndexBuffer);
        RetireResource(geometry.houseBLAS);
        geometry.houseVertexCount = 0;
        geometry.houseIndexCount = 0;
    }

    uint64_t CreateUploadedBuffer(const void* data, VkDeviceSize size, VkBufferUsageFlags usage) {
        if (!data || size == 0) return 0;

        uint64_t bufferId = VulkanResourceManager::CreateBuffer(size, usage, VMA_MEMORY_USAGE_GPU_ONLY);
        VulkanBuffer* buffer = VulkanResourceManager::GetBuffer(bufferId);
        if (!buffer) return 0;

        buffer->UploadData(data, size);
        return bufferId;
    }

    bool HasBuiltAccelerationStructure(uint64_t id) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        return accelerationStructure && accelerationStructure->GetHandle() != VK_NULL_HANDLE && accelerationStructure->GetDeviceAddress() != 0 && accelerationStructure->m_built;
    }

    uint64_t GetAccelerationStructureDeviceAddress(uint64_t id) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        return accelerationStructure ? accelerationStructure->GetDeviceAddress() : 0;
    }

    float DistanceSqToAABB(const glm::vec3& point, const glm::vec3& boundsMin, const glm::vec3& boundsMax) {
        const glm::vec3 closestPoint = glm::clamp(point, boundsMin, boundsMax);
        const glm::vec3 diff = closestPoint - point;
        return glm::dot(diff, diff);
    }

    DDGIVolume* GetDebugDDGIVolume() {
        Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();
        if (ddgiVolumes.empty()) return nullptr;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        if (viewportData.empty()) {
            for (DDGIVolume& ddgiVolume : ddgiVolumes) {
                return &ddgiVolume;
            }
            return nullptr;
        }

        const glm::vec3 cameraPos = glm::vec3(viewportData[0].viewPos);
        DDGIVolume* bestVolume = nullptr;
        float bestDistanceSq = std::numeric_limits<float>::max();

        for (DDGIVolume& ddgiVolume : ddgiVolumes) {
            const float distanceSq = DistanceSqToAABB(cameraPos, ddgiVolume.GetBoundsMin(), ddgiVolume.GetBoundsMax());
            if (distanceSq < bestDistanceSq) {
                bestDistanceSq = distanceSq;
                bestVolume = &ddgiVolume;
            }
        }

        return bestVolume;
    }

    bool PrepareHouseGeometry(DDGIRayQueryVolumeGeometryCache& geometry, DDGIVolume& ddgiVolume) {
        const glm::vec3 boundsMin = ddgiVolume.GetBoundsMin();
        const glm::vec3 boundsMax = ddgiVolume.GetBoundsMax();

        if (geometry.houseVertexBuffer != 0 &&
            geometry.houseIndexBuffer != 0 &&
            geometry.houseBLAS != 0 &&
            geometry.boundsMin == boundsMin &&
            geometry.boundsMax == boundsMax) {
            return true;
        }

        RetireHouseGeometry(geometry);

        CoarseWorldBVH::HouseGeometry houseGeometry = CoarseWorldBVH::BuildHouseGeometry(boundsMin, boundsMax);
        if (houseGeometry.vertices.empty() || houseGeometry.indices.empty()) return false;

        geometry.volumeId = ddgiVolume.GetObjectId();
        geometry.boundsMin = boundsMin;
        geometry.boundsMax = boundsMax;
        geometry.houseVertexCount = static_cast<uint32_t>(houseGeometry.vertices.size());
        geometry.houseIndexCount = static_cast<uint32_t>(houseGeometry.indices.size());
        geometry.houseVertexBuffer = CreateUploadedBuffer(houseGeometry.vertices.data(), sizeof(Vertex) * houseGeometry.vertices.size(), DDGI_VERTEX_BUFFER_USAGE);
        geometry.houseIndexBuffer = CreateUploadedBuffer(houseGeometry.indices.data(), sizeof(uint32_t) * houseGeometry.indices.size(), DDGI_INDEX_BUFFER_USAGE);
        geometry.houseBLAS = VulkanResourceManager::CreateAccelerationStructure();

        return geometry.houseVertexBuffer != 0 &&
               geometry.houseIndexBuffer != 0 &&
               geometry.houseBLAS != 0;
    }

    bool PrepareDoorGeometry() {
        if (g_ddgiRayQueryDoorGeometry.doorVertexBuffer != 0 &&
            g_ddgiRayQueryDoorGeometry.doorIndexBuffer != 0 &&
            g_ddgiRayQueryDoorGeometry.doorBLAS != 0) {
            return true;
        }

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        CoarseWorldBVH::BuildDoorProxyMesh(vertices, indices);
        if (vertices.empty() || indices.empty()) return false;

        g_ddgiRayQueryDoorGeometry.doorVertexCount = static_cast<uint32_t>(vertices.size());
        g_ddgiRayQueryDoorGeometry.doorIndexCount = static_cast<uint32_t>(indices.size());
        g_ddgiRayQueryDoorGeometry.doorVertexBuffer = CreateUploadedBuffer(vertices.data(), sizeof(Vertex) * vertices.size(), DDGI_VERTEX_BUFFER_USAGE);
        g_ddgiRayQueryDoorGeometry.doorIndexBuffer = CreateUploadedBuffer(indices.data(), sizeof(uint32_t) * indices.size(), DDGI_INDEX_BUFFER_USAGE);
        g_ddgiRayQueryDoorGeometry.doorBLAS = VulkanResourceManager::CreateAccelerationStructure();

        return g_ddgiRayQueryDoorGeometry.doorVertexBuffer != 0 &&
               g_ddgiRayQueryDoorGeometry.doorIndexBuffer != 0 &&
               g_ddgiRayQueryDoorGeometry.doorBLAS != 0;
    }

    bool PrepareBLASBuild(uint64_t blasId, uint64_t vertexBufferId, uint64_t indexBufferId, uint32_t vertexCount, uint32_t indexCount, VkAccelerationStructureBuildSizesInfoKHR& sizeInfo) {
        VulkanBuffer* vertexBuffer = VulkanResourceManager::GetBuffer(vertexBufferId);
        VulkanBuffer* indexBuffer = VulkanResourceManager::GetBuffer(indexBufferId);
        if (!vertexBuffer || !indexBuffer) return false;

        RayQueryMeshInstance meshInstance{};
        meshInstance.mesh.baseVertex = 0;
        meshInstance.mesh.baseIndex = 0;
        meshInstance.mesh.vertexCount = vertexCount;
        meshInstance.mesh.indexCount = indexCount;
        meshInstance.material.blendingMode = static_cast<uint32_t>(BlendingMode::DEFAULT);
        meshInstance.material.materialIndex = 0;

        std::vector<RayQueryMeshInstance> meshInstances = { meshInstance };
        sizeInfo = QueryBottomLevelBuildSize(vertexBuffer->GetDeviceAddress(), indexBuffer->GetDeviceAddress(), meshInstances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
        if (sizeInfo.accelerationStructureSize == 0) return false;

        return PrepareAccelerationStructure(blasId, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo);
    }

    void RecordBLASBuild(VkCommandBuffer commandBuffer, uint64_t blasId, uint64_t vertexBufferId, uint64_t indexBufferId, uint32_t vertexCount, uint32_t indexCount, uint64_t scratchAddress) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasId);
        VulkanBuffer* vertexBuffer = VulkanResourceManager::GetBuffer(vertexBufferId);
        VulkanBuffer* indexBuffer = VulkanResourceManager::GetBuffer(indexBufferId);
        if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE) return;
        if (!vertexBuffer || !indexBuffer) return;

        RayQueryMeshInstance meshInstance{};
        meshInstance.mesh.baseVertex = 0;
        meshInstance.mesh.baseIndex = 0;
        meshInstance.mesh.vertexCount = vertexCount;
        meshInstance.mesh.indexCount = indexCount;
        meshInstance.material.blendingMode = static_cast<uint32_t>(BlendingMode::DEFAULT);
        meshInstance.material.materialIndex = 0;

        VkAccelerationStructureGeometryKHR geometry = CreateTriangleGeometry(vertexBuffer->GetDeviceAddress(), indexBuffer->GetDeviceAddress(), meshInstance.mesh, GetRayQueryGeometryFlags(meshInstance.material));

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
        buildInfo.scratchData.deviceAddress = scratchAddress;

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{};
        rangeInfo.primitiveCount = indexCount / 3;
        rangeInfo.primitiveOffset = 0;
        rangeInfo.firstVertex = 0;
        rangeInfo.transformOffset = 0;

        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &rangeInfo;
        vkCmdBuildAccelerationStructuresKHR(commandBuffer, 1, &buildInfo, &rangeInfoPtr);
        accelerationStructure->m_built = true;
    }

    bool UploadTLASInstances(VkCommandBuffer commandBuffer, DDGIRayQueryVolumeGeometryCache& geometry, uint32_t frameIndex, const std::vector<VkAccelerationStructureInstanceKHR>& instances, VulkanBuffer*& instanceBuffer) {
        const VkDeviceSize instanceBufferSize = sizeof(VkAccelerationStructureInstanceKHR) * instances.size();

        if (geometry.instanceBuffer[frameIndex] == 0) {
            geometry.instanceBuffer[frameIndex] = VulkanResourceManager::CreateBuffer(1, DDGI_TLAS_INSTANCE_BUFFER_USAGE, VMA_MEMORY_USAGE_AUTO, DDGI_HOST_BUFFER_VMA_FLAGS);
        }

        if (!EnsureBufferSize(geometry.instanceBuffer[frameIndex], instanceBufferSize)) return false;

        instanceBuffer = VulkanResourceManager::GetBuffer(geometry.instanceBuffer[frameIndex]);
        if (!instanceBuffer) return false;

        instanceBuffer->UpdateData(instances.data(), instanceBufferSize);

        VkBufferMemoryBarrier instanceUploadBarrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
        instanceUploadBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        instanceUploadBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
        instanceUploadBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        instanceUploadBarrier.buffer = instanceBuffer->GetBuffer();
        instanceUploadBarrier.offset = 0;
        instanceUploadBarrier.size = instanceBufferSize;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, 0, 0, nullptr, 1, &instanceUploadBarrier, 0, nullptr);

        return true;
    }
}

bool BuildDDGIRayQueryScene(VkCommandBuffer commandBuffer, DDGIVolume& ddgiVolume, VulkanDescriptorSet* descriptorSet) {
    if (!descriptorSet) return false;
    DDGIRayQueryVolumeGeometryCache& geometry = GetVolumeGeometryCache(ddgiVolume);
    if (!PrepareHouseGeometry(geometry, ddgiVolume)) return false;
    if (!PrepareDoorGeometry()) return false;

    VulkanFrameData& frameData = VulkanRenderer::GetCurrentFrameData();
    const uint32_t frameIndex = GetCurrentFrameIndex();
    VulkanBuffer* houseVertexBuffer = VulkanResourceManager::GetBuffer(geometry.houseVertexBuffer);
    VulkanBuffer* houseIndexBuffer = VulkanResourceManager::GetBuffer(geometry.houseIndexBuffer);
    VulkanBuffer* doorVertexBuffer = VulkanResourceManager::GetBuffer(g_ddgiRayQueryDoorGeometry.doorVertexBuffer);
    VulkanBuffer* doorIndexBuffer = VulkanResourceManager::GetBuffer(g_ddgiRayQueryDoorGeometry.doorIndexBuffer);
    if (!houseVertexBuffer || !houseIndexBuffer || !doorVertexBuffer || !doorIndexBuffer) return false;

    const bool buildHouseBLAS = !HasBuiltAccelerationStructure(geometry.houseBLAS);
    const bool buildDoorBLAS = !HasBuiltAccelerationStructure(g_ddgiRayQueryDoorGeometry.doorBLAS);

    VkAccelerationStructureBuildSizesInfoKHR houseBLASSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
    VkAccelerationStructureBuildSizesInfoKHR doorBLASSize{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };

    VkDeviceSize requiredBLASScratchSize = 0;
    if (buildHouseBLAS) {
        if (!PrepareBLASBuild(geometry.houseBLAS, geometry.houseVertexBuffer, geometry.houseIndexBuffer, geometry.houseVertexCount, geometry.houseIndexCount, houseBLASSize)) return false;
        requiredBLASScratchSize = std::max(requiredBLASScratchSize, houseBLASSize.buildScratchSize);
    }

    if (buildDoorBLAS) {
        if (!PrepareBLASBuild(g_ddgiRayQueryDoorGeometry.doorBLAS, g_ddgiRayQueryDoorGeometry.doorVertexBuffer, g_ddgiRayQueryDoorGeometry.doorIndexBuffer, g_ddgiRayQueryDoorGeometry.doorVertexCount, g_ddgiRayQueryDoorGeometry.doorIndexCount, doorBLASSize)) return false;
        requiredBLASScratchSize = std::max(requiredBLASScratchSize, doorBLASSize.buildScratchSize);
    }

    const uint64_t houseBLASAddress = GetAccelerationStructureDeviceAddress(geometry.houseBLAS);
    const uint64_t doorBLASAddress = GetAccelerationStructureDeviceAddress(g_ddgiRayQueryDoorGeometry.doorBLAS);
    if (houseBLASAddress == 0 || doorBLASAddress == 0) return false;

    std::vector<VkAccelerationStructureInstanceKHR> instances;

    VkAccelerationStructureInstanceKHR houseInstance{};
    houseInstance.transform = TransformMatrixKHR(glm::mat4(1.0f));
    houseInstance.instanceCustomIndex = 0;
    houseInstance.mask = 0xff;
    houseInstance.instanceShaderBindingTableRecordOffset = 0;
    houseInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
    houseInstance.accelerationStructureReference = houseBLASAddress;
    instances.push_back(houseInstance);

    const glm::vec3 volumeBoundsMin = ddgiVolume.GetBoundsMin();
    const glm::vec3 volumeBoundsMax = ddgiVolume.GetBoundsMax();
    const std::vector<CoarseWorldBVH::DoorProxyInstance> doorProxyInstances = CoarseWorldBVH::CollectDoorProxyInstances(volumeBoundsMin, volumeBoundsMax);
    for (const CoarseWorldBVH::DoorProxyInstance& doorProxyInstance : doorProxyInstances) {
        VkAccelerationStructureInstanceKHR doorInstance{};
        doorInstance.transform = TransformMatrixKHR(doorProxyInstance.worldTransform);
        doorInstance.instanceCustomIndex = 1;
        doorInstance.mask = 0xff;
        doorInstance.instanceShaderBindingTableRecordOffset = 0;
        doorInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
        doorInstance.accelerationStructureReference = doorBLASAddress;
        instances.push_back(doorInstance);
    }

    VulkanBuffer* instanceBuffer = nullptr;
    if (!UploadTLASInstances(commandBuffer, geometry, frameIndex, instances, instanceBuffer)) return false;

    const uint32_t instanceCount = static_cast<uint32_t>(instances.size());
    if (geometry.tlas[frameIndex] == 0) {
        geometry.tlas[frameIndex] = VulkanResourceManager::CreateAccelerationStructure();
    }

    if (instanceCount > geometry.tlasInstanceCapacity[frameIndex] ||
        geometry.tlasScratchSize[frameIndex] == 0) {

        VkAccelerationStructureBuildSizesInfoKHR tlasSize = QueryTopLevelBuildSize(instanceBuffer->GetDeviceAddress(), instanceCount);
        if (tlasSize.accelerationStructureSize == 0) return false;
        if (!PrepareAccelerationStructure(geometry.tlas[frameIndex], VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, tlasSize)) return false;

        geometry.tlasInstanceCapacity[frameIndex] = instanceCount;
        geometry.tlasScratchSize[frameIndex] = tlasSize.buildScratchSize;
    }

    const VkDeviceSize scratchAlignment = AccelerationStructureScratchAlignment();
    const VkDeviceSize requiredScratchSize = std::max(requiredBLASScratchSize, geometry.tlasScratchSize[frameIndex]) + scratchAlignment;
    if (!EnsureBufferSize(frameData.buffers.ddgiRayQueryScratch, requiredScratchSize)) return false;

    VulkanBuffer* scratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.ddgiRayQueryScratch);
    if (!scratchBuffer) return false;

    const uint64_t scratchBaseAddress = AlignUp(scratchBuffer->GetDeviceAddress(), scratchAlignment);

    if (buildHouseBLAS) {
        RecordBLASBuild(commandBuffer, geometry.houseBLAS, geometry.houseVertexBuffer, geometry.houseIndexBuffer, geometry.houseVertexCount, geometry.houseIndexCount, scratchBaseAddress);
        RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
    }

    if (buildDoorBLAS) {
        RecordBLASBuild(commandBuffer, g_ddgiRayQueryDoorGeometry.doorBLAS, g_ddgiRayQueryDoorGeometry.doorVertexBuffer, g_ddgiRayQueryDoorGeometry.doorIndexBuffer, g_ddgiRayQueryDoorGeometry.doorVertexCount, g_ddgiRayQueryDoorGeometry.doorIndexCount, scratchBaseAddress);
        RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
    }

    RecordTopLevelBuild(commandBuffer, geometry.tlas[frameIndex], instanceBuffer->GetDeviceAddress(), instanceCount, scratchBaseAddress);
    RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

    VulkanAccelerationStructure* tlas = VulkanResourceManager::GetAccelerationStructure(geometry.tlas[frameIndex]);
    if (!tlas || tlas->GetHandle() == VK_NULL_HANDLE) return false;

    descriptorSet->WriteAccelerationStructure(0, tlas->GetHandle());
    descriptorSet->Update();
    return true;
}

void DestroyDDGIRayQueryScene(uint64_t volumeId) {
    auto it = g_ddgiRayQueryVolumeGeometry.find(volumeId);
    if (it == g_ddgiRayQueryVolumeGeometry.end()) return;

    DDGIRayQueryVolumeGeometryCache& geometry = it->second;
    RetireHouseGeometry(geometry);

    for (uint64_t& tlas : geometry.tlas) {
        RetireResource(tlas);
    }

    for (uint64_t& instanceBuffer : geometry.instanceBuffer) {
        RetireResource(instanceBuffer);
    }

    g_ddgiRayQueryVolumeGeometry.erase(it);
}

void CleanUpDDGIRayQueryScenes() {
    g_ddgiRayQueryVolumeGeometry.clear();
    g_ddgiRayQueryDoorGeometry = {};
}

void DDGIRaytraceScenePass(VkCommandBuffer commandBuffer) {
    ProfilerVulkanZoneFunction();

    if (Renderer::GetCurrentRendererSettings().rendererOverrideState != RendererOverrideState::DDGI_RAYTRACE) return;

    AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
    VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIRaytraceScene");
    VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
    VulkanDescriptorSetResource* ddgiRayQueryDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("DDGIRayQueryDescriptorSet");
    VulkanDescriptorSet* ddgiRayQueryDescriptorSet = ddgiRayQueryDescriptorSetResource ? &ddgiRayQueryDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
    DDGIVolume* ddgiVolume = GetDebugDDGIVolume();

    if (!lightingImage) return;
    if (!pipeline) return;
    if (!staticDescriptorSet) return;
    if (!ddgiRayQueryDescriptorSet) return;
    if (!ddgiVolume) return;

    if (!BuildDDGIRayQueryScene(commandBuffer, *ddgiVolume, ddgiRayQueryDescriptorSet)) return;

    DDGIRayQueryVolumeGeometryCache* geometry = GetVolumeGeometryCache(ddgiVolume->GetObjectId());
    if (!geometry) return;

    VulkanBuffer* houseVertexBuffer = VulkanResourceManager::GetBuffer(geometry->houseVertexBuffer);
    VulkanBuffer* houseIndexBuffer = VulkanResourceManager::GetBuffer(geometry->houseIndexBuffer);
    VulkanBuffer* doorVertexBuffer = VulkanResourceManager::GetBuffer(g_ddgiRayQueryDoorGeometry.doorVertexBuffer);
    VulkanBuffer* doorIndexBuffer = VulkanResourceManager::GetBuffer(g_ddgiRayQueryDoorGeometry.doorIndexBuffer);
    if (!houseVertexBuffer || !houseIndexBuffer || !doorVertexBuffer || !doorIndexBuffer) return;

    PushConstantsDDGIRaytraceScene pushConstants{};
    pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
    pushConstants.houseVertexBufferDeviceAddress = houseVertexBuffer->GetDeviceAddress();
    pushConstants.houseIndexBufferDeviceAddress = houseIndexBuffer->GetDeviceAddress();
    pushConstants.doorVertexBufferDeviceAddress = doorVertexBuffer->GetDeviceAddress();
    pushConstants.doorIndexBufferDeviceAddress = doorIndexBuffer->GetDeviceAddress();
    pushConstants.clearOutput = 1;
    pushConstants.maxRayDistance = 100.0f;

    if (pushConstants.frameAddressTableDeviceAddress == 0) return;

    VkExtent2D extent = lightingImage->GetExtent2D();
    lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

    VkDescriptorSet descriptorSets[] = {
        staticDescriptorSet->GetHandle(),
        ddgiRayQueryDescriptorSet->GetHandle()
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 2, descriptorSets, 0, nullptr);
    vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

    const uint32_t groupCountX = (extent.width + 15) / 16;
    const uint32_t groupCountY = (extent.height + 15) / 16;
    vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
}
}
