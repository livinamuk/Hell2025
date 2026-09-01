#include "VK_render_state.h"

VulkanRenderTargetInfo& VulkanRenderState::AddColorTarget(const std::string& imageName) {
    if (colorTargetCount >= MAX_RENDER_TARGET_COUNT) {
        return colorTargets[MAX_RENDER_TARGET_COUNT - 1];
    }

    VulkanRenderTargetInfo& target = colorTargets[colorTargetCount++];
    target = VulkanRenderTargetInfo();
    target.imageName = imageName;
    return target;
}

VulkanRenderTargetInfo& VulkanRenderState::SetDepthTarget(const std::string& imageName) {
    hasDepthTarget = true;
    depthTarget = VulkanRenderTargetInfo();
    depthTarget.imageName = imageName;
    return depthTarget;
}

void VulkanRenderState::CleanUp() {
    // Intentionally blank
}

size_t VulkanRenderState::GetCPUAllocatedByteCount() const {
    size_t byteCount = sizeof(VulkanRenderState);

    for (uint32_t i = 0; i < colorTargetCount; i++) {
        byteCount += colorTargets[i].imageName.capacity();
    }

    if (hasDepthTarget) {
        byteCount += depthTarget.imageName.capacity();
    }

    return byteCount;
}

size_t VulkanRenderState::GetGPUAllocatedByteCount() const {
    return 0;
}
