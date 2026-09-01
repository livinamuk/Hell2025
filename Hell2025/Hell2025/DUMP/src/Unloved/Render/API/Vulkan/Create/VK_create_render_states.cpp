#include "Unloved/Render/API/Vulkan/VK_renderer_internal.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Unloved/Render/RendererConstants.h"

namespace {

    void CreateVisibilityRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("Visibility");

        VulkanRenderTargetInfo& visibility = state.AddColorTarget("Visibility");
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        visibility.clearValue.color.uint32[0] = 0;
        visibility.clearValue.color.uint32[1] = 0;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.clearValue.depthStencil.depth = 0.0f;
        depth.clearValue.depthStencil.stencil = 0;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = true;
        state.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        state.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_REPLACE;
        state.rasterizer.stencilReadMask = 0xff;
        state.rasterizer.stencilWriteMask = 0xff;
    }

    void CreateVisibilityAlphaDiscardRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("VisibilityAlphaDiscard");

        VulkanRenderTargetInfo& visibility = state.AddColorTarget("Visibility");
        visibility.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
        state.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_REPLACE;
        state.rasterizer.stencilReadMask = 0xff;
        state.rasterizer.stencilWriteMask = 0xff;
    }

    void CreateMaterialResolveRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("MaterialResolve");

        VulkanRenderTargetInfo& baseColor = state.AddColorTarget("BaseColorMetallic");
        baseColor.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        baseColor.clearValue.color.float32[0] = 0.0f;
        baseColor.clearValue.color.float32[1] = 0.0f;
        baseColor.clearValue.color.float32[2] = 0.0f;
        baseColor.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& normal = state.AddColorTarget("NormalXYRoughnessMisc");
        normal.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        normal.clearValue.color.float32[0] = 0.0f;
        normal.clearValue.color.float32[1] = 0.0f;
        normal.clearValue.color.float32[2] = 0.0f;
        normal.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& velocity = state.AddColorTarget("VelocityXYOcclusionSubSurface");
        velocity.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        velocity.clearValue.color.float32[0] = 0.0f;
        velocity.clearValue.color.float32[1] = 0.0f;
        velocity.clearValue.color.float32[2] = 0.0f;
        velocity.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& amdMaterialRoughness = state.AddColorTarget("IndirectSpecularAMDMaterialRoughness");
        amdMaterialRoughness.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        amdMaterialRoughness.clearValue.color.float32[0] = 0.0f;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.cullFaceEnabled = false;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.stencilFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.rasterizer.stencilWriteMask = 0x00;
    }

    void CreateLightingDeferredRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("LightingDeferred");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lighting.clearValue.color.float32[0] = 0.0f;
        lighting.clearValue.color.float32[1] = 0.0f;
        lighting.clearValue.color.float32[2] = 0.0f;
        lighting.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.cullFaceEnabled = false;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilWriteMask = 0x00;

        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.stencilReadMask = STENCIL_BIT_WORLD_LIGHTING | STENCIL_BIT_VIEW_WEAPON_LIGHTING | STENCIL_BIT_GRASS;
        state.rasterizer.stencilRef = STENCIL_BIT_WORLD_LIGHTING;
    }

    void CreateIndirectSpecularAMDInputRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("IndirectSpecularAMDRayInput");

        VulkanRenderTargetInfo& amdInput = state.AddColorTarget("IndirectSpecularAMDRayInput");
        amdInput.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        amdInput.clearValue.color.float32[0] = 0;
        amdInput.clearValue.color.float32[1] = 0;
        amdInput.clearValue.color.float32[2] = 0;
        amdInput.clearValue.color.float32[3] = 0;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateLightingForwardBlendedRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("LightingForwardBlended");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.blendEnabled = true;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateSpriteSheetRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("SpriteSheet");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.blendEnabled = true;
        state.rasterizer.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        state.rasterizer.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
        state.rasterizer.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        state.rasterizer.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateSkyboxRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("Skybox");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.blendEnabled = false;
        state.rasterizer.cullFaceEnabled = false;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.stencilWriteMask = 0x00;
        state.rasterizer.stencilReadMask = 0xFF;
        state.rasterizer.stencilRef = 0; // This is any non-rendered pixel
    }

    void CreateDebug3DRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("Debug3D");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateDDGIProbeDebugRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("DDGIProbeDebug");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateDebug2DRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("Debug2D");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("Lighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = false;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateHairDepthPrepRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("HairDepthPrep");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("HairLighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        lighting.clearValue.color.float32[0] = 0.0f;
        lighting.clearValue.color.float32[1] = 0.0f;
        lighting.clearValue.color.float32[2] = 0.0f;
        lighting.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("HairDepth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depth.clearValue.depthStencil.depth = 0.0f;
        depth.clearValue.depthStencil.stencil = 0;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        state.rasterizer.cullFaceEnabled = false;

        state.rasterizer.stencilTestEnabled = true;
        state.rasterizer.stencilCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.stencilFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilPassOp = VK_STENCIL_OP_KEEP;
        state.rasterizer.stencilReadMask = STENCIL_BIT_HAIR;
        state.rasterizer.stencilWriteMask = 0x00;
    }

    void CreateHairDepthPrePassRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("HairDepthPrePass");

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("HairDepth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = true;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_GREATER;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateEmissiveForwardRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("EmissiveForward");

        VulkanRenderTargetInfo& emissive = state.AddColorTarget("Emissive");
        emissive.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        emissive.clearValue.color.float32[0] = 0.0f;
        emissive.clearValue.color.float32[1] = 0.0f;
        emissive.clearValue.color.float32[2] = 0.0f;
        emissive.clearValue.color.float32[3] = 0.0f;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("Depth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.blendEnabled = false;
        state.rasterizer.cullFaceEnabled = false;
    }

    void CreateHairLightingRenderState() {
        VulkanRenderState& state = VulkanResourceManager::CreateRenderState("HairLighting");

        VulkanRenderTargetInfo& lighting = state.AddColorTarget("HairLighting");
        lighting.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        VulkanRenderTargetInfo& depth = state.SetDepthTarget("HairDepth");
        depth.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;

        state.rasterizer.depthTestEnabled = true;
        state.rasterizer.depthWriteEnabled = false;
        state.rasterizer.depthCompareOp = VK_COMPARE_OP_EQUAL;
        state.rasterizer.cullFaceEnabled = false;
    }
}

namespace VulkanRenderer {

    void CreateRenderStates() {
        CreateVisibilityRenderState();
        CreateVisibilityAlphaDiscardRenderState();
        CreateMaterialResolveRenderState();
        CreateLightingDeferredRenderState();
        CreateLightingForwardBlendedRenderState();
        CreateEmissiveForwardRenderState();
        CreateSpriteSheetRenderState();
        CreateSkyboxRenderState();
        CreateDebug3DRenderState();
        CreateDDGIProbeDebugRenderState();
        CreateDebug2DRenderState();

        CreateIndirectSpecularAMDInputRenderState();

        CreateHairDepthPrepRenderState();
        CreateHairDepthPrePassRenderState();
        CreateHairLightingRenderState();
    }
}
