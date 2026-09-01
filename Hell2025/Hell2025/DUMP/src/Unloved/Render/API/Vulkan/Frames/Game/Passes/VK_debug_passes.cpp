#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_generic_mesh.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/Vulkan/VK_draw.h"
#include "Unloved/Render/API/Vulkan/VK_push_constants.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Systems/DDGI/DDGIManager.h"
#include "Unloved/Viewport/ViewportManager.h"

using namespace Unloved;

namespace VulkanRenderer {
    namespace {
        constexpr const char* DDGI_PROBE_STATES_BUFFER_NAME = "ProbeStates";

        void RecordComputeToVertexReadBarrier(VkCommandBuffer commandBuffer, VulkanBuffer* buffer) {
            if (!buffer || buffer->GetBuffer() == VK_NULL_HANDLE) return;

            VkBufferMemoryBarrier barrier{ VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
            barrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.buffer = buffer->GetBuffer();
            barrier.offset = 0;
            barrier.size = buffer->GetSize();

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, 0, 0, nullptr, 1, &barrier, 0, nullptr);
        }
    }

    void DDGIPointCloudDebugPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        if (!Renderer::GetCurrentRendererSettings().debugDrawPointCloud) return;

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Debug3D");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIPointCloudDebug");

        if (!lightingImage) return;
        if (!renderState) return;
        if (!pipeline) return;

        Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();
        if (ddgiVolumes.empty()) return;

        VkExtent2D extent = lightingImage->GetExtent2D();

        PushConstantsDDGIPointCloudDebug pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        if (pushConstants.frameAddressTableDeviceAddress == 0) return;

        for (DDGIVolume& ddgiVolume : ddgiVolumes) {
            if (ddgiVolume.GetPointCloudCount() == 0) continue;

            VulkanBuffer* pointCloudBuffer = VulkanResourceManager::GetBuffer(ddgiVolume.GetPointCloudSSBOName());
            VulkanBuffer* pointCloudDirtyFlagsBuffer = VulkanResourceManager::GetBuffer(ddgiVolume.GetPointCloudDirtyFlagsSSBOName());

            RecordComputeToVertexReadBarrier(commandBuffer, pointCloudBuffer);
            RecordComputeToVertexReadBarrier(commandBuffer, pointCloudDirtyFlagsBuffer);
        }

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());

        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = viewportIndex;

            for (DDGIVolume& ddgiVolume : ddgiVolumes) {
                pushConstants.pointCount = ddgiVolume.GetPointCloudCount();
                if (pushConstants.pointCount == 0) continue;

                VulkanBuffer* pointCloudBuffer = VulkanResourceManager::GetBuffer(ddgiVolume.GetPointCloudSSBOName());
                VulkanBuffer* pointCloudDirtyFlagsBuffer = VulkanResourceManager::GetBuffer(ddgiVolume.GetPointCloudDirtyFlagsSSBOName());
                if (!pointCloudBuffer || !pointCloudDirtyFlagsBuffer) continue;

                pushConstants.pointCloudDeviceAddress = pointCloudBuffer->GetDeviceAddress();
                pushConstants.pointCloudDirtyFlagsDeviceAddress = pointCloudDirtyFlagsBuffer->GetDeviceAddress();
                if (pushConstants.pointCloudDeviceAddress == 0 || pushConstants.pointCloudDirtyFlagsDeviceAddress == 0) continue;

                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                vkCmdDraw(commandBuffer, pushConstants.pointCount, 1, 0, 0);
            }
        }

        EndRenderState(commandBuffer);
    }

    void DDGIProbeDebugPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        const RendererSettings& rendererSettings = Renderer::GetCurrentRendererSettings();
        if (!rendererSettings.debugDrawIrradianceProbes) return;

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("DDGIProbeDebug");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DDGIProbeDebug");
        VulkanMeshBuffer* assetGeometry = VulkanResourceManager::GetMeshBuffer("AssetGeometry");
        Model* sphereModel = Hell::ResourceManager::GetModelByName("Sphere");

        if (!lightingImage) return;
        if (!renderState) return;
        if (!pipeline) return;
        if (!assetGeometry) return;
        if (!assetGeometry->GetVertexBuffer() || !assetGeometry->GetIndexBuffer()) return;
        if (!sphereModel || sphereModel->GetMeshIndices().empty()) return;

        VulkanBuffer* probeStatesBuffer = VulkanResourceManager::BufferExists(DDGI_PROBE_STATES_BUFFER_NAME) ? VulkanResourceManager::GetBuffer(DDGI_PROBE_STATES_BUFFER_NAME) : nullptr;
        if (!probeStatesBuffer || probeStatesBuffer->GetDeviceAddress() == 0) return;

        const uint32_t meshId = sphereModel->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
        if (!mesh || mesh->indexCount == 0) return;

        Hell::SlotMap<DDGIVolume>& ddgiVolumes = DDGIManager::GetVolumes();
        if (ddgiVolumes.empty()) return;

        PushConstantsDDGIProbeDebug pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        pushConstants.probeStatesDeviceAddress = probeStatesBuffer->GetDeviceAddress();
        pushConstants.probeDebugState = static_cast<uint32_t>(rendererSettings.probeDebugState);
        if (pushConstants.frameAddressTableDeviceAddress == 0) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        RecordComputeToVertexReadBarrier(commandBuffer, probeStatesBuffer);

        if (!BeginRenderState(commandBuffer, *renderState, extent)) return;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        BindVertexBuffer(commandBuffer, assetGeometry->GetVertexBuffer());
        BindIndexBuffer(commandBuffer, assetGeometry->GetIndexBuffer());

        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            pushConstants.viewportIndex = viewportIndex;

            for (DDGIVolume& ddgiVolume : ddgiVolumes) {
                const uint32_t probeCount = ddgiVolume.GetTotalProbeCount();
                if (probeCount == 0) continue;

                const DDGIVolumeGPU volume = ddgiVolume.GetGPUData();
                pushConstants.volumeOrigin = volume.origin;
                pushConstants.probeSpacing = volume.probeSpacing;
                pushConstants.probeCounts = volume.probeCounts;
                pushConstants.probeOffset = ddgiVolume.GetProbeOffset();

                vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                vkCmdDrawIndexed(commandBuffer, mesh->indexCount, probeCount, mesh->baseIndex, mesh->baseVertex, 0);
            }
        }

        EndRenderState(commandBuffer);
    }

    void DebugPass(VkCommandBuffer commandBuffer) {
        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        if (!lightingImage) return;

        const VulkanFrameData& frameData = GetCurrentFrameData();
        VulkanGenericMesh* lines2D = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.debugLines2D);
        VulkanGenericMesh* lines3D = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.debugLines3D);
        VulkanGenericMesh* points2D = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.debugPoints2D);
        VulkanGenericMesh* points3D = VulkanResourceManager::GetGenericMesh(frameData.genericMeshes.debugPoints3D);

        bool has3D = (lines3D && lines3D->GetVertexCount() > 0) || (points3D && points3D->GetVertexCount() > 0);
        bool has2D = (lines2D && lines2D->GetVertexCount() > 0) || (points2D && points2D->GetVertexCount() > 0);
        if (!has3D && !has2D) return;

        VkExtent2D extent = lightingImage->GetExtent2D();

        if (has3D) {
            VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Debug3D");
            VulkanPipeline* linePipeline = VulkanResourceManager::GetPipeline("DebugVertex3DLine");
            VulkanPipeline* pointPipeline = VulkanResourceManager::GetPipeline("DebugVertex3DPoint");
            PushConstantsDebug3D pushConstants{};
            pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();

            if (pushConstants.frameAddressTableDeviceAddress != 0 && renderState && BeginRenderState(commandBuffer, *renderState, extent)) {
                for (uint32_t i = 0; i < 4; i++) {
                    Viewport* viewport = ViewportManager::GetViewportByIndex(i);
                    if (!viewport || !viewport->IsVisible()) continue;

                    pushConstants.viewportIndex = i;
                    SetGameViewportAndScissor(commandBuffer, *viewport, extent);

                    if (linePipeline && lines3D && lines3D->GetVertexCount() > 0) {
                        VulkanBuffer* vertexBuffer = lines3D->GetVertexBuffer();
                        if (vertexBuffer) {
                            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline->GetHandle());
                            vkCmdPushConstants(commandBuffer, linePipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                            BindVertexBuffer(commandBuffer, vertexBuffer);
                            vkCmdDraw(commandBuffer, static_cast<uint32_t>(lines3D->GetVertexCount()), 1, 0, 0);
                        }
                    }

                    if (pointPipeline && points3D && points3D->GetVertexCount() > 0) {
                        VulkanBuffer* vertexBuffer = points3D->GetVertexBuffer();
                        if (vertexBuffer) {
                            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline->GetHandle());
                            vkCmdPushConstants(commandBuffer, pointPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                            BindVertexBuffer(commandBuffer, vertexBuffer);
                            vkCmdDraw(commandBuffer, static_cast<uint32_t>(points3D->GetVertexCount()), 1, 0, 0);
                        }
                    }
                }
                EndRenderState(commandBuffer);
            }
        }

        if (has2D) {
            VulkanRenderState* renderState = VulkanResourceManager::GetRenderState("Debug2D");
            VulkanPipeline* linePipeline = VulkanResourceManager::GetPipeline("DebugVertex2DLine");
            VulkanPipeline* pointPipeline = VulkanResourceManager::GetPipeline("DebugVertex2DPoint");
            PushConstantsDebug2D pushConstants{};
            pushConstants.renderTargetWidth = static_cast<float>(extent.width);
            pushConstants.renderTargetHeight = static_cast<float>(extent.height);

            if (renderState && BeginRenderState(commandBuffer, *renderState, extent)) {
                VkViewport viewport{};
                viewport.width = static_cast<float>(extent.width);
                viewport.height = static_cast<float>(extent.height);
                viewport.maxDepth = 1.0f;
                vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

                VkRect2D scissor{};
                scissor.extent = extent;
                vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

                if (linePipeline && lines2D && lines2D->GetVertexCount() > 0) {
                    VulkanBuffer* vertexBuffer = lines2D->GetVertexBuffer();
                    if (vertexBuffer) {
                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, linePipeline->GetHandle());
                        vkCmdPushConstants(commandBuffer, linePipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                        BindVertexBuffer(commandBuffer, vertexBuffer);
                        vkCmdDraw(commandBuffer, static_cast<uint32_t>(lines2D->GetVertexCount()), 1, 0, 0);
                    }
                }

                if (pointPipeline && points2D && points2D->GetVertexCount() > 0) {
                    VulkanBuffer* vertexBuffer = points2D->GetVertexBuffer();
                    if (vertexBuffer) {
                        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pointPipeline->GetHandle());
                        vkCmdPushConstants(commandBuffer, pointPipeline->GetLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pushConstants), &pushConstants);
                        BindVertexBuffer(commandBuffer, vertexBuffer);
                        vkCmdDraw(commandBuffer, static_cast<uint32_t>(points2D->GetVertexCount()), 1, 0, 0);
                    }
                }
                EndRenderState(commandBuffer);
            }
        }
    }

    void DebugViewPass(VkCommandBuffer commandBuffer) {
        if (!Renderer::OverrideStateUsesDebugViewPass()) return;

        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        AllocatedImage* baseColorImage = VulkanResourceManager::GetAllocatedImage("BaseColorMetallic");
        AllocatedImage* normalImage = VulkanResourceManager::GetAllocatedImage("NormalXYRoughnessMisc");
        AllocatedImage* velocityImage = VulkanResourceManager::GetAllocatedImage("VelocityXYOcclusionSubSurface");
        AllocatedImage* visibilityImage = VulkanResourceManager::GetAllocatedImage("Visibility");
        AllocatedImage* depthImage = VulkanResourceManager::GetAllocatedImage("Depth");
        AllocatedImage* emissiveImage = VulkanResourceManager::GetAllocatedImage("Emissive");
        AllocatedImage* indirectDiffuseImage = VulkanResourceManager::GetAllocatedImage("IndirectDiffuse");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DebugView");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");
        if (!pipeline) return;
        if (!staticDescriptorSet) return;
        if (!lightingImage) return;
        if (!baseColorImage) return;
        if (!normalImage) return;
        if (!velocityImage) return;
        if (!visibilityImage) return;
        if (!depthImage) return;
        if (!emissiveImage) return;
        if (!indirectDiffuseImage) return;
        VkExtent2D extent = lightingImage->GetExtent2D();
        baseColorImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        normalImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        velocityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        visibilityImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        depthImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        emissiveImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        indirectDiffuseImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT);

        VkRenderingAttachmentInfo colorAttachment{ VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
        colorAttachment.imageView = lightingImage->GetImageView();
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        VkRenderingInfo renderingInfo{ VK_STRUCTURE_TYPE_RENDERING_INFO };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetHandle());
        VkDescriptorSet descriptorSets[] = { staticDescriptorSet->GetHandle() };
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->GetLayout(), 0, 1, descriptorSets, 0, nullptr);

        PushConstantsDebugView pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushConstants), &pushConstants);

        for (uint32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            SetGameViewportAndScissor(commandBuffer, *viewport, extent);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }

        vkCmdEndRendering(commandBuffer);
    }

    void DebugTileViewPass(VkCommandBuffer commandBuffer) {
        if (!Renderer::OverrideStateUsesDebugTileViewPass()) return;

        ProfilerVulkanZoneFunction();

        AllocatedImage* lightingImage = VulkanResourceManager::GetAllocatedImage("Lighting");
        VulkanPipeline* pipeline = VulkanResourceManager::GetPipeline("DebugTileView");
        VulkanDescriptorSet* staticDescriptorSet = VulkanResourceManager::GetDescriptorSet("StaticDescriptorSet");

        if (!lightingImage) return;
        if (!pipeline) return;
        if (!staticDescriptorSet) return;

        VkExtent2D extent = lightingImage->GetExtent2D();
        lightingImage->Sync(commandBuffer, VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT);

        PushConstantsDebugTileView pushConstants{};
        pushConstants.frameAddressTableDeviceAddress = GetFrameAddressTableDeviceAddress();
        vkCmdPushConstants(commandBuffer, pipeline->GetLayout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(pushConstants), &pushConstants);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetHandle());
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->GetLayout(), 0, 1, staticDescriptorSet->GetHandlePtr(), 0, nullptr);

        uint32_t groupCountX = (extent.width + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t groupCountY = (extent.height + TILE_SIZE - 1) / TILE_SIZE;
        vkCmdDispatch(commandBuffer, groupCountX, groupCountY, 1);
    }
}
