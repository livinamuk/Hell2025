#include "Unloved/Render/API/OpenGL/GL_renderer.h"

namespace OpenGL::Renderer {

    void CreateShaders() {
        OpenGL::ResourceManager::LoadShader("Present", { "GL_present.vert", "GL_present.frag" });
        OpenGL::ResourceManager::LoadShader("ChristmasLightCulling", { "GL_christmas_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("ChristmasLightsWire", { "GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag" });
        OpenGL::ResourceManager::LoadShader("BlitRoad", { "GL_blit_road.comp" });
        OpenGL::ResourceManager::LoadShader("ComputeSkinning", { "GL_compute_skinning.comp" });
        OpenGL::ResourceManager::LoadShader("TileWorldBounds", { "GL_tile_world_bounds.comp" });
        OpenGL::ResourceManager::LoadShader("VAT", { "GL_vat.vert", "GL_vat.frag" });

        OpenGL::ResourceManager::LoadShader("DecalTest", { "GL_decal_test.vert", "GL_decal_test.frag" });
        OpenGL::ResourceManager::LoadShader("BloodDecalsNew", { "GL_blood_decals_new.vert", "GL_blood_decals_new.frag" });

        OpenGL::ResourceManager::LoadShader("DownSample2xBox", { "GL_down_sample_2x_box.comp" });
        OpenGL::ResourceManager::LoadShader("EditorMesh", { "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        OpenGL::ResourceManager::LoadShader("EmissiveBloomFilter", { "GL_emissive_bloom_filter.comp" });
        OpenGL::ResourceManager::LoadShader("EmissiveBloomComposite", { "GL_emissive_bloom_composite.comp" });
        OpenGL::ResourceManager::LoadShader("ExamineItem", { "GL_examine_item.vert", "GL_examine_item.frag" });
        OpenGL::ResourceManager::LoadShader("FogRayMarch", { "GL_fog_ray_march.comp" });
        OpenGL::ResourceManager::LoadShader("FogComposite", { "GL_fog_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Fur", { "GL_fur.vert", "GL_fur.frag" });
        OpenGL::ResourceManager::LoadShader("FurComposite", { "GL_fur_composite.comp" });
        OpenGL::ResourceManager::LoadShader("GBuffer", { "GL_GBuffer.vert", "GL_gBuffer.frag" });
        OpenGL::ResourceManager::LoadShader("Gizmo", { "GL_gizmo.vert", "GL_gizmo.frag" });
        OpenGL::ResourceManager::LoadShader("Glass", { "GL_glass.vert", "GL_glass.frag" });
        OpenGL::ResourceManager::LoadShader("GlassComposite", { "GL_glass_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Grass", { "GL_grass.vert", "GL_grass.frag" });
        OpenGL::ResourceManager::LoadShader("GrassGeometryGeneration", { "GL_grass_geometry_generation.comp" });
        OpenGL::ResourceManager::LoadShader("GrassPositionGeneration", { "GL_grass_position_generation.comp" });
        OpenGL::ResourceManager::LoadShader("NewGrass", { "GL_new_grass.vert", "GL_grass.frag" });
        OpenGL::ResourceManager::LoadShader("NewGrassPositionGeneration", { "GL_new_grass_position_generation.comp" });
        OpenGL::ResourceManager::LoadShader("NewGrassChunkCulling", { "GL_new_grass_chunk_culling.comp" });
        OpenGL::ResourceManager::LoadShader("NewGrassCulling", { "GL_new_grass_culling.comp" });
        OpenGL::ResourceManager::LoadShader("OcclusionHiZ", { "GL_occlusion_hiz.comp" });
        OpenGL::ResourceManager::LoadShader("GaussianBlurUtil", { "GL_gaussian_blur_util.comp" });
        OpenGL::ResourceManager::LoadShader("HairDepthPeel", { "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        OpenGL::ResourceManager::LoadShader("HairFinalComposite", { "GL_hair_final_composite.comp" });
        OpenGL::ResourceManager::LoadShader("HairLighting", { "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapColor", { "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapVertexGeneration", { "GL_heightmap_vertex_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapBrushPreview", { "RE/GL_fullscreen_triangle.vert", "GL_heightmap_brush_preview.frag" });
        OpenGL::ResourceManager::LoadShader("LightCulling", { "GL_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("Lighting", { "GL_lighting.comp" });
        OpenGL::ResourceManager::LoadShader("GaussianBlur", { "GL_gaussian_blur.comp" }); // am I needed????
        OpenGL::ResourceManager::LoadShader("Outline", { "GL_outline.vert", "GL_outline.frag" });
        OpenGL::ResourceManager::LoadShader("OutlineComposite", { "GL_outline_composite.comp" });
        OpenGL::ResourceManager::LoadShader("OutlineMask", { "GL_outline_mask.vert", "GL_outline_mask.frag" });
        OpenGL::ResourceManager::LoadShader("PerlinNoise3D", { "GL_perlin_noise_3d.comp" });
        OpenGL::ResourceManager::LoadShader("ShadowMap", { "GL_shadow_map.vert", "GL_shadow_map.frag" });
        OpenGL::ResourceManager::LoadShader("ShadowHeightMap", { "GL_shadow_height_map.vert", "GL_shadow_height_map.tesc", "GL_shadow_height_map.tese", "GL_shadow_map.frag" });
        OpenGL::ResourceManager::LoadShader("ShadowCubeMap", { "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        OpenGL::ResourceManager::LoadShader("ShadowCubeMapAlphaDiscard", { "GL_shadow_cube_map.vert", "GL_shadow_cube_map_alpha_discard.frag" });
        OpenGL::ResourceManager::LoadShader("SolidColor", { "GL_solid_color.vert", "GL_solid_color.frag" });
        OpenGL::ResourceManager::LoadShader("Skybox", { "GL_skybox.vert", "GL_skybox.frag" });
        OpenGL::ResourceManager::LoadShader("SpriteSheet", { "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });
        OpenGL::ResourceManager::LoadShader("ScreenspaceReflections", { "GL_screenspace_reflections.comp" });
        OpenGL::ResourceManager::LoadShader("StainedGlass", { "GL_GLASS.vert", "GL_GLASS.frag" });
        OpenGL::ResourceManager::LoadShader("UI", { "GL_ui.vert", "GL_ui.frag" });
        OpenGL::ResourceManager::LoadShader("Winston", { "GL_winston.vert", "GL_winston.frag" });
        OpenGL::ResourceManager::LoadShader("CSMDepth", { "GL_csm_depth.vert", "GL_csm_depth.frag", "GL_csm_depth.geom" });
        OpenGL::ResourceManager::LoadShader("ZeroOut", { "GL_zero_out.comp" });

        OpenGL::ResourceManager::LoadShader("MetaBalls", { "GL_meta_balls.vert", "GL_meta_balls.frag" });
        OpenGL::ResourceManager::LoadShader("ViewspaceDepth", { "GL_viewspace_depth.comp" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyColor", { "GL_depth_peeled_transparency_color.vert", "GL_depth_peeled_transparency_color.frag" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyDepth", { "GL_depth_peeled_transparency_depth.vert", "GL_depth_peeled_transparency_depth.frag" });
        OpenGL::ResourceManager::LoadShader("DepthPeeledTransparencyComposite", { "GL_depth_peeled_transparency_composite.comp" });
        OpenGL::ResourceManager::LoadShader("RaytraceScene", { "GL_raytrace_scene.comp" });
        OpenGL::ResourceManager::LoadShader("Plastic", { "GL_plastic.vert", "GL_plastic.frag" });

        OpenGL::ResourceManager::LoadShader("LightAABBPosition", { "GL_light_aabb_position.vert", "GL_light_aabb_position.frag" });
        OpenGL::ResourceManager::LoadShader("LightAABBMinMax", { "GL_light_aabb_min_max.comp" });

        // Blood
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsCulling", { "GL_blood_decals_culling.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsDraw", { "GL_blood_decals_draw.vert", "GL_blood_decals_draw.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodDecalsComposite", { "GL_blood_decals_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidDepth", { "GL_blood_fluid.vert", "GL_blood_fluid_depth.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidThickness", { "GL_blood_fluid.vert", "GL_blood_fluid_thickness.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "BloodFluidBlur", { "GL_blood_fluid_blur.comp" });
        OpenGL::ResourceManager::LoadShader("Blood", "GenericBloodDecal", { "GL_generic_blood_decal.vert", "GL_generic_blood_decal.frag" });
        OpenGL::ResourceManager::LoadShader("Blood", "VatBlood", { "GL_vat_blood.vert", "GL_vat_blood.frag" });

        // Physics
        OpenGL::ResourceManager::LoadShader("PhysicsShapes", { "GL_physics_shapes.vert", "GL_physics_shapes.frag" });

        // Debug
        OpenGL::ResourceManager::LoadShader("Debug", "DebugHackAABB", { "GL_debug_hack_aabb.vert", "GL_debug_hack_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugLightAABB", { "GL_debug_light_aabb.vert", "GL_debug_light_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugPointCloud", { "GL_debug_point_cloud.vert", "GL_debug_point_cloud.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugProbes", { "GL_debug_probes.vert", "GL_debug_probes.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugSolidColor", { "GL_debug_solid_color.vert", "GL_debug_solid_color.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTextureBlit", { "GL_debug_texture_blit.vert", "GL_debug_texture_blit.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTextured", { "GL_debug_textured.vert", "GL_debug_textured.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugTileView", { "GL_debug_tile_view.comp" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugVertex2D", { "GL_debug_vertex_2D.vert", "GL_debug_vertex_2D.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugVertex3D", { "GL_debug_vertex_3D.vert", "GL_debug_vertex_3D.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugView", { "GL_debug_view.comp" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugViewMSAA", { "GL_debug_view.comp" }, { "MSAA_ENABLED" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugViewRE", { "GL_debug_view.comp" }, { "RE_ENABLED" });

        // DDGI
        OpenGL::ResourceManager::LoadShader("DDGI", "PointCloudBaseColor", { "GL_point_cloud_basecolor.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "PointCloudLighting", { "GL_point_cloud_lighting.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistance", { "GL_probe_distance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceBorder", { "GL_probe_distance_border.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceDispatchArgs", { "GL_probe_distance_dispatch_args.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeDistanceList", { "GL_probe_distance_list.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradiance", { "GL_probe_irradiance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceBorder", { "GL_probe_irradiance_border.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceDirtyPointCheck", { "GL_probe_irradiance_dirty_point_check.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceList", { "GL_probe_irradiance_list.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeIrradianceTexture", { "GL_probe_irradiance_texture.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeLightingDispatchArgs", { "GL_probe_lighting_dispatch_args.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbePointIndices", { "GL_probe_point_indices.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeRelevance", { "GL_probe_relevance.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeRelocation", { "GL_probe_state_update.comp" });
        OpenGL::ResourceManager::LoadShader("DDGI", "ProbeStateUpdate", { "GL_probe_state_update.comp" });

        // Decals
        OpenGL::ResourceManager::LoadShader("Decals", "DecalPaintUVs", { "gl_decal_paint_uvs.vert", "gl_decal_paint_uvs.frag" });
        OpenGL::ResourceManager::LoadShader("Decals", "DecalPaintMask", { "gl_decal_paint_mask.comp" });
        OpenGL::ResourceManager::LoadShader("Decals", "Decals", { "GL_decals.vert", "GL_decals.frag" });

        // Ocean
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix64Vertical", { "GL_ftt_radix_64_vertical.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix8Vertical", { "GL_ftt_radix_8_vertical.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix64Horizontal", { "GL_ftt_radix_64_horizontal.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "FttRadix8Horizontal", { "GL_ftt_radix_8_horizontal.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanFlags", { "GL_ocean_flags.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanSurfaceComposite", { "GL_ocean_surface_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanCalculateSpectrum", { "GL_ocean_calculate_spectrum.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUpdateTextures", { "GL_ocean_update_textures.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUnderwaterComposite", { "GL_ocean_underwater_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanPositionReadback", { "GL_ocean_position_readback.comp" });

        // Post processing
        OpenGL::ResourceManager::LoadShader("PostProcessing", "FXAA", { "GL_fullscreen_triangle.vert", "GL_fxaa.frag" });
        OpenGL::ResourceManager::LoadShader("PostProcessing", "TAA", { "GL_taa.comp" });
        OpenGL::ResourceManager::LoadShader("PostProcessing", "TAAPost", { "GL_taa_post.comp" });
        OpenGL::ResourceManager::LoadShader("PostProcessing", "PostProcessing", { "GL_post_processing.comp" });

        // RE_STYLE ONLY

        OpenGL::ResourceManager::LoadShader("RE", "DepthPrePassRE", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "DepthPrePassAlphaDiscardRE", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "GBuffer", { "GL_gbuffer_re.vert", "GL_gbuffer_re.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "LightingDeferred", { "GL_fullscreen_triangle.vert", "GL_lighting_deferred.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "LightingDeferredViewWeapon", { "GL_fullscreen_triangle.vert", "GL_lighting_deferred.frag" }, { "VIEW_WEAPON" });
        OpenGL::ResourceManager::LoadShader("RE", "LightingDeferredEditorRenderMode", { "GL_fullscreen_triangle.vert", "GL_lighting_deferred_editor_render_mode.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "HairLightingForward", { "GL_hair_lighting_forward.vert", "GL_hair_lighting_forward.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "HairCompositeRE", { "GL_hair_composite_re.comp" });
        OpenGL::ResourceManager::LoadShader("RE", "HairDepthPrep", { "GL_fullscreen_triangle.vert", "GL_hair_depth_prep.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "Visibility", { "GL_visibility.vert", "GL_visibility.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "VisibilityAlphaDiscard", { "GL_visibility.vert", "GL_visibility_alpha_discard.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "VisibilityHeightMap", { "GL_visibility_height_map.vert", "GL_visibility_height_map.tesc", "GL_visibility_height_map.tese", "GL_visibility_height_map.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "TerrainDisplacementBuffer", { "GL_terrain_displacement_buffer.comp" });
        OpenGL::ResourceManager::LoadShader("RE", "MaterialResolve", { "GL_material_resolve.vert", "GL_material_resolve.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "MaterialResolveHeightMap", { "GL_material_resolve.vert", "GL_material_resolve_height_map.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "EmissiveForward", { "GL_gbuffer_re.vert", "GL_emissive_forward.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "LightingForward", { "GL_lighting_forward.vert", "GL_lighting_forward.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "SkyboxRE", { "GL_fullscreen_triangle.vert", "GL_skybox_re.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "OceanLighting", { "GL_ocean_lighting.vert", "GL_ocean_lighting.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "Bubbles", { "GL_bubbles.vert", "GL_bubbles.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "Bubbles2", { "GL_bubbles_2.vert", "GL_bubbles_2.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "Bubbles3", { "GL_bubbles_3.vert", "GL_bubbles_3.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "BubbleDrawCommandArgs", { "GL_bubble_draw_command_args.comp" });

        OpenGL::ResourceManager::LoadShader("RE", "ParticleAdditions", { "GL_particle_additions.comp" });
        OpenGL::ResourceManager::LoadShader("RE", "ParticleColor", { "GL_particle_color.vert", "GL_particle_color.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "ParticleUpdate", { "GL_particle_update.comp" });
    }
}
