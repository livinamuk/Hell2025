#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/VertexAttributes.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_raytracing_scene.h"
#include "Unloved/Render/API/Vulkan/RayTracing/VK_transient_blas_builder.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    namespace {
        struct QueuedProceduralBLASBuild {
            uint64_t blasId = 0;
            uint64_t vertexBufferDeviceAddress = 0;
            uint64_t indexBufferDeviceAddress = 0;
            std::vector<RayQueryMeshInstance> meshInstances;
            VkDeviceSize scratchSize = 0;
            VkDeviceSize scratchOffset = 0;
            uint64_t geometryHash = 0;
        };

        RayQueryScene g_lightingRayQueryScene;
        std::vector<QueuedProceduralBLASBuild> g_proceduralBLASBuilds;
        uint64_t g_proceduralRayQueryBLASId = 0;
        uint64_t g_proceduralBLASGeometryHash = 0;
        VkDeviceSize g_proceduralBLASScratchSize = 0;

        void AddPersistentBLASInstances(RayQueryScene& scene, const std::vector<uint32_t>& sceneRenderItemIndices, const std::vector<RenderItem>& sceneRenderItems, Hell::MeshBuffer& assetMeshData, VulkanMeshBuffer& assetMeshBuffer);
        void BeginProceduralBLASBuilds();
        void AddProceduralBLAS(RayQueryScene& scene, const std::vector<uint32_t>& sceneRenderItemIndices, const std::vector<RenderItem>& sceneRenderItems);
        bool RecordProceduralBLASBuilds(VkCommandBuffer commandBuffer, uint64_t scratchBaseAddress);
        VkDeviceSize GetProceduralBLASScratchSize();
        VkDeviceSize AllocateProceduralBLASScratch(VkDeviceSize scratchSize);
        bool MeshFitsBuffers(const RenderItem& renderItem, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);
        bool HasUsableBLAS(uint64_t blasId);
        uint64_t HashProceduralBLAS(uint64_t vertexBufferDeviceAddress, uint64_t indexBufferDeviceAddress, uint64_t vertexBufferByteSize, uint64_t indexBufferByteSize, uint64_t sourceGeometryVersion, const std::vector<RayQueryMeshInstance>& meshInstances);
        void HashMix(uint64_t& hash, uint64_t value);
        size_t CountTransientMeshInstances(const std::vector<std::vector<uint32_t>>& renderItemGroups);
    }

    void UpdateRayTracing(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        VulkanFrameData& frameData = GetCurrentFrameData();

        VulkanDescriptorSetResource* rayTracingDescriptorSetResource = VulkanResourceManager::GetDescriptorSetResource("RayQueryDescriptorSet");
        VulkanDescriptorSet* rayTracingDescriptorSet = rayTracingDescriptorSetResource ? &rayTracingDescriptorSetResource->GetSet(GetCurrentFrameIndex()) : nullptr;
        VulkanMeshBuffer* assetMeshBuffer = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        VulkanBuffer* skinnedVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);

        if (!rayTracingDescriptorSet) return;
        if (!assetMeshBuffer) return;

        Hell::MeshBuffer& assetMeshData = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        const std::vector<uint32_t>& persistentRenderItemIndices = Unloved::RenderDataManager::GetPersistentRayQueryRenderItemIndices();
        const std::vector<uint32_t>& proceduralRenderItemIndices = Unloved::RenderDataManager::GetProceduralRayQueryRenderItemIndices();
        const std::vector<std::vector<uint32_t>>& transientRenderItemGroups = Unloved::RenderDataManager::GetTransientRayQueryRenderItemGroups();
        const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();

        g_lightingRayQueryScene.Clear();
        g_lightingRayQueryScene.Reserve(
            persistentRenderItemIndices.size() + (proceduralRenderItemIndices.empty() ? 0 : 1) + transientRenderItemGroups.size(),
            persistentRenderItemIndices.size() + proceduralRenderItemIndices.size() + CountTransientMeshInstances(transientRenderItemGroups)
        );

        TransientBLASBuilder::BeginFrame();
        BeginProceduralBLASBuilds();

        AddPersistentBLASInstances(g_lightingRayQueryScene, persistentRenderItemIndices, sceneRenderItems, assetMeshData, *assetMeshBuffer);
        AddProceduralBLAS(g_lightingRayQueryScene, proceduralRenderItemIndices, sceneRenderItems);

        if (skinnedVertexBuffer) {
            TransientBLASBuilder::AddTransientRayQueryBLASInstances(frameData, *assetMeshBuffer, *skinnedVertexBuffer, transientRenderItemGroups, sceneRenderItems, g_lightingRayQueryScene);
        }
        else {
            TransientBLASBuilder::ReleaseFrameSlots(frameData);
        }

        if (!g_lightingRayQueryScene.HasInstances()) return;
        if (!g_lightingRayQueryScene.Upload(commandBuffer, frameData)) return;

        uint32_t instanceCapacity = g_lightingRayQueryScene.GetInstanceCount();
        if (instanceCapacity > frameData.accelerationStructures.rayQueryTLASInstanceCapacity) {
            if (!g_lightingRayQueryScene.ResizeTLAS(frameData, instanceCapacity)) return;
        }

        VkDeviceSize scratchAlignment = AccelerationStructureScratchAlignment();
        VkDeviceSize blasScratchSize = AlignUp(TransientBLASBuilder::GetScratchSize(), scratchAlignment) + GetProceduralBLASScratchSize();
        VkDeviceSize requiredScratchSize = std::max(blasScratchSize, g_lightingRayQueryScene.GetTLASScratchSize(frameData)) + scratchAlignment;

        // One scratch buffer backs BLAS and TLAS builds
        if (!EnsureBufferSize(frameData.buffers.rayQueryScratch, requiredScratchSize)) return;

        VulkanBuffer* scratchBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.rayQueryScratch);
        if (!scratchBuffer) return;

        uint64_t scratchBaseAddress = AlignUp(scratchBuffer->GetDeviceAddress(), scratchAlignment);

        bool recordedBLASBuilds = TransientBLASBuilder::RecordBuilds(commandBuffer, frameData, scratchBaseAddress);
        uint64_t proceduralScratchBaseAddress = scratchBaseAddress + AlignUp(TransientBLASBuilder::GetScratchSize(), scratchAlignment);
        bool recordedProceduralBLASBuilds = RecordProceduralBLASBuilds(commandBuffer, proceduralScratchBaseAddress);
        if (recordedBLASBuilds || recordedProceduralBLASBuilds) {
            RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR);
        }

        // TLAS sees the final instance list
        g_lightingRayQueryScene.RecordTLASBuild(commandBuffer, frameData, scratchBaseAddress);

        // Lighting reads this immediately after
        RecordAccelerationStructureBuildBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR);

        g_lightingRayQueryScene.BindDescriptor(frameData, rayTracingDescriptorSet, 0);
    }

    namespace {
        void AddPersistentBLASInstances(RayQueryScene& scene, const std::vector<uint32_t>& sceneRenderItemIndices, const std::vector<RenderItem>& sceneRenderItems, Hell::MeshBuffer& assetMeshData, VulkanMeshBuffer& assetMeshBuffer) {
            uint64_t vertexBufferDeviceAddress = assetMeshBuffer.GetVertexBufferAddress();
            uint64_t indexBufferDeviceAddress = assetMeshBuffer.GetIndexBufferAddress();
            if (vertexBufferDeviceAddress == 0 || indexBufferDeviceAddress == 0) return;

            // Persistent BLAS is already built by mesh buffer code
            for (uint32_t sceneRenderItemIndex : sceneRenderItemIndices) {
                if (sceneRenderItemIndex >= sceneRenderItems.size()) continue;

                const RenderItem& renderItem = sceneRenderItems[sceneRenderItemIndex];
                Mesh* mesh = assetMeshData.GetMeshById(renderItem.meshId);
                if (!mesh || mesh->vertexCount == 0 || mesh->indexCount < 3 || mesh->vulkanBlasId == 0) continue;

                VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(mesh->vulkanBlasId);
                if (!blas || blas->GetHandle() == VK_NULL_HANDLE || blas->GetDeviceAddress() == 0 || !blas->m_built) continue;

                VkGeometryInstanceFlagsKHR opacityFlags = GetRayQueryGeometryFlags(renderItem) == 0 ? VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR : VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
                scene.AddBLASInstance(blas->GetDeviceAddress(), TransformMatrixKHR(renderItem.modelMatrix), vertexBufferDeviceAddress, indexBufferDeviceAddress, sceneRenderItemIndex, opacityFlags);
            }
        }

        void BeginProceduralBLASBuilds() {
            g_proceduralBLASBuilds.clear();
            g_proceduralBLASScratchSize = 0;
        }

        void AddProceduralBLAS(RayQueryScene& scene, const std::vector<uint32_t>& sceneRenderItemIndices, const std::vector<RenderItem>& sceneRenderItems) {
            if (sceneRenderItemIndices.empty()) return;

            Hell::MeshBuffer& meshData = Hell::ResourceManager::GetMeshBuffer("Procedural");
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer("Procedural");
            if (!meshBuffer) return;

            VulkanBuffer* vertexBuffer = meshBuffer->GetVertexBuffer();
            VulkanBuffer* indexBuffer = meshBuffer->GetIndexBuffer();
            if (!vertexBuffer || !indexBuffer) return;

            uint64_t vertexBufferDeviceAddress = meshBuffer->GetVertexBufferAddress();
            uint64_t indexBufferDeviceAddress = meshBuffer->GetIndexBufferAddress();
            uint64_t vertexBufferByteSize = vertexBuffer->GetSize();
            uint64_t indexBufferByteSize = indexBuffer->GetSize();
            if (vertexBufferDeviceAddress == 0 || indexBufferDeviceAddress == 0) return;

            if (g_proceduralRayQueryBLASId == 0 || !VulkanResourceManager::AccelerationStructureExists(g_proceduralRayQueryBLASId)) {
                g_proceduralRayQueryBLASId = VulkanResourceManager::CreateAccelerationStructure();
                g_proceduralBLASGeometryHash = 0;
            }

            std::vector<RayQueryMeshInstance> meshInstances;
            std::vector<uint32_t> validSceneRenderItemIndices;
            meshInstances.reserve(sceneRenderItemIndices.size());
            validSceneRenderItemIndices.reserve(sceneRenderItemIndices.size());
            for (uint32_t sceneRenderItemIndex : sceneRenderItemIndices) {
                if (sceneRenderItemIndex >= sceneRenderItems.size()) continue;

                const RenderItem& renderItem = sceneRenderItems[sceneRenderItemIndex];
                if (MeshFitsBuffers(renderItem, vertexBufferByteSize, indexBufferByteSize)) {
                    meshInstances.push_back(CreateRayQueryMeshInstance(renderItem));
                    validSceneRenderItemIndices.push_back(sceneRenderItemIndex);
                }
            }
            if (meshInstances.empty()) return;

            uint64_t geometryHash = HashProceduralBLAS(vertexBufferDeviceAddress, indexBufferDeviceAddress, vertexBufferByteSize, indexBufferByteSize, meshData.GetVersion(), meshInstances);
            bool needsBuild = !HasUsableBLAS(g_proceduralRayQueryBLASId) || g_proceduralBLASGeometryHash != geometryHash;

            if (needsBuild) {
                VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryBottomLevelBuildSize(vertexBufferDeviceAddress, indexBufferDeviceAddress, meshInstances, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR);
                if (sizeInfo.accelerationStructureSize == 0 || !PrepareAccelerationStructure(g_proceduralRayQueryBLASId, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo)) return;

                QueuedProceduralBLASBuild& build = g_proceduralBLASBuilds.emplace_back();
                build.blasId = g_proceduralRayQueryBLASId;
                build.vertexBufferDeviceAddress = vertexBufferDeviceAddress;
                build.indexBufferDeviceAddress = indexBufferDeviceAddress;
                build.meshInstances = meshInstances;
                build.scratchSize = sizeInfo.buildScratchSize;
                build.scratchOffset = AllocateProceduralBLASScratch(build.scratchSize);
                build.geometryHash = geometryHash;
            }

            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(g_proceduralRayQueryBLASId);
            if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || accelerationStructure->GetDeviceAddress() == 0) return;

            scene.AddBLASInstance(accelerationStructure->GetDeviceAddress(), TransformMatrixKHR(glm::mat4(1.0f)), vertexBufferDeviceAddress, indexBufferDeviceAddress, validSceneRenderItemIndices);
        }

        bool RecordProceduralBLASBuilds(VkCommandBuffer commandBuffer, uint64_t scratchBaseAddress) {
            if (g_proceduralBLASBuilds.empty()) return false;

            size_t geometryCount = 0;
            for (const QueuedProceduralBLASBuild& build : g_proceduralBLASBuilds) {
                geometryCount += build.meshInstances.size();
            }

            std::vector<VkAccelerationStructureGeometryKHR> geometries;
            std::vector<VkAccelerationStructureBuildGeometryInfoKHR> buildInfos;
            std::vector<VkAccelerationStructureBuildRangeInfoKHR> rangeInfos;
            std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> rangeInfoPtrs;
            std::vector<uint64_t> blasIds;
            std::vector<uint64_t> geometryHashes;

            geometries.reserve(geometryCount);
            buildInfos.reserve(g_proceduralBLASBuilds.size());
            rangeInfos.reserve(geometryCount);
            rangeInfoPtrs.reserve(g_proceduralBLASBuilds.size());
            blasIds.reserve(g_proceduralBLASBuilds.size());
            geometryHashes.reserve(g_proceduralBLASBuilds.size());

            for (const QueuedProceduralBLASBuild& build : g_proceduralBLASBuilds) {
                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(build.blasId);
                if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || build.meshInstances.empty()) continue;

                size_t geometryOffset = geometries.size();
                size_t rangeInfoOffset = rangeInfos.size();

                for (const RayQueryMeshInstance& meshInstance : build.meshInstances) {
                    geometries.push_back(CreateTriangleGeometry(build.vertexBufferDeviceAddress, build.indexBufferDeviceAddress, meshInstance.mesh, GetRayQueryGeometryFlags(meshInstance.material)));

                    VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = rangeInfos.emplace_back();
                    rangeInfo.primitiveCount = meshInstance.mesh.indexCount / 3;
                    rangeInfo.primitiveOffset = 0;
                    rangeInfo.firstVertex = 0;
                    rangeInfo.transformOffset = 0;
                }

                VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
                buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
                buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
                buildInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
                buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
                buildInfo.geometryCount = static_cast<uint32_t>(build.meshInstances.size());
                buildInfo.pGeometries = geometries.data() + geometryOffset;
                buildInfo.scratchData.deviceAddress = scratchBaseAddress + build.scratchOffset;
                buildInfos.push_back(buildInfo);

                rangeInfoPtrs.push_back(rangeInfos.data() + rangeInfoOffset);
                blasIds.push_back(build.blasId);
                geometryHashes.push_back(build.geometryHash);
            }

            if (buildInfos.empty()) return false;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, static_cast<uint32_t>(buildInfos.size()), buildInfos.data(), rangeInfoPtrs.data());

            for (size_t i = 0; i < blasIds.size(); i++) {
                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasIds[i]);
                if (accelerationStructure) {
                    accelerationStructure->m_built = true;
                }
                g_proceduralBLASGeometryHash = geometryHashes[i];
            }

            return true;
        }

        VkDeviceSize GetProceduralBLASScratchSize() {
            return g_proceduralBLASScratchSize;
        }

        VkDeviceSize AllocateProceduralBLASScratch(VkDeviceSize scratchSize) {
            VkDeviceSize scratchOffset = AlignUp(g_proceduralBLASScratchSize, AccelerationStructureScratchAlignment());
            g_proceduralBLASScratchSize = scratchOffset + scratchSize;
            return scratchOffset;
        }

        bool MeshFitsBuffers(const RenderItem& renderItem, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize) {
            if (renderItem.vertexCount == 0 || renderItem.indexCount < 3) return false;
            if (vertexBufferSize == 0 || indexBufferSize == 0) return false;

            uint64_t vertexEnd = static_cast<uint64_t>(renderItem.baseVertex) + static_cast<uint64_t>(renderItem.vertexCount);
            uint64_t indexEnd = static_cast<uint64_t>(renderItem.baseIndex) + static_cast<uint64_t>(renderItem.indexCount);
            uint64_t vertexCapacity = vertexBufferSize / sizeof(Vertex);
            uint64_t indexCapacity = indexBufferSize / sizeof(uint32_t);
            return vertexEnd <= vertexCapacity && indexEnd <= indexCapacity;
        }

        bool HasUsableBLAS(uint64_t blasId) {
            VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(blasId);
            return blas && blas->GetHandle() != VK_NULL_HANDLE && blas->GetDeviceAddress() != 0 && blas->m_built;
        }

        uint64_t HashProceduralBLAS(uint64_t vertexBufferDeviceAddress, uint64_t indexBufferDeviceAddress, uint64_t vertexBufferByteSize, uint64_t indexBufferByteSize, uint64_t sourceGeometryVersion, const std::vector<RayQueryMeshInstance>& meshInstances) {
            uint64_t hash = 1469598103934665603ull;
            HashMix(hash, vertexBufferDeviceAddress);
            HashMix(hash, indexBufferDeviceAddress);
            HashMix(hash, vertexBufferByteSize);
            HashMix(hash, indexBufferByteSize);
            HashMix(hash, sourceGeometryVersion);
            HashMix(hash, meshInstances.size());

            for (const RayQueryMeshInstance& meshInstance : meshInstances) {
                HashMix(hash, meshInstance.mesh.baseVertex);
                HashMix(hash, meshInstance.mesh.baseIndex);
                HashMix(hash, meshInstance.mesh.vertexCount);
                HashMix(hash, meshInstance.mesh.indexCount);
                HashMix(hash, GetRayQueryGeometryFlags(meshInstance.material));
            }

            return hash;
        }

        void HashMix(uint64_t& hash, uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ull;
        }

        size_t CountTransientMeshInstances(const std::vector<std::vector<uint32_t>>& renderItemGroups) {
            size_t count = 0;
            for (const std::vector<uint32_t>& renderItemGroup : renderItemGroups) {
                count += renderItemGroup.size();
            }
            return count;
        }
    }
}
