#include "vk_raytracing_manager.h"

#include "Hell/Render/API/Vulkan/Managers/vk_command_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_memory_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

#include "Hell/Logging.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/MeshBuffer.h"

using namespace Hell;

namespace VulkanRaytracingManager {

    VulkanBuffer CreateScratchBuffer(VkDeviceSize size) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        return VulkanBuffer(size, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    }

    void CreateASBuffer(VulkanAccelerationStructure& accelerationStructure, VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo) {
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        accelerationStructure.m_buffer = VulkanBuffer(buildSizeInfo.accelerationStructureSize, usage, VMA_MEMORY_USAGE_AUTO);
    }

    void CreateTLAS(uint64_t id, const std::vector<VkAccelerationStructureInstanceKHR>& instances) {
        //Logging::Debug() << "CreateTLAS() " << id << " " << instances.size() << " instances\n";

        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        if (!accelerationStructure) return;

        // Bail if no instances
        if (instances.empty()) return;

        VkDevice device = VulkanDeviceManager::GetDevice();

        const size_t bufferSize = instances.size() * sizeof(VkAccelerationStructureInstanceKHR);
    
        // GPU Instance Buffer - replaced manual staging with VulkanBuffer::UploadData
        VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
        VulkanBuffer instanceBuffer(bufferSize, usage, VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        instanceBuffer.UploadData(instances.data(), bufferSize);
    
        // Geometry Setup
        VkAccelerationStructureGeometryInstancesDataKHR tlasInstances{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR };
        tlasInstances.data.deviceAddress = instanceBuffer.GetDeviceAddress();
    
        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometry.instances = tlasInstances;
    
        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;
    
        const uint32_t numInstances = static_cast<uint32_t>(instances.size());
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numInstances, &sizeInfo);
    
        // This TLAS is rebuilt in place. Release the previous handle and backing
        // buffer before replacing them.
        accelerationStructure->Cleanup();

        // Create TLAS handle and buffer
        accelerationStructure->CreateBuffer(sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = accelerationStructure->GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle);
    
        // Build the TLAS using a managed scratch buffer
        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
        buildInfo.dstAccelerationStructure = accelerationStructure->m_handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();
    
        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numInstances, 0, 0, 0 };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };
    
        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
            });
    
        // Get final address
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = accelerationStructure->m_handle;
        accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);

        // Both buffers are only needed while the synchronous build is running.
        scratchBuffer.Cleanup();
        instanceBuffer.Cleanup();
    }

    bool BuildBottomLevelAS(uint64_t id, VulkanMeshBuffer& meshBuffer, const Mesh& mesh) {
        if (mesh.vertexCount == 0 || mesh.indexCount < 3) return false;

        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        if (!accelerationStructure) return false;

        uint64_t vertexBufferAddress = meshBuffer.GetVertexBufferAddress();
        uint64_t indexBufferAddress = meshBuffer.GetIndexBufferAddress();
        if (vertexBufferAddress == 0 || indexBufferAddress == 0) return false;

        VkDevice device = VulkanDeviceManager::GetDevice();

        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        vertexBufferDeviceAddress.deviceAddress = vertexBufferAddress + static_cast<uint64_t>(mesh.baseVertex) * sizeof(Vertex);

        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
        indexBufferDeviceAddress.deviceAddress = indexBufferAddress + static_cast<uint64_t>(mesh.baseIndex) * sizeof(uint32_t);

        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.flags = 0;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        geometry.geometry.triangles.maxVertex = mesh.vertexCount - 1;
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
        geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        uint32_t primitiveCount = mesh.indexCount / 3;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &primitiveCount, &sizeInfo);
        if (sizeInfo.accelerationStructureSize == 0) return false;

        accelerationStructure->Cleanup();
        accelerationStructure->CreateBuffer(sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = accelerationStructure->GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        if (vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle) != VK_SUCCESS) {
            accelerationStructure->Cleanup();
            return false;
        }

        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
        buildInfo.dstAccelerationStructure = accelerationStructure->m_handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ primitiveCount, 0, 0, 0 };
        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = &rangeInfo;

        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfoPtr);
        });

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = accelerationStructure->m_handle;
        accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);
        accelerationStructure->m_built = accelerationStructure->m_deviceAddress != 0;

        scratchBuffer.Cleanup();
        return accelerationStructure->m_built;
    }

    void BuildBottomLevelASFromMeshes(uint64_t id, VulkanMeshBuffer& meshBuffer, const std::vector<Mesh*>& meshes) {
        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(id);
        if (!accelerationStructure) return;

        VulkanBuffer* vertexBuffer = meshBuffer.GetVertexBuffer();
        VulkanBuffer* indexBuffer = meshBuffer.GetIndexBuffer();
        if (!vertexBuffer || !indexBuffer) return;

        const uint64_t vertexBufferAddress = meshBuffer.GetVertexBufferAddress();
        const uint64_t indexBufferAddress = meshBuffer.GetIndexBufferAddress();
        if (vertexBufferAddress == 0 || indexBufferAddress == 0) return;

        std::vector<VkAccelerationStructureGeometryKHR> geometries;
        std::vector<uint32_t> primitiveCounts;
        geometries.reserve(meshes.size());
        primitiveCounts.reserve(meshes.size());

        for (Mesh* mesh : meshes) {
            if (!mesh || mesh->vertexCount == 0 || mesh->indexCount < 3) continue;

            VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
            vertexBufferDeviceAddress.deviceAddress = vertexBufferAddress + static_cast<uint64_t>(mesh->baseVertex) * sizeof(Vertex);

            VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
            indexBufferDeviceAddress.deviceAddress = indexBufferAddress + static_cast<uint64_t>(mesh->baseIndex) * sizeof(uint32_t);

            VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

            VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
            geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
            geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
            geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
            geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
            geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
            geometry.geometry.triangles.maxVertex = mesh->vertexCount - 1;
            geometry.geometry.triangles.vertexStride = sizeof(Vertex);
            geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
            geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
            geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

            geometries.push_back(geometry);
            primitiveCounts.push_back(mesh->indexCount / 3);
        }

        if (geometries.empty()) {
            accelerationStructure->Cleanup();
            return;
        }

        VkDevice device = VulkanDeviceManager::GetDevice();

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.geometryCount = static_cast<uint32_t>(geometries.size());
        buildInfo.pGeometries = geometries.data();

        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, primitiveCounts.data(), &sizeInfo);

        accelerationStructure->Cleanup();
        accelerationStructure->CreateBuffer(sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = accelerationStructure->GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle);

        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);
        buildInfo.dstAccelerationStructure = accelerationStructure->m_handle;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
        rangeInfos.reserve(primitiveCounts.size());

        for (uint32_t primitiveCount : primitiveCounts) {
            VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = rangeInfos.emplace_back();
            rangeInfo.primitiveCount = primitiveCount;
            rangeInfo.primitiveOffset = 0;
            rangeInfo.firstVertex = 0;
            rangeInfo.transformOffset = 0;
        }

        const VkAccelerationStructureBuildRangeInfoKHR* rangeInfoPtr = rangeInfos.data();
        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, &rangeInfoPtr);
        });

        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = accelerationStructure->m_handle;
        accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);

        scratchBuffer.Cleanup();
    }

    uint64_t CreateBottomLevelAS(Mesh* mesh) {
        if (!mesh || mesh->vertexCount == 0 || mesh->indexCount == 0) return 0;

        MeshBuffer* staticGeometry = ResourceManager::GetMeshBufferPtr("AssetGeometry");
        if (!staticGeometry || staticGeometry->GetVulkanId() == 0) return 0;

        VulkanMeshBuffer* vulkanMeshBuffer = VulkanResourceManager::GetMeshBuffer(staticGeometry->GetVulkanId());
        if (!vulkanMeshBuffer) return 0;

        uint64_t accelerationStructureId = VulkanResourceManager::CreateAccelerationStructure();

        VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(accelerationStructureId);
        if (!accelerationStructure) return 0;

        VkDevice device = VulkanDeviceManager::GetDevice();

        VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
        VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

        vertexBufferDeviceAddress.deviceAddress = vulkanMeshBuffer->GetVertexBufferAddress() + mesh->baseVertex * sizeof(Vertex);
        indexBufferDeviceAddress.deviceAddress = vulkanMeshBuffer->GetIndexBufferAddress() + mesh->baseIndex * sizeof(uint32_t);
        transformBufferDeviceAddress.deviceAddress = {};

        // Standard geometry setup for triangles
        VkAccelerationStructureGeometryKHR geometry{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR };
        geometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
        geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
        geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
        geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
        geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
        geometry.geometry.triangles.maxVertex = mesh->vertexCount - 1;
        geometry.geometry.triangles.vertexStride = sizeof(Vertex);
        geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
        geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
        geometry.geometry.triangles.transformData = transformBufferDeviceAddress;

        VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
        buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
        buildInfo.geometryCount = 1;
        buildInfo.pGeometries = &geometry;

        const uint32_t numTriangles = mesh->indexCount / 3;
        VkAccelerationStructureBuildSizesInfoKHR sizeInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR };
        vkGetAccelerationStructureBuildSizesKHR(device, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &buildInfo, &numTriangles, &sizeInfo);

        // Create the BLAS container and its internal VulkanBuffer
        accelerationStructure->CreateBuffer(sizeInfo);

        VkAccelerationStructureCreateInfoKHR createInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR };
        createInfo.buffer = accelerationStructure->GetBuffer();
        createInfo.size = sizeInfo.accelerationStructureSize;
        createInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
        vkCreateAccelerationStructureKHR(device, &createInfo, nullptr, &accelerationStructure->m_handle);

        // Managed scratch buffer (VulkanBuffer) still used for the build process
        VulkanBuffer scratchBuffer = CreateScratchBuffer(sizeInfo.buildScratchSize);

        buildInfo.dstAccelerationStructure = accelerationStructure->m_handle;
        buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
        buildInfo.scratchData.deviceAddress = scratchBuffer.GetDeviceAddress();

        VkAccelerationStructureBuildRangeInfoKHR rangeInfo{ numTriangles, 0, 0, 0 };
        std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pRangeInfos = { &rangeInfo };

        VulkanCommandManager::SubmitImmediate([&](VkCommandBuffer cmd) {
            vkCmdBuildAccelerationStructuresKHR(cmd, 1, &buildInfo, pRangeInfos.data());
        });

        // Get final address for the BLAS handle itself
        VkAccelerationStructureDeviceAddressInfoKHR addressInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR };
        addressInfo.accelerationStructure = accelerationStructure->m_handle;
        accelerationStructure->m_deviceAddress = vkGetAccelerationStructureDeviceAddressKHR(device, &addressInfo);

        // Cleanup
        scratchBuffer.Cleanup();

        return accelerationStructureId;
    }

}
