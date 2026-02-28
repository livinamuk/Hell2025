#include "GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "API/OpenGL/GL_Util.h"
#include "API/OpenGL/Types/GL_indirectBuffer.hpp"
#include "API/OpenGL/Types/GL_pbo.hpp"
#include "API/OpenGL/Types/GL_shader.h"
#include "API/OpenGL/Types/GL_ssbo.hpp"
#include "AssetManagement/AssetManager.h"
#include "BackEnd/BackEnd.h"
#include "Audio/Audio.h"
#include "Core/Game.h"
#include "Config/Config.h"
#include "Input/Input.h"
#include "Ocean/Ocean.h"
#include "Player/Player.h"
#include "Renderer/RenderDataManager.h"
#include "Util/Util.h"
#include "UI/UIBackEnd.h"
#include "UI/TextBlitter.h"
#include "Types/Game/GameObject.h"
#include "../Timer.hpp"
#include <glm/gtx/matrix_decompose.hpp>

#include "Editor/Editor.h"
#include "Editor/Gizmo.h"
#include "Viewport/ViewportManager.h"

#include "API/OpenGL/Types/GL_texture_readback.h"
#include "Tools/ImageTools.h"

#include "HellLogging.h"
#include "World/World.h"

#define NONE_BIT 0

namespace OpenGLRenderer {
    std::unordered_map<std::string, OpenGLShader> g_shaders;
    std::unordered_map<std::string, OpenGLFrameBuffer> g_frameBuffers;
    std::unordered_map<std::string, OpenGLShadowMap> g_shadowMaps;
    std::unordered_map<std::string, OpenGLCubemapView> g_cubemapViews;
    std::unordered_map<std::string, OpenGLSSBO> g_ssbos;
    std::unordered_map<std::string, OpenGLRasterizerState> g_rasterizerStates;
    std::unordered_map<std::string, OpenGLShadowCubeMapArray> g_shadowCubeMapArrays;
    std::unordered_map<std::string, OpenGLShadowMapArray> g_shadowMapArrays;
    std::unordered_map<std::string, OpenGLTextureArray> g_textureArrays;
    std::unordered_map<std::string, OpenGLTexture3D> g_3dTextures;

    OpenGLMeshPatch g_tesselationPatch;

    OpenGLFrameBuffer g_blurBuffers[4][4] = {};

