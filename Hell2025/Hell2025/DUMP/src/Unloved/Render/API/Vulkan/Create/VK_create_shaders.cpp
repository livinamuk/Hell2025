#include "Unloved/Render/API/Vulkan/VK_renderer.h"

#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace VulkanRenderer {

    void CreateShaders() {

        // Debug
        VulkanResourceManager::CreateShader("DebugView", { "VK_fullscreen_triangle.vert", "VK_debug_view.frag" });
        VulkanResourceManager::CreateShader("DebugVertex2D", { "VK_debug_vertex_2D.vert", "VK_debug_vertex.frag" });
        VulkanResourceManager::CreateShader("DebugVertex3D", { "VK_debug_vertex_3D.vert", "VK_debug_vertex.frag" });
        VulkanResourceManager::CreateShader("DebugTileView", { "VK_debug_tile_view.comp" });
        VulkanResourceManager::CreateShader("DDGIRaytraceScene", { "VK_ddgi_raytrace_scene.comp" });
        VulkanResourceManager::CreateShader("DDGIPointCloudBaseColor", { "VK_ddgi_point_cloud_basecolor.comp" });
        VulkanResourceManager::CreateShader("DDGIPointCloudLighting", { "VK_ddgi_point_cloud_lighting.comp" });
        VulkanResourceManager::CreateShader("DDGIProbePointIndices", { "VK_ddgi_probe_point_indices.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeStateUpdate", { "VK_ddgi_probe_state_update.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeRelevance", { "VK_ddgi_probe_relevance.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeDistanceList", { "VK_ddgi_probe_distance_list.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeDistanceDispatchArgs", { "VK_ddgi_probe_distance_dispatch_args.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeDistance", { "VK_ddgi_probe_distance.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeDistanceBorder", { "VK_ddgi_probe_distance_border.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradianceDirtyPointCheck", { "VK_ddgi_probe_irradiance_dirty_point_check.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradianceList", { "VK_ddgi_probe_irradiance_list.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradianceDispatchArgs", { "VK_ddgi_probe_irradiance_dispatch_args.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradiance", { "VK_ddgi_probe_irradiance.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradianceBorder", { "VK_ddgi_probe_irradiance_border.comp" });
        VulkanResourceManager::CreateShader("DDGIProbeIrradianceTexture", { "VK_ddgi_probe_irradiance_texture.comp" });
        VulkanResourceManager::CreateShader("DDGIPointCloudDebug", { "VK_ddgi_point_cloud_debug.vert", "VK_debug_vertex.frag" });
        VulkanResourceManager::CreateShader("DDGIProbeDebug", { "VK_ddgi_probe_debug.vert", "VK_debug_vertex.frag" });

        // Compute
        VulkanResourceManager::CreateShader("ComputeSkinning", { "VK_compute_skinning.comp" });

        VulkanResourceManager::CreateShader("Present", { "VK_fullscreen_triangle.vert", "VK_present.frag" });
        VulkanResourceManager::CreateShader("Skybox", { "VK_fullscreen_triangle.vert", "VK_skybox.frag" });

        // Hair
        VulkanResourceManager::CreateShader("HairDepthPrep", { "VK_fullscreen_triangle.vert", "VK_hair_depth_prep.frag" });
        VulkanResourceManager::CreateShader("HairDepthPrePass", { "VK_hair_depth_prepass.vert", "VK_hair_depth_prepass.frag" });
        VulkanResourceManager::CreateShader("HairLighting", { "VK_hair_lighting.vert", "VK_hair_lighting.frag" });
        VulkanResourceManager::CreateShader("HairSurfaceLighting", { "VK_hair_lighting.vert", "VK_hair_surface_lighting.frag" });
        VulkanResourceManager::CreateShader("HairComposite", { "VK_hair_composite.comp" });

        // Material Resolve
        VulkanResourceManager::CreateShader("MaterialResolve", { "VK_fullscreen_triangle.vert", "VK_material_resolve.frag" });

        // Emissive bloom
        VulkanResourceManager::CreateShader("EmissiveForward", { "VK_emissive_forward.vert", "VK_emissive_forward.frag" });
        VulkanResourceManager::CreateShader("EmissiveBloomFilter", { "VK_emissive_bloom_filter.comp" });
        VulkanResourceManager::CreateShader("EmissiveBloomComposite", { "VK_emissive_bloom_composite.comp" });

        // Lighting
        VulkanResourceManager::CreateShader("LightingDeferred", { "VK_fullscreen_triangle.vert", "VK_lighting_deferred.frag" });
        VulkanResourceManager::CreateShader("LightingForward", { "VK_lighting_forward.vert", "VK_lighting_forward.frag" });
        VulkanResourceManager::CreateShader("PointShadow", { "VK_point_shadow.vert", "VK_point_shadow.frag" });
        VulkanResourceManager::CreateShader("PointShadowAlphaDiscard", { "VK_point_shadow_alpha_discard.vert", "VK_point_shadow_alpha_discard.frag" });
        VulkanResourceManager::CreateShader("SpriteSheet", { "VK_sprite_sheet.vert", "VK_sprite_sheet.frag" });

        // Hierarchical depth buffer
        VulkanResourceManager::CreateShader("HiZ", { "VK_hiz.comp" });

        // Indirect specular
        VulkanResourceManager::CreateShader("IndirectSpecularAMDInput", { "VK_fullscreen_triangle.vert", "VK_indirect_specular.frag" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDResolveRayInput", { "VK_indirect_specular_amd_resolve_ray_input.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDClassifyTiles", { "VK_indirect_specular_amd_classify_tiles.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDPrepareIndirectArgs", { "VK_indirect_specular_amd_prepare_indirect_args.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDReproject", { "VK_indirect_specular_amd_reproject.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDPrefilter", { "VK_indirect_specular_amd_prefilter.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDResolveTemporal", { "VK_indirect_specular_amd_resolve_temporal.comp" });
        VulkanResourceManager::CreateShader("IndirectSpecularAMDStoreHistory", { "VK_indirect_specular_amd_store_history.comp" });

        // Post Processing
        VulkanResourceManager::CreateShader("PostProcessing", { "VK_post_processing.comp" });

        // Test
        VulkanResourceManager::CreateShader("FullscreenTriangle", { "VK_fullscreen_triangle.vert", "VK_solid_color.frag" });
        VulkanResourceManager::CreateShader("ComputeRedTest", { "VK_compute_red_test.comp" });

        // Tile culling
        VulkanResourceManager::CreateShader("TileLightCulling", { "VK_tile_light_culling.comp" });
        VulkanResourceManager::CreateShader("TileWorldBounds", { "VK_tile_world_bounds.comp" });

        // UI
        VulkanResourceManager::CreateShader("UI", { "VK_ui.vert", "VK_ui.frag" });

        // Vis buffer
        VulkanResourceManager::CreateShader("Visibility", { "VK_visibility.vert", "VK_visibility.frag" });
        VulkanResourceManager::CreateShader("VisibilityAlphaDiscard", { "VK_visibility.vert", "VK_visibility_alpha_discard.frag" });
    }
}
