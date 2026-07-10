#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace VulkanRenderer {

    void CreateShaders() {

        // Debug
        VulkanResourceManager::CreateShader("DebugView", { "VK_fullscreen_triangle.vert", "VK_debug_view.frag" });
        VulkanResourceManager::CreateShader("DebugTileView", { "VK_debug_tile_view.comp" });

        // Compute
        VulkanResourceManager::CreateShader("ComputeSkinning", { "VK_compute_skinning.comp" });

        VulkanResourceManager::CreateShader("Present", { "VK_fullscreen_triangle.vert", "VK_present.frag" });
        VulkanResourceManager::CreateShader("Skybox", { "VK_fullscreen_triangle.vert", "VK_skybox.frag" });

        // Hair
        VulkanResourceManager::CreateShader("HairDepthPrep", { "VK_fullscreen_triangle.vert", "VK_hair_depth_prep.frag" });
        VulkanResourceManager::CreateShader("HairDepthPrePass", { "VK_visibility.vert", "VK_hair_depth_prepass.frag" });
        VulkanResourceManager::CreateShader("HairLighting", { "VK_hair_lighting.vert", "VK_hair_lighting.frag" });
        VulkanResourceManager::CreateShader("HairComposite", { "VK_hair_composite.comp" });

        // Material Resolve
        VulkanResourceManager::CreateShader("MaterialResolve", { "VK_fullscreen_triangle.vert", "VK_material_resolve.frag" });

        // Lighting
        VulkanResourceManager::CreateShader("LightingDeferred", { "VK_fullscreen_triangle.vert", "VK_lighting_deferred.frag" });
        VulkanResourceManager::CreateShader("LightingForward", { "VK_lighting_forward.vert", "VK_lighting_forward.frag" });

        // Post Processing
        VulkanResourceManager::CreateShader("PostProcessing", { "VK_post_processing.comp" });

        // Test
        VulkanResourceManager::CreateShader("FullscreenTriangle", { "VK_fullscreen_triangle.vert", "VK_solid_color.frag" });
        VulkanResourceManager::CreateShader("ComputeRedTest", { "VK_compute_red_test.comp" });

        // UI
        VulkanResourceManager::CreateShader("UI", { "VK_ui.vert", "VK_ui.frag" });

        // Vis buffer
        VulkanResourceManager::CreateShader("Visibility", { "VK_visibility.vert", "VK_visibility.frag" });
        VulkanResourceManager::CreateShader("VisibilityAlphaDiscard", { "VK_visibility.vert", "VK_visibility_alpha_discard.frag" });
    }
}
