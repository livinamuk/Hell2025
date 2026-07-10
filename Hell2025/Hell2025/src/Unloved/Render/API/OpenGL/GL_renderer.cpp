#include "GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_rasterizer_state_manager.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
#include "Hell/Render/API/OpenGL/Types/GL_indirectBuffer.hpp"
#include "Hell/Render/API/OpenGL/Types/GL_pbo.hpp"
#include "Hell/Render/API/OpenGL/Types/GL_shader.h"
#include "Hell/Render/API/OpenGL/Types/GL_ssbo.h"
#include "Hell/Backend/BackEnd.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/UI/TextBlitter.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Legacy/Timer.hpp"

#include "Unloved/Editor/Editor.h"
#include "Unloved/Editor/Gizmo.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Render/API/OpenGL/Types/GL_texture_readback.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <unordered_map>




namespace OpenGL::Renderer {
    using namespace Unloved;


    OpenGLMeshPatch g_tesselationPatch;

    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f };
    const glm::vec3 g_lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));
    unsigned int g_lightFBO;
    unsigned int g_lightDepthMaps;
    constexpr unsigned int g_depthMapResolution = 4096;

    GLuint g_emptyVao = 0;
    std::unordered_map<std::string, GLuint> g_cachedTextureHandles;

    void LoadShaders();

    IndirectBuffer g_indirectBuffer;                                 // TODO: Make me an SSBO and get me the fuck out of here
    IndirectBuffer& GetIndirectBuffer() { return g_indirectBuffer; } // TODO: Make me an SSBO and get me the fuck out of here

    struct Cubemaps {
        OpenGLCubemapView g_skyboxView;
    } g_cubemaps;

    void Init() {

        Ocean::Init();

        uint64_t perlinNoiseId = OpenGL::ResourceManager::CreateTexture3D("PerlinNoise");
        OpenGLTexture3D& perlinNoise = OpenGL::ResourceManager::GetTexture3DById(perlinNoiseId);
        perlinNoise.Create(128, GL_R32F, true);

        uint64_t flashlightShadowMapsId = OpenGL::ResourceManager::CreateShadowMap("FlashlightShadowMaps");
        OpenGL::ResourceManager::GetShadowMapById(flashlightShadowMapsId) = OpenGLShadowMap("FlashlightShadowMaps", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE, 4);

        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        CreateFramebuffers();
        CreateSSBOs();
        CreateTextureArrays();

        InitSSBOs();
        LoadShaders();

        OpenGLRasterizerState* decalPass = OpenGL::RasterizerStateManager::CreateRasterizerState("DecalPass");
        decalPass->depthTestEnabled = true;
        decalPass->blendEnable = true;
        decalPass->cullfaceEnable = true;
        decalPass->depthMask = false;
        decalPass->depthFunc = GL_GREATER;
        decalPass->blendFuncSrcfactor = GL_SRC_ALPHA;
        decalPass->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* emissivePass = OpenGL::RasterizerStateManager::CreateRasterizerState("EmissivePass");
        emissivePass->depthTestEnabled = true;
        emissivePass->blendEnable = false;
        emissivePass->cullfaceEnable = true;
        emissivePass->depthMask = false;
        emissivePass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassDefault = OpenGL::RasterizerStateManager::CreateRasterizerState("GeometryPass_Default");
        geometryPassDefault->depthTestEnabled = true;
        geometryPassDefault->blendEnable = false;
        geometryPassDefault->cullfaceEnable = true;
        geometryPassDefault->depthMask = true;
        geometryPassDefault->depthFunc = GL_GREATER;

        OpenGLRasterizerState* geometryPassAlphaDiscard = OpenGL::RasterizerStateManager::CreateRasterizerState("GeometryPass_AlphaDiscard");
        geometryPassAlphaDiscard->depthTestEnabled = true;
        geometryPassAlphaDiscard->blendEnable = false;
        geometryPassAlphaDiscard->cullfaceEnable = true;
        geometryPassAlphaDiscard->depthMask = true;
        geometryPassAlphaDiscard->depthFunc = GL_GEQUAL;

        OpenGLRasterizerState* geometryPassBlended = OpenGL::RasterizerStateManager::CreateRasterizerState("GeometryPass_Blended");
        geometryPassBlended->depthTestEnabled = true;
        geometryPassBlended->blendEnable = true;
        geometryPassBlended->cullfaceEnable = false;
        geometryPassBlended->depthMask = false;
        geometryPassBlended->depthFunc = GL_GEQUAL;
        geometryPassBlended->blendFuncSrcfactor = GL_SRC_ALPHA;
        geometryPassBlended->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

        OpenGLRasterizerState* glassPass = OpenGL::RasterizerStateManager::CreateRasterizerState("GlassPass");
        glassPass->depthTestEnabled = true;
        glassPass->blendEnable = false;
        glassPass->cullfaceEnable = true;
        glassPass->depthMask = false;
        glassPass->depthFunc = GL_GREATER;

        OpenGLRasterizerState* hairPassViewspaceDepth = OpenGL::RasterizerStateManager::CreateRasterizerState("HairViewspaceDepth");
        hairPassViewspaceDepth->depthTestEnabled = true;
        hairPassViewspaceDepth->blendEnable = false;
        hairPassViewspaceDepth->cullfaceEnable = true;
        hairPassViewspaceDepth->depthMask = true;
        hairPassViewspaceDepth->depthFunc = GL_GREATER;
        hairPassViewspaceDepth->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassViewspaceDepth->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassViewspaceDepth->pointSize = 8;

        OpenGLRasterizerState* hairPassLighting = OpenGL::RasterizerStateManager::CreateRasterizerState("HairLighting");
        hairPassLighting->depthTestEnabled = true;
        hairPassLighting->blendEnable = false;
        hairPassLighting->cullfaceEnable = true;
        hairPassLighting->depthMask = true;
        hairPassLighting->depthFunc = GL_EQUAL;
        hairPassLighting->blendFuncSrcfactor = GL_SRC_ALPHA;
        hairPassLighting->blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;
        hairPassLighting->pointSize = 8;

        OpenGLRasterizerState* spriteSheet = OpenGL::RasterizerStateManager::CreateRasterizerState("SpriteSheetPass");
        spriteSheet->depthTestEnabled = true;
        spriteSheet->blendEnable = true;
        spriteSheet->cullfaceEnable = false;
        spriteSheet->depthMask = false;
        spriteSheet->depthFunc = GL_GREATER;
        spriteSheet->blendFuncSrcfactor = GL_SRC_ALPHA;
        spriteSheet->blendFuncDstfactor = GL_ONE; // was GL_ONE_MINUS_SRC_ALPHA

        OpenGLRasterizerState* skybox = OpenGL::RasterizerStateManager::CreateRasterizerState("SkyBox");
        skybox->depthTestEnabled = false;
        skybox->blendEnable = false;
        skybox->cullfaceEnable = false;
        skybox->depthMask = false;
        skybox->depthFunc = GL_GREATER;

        // Allocate shadow map array memory
        OpenGLShadowCubeMapArray& hiResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("HiRes");   // THESE ARE DYNAMIC. RENAM<E IF U EVER GET HTIS WOKRING
        hiResShadowMapArray.Init(ShadowMapManager::GetShadowMapHiResMaxCount(), 1024);                                // THESE ARE DYNAMIC. RENAM<E IF U EVER GET HTIS WOKRING
                                                                                                                      // THESE ARE DYNAMIC. RENAM<E IF U EVER GET HTIS WOKRING
        OpenGLShadowCubeMapArray& lowResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("LowRes"); // THESE ARE DYNAMIC. RENAM<E IF U EVER GET HTIS WOKRING
        lowResShadowMapArray.Init(ShadowMapManager::GetShadowMapLowResMaxCount(), 512);                               // THESE ARE DYNAMIC. RENAM<E IF U EVER GET HTIS WOKRING

        OpenGLShadowCubeMapArray& staticHiResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("StaticHiRes");
        hiResShadowMapArray.Init(ShadowMapManager::GetShadowMapHiResMaxCount(), 1024);

        OpenGLShadowCubeMapArray& staticLowResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("StaticLowRes");
        lowResShadowMapArray.Init(ShadowMapManager::GetShadowMapLowResMaxCount(), 512);


        // Moon light shadow maps
        float depthMapResolution = SHADOW_MAP_CSM_SIZE;
        int cascadeCount = int(g_shadowCascadeLevels.size()) + 1;
        int playerCount = 2;
        int layerCount = playerCount * cascadeCount;
        uint64_t moonlightCSMId = OpenGL::ResourceManager::CreateShadowMapArray("MoonlightCSM");
        OpenGLShadowMapArray& moonlightCSM = OpenGL::ResourceManager::GetShadowMapArrayById(moonlightCSMId);
        if (moonlightCSM.GetHandle() != 0) {
            moonlightCSM.CleanUp();
        }
        moonlightCSM.Init(layerCount, depthMapResolution, GL_DEPTH_COMPONENT32F);

        InitFog();
        InitGrass();
        InitOceanHeightReadback();
    }

    void InitMain() {
        // Attempt to load skybox
        std::vector<Texture*> textures = {
            Hell::ResourceManager::GetTextureByName("px"),
            Hell::ResourceManager::GetTextureByName("nx"),
            Hell::ResourceManager::GetTextureByName("py"),
            Hell::ResourceManager::GetTextureByName("ny"),
            Hell::ResourceManager::GetTextureByName("pz"),
            Hell::ResourceManager::GetTextureByName("nz"),
        };
        std::vector<GLuint> texturesHandles;
        for (Texture* texture : textures) {
            if (!texture) continue;
            texturesHandles.push_back(texture->GetGLTexture().GetHandle());
        }
        if (texturesHandles.size() == 6) {
            uint64_t skyboxNightSkyId = OpenGL::ResourceManager::CreateCubemapView("SkyboxNightSky");
            OpenGL::ResourceManager::GetCubemapViewById(skyboxNightSkyId).CreateCubemap(texturesHandles);
        }

        CreateBlurBuffers();

        // Upload materials
        std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        OpenGL::UpdateSSBO("Materials", materials.size() * sizeof(Material), materials.data());
    }



    void LoadShaders() {
        OpenGL::ResourceManager::LoadShader("Present", { "GL_present.vert", "GL_present.frag" });
        OpenGL::ResourceManager::LoadShader("ChristmasLightCulling", { "GL_christmas_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("ChristmasLightsWire", { "GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag" });
        OpenGL::ResourceManager::LoadShader("BlitRoad", { "GL_blit_road.comp" });
        OpenGL::ResourceManager::LoadShader("BlurHorizontal", { "GL_blur_horizontal.vert", "GL_blur.frag" });
        OpenGL::ResourceManager::LoadShader("BlurVertical", { "GL_blur_vertical.vert", "GL_blur.frag" });
        OpenGL::ResourceManager::LoadShader("ComputeSkinning", { "GL_compute_skinning.comp" });
        OpenGL::ResourceManager::LoadShader("TileWorldBounds", { "GL_tile_world_bounds.comp" });

        OpenGL::ResourceManager::LoadShader("DownSample2xBox", { "GL_down_sample_2x_box.comp" });
        OpenGL::ResourceManager::LoadShader("EditorMesh", { "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        OpenGL::ResourceManager::LoadShader("EmissiveComposite", { "GL_emissive_composite.comp" });
        OpenGL::ResourceManager::LoadShader("EmissiveCompositeNew", { "GL_emissive_composite_new.comp" });
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
        OpenGL::ResourceManager::LoadShader("GaussianBlurUtil", { "GL_gaussian_blur_util.comp" });
        OpenGL::ResourceManager::LoadShader("HairDepthPeel", { "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        OpenGL::ResourceManager::LoadShader("HairFinalComposite", { "GL_hair_final_composite.comp" });
        OpenGL::ResourceManager::LoadShader("HairLighting", { "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapColor", { "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        OpenGL::ResourceManager::LoadShader("HeightMapImageGeneration", { "GL_heightmap_image_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapPhysxTextureGeneration", { "GL_heightmap_physx_texture_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapToWorldBlit", { "GL_heightmap_to_world_blit.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapVertexGeneration", { "GL_heightmap_vertex_generation.comp" });
        OpenGL::ResourceManager::LoadShader("HeightMapPaint", { "GL_heightmap_paint.comp" });
        OpenGL::ResourceManager::LoadShader("LightCulling", { "GL_light_culling.comp" });
        OpenGL::ResourceManager::LoadShader("Lighting", { "GL_lighting.comp" });
        OpenGL::ResourceManager::LoadShader("GaussianBlur", { "GL_gaussian_blur.comp" }); // am I needed????
        OpenGL::ResourceManager::LoadShader("Outline", { "GL_outline.vert", "GL_outline.frag" });
        OpenGL::ResourceManager::LoadShader("OutlineComposite", { "GL_outline_composite.comp" });
        OpenGL::ResourceManager::LoadShader("OutlineMask", { "GL_outline_mask.vert", "GL_outline_mask.frag" });
        OpenGL::ResourceManager::LoadShader("PerlinNoise3D", { "GL_perlin_noise_3d.comp" });
        OpenGL::ResourceManager::LoadShader("ShadowMap", { "GL_shadow_map.vert", "GL_shadow_map.frag" });
        OpenGL::ResourceManager::LoadShader("ShadowCubeMap", { "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        OpenGL::ResourceManager::LoadShader("SolidColor", { "GL_solid_color.vert", "GL_solid_color.frag" });
        OpenGL::ResourceManager::LoadShader("Skybox", { "GL_skybox.vert", "GL_skybox.frag" });
        OpenGL::ResourceManager::LoadShader("SpriteSheet", { "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });
        OpenGL::ResourceManager::LoadShader("ScreenspaceReflections", { "GL_screenspace_reflections.comp" });
        OpenGL::ResourceManager::LoadShader("StainedGlass", { "GL_stained_glass.vert", "GL_stained_glass.frag" });
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
        OpenGL::ResourceManager::LoadShader("Blood", "VatBlood", { "GL_vat_blood.vert", "GL_vat_blood.frag" });

        // Debug
        OpenGL::ResourceManager::LoadShader("Debug", "DebugHackAABB", { "GL_debug_hack_aabb.vert", "GL_debug_hack_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugLightAABB", { "GL_debug_light_aabb.vert", "GL_debug_light_aabb.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugPointCloud", { "GL_debug_point_cloud.vert", "GL_debug_point_cloud.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugProbes", { "GL_debug_probes.vert", "GL_debug_probes.frag" });
        OpenGL::ResourceManager::LoadShader("Debug", "DebugRagdoll", { "GL_debug_ragdoll.vert", "GL_debug_ragdoll.frag" });
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
        OpenGL::ResourceManager::LoadShader("Water", "OceanGeometry", { "GL_ocean_geometry.vert", "GL_ocean_geometry.frag", "GL_ocean_geometry.tesc", "GL_ocean_geometry.tese" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanCalculateSpectrum", { "GL_ocean_calculate_spectrum.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUpdateTextures", { "GL_ocean_update_textures.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanUnderwaterComposite", { "GL_ocean_underwater_composite.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanTesseleationEdgeTransitionCleanUp", { "GL_ocean_tessellation_edge_transition_cleanup.comp" });
        OpenGL::ResourceManager::LoadShader("Water", "OceanPositionReadback", { "GL_ocean_position_readback.comp" });

        // Post processing
        OpenGL::ResourceManager::LoadShader("PostProcessing", "FXAA", { "GL_fxaa.comp" });
        OpenGL::ResourceManager::LoadShader("PostProcessing", "TAA", { "GL_taa.comp" });
		OpenGL::ResourceManager::LoadShader("PostProcessing", "PostProcessing", { "GL_post_processing.comp" });

        // RE_STYLE ONLY

        OpenGL::ResourceManager::LoadShader("RE", "DepthPrePassRE", { "GL_depth_prepass.vert", "GL_depth_prepass.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "DepthPrePassAlphaDiscardRE", { "GL_depth_prepass_alpha_discard.vert", "GL_depth_prepass_alpha_discard.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "GBuffer", { "GL_gbuffer_re.vert", "GL_gbuffer_re.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "LightingDeferred", { "GL_fullscreen_triangle.vert", "GL_lighting_deferred.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "HairLightingForward", { "GL_hair_lighting_forward.vert", "GL_hair_lighting_forward.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "HairCompositeRE", { "GL_hair_composite_re.comp" });
        OpenGL::ResourceManager::LoadShader("RE", "HairDepthPrep", { "GL_fullscreen_triangle.vert", "GL_hair_depth_prep.frag" });

        OpenGL::ResourceManager::LoadShader("RE", "Visibility", { "GL_visibility.vert", "GL_visibility.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "VisibilityAlphaDiscard", { "GL_visibility.vert", "GL_visibility_alpha_discard.frag" });
        OpenGL::ResourceManager::LoadShader("RE", "MaterialResolve", { "GL_material_resolve.vert", "GL_material_resolve.frag" });

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



    void InitSSBOs() {
        //DispatchIndirectCommand command = { 1, 1, 1 };
        //OpenGL::UpdateSSBO("ProbeDispatchArgs", sizeof(DispatchIndirectCommand), &command);

        // HO
        const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
        const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
        if (OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band0")) {
            ssbo->CopyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
        }
        if (OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band1")) {
            ssbo->CopyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());
        }

    }

    void UpdateSSBOS() {
        OpenGL::UpdateSSBO("Samplers", sizeof(GLuint64) * OpenGL::BackEnd::GetBindlessTextureIDs().size(), OpenGL::BackEnd::GetBindlessTextureIDs().data());
        const std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        OpenGL::UpdateSSBO("Materials", materials.size() * sizeof(Material), materials.data());

        const RendererData& rendererData = Unloved::RenderDataManager::GetRendererData();
        const std::vector<BloodDecalInstanceData>& bloodScreenSpaceDecalInstances = Unloved::RenderDataManager::GetBloodScreenSpaceDecalInstanceData();
        const std::vector<GPULight>& gpuLights = Unloved::RenderDataManager::GetGPULights();
        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();
        const std::vector<ViewportData>& playerData = Unloved::RenderDataManager::GetViewportData();
        const std::vector<glm::mat4>&oceanPatchTransforms = Unloved::RenderDataManager::GetOceanPatchTransforms();

        GLuint zero = 0;

        OpenGL::UpdateSSBO("BloodDecalCounter", sizeof(uint32_t), &zero);
        OpenGL::UpdateSSBO("BloodDecalInstances", bloodScreenSpaceDecalInstances.size() * sizeof(BloodDecalInstanceData), bloodScreenSpaceDecalInstances.data());
        OpenGL::UpdateSSBO("ChristmasLightCounter", sizeof(uint32_t), &zero);
        OpenGL::UpdateSSBO("InstanceData", instanceData.size() * sizeof(RenderItem), instanceData.data());
        OpenGL::UpdateSSBO("Lights", gpuLights.size() * sizeof(GPULight), gpuLights.data());
        OpenGL::UpdateSSBO("RendererData", sizeof(RendererData), (void*)&rendererData);
        OpenGL::UpdateSSBO("ViewportData", playerData.size() * sizeof(ViewportData), playerData.data());
        OpenGL::UpdateSSBO("OceanPatchTransforms", oceanPatchTransforms.size() * sizeof(glm::mat4), oceanPatchTransforms.data());

        const std::vector<RenderItemUI>& renderItemsUI = UIBackEnd::GetRenderItems();
        OpenGL::UpdateSSBO("RenderItemsUI", renderItemsUI.size() * sizeof(RenderItemUI), renderItemsUI.data());

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        OpenGL::BindSSBO(0, "Samplers");
        OpenGL::BindSSBO(1, "Materials");
        OpenGL::BindSSBO(2, "RendererData");
        OpenGL::BindSSBO(3, "ViewportData");
        OpenGL::BindSSBO(4, "InstanceData");
        OpenGL::BindSSBO(5, "Lights");
    }

    void PreGameLogicComputePasses() {
        PaintHeightMap();
    }


    void RenderDebugHackAABB() {
        static GLuint vao = 0;
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);
        }

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        gBuffer->Bind();
        gBuffer->DrawBuffer("Lighting");

        OpenGL::BindShader("DebugHackAABB");
        glBindVertexArray(vao);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(gBuffer, viewport);
                OpenGL::SetUniformMat4("u_projectionView", Unloved::RenderDataManager::GetViewportData()[i].projectionView);
                glDrawArrays(GL_LINE_STRIP, 0, 16);
            }
        }
    }

    void MultiDrawIndirect(const std::vector<DrawIndexedIndirectCommand>& commands) {
        if (commands.size()) {
            // Feed the draw command data to the gpu
            g_indirectBuffer.Bind();
            g_indirectBuffer.Update(sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data());

            // Fire of the commands
            glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, (GLvoid*)0, (GLsizei)commands.size(), 0);
        }
    }

    void SplitMultiDrawIndirect(OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand>& commands, bool bindMaterial, bool bindWoundMaterial) {
        if (!shader) {
            Logging::Fatal() << "SplitMultiDrawIndirect(..) was called with nullptr shader\n";
            return;
        }

        const std::vector<RenderItem>& instanceData = Unloved::RenderDataManager::GetInstanceData();

        for (const DrawIndexedIndirectCommand& command : commands) {
            int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
            int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

            for (GLuint i = 0; i < command.instanceCount; ++i) {
                const RenderItem& renderItem = instanceData[instanceOffset + i];

                OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);
                OpenGL::SetUniformInt("u_globalInstanceIndex", instanceOffset + i);

                if (bindMaterial) {
                    Material* material = Hell::ResourceManager::GetMaterialByIndex(renderItem.materialIndex);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE3);
                }
                if (bindWoundMaterial) {
                    Material* material = Hell::ResourceManager::GetMaterialByIndex(renderItem.woundMaterialIndex);
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());
                }

                glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
            }
        }
    }

    void DrawFullscreenTriangle() {
        BindEmptyVAO();
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    void PresentFinalImage(OpenGLFrameBuffer& presentFbo) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Present");
        if (!shader) return;

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDrawBuffer(GL_BACK);
        glViewport(0, 0, Hell::BackEnd::GetCurrentWindowWidth(), Hell::BackEnd::GetCurrentWindowHeight());
        glDisable(GL_SCISSOR_TEST);

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.cullfaceEnable = false;
        state.blendEnable = false;
        state.colorMask = true;
        OpenGL::RasterizerStateManager::ForceRasterizerState(state);

        OpenGL::BindShader("Present");
        OpenGL::BindTextureUnit(0, presentFbo.GetColorAttachmentHandleByName("Color"));
        DrawFullscreenTriangle();
    }

    void DebugHack(const std::string& message) {

    }

    void CreateBlurBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        // Iterate each viewport
        for (int x = 0; x < 4; x++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(x);

            // Start the first blur buffer at the full viewport dimensions
            Unloved::SpaceCoords spaceCoords = viewport->GetGBufferSpaceCoords();
            float width = spaceCoords.width;
            float height = spaceCoords.height;

            // Create framebuffers, downscale by 50% each time
            for (int y = 0; y < 4; y++) {

                std::string blurBufferName = "BlurBuffer_" + std::to_string(x) + "_" + std::to_string(y);
                OpenGLFrameBuffer& blurBuffer = OpenGL::ResourceManager::CreateFrameBuffer(blurBufferName);
                blurBuffer.Create(blurBufferName, (int)width, (int)height);
                blurBuffer.CreateAttachment("ColorA", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
                blurBuffer.CreateAttachment("ColorB", GL_RGBA8, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
                width *= 0.5f;
                height *= 0.5f;
            }
        }
    }

    void BindEmptyVAO() {
        if (g_emptyVao == 0) glGenVertexArrays(1, &g_emptyVao);
        glBindVertexArray(g_emptyVao);
    }

	void MultiDrawPerViewport(OpenGLFrameBuffer* fbo, OpenGLShader* shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		OpenGL::RasterizerStateManager::SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGL::Renderer::SetViewport(fbo, viewport);
				if (Hell::BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
        OpenGL::RasterizerStateManager::SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(&fbo, viewport);
                MultiDrawIndirect(drawCommands[i]);
            }
        }
    }

	void MultiDrawPerViewport(OpenGLFrameBuffer& fbo, OpenGLShader& shader, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
		OpenGL::RasterizerStateManager::SetRasterizerState(rasterizerState);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGL::Renderer::SetViewport(&fbo, viewport);
				if (Hell::BackEnd::RenderDocFound()) {
					SplitMultiDrawIndirect(&shader, drawCommands[i], true, false);
				}
				else {
					MultiDrawIndirect(drawCommands[i]);
				}
			}
		}
	}

    GLuint GetTextureHandleByName(const std::string& name) {
        if (auto it = g_cachedTextureHandles.find(name); it != g_cachedTextureHandles.end()) {
            return it->second;
        }

        Texture* texture = Hell::ResourceManager::GetTextureByName(name);
        if (!texture) {
            Logging::Fatal() << "OpenGL::Renderer::GetTextureHandleByName() failed because '" << name << "' does not exist\n";
            return 0;
        }

        const GLuint textureHandle = texture->GetGLTexture().GetHandle();
        g_cachedTextureHandles.emplace(name, textureHandle);
        return textureHandle;
    }

    OpenGLMeshPatch* GetOceanMeshPatch() {
        return &g_tesselationPatch;
    }



    void CleanUp() {
        if (g_emptyVao != 0) {
            glDeleteVertexArrays(1, &g_emptyVao);
            g_emptyVao = 0;
        }
    }

    std::vector<float>& GetShadowCascadeLevels() {
        return g_shadowCascadeLevels;
    }

    void EditorRasterizerStateOverride() {
        if (Editor::IsOpen() && Editor::BackfaceCullingDisabled()) {
            glDisable(GL_CULL_FACE);
        }
    }

    const std::string& GetZoneNames() {
        return ProfilerOpenGLZoneNames();
    }

    const std::string& GetZoneGPUTimings() {
        return ProfilerOpenGLGpuTimings();
    }

    const std::string& GetZoneCPUTimings() {
        return ProfilerOpenGLCpuTimings();
    }

    const std::string& GetTotalGPUTime() {
        return ProfilerOpenGLTotalGPU();
    }

    const std::string& GetTotalCPUTime() {
        return ProfilerOpenGLTotalCPU();
    }

    uint32_t GetTileCount() { return Unloved::Renderer::GetTileCount(); }
	uint32_t GetTileCountX() { return Unloved::Renderer::GetTileCountX(); }
	uint32_t GetTileCountY() { return Unloved::Renderer::GetTileCountY(); }
}