    //std::vector<float> g_shadowCascadeLevels{ FAR_PLANE / 50.0f, FAR_PLANE / 25.0f, FAR_PLANE / 10.0f, FAR_PLANE / 2.0f };
    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f };
    const glm::vec3 g_lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));
    unsigned int g_lightFBO;
    unsigned int g_lightDepthMaps;
    constexpr unsigned int g_depthMapResolution = 4096;

    int g_fftDisplayMode = 0;
    int g_fftEditBand = 0;

    void LoadShaders();

    IndirectBuffer g_indirectBuffer;

    struct Cubemaps {
        OpenGLCubemapView g_skyboxView;
    } g_cubemaps;

    void ClearRenderTargets();

    int GetFftDisplayMode() {
        return g_fftDisplayMode;
    }

    void Init() {
        const Resolutions& resolutions = Config::GetResolutions();

        Ocean::Init();

        g_3dTextures["PerlinNoise"] = OpenGLTexture3D();
        g_3dTextures["PerlinNoise"].Create(128, GL_R32F, true);



		g_frameBuffers["DepthPeeledTransparency"] = OpenGLFrameBuffer("DepthPeeledTransparency", resolutions.gBuffer);
		g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("Color", GL_RGBA8);
		g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("ViewspaceDepth", GL_R32F);
		g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
		g_frameBuffers["DepthPeeledTransparency"].CreateAttachment("Composite", GL_RGBA8);
		g_frameBuffers["DepthPeeledTransparency"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);


        g_frameBuffers["BloodFluid"] = OpenGLFrameBuffer("BloodFluid", resolutions.gBuffer);
        g_frameBuffers["BloodFluid"].CreateAttachment("Depth", GL_R32F);
        g_frameBuffers["BloodFluid"].CreateAttachment("Thickness", GL_R32F);
        g_frameBuffers["BloodFluid"].CreateAttachment("BlurIntermediate", GL_R32F);

        g_frameBuffers["GaussianBlur"] = OpenGLFrameBuffer("GaussianBlur", resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        g_frameBuffers["GaussianBlur"].CreateAttachment("ColorA", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);
        g_frameBuffers["GaussianBlur"].CreateAttachment("ColorB", GL_RGBA16F, GL_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE);

        g_frameBuffers["DecalPainting"] = OpenGLFrameBuffer("DecalPainting", 512, 512);
        g_frameBuffers["DecalPainting"].CreateAttachment("UVMap", GL_RGBA8, GL_LINEAR, GL_LINEAR);
        g_frameBuffers["DecalPainting"].CreateDepthAttachment(GL_DEPTH_COMPONENT24);

        g_textureArrays["WoundMasks"] = OpenGLTextureArray();
        g_textureArrays["WoundMasks"].AllocateMemory(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE, GL_R8, 1, WOUND_MASK_TEXTURE_ARRAY_SIZE); // consider adding mipmaps

        g_frameBuffers["DecalMasks"] = OpenGLFrameBuffer("DecalMasks", WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE);

        g_frameBuffers["GBuffer"] = OpenGLFrameBuffer("GBuffer", resolutions.gBuffer);
        g_frameBuffers["GBuffer"].CreateAttachment("BaseColor", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateAttachment("Normal", GL_RGBA16F);
        g_frameBuffers["GBuffer"].CreateAttachment("RMA", GL_RGBA8); // In alpha is screenspace blood decal mask
        g_frameBuffers["GBuffer"].CreateAttachment("FinalLighting", GL_RGBA16F, GL_LINEAR, GL_LINEAR);
        g_frameBuffers["GBuffer"].CreateAttachment("WorldPosition", GL_RGBA32F);
        g_frameBuffers["GBuffer"].CreateAttachment("Emissive", GL_RGBA8);
        g_frameBuffers["GBuffer"].CreateAttachment("Glass", GL_RGBA16F);
        g_frameBuffers["GBuffer"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers["GBufferBackup"] = OpenGLFrameBuffer("GBufferBackup", resolutions.gBuffer);
        g_frameBuffers["GBufferBackup"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers["Fog"] = OpenGLFrameBuffer("Fog", resolutions.gBuffer / 2);
        g_frameBuffers["Fog"].CreateAttachment("Color", GL_RGBA16F, GL_LINEAR, GL_LINEAR);

        g_frameBuffers["QuarterSize"].Create("QuarterSize", resolutions.gBuffer.x / 4, resolutions.gBuffer.y / 4);
        g_frameBuffers["QuarterSize"].CreateAttachment("DownsampledFinalLighting", GL_RGBA16F);

        g_frameBuffers["HalfSize"].Create("QuarterSize", resolutions.gBuffer.x / 2, resolutions.gBuffer.y / 2);
        g_frameBuffers["HalfSize"].CreateAttachment("DownsampledFinalLighting", GL_RGBA16F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_CLAMP_TO_EDGE, true);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRHistoryA", GL_RGBA16F);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRHistoryB", GL_RGBA16F);
        g_frameBuffers["HalfSize"].CreateAttachment("SSRCurrent", GL_RGBA16F);

        g_frameBuffers["MiscFullSize"].Create("FullSize", resolutions.gBuffer);
        g_frameBuffers["MiscFullSize"].CreateAttachment("GaussianFinalLightingIntermediate", GL_RGBA16F);
        g_frameBuffers["MiscFullSize"].CreateAttachment("GaussianFinalLighting", GL_RGBA16F);
        g_frameBuffers["MiscFullSize"].CreateAttachment("ScreenSpaceBloodDecalMask", GL_R8);
        g_frameBuffers["MiscFullSize"].CreateAttachment("ViewportIndex", GL_R8UI, GL_NEAREST, GL_NEAREST);
        g_frameBuffers["MiscFullSize"].CreateAttachment("ViewspaceDepth", GL_R32F, GL_NEAREST, GL_NEAREST);

        g_frameBuffers["Water"] = OpenGLFrameBuffer("Water", resolutions.gBuffer);
        g_frameBuffers["Water"].CreateAttachment("Color", GL_RGBA16F);
        g_frameBuffers["Water"].CreateAttachment("UnderwaterMask", GL_R8);
        g_frameBuffers["Water"].CreateAttachment("WorldPosition", GL_RGBA32F);
        g_frameBuffers["Water"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);

        g_frameBuffers["WIP"] = OpenGLFrameBuffer("WIP", resolutions.gBuffer);
        g_frameBuffers["WIP"].CreateAttachment("WorldPosition", GL_RGBA32F);

        g_frameBuffers["Outline"] = OpenGLFrameBuffer("Outline", resolutions.gBuffer);
        g_frameBuffers["Outline"].CreateAttachment("Mask", GL_R8);
        g_frameBuffers["Outline"].CreateAttachment("Result", GL_R8);

        g_frameBuffers["Hair"] = OpenGLFrameBuffer("Hair", resolutions.hair);
        g_frameBuffers["Hair"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
        g_frameBuffers["Hair"].CreateAttachment("Lighting", GL_RGBA16F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepth", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
        g_frameBuffers["Hair"].CreateAttachment("Composite", GL_RGBA16F);

        g_frameBuffers["FinalImage"] = OpenGLFrameBuffer("FinalImage", resolutions.finalImage);
        g_frameBuffers["FinalImage"].CreateAttachment("Color", GL_RGBA16F);

        g_frameBuffers["UI"] = OpenGLFrameBuffer("UI", resolutions.ui);
        g_frameBuffers["UI"].CreateAttachment("Color", GL_RGBA8, GL_NEAREST, GL_NEAREST);

        g_frameBuffers["World"] = OpenGLFrameBuffer("World", 1, 1);
        g_frameBuffers["World"].CreateAttachment("HeightMap", GL_R16F);

        g_frameBuffers["Road"] = OpenGLFrameBuffer("Road", 1, 1);
        g_frameBuffers["Road"].CreateAttachment("RoadMask", GL_R16F);

        g_frameBuffers["HeightMapBlitBuffer"] = OpenGLFrameBuffer("HeightMapBlitBuffer", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);

        g_frameBuffers["HeightMap"] = OpenGLFrameBuffer("HeightMap", HEIGHT_MAP_SIZE, HEIGHT_MAP_SIZE);
        g_frameBuffers["HeightMap"].CreateAttachment("Color", GL_R16F);

        g_frameBuffers["FFT_band0"].Create("FFT_band0", Ocean::GetFFTResolution(0).x, Ocean::GetFFTResolution(0).y);
        g_frameBuffers["FFT_band0"].CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT);
        g_frameBuffers["FFT_band0"].CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        g_frameBuffers["FFT_band1"].Create("FFT_band1", Ocean::GetFFTResolution(1).x, Ocean::GetFFTResolution(1).y);
        g_frameBuffers["FFT_band1"].CreateAttachment("Displacement", GL_RGBA32F, GL_LINEAR, GL_LINEAR, GL_REPEAT, true);
        g_frameBuffers["FFT_band1"].CreateAttachment("Normals", GL_RGBA32F, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR, GL_REPEAT, true);

        g_shadowMaps["FlashlightShadowMaps"] = OpenGLShadowMap("FlashlightShadowMaps", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE, 4);

        GLbitfield staticFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
        GLbitfield dynamicFlags = GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;

        // Create ssbos
        g_ssbos["Samplers"] = OpenGLSSBO(sizeof(glm::uvec2), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["ViewportData"] = OpenGLSSBO(sizeof(ViewportData) * 4, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["RendererData"] = OpenGLSSBO(sizeof(RendererData), GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["InstanceData"] = OpenGLSSBO(sizeof(RenderItem) * MAX_INSTANCE_DATA_COUNT, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["SkinningTransforms"] = OpenGLSSBO(sizeof(glm::mat4) * MAX_ANIMATED_TRANSFORMS, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["Lights"] = OpenGLSSBO(sizeof(GPULight) * MAX_GPU_LIGHTS, GL_DYNAMIC_STORAGE_BIT);
        g_ssbos["BloodDecalInstances"] = OpenGLSSBO(sizeof(BloodDecalInstanceData) * MAX_SCREEN_SPACE_BLOOD_DECAL_COUNT, GL_DYNAMIC_STORAGE_BIT);

        //g_ssbos["ffth0"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), staticFlags);

        g_ssbos["ffth0Band0"] = OpenGLSSBO(Ocean::GetFFTResolution(0).x * Ocean::GetFFTResolution(0).y * sizeof(std::complex<float>), staticFlags);
        g_ssbos["ffth0Band1"] = OpenGLSSBO(Ocean::GetFFTResolution(1).x * Ocean::GetFFTResolution(1).y * sizeof(std::complex<float>), staticFlags);

        const glm::uvec2 oceanSize = Ocean::GetBaseFFTResolution(); // WARNING!!! This size must bit your largest FFT dimensions
        g_ssbos["fftSpectrumInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftSpectrumOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftDispInXSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftDispZInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftGradXInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftGradZInSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftDispXOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftDispZOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftGradXOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);
        g_ssbos["fftGradZOutSSBO"] = OpenGLSSBO(oceanSize.x * oceanSize.y * sizeof(std::complex<float>), dynamicFlags);

        int dummySize = 64;
        CreateSSBO("TriangleData", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("SceneBvh", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("MeshesBvh", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("EntityInstances", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("PointGridBuffer", dummySize, GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("PointIndicesBuffer", dummySize, GL_DYNAMIC_STORAGE_BIT);

        g_tesselationPatch.Resize2(Ocean::GetTesslationMeshSize().x, Ocean::GetTesslationMeshSize().y);

        // Upload HO
        const std::vector<std::complex<float>>& h0Band0 = Ocean::GetH0(0);
        const std::vector<std::complex<float>>& h0Band1 = Ocean::GetH0(1);
        g_ssbos["ffth0Band0"].CopyFrom(h0Band0.data(), sizeof(std::complex<float>) * h0Band0.size());
        g_ssbos["ffth0Band1"].CopyFrom(h0Band1.data(), sizeof(std::complex<float>) * h0Band1.size());

        int tileXCount = g_frameBuffers["GBuffer"].GetWidth() / TILE_SIZE;
        int tileYCount = g_frameBuffers["GBuffer"].GetHeight() / TILE_SIZE;
        int tileCount = tileXCount * tileYCount;

        CreateSSBO("TileChristmasLights", tileCount * sizeof(TileInstanceData), NONE_BIT);
        CreateSSBO("TileBloodDecals", tileCount * sizeof(TileInstanceData), NONE_BIT);
        CreateSSBO("TileLights",      tileCount * sizeof(TileLights), NONE_BIT);
        CreateSSBO("TileWorldBounds", tileCount * sizeof(TileWorldBounds), NONE_BIT);

        CreateSSBO("MetaBalls", sizeof(glm::vec4) * 1000, GL_DYNAMIC_STORAGE_BIT);

        CreateSSBO("ChristmasLightInstances", MAX_CHRISTMAS_LIGHTS * sizeof(GPUChristmasLight), GL_DYNAMIC_STORAGE_BIT);
        CreateSSBO("ChristmasLightIndices", sizeof(uint32_t) * tileCount * 256, NONE_BIT);
        CreateSSBO("ChristmasLightCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);

        CreateSSBO("BloodDecalIndices", sizeof(uint32_t) * tileCount * 256, NONE_BIT);
        CreateSSBO("BloodDecalCounter", sizeof(uint32_t), GL_DYNAMIC_STORAGE_BIT);


        int MAX_OCEAN_PATCHES = 500;
        CreateSSBO("OceanPatchTransforms", sizeof(glm::mat4) * MAX_OCEAN_PATCHES, GL_DYNAMIC_STORAGE_BIT);

        // Preallocate the indirect command buffer
        g_indirectBuffer.PreAllocate(sizeof(DrawIndexedIndirectCommand) * MAX_INDIRECT_DRAW_COMMAND_COUNT);

        LoadShaders();

        // Allocate shadow map array memory
        g_shadowCubeMapArrays["HiRes"] = OpenGLShadowCubeMapArray();
        g_shadowCubeMapArrays["HiRes"].Init(SHADOWMAP_HI_RES_COUNT, SHADOW_MAP_HI_RES_SIZE);

        // Moon light shadow maps
        float depthMapResolution = SHADOW_MAP_CSM_SIZE;
        int cascadeCount = int(g_shadowCascadeLevels.size()) + 1;
        int playerCount = 2;
        int layerCount = playerCount * cascadeCount;
        g_shadowMapArrays["MoonlightCSM"] = OpenGLShadowMapArray();
        g_shadowMapArrays["MoonlightCSM"].Init(layerCount, depthMapResolution, GL_DEPTH_COMPONENT32F);

        InitFog();
        InitGrass();
        InitOceanHeightReadback();
    }

    void InitMain() {
        InitRasterizerStates();

        // Attempt to load skybox
        std::vector<Texture*> textures = {
            AssetManager::GetTextureByName("px"),
            AssetManager::GetTextureByName("nx"),
            AssetManager::GetTextureByName("py"),
            AssetManager::GetTextureByName("ny"),
            AssetManager::GetTextureByName("pz"),
            AssetManager::GetTextureByName("nz"),
            //AssetManager::GetTextureByName("NightSky_Right"),
            //AssetManager::GetTextureByName("NightSky_Left"),
            //AssetManager::GetTextureByName("NightSky_Top"),
            //AssetManager::GetTextureByName("NightSky_Bottom"),
            //AssetManager::GetTextureByName("NightSky_Front"),
            //AssetManager::GetTextureByName("NightSky_Back"),
        };
        std::vector<GLuint> texturesHandles;
        for (Texture* texture : textures) {
            if (!texture) continue;
            texturesHandles.push_back(texture->GetGLTexture().GetHandle());
        }
        if (texturesHandles.size() == 6) {
            g_cubemapViews["SkyboxNightSky"] = OpenGLCubemapView(texturesHandles);
        }

        CreateBlurBuffers();
    }

    void LoadShaders() {
        g_shaders["ChristmasLightCulling"] = OpenGLShader({ "GL_christmas_light_culling.comp" });
        g_shaders["ChristmasLightsWire"] = OpenGLShader({ "GL_christmas_light_wire.vert", "GL_christmas_light_wire.frag" });

        g_shaders["RaytraceScene"] = OpenGLShader({ "GL_raytrace_scene.comp" });

        g_shaders["BlitRoad"] = OpenGLShader({ "GL_blit_road.comp" });
        g_shaders["BlurHorizontal"] = OpenGLShader({ "GL_blur_horizontal.vert", "GL_blur.frag" });
        g_shaders["BlurVertical"] = OpenGLShader({ "GL_blur_vertical.vert", "GL_blur.frag" });
        g_shaders["ComputeSkinning"] = OpenGLShader({ "GL_compute_skinning.comp" });
        g_shaders["ComputeTileWorldBounds"] = OpenGLShader({ "GL_compute_tile_world_bounds.comp" });
        g_shaders["DebugLightVolume"] = OpenGLShader({ "GL_debug_light_volume.vert", "GL_debug_light_volume.frag" });
        g_shaders["DebugPointCloud"] = OpenGLShader({ "GL_debug_point_cloud.vert", "GL_debug_point_cloud.frag" });
        g_shaders["DebugSolidColor"] = OpenGLShader({ "GL_debug_solid_color.vert", "GL_debug_solid_color.frag" });
        g_shaders["DebugRagdoll"] = OpenGLShader({ "GL_debug_ragdoll.vert", "GL_debug_ragdoll.frag" });
        g_shaders["DebugTextureBlit"] = OpenGLShader({ "GL_debug_texture_blit.vert", "GL_debug_texture_blit.frag" });
        g_shaders["DebugTextured"] = OpenGLShader({ "GL_debug_textured.vert", "GL_debug_textured.frag" });
        g_shaders["DebugView"] = OpenGLShader({ "GL_debug_view.comp" });
        g_shaders["DebugTileView"] = OpenGLShader({ "GL_debug_tile_view.comp" });
        g_shaders["DebugVertex2D"] = OpenGLShader({ "GL_debug_vertex_2D.vert", "GL_debug_vertex_2D.frag" });
        g_shaders["DebugVertex3D"] = OpenGLShader({ "GL_debug_vertex_3D.vert", "GL_debug_vertex_3D.frag" });
        g_shaders["DecalPaintUVs"] = OpenGLShader({ "gl_decal_paint_uvs.vert", "gl_decal_paint_uvs.frag" });
        g_shaders["DecalPaintMask"] = OpenGLShader({ "gl_decal_paint_mask.comp" });
        g_shaders["Decals"] = OpenGLShader({ "GL_decals.vert", "GL_decals.frag" });
        g_shaders["DownSample2xBox"] = OpenGLShader({ "GL_down_sample_2x_box.comp" });


        g_shaders["EditorMesh"] = OpenGLShader({ "GL_editor_mesh.vert", "GL_editor_mesh.frag" });
        g_shaders["ExamineItem"] = OpenGLShader({ "GL_examine_item.vert", "GL_examine_item.frag" });
        g_shaders["FogRayMarch"] = OpenGLShader({ "GL_fog_ray_march.comp" });
        g_shaders["FogComposite"] = OpenGLShader({ "GL_fog_composite.comp" });
        g_shaders["FttRadix64Vertical"] = OpenGLShader({ "GL_ftt_radix_64_vertical.comp" });
        g_shaders["FttRadix8Vertical"] = OpenGLShader({ "GL_ftt_radix_8_vertical.comp" });
        g_shaders["FttRadix64Horizontal"] = OpenGLShader({ "GL_ftt_radix_64_horizontal.comp" });
        g_shaders["FttRadix8Horizontal"] = OpenGLShader({ "GL_ftt_radix_8_horizontal.comp" });
        g_shaders["Fur"] = OpenGLShader({ "GL_fur.vert", "GL_fur.frag" });
        g_shaders["FurComposite"] = OpenGLShader({ "GL_fur_composite.comp" });
        g_shaders["EmissiveComposite"] = OpenGLShader({ "GL_emissive_composite.comp" });
        g_shaders["GBuffer"] = OpenGLShader({ "GL_GBuffer.vert", "GL_gBuffer.frag" });
        g_shaders["Gizmo"] = OpenGLShader({ "GL_gizmo.vert", "GL_gizmo.frag" });
        g_shaders["Glass"] = OpenGLShader({ "GL_glass.vert", "GL_glass.frag" });
        g_shaders["GlassComposite"] = OpenGLShader({ "GL_glass_composite.comp" });
        g_shaders["Grass"] = OpenGLShader({ "GL_grass.vert", "GL_grass.frag" });
        g_shaders["GrassGeometryGeneration"] = OpenGLShader({ "GL_grass_geometry_generation.comp" });
        g_shaders["GrassPositionGeneration"] = OpenGLShader({ "GL_grass_position_generation.comp" });
        g_shaders["GaussianBlurUtil"] = OpenGLShader({ "GL_gaussian_blur_util.comp" });
        g_shaders["HairDepthPeel"] = OpenGLShader({ "GL_hair_depth_peel.vert", "GL_hair_depth_peel.frag" });
        g_shaders["HairFinalComposite"] = OpenGLShader({ "GL_hair_final_composite.comp" });
        g_shaders["HairLayerComposite"] = OpenGLShader({ "GL_hair_layer_composite.comp" });
        g_shaders["HairLighting"] = OpenGLShader({ "GL_hair_lighting.vert", "GL_hair_lighting.frag" });
        g_shaders["HeightMapColor"] = OpenGLShader({ "GL_heightmap_color.vert", "GL_heightmap_color.frag" });
        g_shaders["HeightMapImageGeneration"] = OpenGLShader({ "GL_heightmap_image_generation.comp" });
        g_shaders["HeightMapPhysxTextureGeneration"] = OpenGLShader({ "GL_heightmap_physx_texture_generation.comp" });
        g_shaders["HeightMapToWorldBlit"] = OpenGLShader({ "GL_heightmap_to_world_blit.comp" });
        g_shaders["HeightMapVertexGeneration"] = OpenGLShader({ "GL_heightmap_vertex_generation.comp" });
        g_shaders["HeightMapPaint"] = OpenGLShader({ "GL_heightmap_paint.comp" });
        g_shaders["LightCulling"] = OpenGLShader({ "GL_light_culling.comp" });
        g_shaders["LightVolumeLighting"] = OpenGLShader({ "GL_light_volume_lighting.comp" });
        g_shaders["LightVolumeMask"] = OpenGLShader({ "GL_light_volume_mask.comp" });
        g_shaders["Lighting"] = OpenGLShader({ "GL_lighting.comp" });
        g_shaders["CSMLighting"] = OpenGLShader({ "GL_lighting.vert", "GL_lighting.frag" });
        g_shaders["OceanSurfaceComposite"] = OpenGLShader({ "GL_ocean_surface_composite.comp" });
        g_shaders["OceanGeometry"] = OpenGLShader({ "GL_ocean_geometry.vert", "GL_ocean_geometry.frag", "GL_ocean_geometry.tesc", "GL_ocean_geometry.tese" });
        g_shaders["OceanCalculateSpectrum"] = OpenGLShader({ "GL_ocean_calculate_spectrum.comp" });
        g_shaders["OceanUpdateTextures"] = OpenGLShader({ "GL_ocean_update_textures.comp" });
        g_shaders["OceanUnderwaterComposite"] = OpenGLShader({ "GL_ocean_underwater_composite.comp" });
        g_shaders["OceanUnderwaterMaskPreProcess"] = OpenGLShader({ "GL_ocean_underwater_mask_preprocess.comp" });
        g_shaders["OceanTesseleationEdgeTransitionCleanUp"] = OpenGLShader({ "GL_ocean_tessellation_edge_transition_cleanup.comp" });
        g_shaders["OceanPositionReadback"] = OpenGLShader({ "GL_ocean_position_readback.comp" });
        g_shaders["GaussianBlur"] = OpenGLShader({ "GL_gaussian_blur.comp" }); // am I needed????
        g_shaders["Outline"] = OpenGLShader({ "GL_outline.vert", "GL_outline.frag" });
        g_shaders["OutlineComposite"] = OpenGLShader({ "GL_outline_composite.comp" });
        g_shaders["OutlineMask"] = OpenGLShader({ "GL_outline_mask.vert", "GL_outline_mask.frag" });
        g_shaders["PointCloudLighting"] = OpenGLShader({ "GL_point_cloud_lighting.comp" });
        g_shaders["PerlinNoise3D"] = OpenGLShader({ "GL_perlin_noise_3d.comp" });
        g_shaders["PostProcessing"] = OpenGLShader({ "GL_post_processing.comp" });
        g_shaders["ShadowMap"] = OpenGLShader({ "GL_shadow_map.vert", "GL_shadow_map.frag" });
        g_shaders["ShadowCubeMap"] = OpenGLShader({ "GL_shadow_cube_map.vert", "GL_shadow_cube_map.frag" });
        g_shaders["SolidColor"] = OpenGLShader({ "GL_solid_color.vert", "GL_solid_color.frag" });
        g_shaders["Skybox"] = OpenGLShader({ "GL_skybox.vert", "GL_skybox.frag" });
        g_shaders["SpriteSheet"] = OpenGLShader({ "GL_sprite_sheet.vert", "GL_sprite_sheet.frag" });

        g_shaders["ScreenspaceReflections"] = OpenGLShader({ "GL_screenspace_reflections.comp" });

        g_shaders["StainedGlass"] = OpenGLShader({ "GL_stained_glass.vert", "GL_stained_glass.frag" });
        g_shaders["UI"] = OpenGLShader({ "GL_ui.vert", "GL_ui.frag" });
        g_shaders["Winston"] = OpenGLShader({ "GL_winston.vert", "GL_winston.frag" });
        g_shaders["CSMDepth"] = OpenGLShader({ "GL_csm_depth.vert", "GL_csm_depth.frag", "GL_csm_depth.geom" });
        g_shaders["ZeroOut"] = OpenGLShader({ "GL_zero_out.comp" });
        g_shaders["VatBlood"] = OpenGLShader({ "GL_vat_blood.vert", "GL_vat_blood.frag" });

        g_shaders["BloodDecalsCulling"] = OpenGLShader({ "GL_blood_decals_culling.comp" });
        g_shaders["BloodDecalsDraw"] = OpenGLShader({ "GL_blood_decals_draw.vert", "GL_blood_decals_draw.frag" });
        g_shaders["BloodDecalsComposite"] = OpenGLShader({ "GL_blood_decals_composite.comp" });


        g_shaders["BloodDecalsRaster"] = OpenGLShader({ "GL_blood_decals_raster.vert", "GL_blood_decals_raster.frag" });

        //g_shaders["MetaBallsOLD"] = OpenGLShader({ "GL_metaballs_OLD.comp" });
        g_shaders["MetaBalls"] = OpenGLShader({ "GL_meta_balls.vert", "GL_meta_balls.frag"});


        g_shaders["BloodFluidDepth"] = OpenGLShader({ "GL_blood_fluid.vert", "GL_blood_fluid_depth.frag" });
        g_shaders["BloodFluidThickness"] = OpenGLShader({ "GL_blood_fluid.vert", "GL_blood_fluid_thickness.frag" });
        g_shaders["BloodFluidBlur"] = OpenGLShader({ "GL_blood_fluid_blur.comp" });

        g_shaders["MetaBalls"] = OpenGLShader({ "GL_meta_balls.vert", "GL_meta_balls.frag" });

		g_shaders["ViewspaceDepth"] = OpenGLShader({ "GL_viewspace_depth.comp" });

		g_shaders["DepthPeeledTransparencyColor"] = OpenGLShader({ "GL_depth_peeled_transparency_color.vert", "GL_depth_peeled_transparency_color.frag" });
		g_shaders["DepthPeeledTransparencyDepth"] = OpenGLShader({ "GL_depth_peeled_transparency_depth.vert", "GL_depth_peeled_transparency_depth.frag" });
		g_shaders["DepthPeeledTransparencyComposite"] = OpenGLShader({ "GL_depth_peeled_transparency_composite.comp" });

    }

    void UpdateSSBOS() {
        const std::vector<GLuint64>& bindlessTextureIDs = OpenGLBackEnd::GetBindlessTextureIDs();
        const void* bindlessTextureData = bindlessTextureIDs.empty() ? nullptr : bindlessTextureIDs.data();
        g_ssbos["Samplers"].Update(bindlessTextureIDs.size() * sizeof(GLuint64), bindlessTextureData);
        g_ssbos["Samplers"].Bind(0);

        const RendererData& rendererData = RenderDataManager::GetRendererData();
        g_ssbos["RendererData"].Update(sizeof(RendererData), (void*)&rendererData);
        g_ssbos["RendererData"].Bind(1);

        const std::vector<ViewportData>& playerData = RenderDataManager::GetViewportData();
        const void* playerDataPtr = playerData.empty() ? nullptr : playerData.data();
        g_ssbos["ViewportData"].Update(playerData.size() * sizeof(ViewportData), playerDataPtr);
        g_ssbos["ViewportData"].Bind(2);

        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();
        const void* instanceDataPtr = instanceData.empty() ? nullptr : instanceData.data();
        g_ssbos["InstanceData"].Update(instanceData.size() * sizeof(RenderItem), instanceDataPtr);
        g_ssbos["InstanceData"].Bind(3);

        const std::vector<GPULight>& gpuLightsHighRes = RenderDataManager::GetGPULightsHighRes();
        const void* lightsDataPtr = gpuLightsHighRes.empty() ? nullptr : gpuLightsHighRes.data();
        g_ssbos["Lights"].Update(gpuLightsHighRes.size() * sizeof(GPULight), lightsDataPtr);
        g_ssbos["Lights"].Bind(4);

        const std::vector<BloodDecalInstanceData>& screenSpaceBloodDecalInstances = RenderDataManager::GetScreenSpaceBloodDecalInstanceData();
        const void* bloodDecalDataPtr = screenSpaceBloodDecalInstances.empty() ? nullptr : screenSpaceBloodDecalInstances.data();
        g_ssbos["BloodDecalInstances"].Update(screenSpaceBloodDecalInstances.size() * sizeof(BloodDecalInstanceData), bloodDecalDataPtr);


        GLuint zero = 0;

        UpdateSSBO("BloodDecalCounter", sizeof(uint32_t), &zero);
        UpdateSSBO("ChristmasLightCounter", sizeof(uint32_t), &zero);

        g_ssbos["BloodDecalInstances"].Update(screenSpaceBloodDecalInstances.size() * sizeof(BloodDecalInstanceData), bloodDecalDataPtr);


        const std::vector<glm::mat4>& oceanPatchTransforms = RenderDataManager::GetOceanPatchTransforms();
        const void* oceanPatchTransformData = oceanPatchTransforms.empty() ? nullptr : oceanPatchTransforms.data();
        UpdateSSBO("OceanPatchTransforms", oceanPatchTransforms.size() * sizeof(glm::mat4), oceanPatchTransformData);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    void PreGameLogicComputePasses() {
        PaintHeightMap();
        ComputeOceanFFTPass();
        OceanHeightReadback();
    }

    void RenderGame() {
        ProfilerOpenGLFrame();

        OpenGLFrameBuffer& gBuffer = g_frameBuffers["GBuffer"];
        OpenGLFrameBuffer& hairFrameBuffer = g_frameBuffers["Hair"];
        OpenGLFrameBuffer& finalImageBuffer = g_frameBuffers["FinalImage"];

        glDisable(GL_DITHER);

        if (Input::KeyPressed(HELL_KEY_N)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            FlipNormalMapY();
        }

        //BlitRoads();

        //UpdateGlobalIllumintation();
        //PointCloudDirectLighting();
        //ComputeLightVolumeMask();
        //ComputeProbeLighting();

        ComputeSkinningPass();
        ClearRenderTargets();

        UpdateSSBOS();
        RenderShadowMaps();
        SkyBoxPass();
        HeightMapPass();
        DecalPaintingPass();
        HouseGeometryPass();
        GeometryPass();
        GrassPass();
        MirrorGeometryPass();
        WeatherBoardsPass();
        VatBloodPass();

        //MetaBallsPass();

        ComputeTileWorldBounds();
        ChristmasLightCullingPass();
        LightCullingPass();

        BloodDecalsPass();
        ComputeViewspaceDepth();
        TextureReadBackPass();

        LightingPass();

        //FurPass();
        OceanGeometryPass();
        OceanSurfaceCompositePass();
        GlassPass();
        DecalPass();
        EmissivePass();
		ScreenspaceReflectionsPass();
		HairPass();
		DepthPeeledTransparencyPass();
        RayMarchFog();
        OceanUnderwaterCompositePass();
        StainedGlassPass();
        WinstonPass();
        SpriteSheetPass(); // Muzzle flash, etc
        InventoryGaussianPass();
        PostProcessingPass();
        DebugViewPass();
        DebugPass();
        ExamineItemPass();
        EditorPass();
        OutlinePass();

        DrawPointCloud();
        DrawLightVolume();

        // Downscale blit
        OpenGLRenderer::BlitFrameBuffer(&gBuffer, &finalImageBuffer, "FinalLighting", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //DownSampleFinalImage();

        //if (Input::KeyDown(HELL_KEY_U)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Depth", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_Y)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "Thickness", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}
        //if (Input::KeyDown(HELL_KEY_T)) {
        //    OpenGLFrameBuffer* bloodFluidFbo = GetFrameBuffer("BloodFluid");
        //    OpenGLRenderer::BlitFrameBuffer(bloodFluidFbo, &finalImageBuffer, "BlurIntermediate", "Color", GL_COLOR_BUFFER_BIT, GL_LINEAR);
        //}

        // Blit to swapchain
        OpenGLRenderer::BlitToDefaultFrameBuffer(&finalImageBuffer, "Color", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        // Blit to swapchain
        //OpenGLRenderer::BlitToDefaultFrameBuffer(&gBuffer, "FinalLighting", GL_COLOR_BUFFER_BIT, GL_NEAREST);

        //BlitFog();

        UIPass();
        ImGuiPass();

        BlitDebugTextures();

        // DEBUG RENDER FFT TEXTURES TO THE SCREEN
        if (Input::KeyPressed(HELL_KEY_5)) {
            g_fftDisplayMode = 1;
            g_fftEditBand = 0;
        }
        if (Input::KeyPressed(HELL_KEY_6)) {
            g_fftDisplayMode = 2;
            g_fftEditBand = 1;
        }
        if (Input::KeyPressed(HELL_KEY_7)) {
            g_fftDisplayMode = 0;
        }
    }

    void ClearRenderTargets() {
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
        OpenGLFrameBuffer* waterFrameBuffer = GetFrameBuffer("Water");
        OpenGLFrameBuffer* finalImageFBO = GetFrameBuffer("FinalImage");
        OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBuffer("MiscFullSize");

        // Water
        waterFrameBuffer->Bind();
        waterFrameBuffer->ClearAttachment("Color", 0, 0, 0, 0);
        waterFrameBuffer->ClearAttachment("UnderwaterMask", 0);
        waterFrameBuffer->ClearAttachment("WorldPosition", 0, 0, 0, 0);

        // GBuffer
        glDepthMask(GL_TRUE);
        gBuffer->Bind();
        gBuffer->ClearAttachment("BaseColor", 0, 0, 0, 0);
        gBuffer->ClearAttachment("Normal", 0, 0, 0, 0);
        gBuffer->ClearAttachment("RMA", 0, 0, 0, 0);
        gBuffer->ClearAttachment("WorldPosition", 0, 0);
        gBuffer->ClearAttachment("Emissive", 0, 0, 0, 0);
        gBuffer->ClearAttachment("Glass", 0, 1, 0, 0);
        gBuffer->ClearDepthAttachment();

        // Decal mask
        miscFullSizeFBO->Bind();
        miscFullSizeFBO->ClearTexImage("ScreenSpaceBloodDecalMask", 0, 0, 0, 0);

        // Viewport index
        for (unsigned int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGLRenderer::ClearFrameBufferByViewportUInt(miscFullSizeFBO, "ViewportIndex", viewport, i);
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
        const std::vector<RenderItem>& instanceData = RenderDataManager::GetInstanceData();

        for (const DrawIndexedIndirectCommand& command : commands) {
            int viewportIndex = command.baseInstance >> VIEWPORT_INDEX_SHIFT;
            int instanceOffset = command.baseInstance & ((1 << VIEWPORT_INDEX_SHIFT) - 1);

            for (GLuint i = 0; i < command.instanceCount; ++i) {
                const RenderItem& renderItem = instanceData[instanceOffset + i];

                shader->SetInt("u_viewportIndex", viewportIndex);
                shader->SetInt("u_globalInstanceIndex", instanceOffset + i);

                if (bindMaterial) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.baseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE1);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.normalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE2);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.rmaTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE3);

                    // Try bind emissive texture
                    if (renderItem.emissiveTextureIndex != -1) {
                        if (Texture* texture = AssetManager::GetTextureByIndex(renderItem.emissiveTextureIndex)) {
                            glBindTexture(GL_TEXTURE_2D, texture->GetGLTexture().GetHandle());
                        }
                    }
                    // Fall back to black
                    else if (Texture* blackTexture = AssetManager::GetTextureByName("Black")) {
                        glBindTexture(GL_TEXTURE_2D, blackTexture->GetGLTexture().GetHandle());
                    }
                }
                if (bindWoundMaterial) {
                    glActiveTexture(GL_TEXTURE4);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.woundBaseColorTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE5);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.woundNormalMapTextureIndex)->GetGLTexture().GetHandle());
                    glActiveTexture(GL_TEXTURE6);
                    glBindTexture(GL_TEXTURE_2D, AssetManager::GetTextureByIndex(renderItem.woundRmaTextureIndex)->GetGLTexture().GetHandle());
                }

                glDrawElementsBaseVertex(GL_TRIANGLES, command.indexCount, GL_UNSIGNED_INT, (GLvoid*)(command.firstIndex * sizeof(GLuint)), command.baseVertex);
            }
        }
    }


    void HotloadShaders() {
        bool allSucceeded = true;
        for (auto& [_, shader] : g_shaders) {
            if (!shader.Hotload()) {
                allSucceeded = false;
            }
        }
        if (allSucceeded) {
            std::cout << "Hotloaded shaders\n";
        }
    }

    void DrawQuad() {
        static Mesh* mesh = AssetManager::GetMeshByModelNameMeshName("Primitives", "Quad");
        glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), mesh->baseVertex);
    }

    void CreateBlurBuffers() {
        const Resolutions& resolutions = Config::GetResolutions();

        // Iterate each viewport
        for (int x = 0; x < 4; x++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(x);

            // Start the first blur buffer at the full viewport dimensions
            SpaceCoords spaceCoords = viewport->GetGBufferSpaceCoords();
            float width = resolutions.gBuffer.x;
            float height = resolutions.gBuffer.y;

            // Create framebuffers, downscale by 50% each time
            for (int y = 0; y < 4; y++) {

                // Clean up existing framebuffer
                g_blurBuffers[x][y].Create("BlurBuffer", width, height);
                g_blurBuffers[x][y].CreateAttachment("ColorA", GL_RGBA8);
                g_blurBuffers[x][y].CreateAttachment("ColorB", GL_RGBA8);
                width *= 0.5f;
                height *= 0.5f;
            }
        }
    }

    int GetTileCountX() {
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
        if (!gBuffer) return 0;
        return gBuffer->GetWidth() / TILE_SIZE;
    }

    int GetTileCountY() {
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");
        if (!gBuffer) return 0;
        return gBuffer->GetHeight() / TILE_SIZE;
    }

    void CreateSSBO(const std::string& name, size_t size, GLbitfield flags) {
        g_ssbos[name] = OpenGLSSBO(size, flags);
    }

    void BindSSBO(const std::string& name, unsigned int bindingIndex) {
        if (OpenGLSSBO* ssbo = GetSSBO(name)) {
            ssbo->Bind(bindingIndex);
        }
    }

    OpenGLMeshPatch* GetOceanMeshPatch() {
        return &g_tesselationPatch;
    }

    OpenGLShader* GetShader(const std::string& name) {
        auto it = g_shaders.find(name);
        return (it != g_shaders.end()) ? &it->second : nullptr;
    }

    OpenGLFrameBuffer* GetFrameBuffer(const std::string& name) {
        auto it = g_frameBuffers.find(name);
        return (it != g_frameBuffers.end()) ? &it->second : nullptr;
    }

    OpenGLShadowMap* GetShadowMap(const std::string& name) {
        auto it = g_shadowMaps.find(name);
        return (it != g_shadowMaps.end()) ? &it->second : nullptr;
    }

    OpenGLShadowCubeMapArray* GetShadowCubeMapArray(const std::string& name) {
        auto it = g_shadowCubeMapArrays.find(name);
        return (it != g_shadowCubeMapArrays.end()) ? &it->second : nullptr;
    }

    OpenGLShadowMapArray* GetShadowMapArray(const std::string& name) {
        auto it = g_shadowMapArrays.find(name);
        return (it != g_shadowMapArrays.end()) ? &it->second : nullptr;
    }

    OpenGLTextureArray* GetTextureArray(const std::string& name) {
        auto it = g_textureArrays.find(name);
        return (it != g_textureArrays.end()) ? &it->second : nullptr;
    }

    OpenGLTexture3D* GetTexture3D(const std::string& name) {
        auto it = g_3dTextures.find(name);
        return (it != g_3dTextures.end()) ? &it->second : nullptr;
    }

    OpenGLFrameBuffer* GetBlurBuffer(int viewportIndex, int bufferIndex) {
        if (viewportIndex < 0 || viewportIndex >= 4) return nullptr;
        if (bufferIndex < 0 || bufferIndex >= 4) return nullptr;
        return &g_blurBuffers[viewportIndex][bufferIndex];
    }

    OpenGLSSBO* GetSSBO(const std::string& name) {
        auto it = g_ssbos.find(name);
        return (it != g_ssbos.end()) ? &it->second : nullptr;
    }

    OpenGLCubemapView* GetCubemapView(const std::string& name) {
        auto it = g_cubemapViews.find(name);
        return (it != g_cubemapViews.end()) ? &it->second : nullptr;
    }

    OpenGLRasterizerState* GetRasterizerState(const std::string& name) {
        auto it = g_rasterizerStates.find(name);
        return (it != g_rasterizerStates.end()) ? &it->second : nullptr;
    }

    OpenGLRasterizerState* CreateRasterizerState(const std::string& name) {
        g_rasterizerStates[name] = OpenGLRasterizerState();
        return &g_rasterizerStates[name];
    }

    void SetRasterizerState(const std::string& name) {
        OpenGLRasterizerState* rasterizerState = GetRasterizerState(name);
        if (!rasterizerState) {
            std::cout << "OpenGLRenderer::SetRasterizerState(const std::string& name) failed! " << name << " does not exist!\n";
            return;
        }

        rasterizerState->blendEnable ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        rasterizerState->cullfaceEnable ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
        rasterizerState->depthMask ? glDepthMask(GL_TRUE) : glDepthMask(GL_FALSE);
        rasterizerState->depthTestEnabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);

        if (rasterizerState->blendEnable) {
            glBlendFunc(rasterizerState->blendFuncSrcfactor, rasterizerState->blendFuncDstfactor);
        }
        if (rasterizerState->depthTestEnabled) {
            glDepthFunc(rasterizerState->depthFunc);
        }
        if (rasterizerState->pointSize > 1.0f) {
            glPointSize(rasterizerState->pointSize);
        }
    }

    std::vector<float>& GetShadowCascadeLevels() {
        return g_shadowCascadeLevels;
    }

    void UpdateSSBO(const std::string& name, size_t size, const void* data) {
        OpenGLSSBO* ssbo = GetSSBO(name);
        if (ssbo && size > 0) {
            ssbo->Update(size, data);
        }
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

}
