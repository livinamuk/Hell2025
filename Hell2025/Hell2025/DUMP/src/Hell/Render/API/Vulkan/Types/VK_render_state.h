#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"

#include <cstddef>
#include <string>

struct VulkanRenderTargetInfo {
    std::string imageName = "";
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    VkClearValue clearValue{};
};

struct VulkanRasterizerState {
    bool depthTestEnabled = false;
    bool depthWriteEnabled = false;
    VkCompareOp depthCompareOp = VK_COMPARE_OP_ALWAYS;

    bool blendEnabled = false;
    VkBlendFactor srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    VkBlendFactor dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    VkBlendOp colorBlendOp = VK_BLEND_OP_ADD;
    VkBlendFactor srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    VkBlendFactor dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    VkBlendOp alphaBlendOp = VK_BLEND_OP_ADD;

    bool cullFaceEnabled = false;
    VkCullModeFlags cullMode = VK_CULL_MODE_BACK_BIT;
    VkFrontFace frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

    bool stencilTestEnabled = false;
    uint32_t stencilRef = 0;
    uint32_t stencilReadMask = 0xff;
    uint32_t stencilWriteMask = 0xff;
    VkCompareOp stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    VkStencilOp stencilFailOp = VK_STENCIL_OP_KEEP;
    VkStencilOp stencilDepthFailOp = VK_STENCIL_OP_KEEP;
    VkStencilOp stencilPassOp = VK_STENCIL_OP_KEEP;
};

struct VulkanRenderState {
    inline static constexpr uint32_t MAX_RENDER_TARGET_COUNT = 8;

    VulkanRenderTargetInfo colorTargets[MAX_RENDER_TARGET_COUNT];
    uint32_t colorTargetCount = 0;
    VulkanRenderTargetInfo depthTarget;
    bool hasDepthTarget = false;
    VulkanRasterizerState rasterizer;

    VulkanRenderTargetInfo& AddColorTarget(const std::string& imageName = "");
    VulkanRenderTargetInfo& SetDepthTarget(const std::string& imageName = "");
    void CleanUp();
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
};
