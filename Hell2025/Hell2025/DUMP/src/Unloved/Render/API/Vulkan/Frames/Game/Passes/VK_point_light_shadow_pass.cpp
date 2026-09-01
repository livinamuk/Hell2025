#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_cube_map_array.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/DrawCommandSets.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/World/World.h"

#include <initializer_list>

using namespace Unloved;

namespace VulkanRenderer {
    namespace {
        constexpr VkFormat POINT_SHADOW_FORMAT = VK_FORMAT_D16_UNORM;
        constexpr const char* POINT_SHADOW_STATIC_HI_RES = "PointShadowStaticHiRes";
        constexpr const char* POINT_SHADOW_STATIC_LOW_RES = "PointShadowStaticLowRes";
        constexpr const char* POINT_SHADOW_HI_RES = "PointShadowHiRes";
        constexpr const char* POINT_SHADOW_LOW_RES = "PointShadowLowRes";

        glm::mat4 GetVulkanPointShadowProjectionView(const Light& light, uint32_t faceIndex) {
            glm::mat4 undoOpenGLClipOriginYFlip(1.0f);
            undoOpenGLClipOriginYFlip[1][1] = -1.0f;
            return undoOpenGLClipOriginYFlip * light.GetProjectionView(faceIndex);
        }

        void CreatePointShadowMap(const char* name, uint32_t count, uint32_t size, bool createSampler) {
            VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            VulkanCubeMapArray& shadowMaps = VulkanResourceManager::CreateCubeMapArray(name);
            shadowMaps.Create(count, size, POINT_SHADOW_FORMAT, 1, usage, name);
            if (createSampler) {
                shadowMaps.CreateSampler(TextureWrapMode::CLAMP_TO_EDGE, TextureFilter::LINEAR, TextureFilter::LINEAR, glm::vec4(1.0f), true, VK_COMPARE_OP_LESS_OR_EQUAL);
            }
        }

        bool PointShadowResourcesValid(VulkanCubeMapArray* shadowMaps, VulkanPipeline* opaquePipeline, VulkanPipeline* alphaTestedPipeline, VulkanDescriptorSet* staticDescriptorSet, VulkanMeshBuffer* assetGeometry, VulkanMeshBuffer* proceduralGeometry, VulkanBuffer* skinnedVertexBuffer) {
            if (!shadowMaps || !opaquePipeline || !alphaTestedPipeline || !staticDescriptorSet || !assetGeometry || !proceduralGeometry || !skinnedVertexBuffer) return false;
            if (shadowMaps->GetImage() == VK_NULL_HANDLE || shadowMaps->GetSize() == 0) return false;
            if (opaquePipeline->GetHandle() == VK_NULL_HANDLE || opaquePipeline->GetLayout() == VK_NULL_HANDLE) return false;
            if (alphaTestedPipeline->GetHandle() == VK_NULL_HANDLE || alphaTestedPipeline->GetLayout() == VK_NULL_HANDLE) return false;
            if (!assetGeometry->GetVertexBuffer() || !assetGeometry->GetIndexBuffer()) return false;
            if (!proceduralGeometry->GetVertexBuffer() || !proceduralGeometry->GetIndexBuffer()) return false;
            return GetFrameAddressTableDeviceAddress() != 0;
        }

        void SetPointShadowViewport(VkCommandBuffer commandBuffer, uint32_t size) {
            VkViewport viewport{};
            viewport.width = static_cast<float>(size);
            viewport.height = static_cast<float>(size);
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.extent = { size, size };
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        }

        size_t GetPointShadowFaceDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, uint32_t shadowMapIndex, uint32_t faceIndex) {
            return drawCommands.procedural[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometry[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometryHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex].size()
                + drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex].size();
        }

