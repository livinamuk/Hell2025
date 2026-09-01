#include "VK_transient_blas_builder.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Render/API/Vulkan/RayTracing/VK_acceleration_structure_utils.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace VulkanRenderer {
    namespace {
        struct QueuedBuild {
            uint64_t blasId = 0;
            size_t slotIndex = 0;
            std::vector<RayQueryMeshInstance> meshInstances;
            VkDeviceSize scratchSize = 0;
            VkDeviceSize scratchOffset = 0;
            bool update = false;
        };

        std::vector<QueuedBuild> g_builds;
        std::vector<VkAccelerationStructureGeometryKHR> g_geometries;
        std::vector<VkAccelerationStructureBuildGeometryInfoKHR> g_buildInfos;
        std::vector<VkAccelerationStructureBuildRangeInfoKHR> g_rangeInfos;
        std::vector<const VkAccelerationStructureBuildRangeInfoKHR*> g_rangeInfoPtrs;
        std::vector<uint64_t> g_blasIds;
        std::vector<size_t> g_slotIndices;

        VkDeviceSize g_scratchSize = 0;
        uint64_t g_vertexBufferAddress = 0;
        uint64_t g_indexBufferAddress = 0;

        void ClearBuildBatch();
        void ReserveBuildBatch(size_t buildCount, size_t geometryCount);
        void QueueBuild(uint64_t blasId, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances, uint64_t scratchBufferAddress, bool update, size_t slotIndex);
        bool RecordBuildBatch(VkCommandBuffer commandBuffer, VulkanFrameData& frameData);
        VkDeviceSize AllocateScratch(VkDeviceSize scratchSize);

        void HashMix(uint64_t& hash, uint64_t value);
        uint64_t HashMeshInstances(const std::vector<RayQueryMeshInstance>& meshInstances);
        bool SlotMatches(const VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryMeshInstance>& meshInstances);
        void StoreSlotMetadata(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryMeshInstance>& meshInstances, const VkAccelerationStructureBuildSizesInfoKHR& sizeInfo);
        bool MeshFitsBuffers(const RayQueryMesh& mesh, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize);
        void DestroySlot(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot);
        void DestroySlots(std::vector<VulkanFrameData::AccelerationStructures::SkinnedBLASSlot>& slots, size_t firstSlot);
    }

    namespace TransientBLASBuilder {
        void BeginFrame() {
            g_builds.clear();
            ClearBuildBatch();
            g_scratchSize = 0;
            g_vertexBufferAddress = 0;
            g_indexBufferAddress = 0;
        }

        void ReleaseFrameSlots(VulkanFrameData& frameData) {
            DestroySlots(frameData.accelerationStructures.skinnedBLAS, 0);
            frameData.accelerationStructures.skinnedVertexBufferAddress = 0;
        }

        void AddTransientRayQueryBLASInstances(VulkanFrameData& frameData, VulkanMeshBuffer& assetMeshBuffer, VulkanBuffer& skinnedVertexBuffer, const std::vector<std::vector<uint32_t>>& renderItemGroups, const std::vector<RenderItem>& sceneRenderItems, RayQueryScene& scene) {
            g_vertexBufferAddress = skinnedVertexBuffer.GetDeviceAddress();
            g_indexBufferAddress = assetMeshBuffer.GetIndexBufferAddress();

            VulkanBuffer* assetIndexBuffer = assetMeshBuffer.GetIndexBuffer();
            if (g_vertexBufferAddress != frameData.accelerationStructures.skinnedVertexBufferAddress) {
                ReleaseFrameSlots(frameData);
                frameData.accelerationStructures.skinnedVertexBufferAddress = g_vertexBufferAddress;
            }

            if (g_vertexBufferAddress == 0 || g_indexBufferAddress == 0 || !assetIndexBuffer) {
                ReleaseFrameSlots(frameData);
                return;
            }

            VkDeviceSize skinnedVertexBufferSize = skinnedVertexBuffer.GetSize();
            VkDeviceSize assetIndexBufferSize = assetIndexBuffer->GetSize();
            size_t activeSlotCount = 0;

            g_builds.reserve(renderItemGroups.size());

            for (const std::vector<uint32_t>& sceneRenderItemIndices : renderItemGroups) {
                if (sceneRenderItemIndices.empty()) continue;

                std::vector<RayQueryMeshInstance> meshInstances;
                std::vector<uint32_t> validSceneRenderItemIndices;
                meshInstances.reserve(sceneRenderItemIndices.size());
                validSceneRenderItemIndices.reserve(sceneRenderItemIndices.size());
                for (uint32_t sceneRenderItemIndex : sceneRenderItemIndices) {
                    if (sceneRenderItemIndex >= sceneRenderItems.size()) continue;

                    const RenderItem& renderItem = sceneRenderItems[sceneRenderItemIndex];
                    RayQueryMeshInstance meshInstance = CreateRayQueryMeshInstance(renderItem);
                    if (MeshFitsBuffers(meshInstance.mesh, skinnedVertexBufferSize, assetIndexBufferSize)) {
                        meshInstances.push_back(meshInstance);
                        validSceneRenderItemIndices.push_back(sceneRenderItemIndex);
                    }
                }
                if (meshInstances.empty()) continue;

                size_t slotIndex = activeSlotCount;
                if (frameData.accelerationStructures.skinnedBLAS.size() <= slotIndex) {
                    frameData.accelerationStructures.skinnedBLAS.resize(slotIndex + 1);
                }

                VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot = frameData.accelerationStructures.skinnedBLAS[slotIndex];
                if (slot.id == 0) {
                    slot.id = VulkanResourceManager::CreateAccelerationStructure();
                }

                VulkanAccelerationStructure* blas = VulkanResourceManager::GetAccelerationStructure(slot.id);
                bool slotHasUsableBLAS = blas && blas->GetHandle() != VK_NULL_HANDLE && blas->GetDeviceAddress() != 0 && blas->m_built;

                // Same layout as last frame means update the existing BLAS
                bool update = slotHasUsableBLAS && SlotMatches(slot, meshInstances);
                VkDeviceSize scratchSize = update ? (slot.updateScratchSize != 0 ? slot.updateScratchSize : slot.buildScratchSize) : 0;
                bool blasReady = true;

                if (!update) {
                    // New layout or missing BLAS means full rebuild
                    VkAccelerationStructureBuildSizesInfoKHR sizeInfo = QueryBottomLevelBuildSize(g_vertexBufferAddress, g_indexBufferAddress, meshInstances, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR);
                    if (sizeInfo.accelerationStructureSize == 0 || !PrepareAccelerationStructure(slot.id, VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR, sizeInfo)) {
                        blasReady = false;
                    }
                    else {
                        StoreSlotMetadata(slot, meshInstances, sizeInfo);
                        scratchSize = sizeInfo.buildScratchSize;
                        blas = VulkanResourceManager::GetAccelerationStructure(slot.id);
                    }
                }

                if (!blasReady || !blas || blas->GetDeviceAddress() == 0) continue;

                activeSlotCount++;

                QueuedBuild& build = g_builds.emplace_back();
                build.blasId = slot.id;
                build.slotIndex = slotIndex;
                build.meshInstances = meshInstances;
                build.update = update;
                build.scratchSize = scratchSize;
                build.scratchOffset = AllocateScratch(build.scratchSize);

                const RenderItem& firstRenderItem = sceneRenderItems[validSceneRenderItemIndices.front()];
                scene.AddBLASInstance(blas->GetDeviceAddress(), TransformMatrixKHR(firstRenderItem.modelMatrix), g_vertexBufferAddress, g_indexBufferAddress, validSceneRenderItemIndices);
            }

            DestroySlots(frameData.accelerationStructures.skinnedBLAS, activeSlotCount);
        }

        VkDeviceSize GetScratchSize() {
            return g_scratchSize;
        }

        bool RecordBuilds(VkCommandBuffer commandBuffer, VulkanFrameData& frameData, uint64_t scratchBaseAddress) {
            if (g_builds.empty()) return false;

            size_t geometryCount = 0;
            for (const QueuedBuild& build : g_builds) {
                geometryCount += build.meshInstances.size();
            }

            ClearBuildBatch();
            ReserveBuildBatch(g_builds.size(), geometryCount);

            for (const QueuedBuild& build : g_builds) {
                QueueBuild(build.blasId, g_vertexBufferAddress, g_indexBufferAddress, build.meshInstances, scratchBaseAddress + build.scratchOffset, build.update, build.slotIndex);
            }

            return RecordBuildBatch(commandBuffer, frameData);
        }
    }

    namespace {
        void ClearBuildBatch() {
            g_geometries.clear();
            g_buildInfos.clear();
            g_rangeInfos.clear();
            g_rangeInfoPtrs.clear();
            g_blasIds.clear();
            g_slotIndices.clear();
        }

        void ReserveBuildBatch(size_t buildCount, size_t geometryCount) {
            g_geometries.reserve(geometryCount);
            g_buildInfos.reserve(buildCount);
            g_rangeInfos.reserve(geometryCount);
            g_rangeInfoPtrs.reserve(buildCount);
            g_blasIds.reserve(buildCount);
            g_slotIndices.reserve(buildCount);
        }

        void QueueBuild(uint64_t blasId, uint64_t vertexBufferAddress, uint64_t indexBufferAddress, const std::vector<RayQueryMeshInstance>& meshInstances, uint64_t scratchBufferAddress, bool update, size_t slotIndex) {
            // Queue only, record later as one batch
            VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(blasId);
            if (!accelerationStructure || accelerationStructure->GetHandle() == VK_NULL_HANDLE || meshInstances.empty()) return;

            size_t geometryOffset = g_geometries.size();
            size_t rangeInfoOffset = g_rangeInfos.size();

            for (const RayQueryMeshInstance& meshInstance : meshInstances) {
                g_geometries.emplace_back(CreateTriangleGeometry(vertexBufferAddress, indexBufferAddress, meshInstance.mesh, GetRayQueryGeometryFlags(meshInstance.material)));

                VkAccelerationStructureBuildRangeInfoKHR& rangeInfo = g_rangeInfos.emplace_back();
                rangeInfo.primitiveCount = meshInstance.mesh.indexCount / 3;
                rangeInfo.primitiveOffset = 0;
                rangeInfo.firstVertex = 0;
                rangeInfo.transformOffset = 0;
            }

            VkAccelerationStructureBuildGeometryInfoKHR buildInfo{ VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR };
            buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
            buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR | VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
            buildInfo.mode = update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
            buildInfo.srcAccelerationStructure = update ? accelerationStructure->GetHandle() : VK_NULL_HANDLE;
            buildInfo.dstAccelerationStructure = accelerationStructure->GetHandle();
            buildInfo.geometryCount = static_cast<uint32_t>(meshInstances.size());
            buildInfo.pGeometries = g_geometries.data() + geometryOffset;
            buildInfo.scratchData.deviceAddress = scratchBufferAddress;
            g_buildInfos.push_back(buildInfo);

            g_rangeInfoPtrs.push_back(g_rangeInfos.data() + rangeInfoOffset);
            g_blasIds.push_back(blasId);
            g_slotIndices.push_back(slotIndex);
        }

        bool RecordBuildBatch(VkCommandBuffer commandBuffer, VulkanFrameData& frameData) {
            if (g_buildInfos.empty()) return false;

            vkCmdBuildAccelerationStructuresKHR(commandBuffer, static_cast<uint32_t>(g_buildInfos.size()), g_buildInfos.data(), g_rangeInfoPtrs.data());

            for (size_t i = 0; i < g_blasIds.size(); i++) {
                VulkanAccelerationStructure* accelerationStructure = VulkanResourceManager::GetAccelerationStructure(g_blasIds[i]);
                if (accelerationStructure) {
                    accelerationStructure->m_built = true;
                }

                size_t slotIndex = g_slotIndices[i];
                if (slotIndex < frameData.accelerationStructures.skinnedBLAS.size()) {
                    frameData.accelerationStructures.skinnedBLAS[slotIndex].built = true;
                }
            }

            return true;
        }

        VkDeviceSize AllocateScratch(VkDeviceSize scratchSize) {
            VkDeviceSize scratchOffset = AlignUp(g_scratchSize, AccelerationStructureScratchAlignment());
            g_scratchSize = scratchOffset + scratchSize;
            return scratchOffset;
        }

        void HashMix(uint64_t& hash, uint64_t value) {
            hash ^= value;
            hash *= 1099511628211ull;
        }

        uint64_t HashMeshInstances(const std::vector<RayQueryMeshInstance>& meshInstances) {
            // Hash BLAS shape only
            // Skinned vertex positions change every frame and still use this layout
            uint64_t hash = 1469598103934665603ull;
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

        bool SlotMatches(const VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryMeshInstance>& meshInstances) {
            // Matching hash means update, mismatched hash means rebuild
            return slot.built &&
                slot.geometryCount == static_cast<uint32_t>(meshInstances.size()) &&
                slot.geometryHash == HashMeshInstances(meshInstances) &&
                slot.accelerationStructureSize != 0 &&
                (slot.updateScratchSize != 0 || slot.buildScratchSize != 0);
        }

        void StoreSlotMetadata(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot, const std::vector<RayQueryMeshInstance>& meshInstances, const VkAccelerationStructureBuildSizesInfoKHR& sizeInfo) {
            slot.geometryCount = static_cast<uint32_t>(meshInstances.size());
            slot.geometryHash = HashMeshInstances(meshInstances);
            slot.accelerationStructureSize = sizeInfo.accelerationStructureSize;
            slot.buildScratchSize = sizeInfo.buildScratchSize;
            slot.updateScratchSize = sizeInfo.updateScratchSize;
            slot.built = false;
        }

        bool MeshFitsBuffers(const RayQueryMesh& mesh, VkDeviceSize vertexBufferSize, VkDeviceSize indexBufferSize) {
            // Do not feed bad ranges into the driver
            if (mesh.vertexCount == 0 || mesh.indexCount < 3) return false;

            uint64_t vertexEnd = static_cast<uint64_t>(mesh.baseVertex) + static_cast<uint64_t>(mesh.vertexCount);
            uint64_t indexEnd = static_cast<uint64_t>(mesh.baseIndex) + static_cast<uint64_t>(mesh.indexCount);
            uint64_t vertexCapacity = vertexBufferSize / sizeof(Vertex);
            uint64_t indexCapacity = indexBufferSize / sizeof(uint32_t);
            return vertexEnd <= vertexCapacity && indexEnd <= indexCapacity;
        }

        void DestroySlot(VulkanFrameData::AccelerationStructures::SkinnedBLASSlot& slot) {
            if (slot.id != 0 && VulkanResourceManager::AccelerationStructureExists(slot.id)) {
                VulkanResourceManager::RemoveAccelerationStructure(slot.id);
            }

            slot = {};
        }

        void DestroySlots(std::vector<VulkanFrameData::AccelerationStructures::SkinnedBLASSlot>& slots, size_t firstSlot) {
            if (firstSlot >= slots.size()) return;

            for (size_t i = firstSlot; i < slots.size(); i++) {
                DestroySlot(slots[i]);
            }

            slots.resize(firstSlot);
        }
    }
}