        size_t GetPointShadowDrawCommandCount(const PointLightShadowMapDrawCommands& drawCommands, const std::vector<ShadowMapInfo>& shadowMapInfos) {
            size_t count = 0;
            for (const ShadowMapInfo& shadowMapInfo : shadowMapInfos) {
                if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS) continue;
                const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
                for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
                    count += GetPointShadowFaceDrawCommandCount(drawCommands, shadowMapIndex, faceIndex);
                }
            }
            return count;
        }

        bool EnsurePointShadowDrawCommandCapacity(VulkanBuffer& drawCommandBuffer, const DrawCommandsSet& drawCommands, const std::vector<ShadowMapInfo>& staticHiResInfos, const std::vector<ShadowMapInfo>& staticLowResInfos, const std::vector<ShadowMapInfo>& compositeHiResInfos, const std::vector<ShadowMapInfo>& compositeLowResInfos) {
            size_t shadowCommandCount = GetPointShadowDrawCommandCount(drawCommands.staticHiResShadowMapDrawCommands, staticHiResInfos);
            shadowCommandCount += GetPointShadowDrawCommandCount(drawCommands.staticLowResShadowMapDrawCommands, staticLowResInfos);
            shadowCommandCount += GetPointShadowDrawCommandCount(drawCommands.compositeHiResShadowMapDrawCommands, compositeHiResInfos);
            shadowCommandCount += GetPointShadowDrawCommandCount(drawCommands.compositeLowResShadowMapDrawCommands, compositeLowResInfos);

            const VkDeviceSize requiredSize = sizeof(DrawIndexedIndirectCommand) * (shadowCommandCount + MAX_INDIRECT_DRAW_COMMAND_COUNT);
            if (drawCommandBuffer.GetSize() >= requiredSize) return true;

            const VkDeviceSize doubledSize = drawCommandBuffer.GetSize() * 2;
            return drawCommandBuffer.EnsureSize(requiredSize > doubledSize ? requiredSize : doubledSize);
        }

        void AppendPointShadowDrawCommands(std::vector<DrawIndexedIndirectCommand>& destinationCommands, std::vector<uint32_t>& destinationFaceDataIndices, uint32_t faceDataIndex, std::initializer_list<const std::vector<DrawIndexedIndirectCommand>*> sources) {
            for (const std::vector<DrawIndexedIndirectCommand>* source : sources) {
                if (!source || source->empty()) continue;
                for (const DrawIndexedIndirectCommand& command : *source) {
                    destinationCommands.push_back(command);
                    destinationFaceDataIndices.push_back(faceDataIndex);
                }
            }
        }

        VkDeviceSize AlignPointShadowFaceDataOffset(VkDeviceSize offset) {
            constexpr VkDeviceSize ALIGNMENT = 16;
            return (offset + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
        }

        bool UploadPointShadowFaceData(VulkanBuffer& faceDataBuffer, VkDeviceSize& faceDataOffset, const std::vector<PointShadowFaceData>& faceData, uint64_t& deviceAddress) {
            if (faceData.empty()) return false;
            faceDataOffset = AlignPointShadowFaceDataOffset(faceDataOffset);
            const VkDeviceSize uploadSize = sizeof(PointShadowFaceData) * faceData.size();
            if (faceDataOffset > faceDataBuffer.GetSize() || uploadSize > faceDataBuffer.GetSize() - faceDataOffset) return false;
            faceDataBuffer.UpdateData(faceData.data(), uploadSize, faceDataOffset);
            deviceAddress = faceDataBuffer.GetDeviceAddress() + faceDataOffset;
            faceDataOffset += uploadSize;
            return deviceAddress != 0;
        }

        bool UploadPointShadowDrawFaceDataIndices(VulkanBuffer& faceDataBuffer, VkDeviceSize& faceDataOffset, const std::vector<uint32_t>& faceDataIndices, uint64_t& deviceAddress) {
            deviceAddress = 0;
            if (faceDataIndices.empty()) return true;

            constexpr VkDeviceSize ALIGNMENT = alignof(uint32_t);
            faceDataOffset = (faceDataOffset + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
            const VkDeviceSize uploadSize = sizeof(uint32_t) * faceDataIndices.size();
            if (faceDataOffset > faceDataBuffer.GetSize() || uploadSize > faceDataBuffer.GetSize() - faceDataOffset) return false;

            faceDataBuffer.UpdateData(faceDataIndices.data(), uploadSize, faceDataOffset);
            deviceAddress = faceDataBuffer.GetDeviceAddress() + faceDataOffset;
            faceDataOffset += uploadSize;
            return deviceAddress != 0;
        }

        void RenderPointShadowMaps(VkCommandBuffer commandBuffer, VulkanBuffer& drawCommandBuffer, VulkanBuffer& faceDataBuffer, VkDeviceSize& faceDataOffset, VulkanCubeMapArray* shadowMaps, const std::vector<ShadowMapInfo>& shadowMapInfos, const PointLightShadowMapDrawCommands& drawCommands, VkAttachmentLoadOp loadOp) {
            if (!shadowMaps || shadowMapInfos.empty()) return;

            VulkanPipeline* opaquePipeline = VulkanResourceManager::GetPipeline("PointShadowOpaque");
            VulkanPipeline* alphaTestedPipeline = VulkanResourceManager::GetPipeline("PointShadowAlphaDiscard");
            VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
            VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
            VulkanMeshBuffer* proceduralGeometry = VulkanResourceManager::GetMeshBuffer("Procedural");
            const VulkanFrameData& frameData = GetCurrentFrameData();
            VulkanBuffer* skinnedVertexBuffer = VulkanResourceManager::GetBuffer(frameData.buffers.skinnedVertices);
            if (!PointShadowResourcesValid(shadowMaps, opaquePipeline, alphaTestedPipeline, staticDescriptorSet, assetGeometry, proceduralGeometry, skinnedVertexBuffer)) return;
            if (shadowMaps->GetArrayImageView() == VK_NULL_HANDLE) return;

            std::vector<PointShadowFaceData> faceData;
            std::vector<VkClearRect> clearRects;
            std::vector<DrawIndexedIndirectCommand> proceduralCommands;
            std::vector<DrawIndexedIndirectCommand> opaqueAssetCommands;
            std::vector<DrawIndexedIndirectCommand> opaqueSkinnedCommands;
            std::vector<DrawIndexedIndirectCommand> alphaTestedAssetCommands;
            std::vector<DrawIndexedIndirectCommand> alphaTestedSkinnedCommands;
            std::vector<uint32_t> proceduralFaceDataIndices;
            std::vector<uint32_t> opaqueAssetFaceDataIndices;
            std::vector<uint32_t> opaqueSkinnedFaceDataIndices;
            std::vector<uint32_t> alphaTestedAssetFaceDataIndices;
            std::vector<uint32_t> alphaTestedSkinnedFaceDataIndices;
            faceData.reserve(shadowMapInfos.size() * 6);
            clearRects.reserve(shadowMapInfos.size());

            const size_t drawCommandCount = GetPointShadowDrawCommandCount(drawCommands, shadowMapInfos);
            proceduralCommands.reserve(drawCommandCount / 5);
            opaqueAssetCommands.reserve(drawCommandCount / 5);
            opaqueSkinnedCommands.reserve(drawCommandCount / 5);
            alphaTestedAssetCommands.reserve(drawCommandCount / 5);
            alphaTestedSkinnedCommands.reserve(drawCommandCount / 5);
            proceduralFaceDataIndices.reserve(drawCommandCount / 5);
            opaqueAssetFaceDataIndices.reserve(drawCommandCount / 5);
            opaqueSkinnedFaceDataIndices.reserve(drawCommandCount / 5);
            alphaTestedAssetFaceDataIndices.reserve(drawCommandCount / 5);
            alphaTestedSkinnedFaceDataIndices.reserve(drawCommandCount / 5);

            for (const ShadowMapInfo& shadowMapInfo : shadowMapInfos) {
                if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= MAX_SHADOW_MAP_ARRAY_LEVELS || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(shadowMaps->GetCubeMapCount())) continue;

                Light* light = World::GetLightByObjectId(shadowMapInfo.lightId);
                if (!light) continue;

                const uint32_t shadowMapIndex = static_cast<uint32_t>(shadowMapInfo.shadowMapIndex);
                if (loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
                    VkClearRect& clearRect = clearRects.emplace_back();
                    clearRect.rect.extent = { shadowMaps->GetSize(), shadowMaps->GetSize() };
                    clearRect.baseArrayLayer = shadowMapIndex * 6;
                    clearRect.layerCount = 6;
                }

                for (uint32_t faceIndex = 0; faceIndex < 6; faceIndex++) {
                    const uint32_t faceDataIndex = static_cast<uint32_t>(faceData.size());
                    PointShadowFaceData& currentFaceData = faceData.emplace_back();
                    currentFaceData.projectionView = GetVulkanPointShadowProjectionView(*light, faceIndex);
                    currentFaceData.lightPositionRadius = glm::vec4(light->GetPosition(), light->GetRadius());
                    currentFaceData.arrayLayer = shadowMapIndex * 6 + faceIndex;

                    AppendPointShadowDrawCommands(proceduralCommands, proceduralFaceDataIndices, faceDataIndex, { &drawCommands.procedural[shadowMapIndex][faceIndex] });
                    AppendPointShadowDrawCommands(opaqueAssetCommands, opaqueAssetFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometry[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeforming[shadowMapIndex][faceIndex] });
                    AppendPointShadowDrawCommands(opaqueSkinnedCommands, opaqueSkinnedFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometrySkinned[shadowMapIndex][faceIndex] });
                    AppendPointShadowDrawCommands(alphaTestedAssetCommands, alphaTestedAssetFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometryAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometryHair[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedNonDeformingHair[shadowMapIndex][faceIndex] });
                    AppendPointShadowDrawCommands(alphaTestedSkinnedCommands, alphaTestedSkinnedFaceDataIndices, faceDataIndex, { &drawCommands.assetGeometrySkinnedAlphaDiscard[shadowMapIndex][faceIndex], &drawCommands.assetGeometrySkinnedHair[shadowMapIndex][faceIndex] });
                }
            }

            if (faceData.empty()) return;

            uint64_t faceDataDeviceAddress = 0;
            if (!UploadPointShadowFaceData(faceDataBuffer, faceDataOffset, faceData, faceDataDeviceAddress)) return;

            uint64_t proceduralFaceDataIndicesDeviceAddress = 0;
            uint64_t opaqueAssetFaceDataIndicesDeviceAddress = 0;
            uint64_t opaqueSkinnedFaceDataIndicesDeviceAddress = 0;
            uint64_t alphaTestedAssetFaceDataIndicesDeviceAddress = 0;
            uint64_t alphaTestedSkinnedFaceDataIndicesDeviceAddress = 0;
            if (!UploadPointShadowDrawFaceDataIndices(faceDataBuffer, faceDataOffset, proceduralFaceDataIndices, proceduralFaceDataIndicesDeviceAddress)) return;
            if (!UploadPointShadowDrawFaceDataIndices(faceDataBuffer, faceDataOffset, opaqueAssetFaceDataIndices, opaqueAssetFaceDataIndicesDeviceAddress)) return;
            if (!UploadPointShadowDrawFaceDataIndices(faceDataBuffer, faceDataOffset, opaqueSkinnedFaceDataIndices, opaqueSkinnedFaceDataIndicesDeviceAddress)) return;
            if (!UploadPointShadowDrawFaceDataIndices(faceDataBuffer, faceDataOffset, alphaTestedAssetFaceDataIndices, alphaTestedAssetFaceDataIndicesDeviceAddress)) return;
            if (!UploadPointShadowDrawFaceDataIndices(faceDataBuffer, faceDataOffset, alphaTestedSkinnedFaceDataIndices, alphaTestedSkinnedFaceDataIndicesDeviceAddress)) return;

            const VulkanDrawCommandBatch proceduralBatch = WriteDrawCommands(drawCommandBuffer, proceduralCommands);
            const VulkanDrawCommandBatch opaqueAssetBatch = WriteDrawCommands(drawCommandBuffer, opaqueAssetCommands);
            const VulkanDrawCommandBatch opaqueSkinnedBatch = WriteDrawCommands(drawCommandBuffer, opaqueSkinnedCommands);
            const VulkanDrawCommandBatch alphaTestedAssetBatch = WriteDrawCommands(drawCommandBuffer, alphaTestedAssetCommands);
            const VulkanDrawCommandBatch alphaTestedSkinnedBatch = WriteDrawCommands(drawCommandBuffer, alphaTestedSkinnedCommands);

            PushConstantsPointShadow pushConstants{};
            pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
            pushConstants.faceDataDeviceAddress = faceDataDeviceAddress;

            shadowMaps->Sync(commandBuffer, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT);
            SetPointShadowViewport(commandBuffer, shadowMaps->GetSize());

            VkRenderingAttachmentInfo depthAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
            depthAttachment.imageView = shadowMaps->GetArrayImageView();
            depthAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
            renderingInfo.renderArea.extent = { shadowMaps->GetSize(), shadowMaps->GetSize() };
            renderingInfo.layerCount = shadowMaps->GetArrayLayerCount();
            renderingInfo.pDepthAttachment = &depthAttachment;
            vkCmdBeginRendering(commandBuffer, &renderingInfo);

            if (!clearRects.empty()) {
                VkClearAttachment clearAttachment{};
                clearAttachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                clearAttachment.clearValue.depthStencil.depth = 1.0f;
                vkCmdClearAttachments(commandBuffer, 1, &clearAttachment, static_cast<uint32_t>(clearRects.size()), clearRects.data());
            }

            if (proceduralBatch.count > 0 || opaqueAssetBatch.count > 0 || opaqueSkinnedBatch.count > 0) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaquePipeline->GetHandle());

                if (proceduralBatch.count > 0) {
                    pushConstants.drawFaceDataIndicesDeviceAddress = proceduralFaceDataIndicesDeviceAddress;
                    vkCmdPushConstants(commandBuffer, opaquePipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                    BindVertexBuffer(commandBuffer, proceduralGeometry->GetVertexBuffer());
                    BindIndexBuffer(commandBuffer, proceduralGeometry->GetIndexBuffer());
                    MultiDrawIndexedCommands(commandBuffer, drawCommandBuffer, proceduralBatch.offset, proceduralBatch.count);
                }
                if (opaqueAssetBatch.count > 0) {
                    pushConstants.drawFaceDataIndicesDeviceAddress = opaqueAssetFaceDataIndicesDeviceAddress;
                    vkCmdPushConstants(commandBuffer, opaquePipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                    BindVertexBuffer(commandBuffer, assetGeometry->GetVertexBuffer());
                    BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                    MultiDrawIndexedCommands(commandBuffer, drawCommandBuffer, opaqueAssetBatch.offset, opaqueAssetBatch.count);
                }
                if (opaqueSkinnedBatch.count > 0) {
                    pushConstants.drawFaceDataIndicesDeviceAddress = opaqueSkinnedFaceDataIndicesDeviceAddress;
                    vkCmdPushConstants(commandBuffer, opaquePipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                    BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
                    BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                    MultiDrawIndexedCommands(commandBuffer, drawCommandBuffer, opaqueSkinnedBatch.offset, opaqueSkinnedBatch.count);
                }
            }

            if (alphaTestedAssetBatch.count > 0 || alphaTestedSkinnedBatch.count > 0) {
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaTestedPipeline->GetHandle());
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, alphaTestedPipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

                if (alphaTestedAssetBatch.count > 0) {
                    pushConstants.drawFaceDataIndicesDeviceAddress = alphaTestedAssetFaceDataIndicesDeviceAddress;
                    vkCmdPushConstants(commandBuffer, alphaTestedPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                    BindVertexBuffer(commandBuffer, assetGeometry->GetVertexBuffer());
                    BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                    MultiDrawIndexedCommands(commandBuffer, drawCommandBuffer, alphaTestedAssetBatch.offset, alphaTestedAssetBatch.count);
                }
                if (alphaTestedSkinnedBatch.count > 0) {
                    pushConstants.drawFaceDataIndicesDeviceAddress = alphaTestedSkinnedFaceDataIndicesDeviceAddress;
                    vkCmdPushConstants(commandBuffer, alphaTestedPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);
                    BindVertexBuffer(commandBuffer, skinnedVertexBuffer);
                    BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());
                    MultiDrawIndexedCommands(commandBuffer, drawCommandBuffer, alphaTestedSkinnedBatch.offset, alphaTestedSkinnedBatch.count);
                }
            }

            vkCmdEndRendering(commandBuffer);
        }

        void CopyPointShadowMaps(VkCommandBuffer commandBuffer, VulkanCubeMapArray* source, VulkanCubeMapArray* destination, const std::vector<ShadowMapInfo>& shadowMapInfos) {
            if (!source || !destination || shadowMapInfos.empty()) return;

            source->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);
            destination->Sync(commandBuffer, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_TRANSFER_BIT);

            for (const ShadowMapInfo& shadowMapInfo : shadowMapInfos) {
                if (shadowMapInfo.shadowMapIndex < 0 || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(source->GetCubeMapCount()) || shadowMapInfo.shadowMapIndex >= static_cast<int32_t>(destination->GetCubeMapCount())) continue;

                VkImageCopy copyRegion{};
                copyRegion.srcSubresource = { VK_IMAGE_ASPECT_DEPTH_BIT, 0, static_cast<uint32_t>(shadowMapInfo.shadowMapIndex) * 6, 6 };
                copyRegion.dstSubresource = copyRegion.srcSubresource;
                copyRegion.extent = { source->GetSize(), source->GetSize(), 1 };
                vkCmdCopyImage(commandBuffer, source->GetImage(), VK_IMAGE_LAYOUT_GENERAL, destination->GetImage(), VK_IMAGE_LAYOUT_GENERAL, 1, &copyRegion);
            }
        }
    }

    void CreatePointShadowMaps() {
        CreatePointShadowMap(POINT_SHADOW_STATIC_HI_RES, static_cast<uint32_t>(ShadowMapManager::GetShadowMapHiResMaxCount()), static_cast<uint32_t>(ShadowMapManager::GetShadowMapHiResResolution()), false);
        CreatePointShadowMap(POINT_SHADOW_STATIC_LOW_RES, static_cast<uint32_t>(ShadowMapManager::GetShadowMapLowResMaxCount()), static_cast<uint32_t>(ShadowMapManager::GetShadowMapLowResResolution()), false);
        CreatePointShadowMap(POINT_SHADOW_HI_RES, static_cast<uint32_t>(ShadowMapManager::GetShadowMapHiResMaxCount()), static_cast<uint32_t>(ShadowMapManager::GetShadowMapHiResResolution()), true);
        CreatePointShadowMap(POINT_SHADOW_LOW_RES, static_cast<uint32_t>(ShadowMapManager::GetShadowMapLowResMaxCount()), static_cast<uint32_t>(ShadowMapManager::GetShadowMapLowResResolution()), true);
        UpdateBindlessRenderTargetDescriptors();
    }

    void PointLightShadowPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const std::vector<ShadowMapInfo>& staticHiResInfos = ShadowMapManager::GetStaticDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& staticLowResInfos = ShadowMapManager::GetStaticDirtyLowResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeHiResInfos = ShadowMapManager::GetCompositeDirtyHiResShadowMaps();
        const std::vector<ShadowMapInfo>& compositeLowResInfos = ShadowMapManager::GetCompositeDirtyLowResShadowMaps();
        if (staticHiResInfos.empty() && staticLowResInfos.empty() && compositeHiResInfos.empty() && compositeLowResInfos.empty()) return;

        VulkanCubeMapArray* staticHiRes = VulkanResourceManager::GetCubeMapArray(POINT_SHADOW_STATIC_HI_RES);
        VulkanCubeMapArray* staticLowRes = VulkanResourceManager::GetCubeMapArray(POINT_SHADOW_STATIC_LOW_RES);
        VulkanCubeMapArray* compositeHiRes = VulkanResourceManager::GetCubeMapArray(POINT_SHADOW_HI_RES);
        VulkanCubeMapArray* compositeLowRes = VulkanResourceManager::GetCubeMapArray(POINT_SHADOW_LOW_RES);
        VulkanBuffer* drawCommandBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.drawCommands);
        VulkanBuffer* faceDataBuffer = VulkanResourceManager::GetBuffer(GetCurrentFrameData().buffers.pointShadowFaceData);
        if (!staticHiRes || !staticLowRes || !compositeHiRes || !compositeLowRes || !drawCommandBuffer || !faceDataBuffer) return;

        const DrawCommandsSet& drawCommands = RenderDataManager::GetDrawInfoSet();
        const bool staticCacheEnabled = ShadowMapManager::StaticCacheEnabled();
        if (!EnsurePointShadowDrawCommandCapacity(*drawCommandBuffer, drawCommands, staticHiResInfos, staticLowResInfos, compositeHiResInfos, compositeLowResInfos)) return;

        const size_t maxFaceDataCount = (staticHiResInfos.size() + staticLowResInfos.size() + compositeHiResInfos.size() + compositeLowResInfos.size()) * 6;
        size_t maxDrawFaceDataIndexCount = GetPointShadowDrawCommandCount(drawCommands.staticHiResShadowMapDrawCommands, staticHiResInfos);
        maxDrawFaceDataIndexCount += GetPointShadowDrawCommandCount(drawCommands.staticLowResShadowMapDrawCommands, staticLowResInfos);
        maxDrawFaceDataIndexCount += GetPointShadowDrawCommandCount(drawCommands.compositeHiResShadowMapDrawCommands, compositeHiResInfos);
        maxDrawFaceDataIndexCount += GetPointShadowDrawCommandCount(drawCommands.compositeLowResShadowMapDrawCommands, compositeLowResInfos);
        const VkDeviceSize requiredFaceDataSize = sizeof(PointShadowFaceData) * maxFaceDataCount + sizeof(uint32_t) * maxDrawFaceDataIndexCount + 128;
        if (!faceDataBuffer->EnsureSize(requiredFaceDataSize)) return;
        VkDeviceSize faceDataOffset = 0;

        {
            ProfilerVulkanZone("Point Shadow Static Cache");
            RenderPointShadowMaps(commandBuffer, *drawCommandBuffer, *faceDataBuffer, faceDataOffset, staticHiRes, staticHiResInfos, drawCommands.staticHiResShadowMapDrawCommands, VK_ATTACHMENT_LOAD_OP_CLEAR);
            RenderPointShadowMaps(commandBuffer, *drawCommandBuffer, *faceDataBuffer, faceDataOffset, staticLowRes, staticLowResInfos, drawCommands.staticLowResShadowMapDrawCommands, VK_ATTACHMENT_LOAD_OP_CLEAR);
        }

        {
            ProfilerVulkanZone("Point Shadow Composite");
            if (staticCacheEnabled) {
                CopyPointShadowMaps(commandBuffer, staticHiRes, compositeHiRes, compositeHiResInfos);
                CopyPointShadowMaps(commandBuffer, staticLowRes, compositeLowRes, compositeLowResInfos);
            }

            const VkAttachmentLoadOp loadOp = staticCacheEnabled ? VK_ATTACHMENT_LOAD_OP_LOAD : VK_ATTACHMENT_LOAD_OP_CLEAR;
            RenderPointShadowMaps(commandBuffer, *drawCommandBuffer, *faceDataBuffer, faceDataOffset, compositeHiRes, compositeHiResInfos, drawCommands.compositeHiResShadowMapDrawCommands, loadOp);
            RenderPointShadowMaps(commandBuffer, *drawCommandBuffer, *faceDataBuffer, faceDataOffset, compositeLowRes, compositeLowResInfos, drawCommands.compositeLowResShadowMapDrawCommands, loadOp);
        }

        if (!compositeHiResInfos.empty()) compositeHiRes->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        if (!compositeLowResInfos.empty()) compositeLowRes->Sync(commandBuffer, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
    }
}
