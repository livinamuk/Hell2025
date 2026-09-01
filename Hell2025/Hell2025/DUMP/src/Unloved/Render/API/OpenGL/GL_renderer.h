#pragma once

#include "Hell/Render/API/OpenGL/Types/GL_cubemapView.h"
#include "Hell/Render/API/OpenGL/Types/GL_cubemap_frame_buffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_mesh_buffer_old.h"
#include "Hell/Render/API/OpenGL/Types/GL_frameBuffer.h"
#include "Hell/Render/API/OpenGL/Types/GL_shader.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_map.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_cube_map_array.h"
#include "Hell/Render/API/OpenGL/Types/GL_shadow_map_array.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_array.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_3d.h"
#include "Hell/Render/API/OpenGL/Types/GL_timer.h"
#include "Hell/Render/API/OpenGL/GL_commands.h"
#include "Hell/Render/API/OpenGL/GL_rasterizer_state_manager.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"

#include "Hell/Math/AABB.h"
#include "Hell/Math/OBB.h"
#include "Hell/Render/DrawCommandTypes.h"
#include "Hell/Render/VertexAttributes.h"

#include "res/shaders/common/OpenGL/GL_binding_indices.glsl"

#include "Unloved/Systems/DDGI/DDGIVolume.h"

#include "Unloved/Maps/MapData.h"
#include "Unloved/Viewport/Viewport.h"

#include <string>


#include "Hell/Render/API/OpenGL/Types/GL_indirectBuffer.hpp" // TODO: Make me an SSBO and get me the fuck out of here

namespace OpenGL::Renderer {

    IndirectBuffer& GetIndirectBuffer();  // TODO: Make me an SSBO and get me the fuck out of here

    void Init();
    void InitMain();
    void CleanUp();

	void RenderLoadingScreen();
    void RenderBlackFrame();

    void PreGameLogicComputePasses();
    void RenderGame();

	void RenderGameREStyle();

    // Create
    void CreateFramebuffers();
    void CreateShaders();
    void CreateSSBOs();
    void CreateTextureArrays();

    void InitSSBOs();
    void UpdateSSBOS();

    void ParticlePass();

    // Compute passes
    void BlitRoads();
    //void ComputeLightVolumeMask();
    //void ComputeProbeLighting();
    void ComputeOceanFFTPass();
    void ComputeSkinningPass();
    void UpdateTerrainDisplacementBuffer();
    void ComputeTileWorldBounds();
    void OceanHeightReadback();
    void ComputeViewspaceDepth();

    // Init passes
    void InitGrass();
    void InitOceanHeightReadback();

    // Render passes
    void ChristmasLightCullingPass();
    void DebugPass();
    void DebugViewPass();
    void DecalPaintingPass();
    void DownSampleFinalImage();
    void EditorPass();
    void EmissivePass();
    void FirePass();
    void FurPass();
    void GeometryPass();
    void MetaBallsPass();
    void MirrorGeometryPass();
    void GlassPass();
    void GrassPass();
    void HairPass();
    void HeightMapBrushPreviewPass();
    void HeightMapPass();
    void OcclusionHiZPass();
    void ProceduralGeometryPass();
    void InventoryGaussianPass();
    void LightCullingPass();
    void LightingPass();
    void OceanGeometryPass();
    void OceanUnderWaterFlags();
    void OceanSurfaceCompositePass();
    void OceanUnderwaterCompositePass();
    void OutlinePass();
    void PostProcessingPass();
    void PlasticPass();
    void WinstonPass();
    void BloodDecalsPass();
    void SkyBoxPass();
    void SpriteSheetPass();
    void ScreenspaceReflectionsPass();
    void GameUIPass();
    void EditorUIPass();
    void VatBloodPass();
    void ChristmasLightsPass();
    void ExamineItemPass();
    void DepthPeeledTransparencyPass();
    void VATPass();

    void ComputeLightAABBs();
    void ReserveLightAABBSSBOStorage();
    void DebugDrawLightAABBs();

    void OceanUnderwaterBlurPass();


    // Requiem functions
    void VisibilityPass();
    void VisibilityHeightMapPass();
    void VisibilityAlphaDiscardPass();
    void VisibilitySkinnedPass();
    void VisibilitySkinnedHairPass();

    void MaterialResolvePass();
    void MaterialResolveHeightMapPass();
    void MaterialResolveSkinnedPass();
    void MaterialResolveProceduralPass();

    void HairPassRE();

    void PostProcessingPassRE();

    void BindShadowMapsRE();
    void RenderFullscreenTriangle();
    void PresentFinalImage(OpenGLFrameBuffer& presentFbo);


    // Debug passes
    void RaytracedSceneDebug();
    void DrawPointCloud(Unloved::DDGIVolume& ddgiVolume);
    void DrawPointCloudGrid(Unloved::DDGIVolume& ddgiVolume);
    void DrawProbes(Unloved::DDGIVolume& ddgiVolume);
    void DrawGPUBvhSceneNodes(Unloved::DDGIVolume& volume, const glm::vec4& color);
    void DrawGPUBvhSceneLeafNodes(Unloved::DDGIVolume& volume, const glm::vec4& color);
    void DrawRaytracingBvh(Unloved::DDGIVolume& volume);
    void PhysicsShapesPass();

    // Global illumination
    void UpdateGlobalIllumintation();

    // Utility passes
    void UploadWorldHeightData();
    void UploadTerrainControlData(const Unloved::MapData* mapData, int32_t minimumX, int32_t minimumZ, int32_t maximumX, int32_t maximumZ);
    void RecalculateAllHeightMapData(bool uploadWorldHeightData, bool updatePhysics);
    void ClearAllWoundMasks();

    // Render tasks
    void RenderShadowMaps();

    void InitFog();
    void BlitFog();
    void RayMarchFog();

    // Debug
    void DebugBlitFrameBufferTexture(const std::string& frameBufferName, const std::string& attachmentName, GLint dstX, GLint dstY, GLint width, GLint height);
    void DebugBlitOpenGLTexture(GLuint textureHandle, float scale);
    void BlitDebugTextures();

    void DrawItemExamineLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color);
    void DrawItemExamineAABB(const AABB& aabb, const glm::vec4& color);


    void DrawFullscreenTriangle();

    void BindEmptyVAO();
    GLuint GetTextureHandleByName(const std::string& name);

    std::vector<float>& GetShadowCascadeLevels();
    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);
    void MultiDrawPatchesPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState);

    // Misc
    void CreateGrassGeometry();

    void DebugHack(const std::string& message);

    // Drawing
    void MultiDrawIndirect(const std::vector<DrawIndexedIndirectCommand>& commands);
    void MultiDrawIndirectPatches(const std::vector<DrawIndexedIndirectCommand>& commands);

    // Util
    void SetViewport(OpenGLFrameBuffer* framebuffer, Unloved::Viewport* viewport);
    void ClearFrameBufferByViewport(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLfloat r, GLfloat g = 0.0f, GLfloat b = 0.0f, GLfloat a = 0.0f);
    void ClearFrameBufferByViewportInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLint r, GLint g = 0.0f, GLint b = 0.0f, GLint a = 0.0f);
    void ClearFrameBufferByViewportUInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLuint r, GLuint g = 0.0f, GLuint b = 0.0f, GLuint a = 0.0f);
    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const Unloved::Viewport* viewport);

    BlitRect BlitRectFromFrameBufferViewport(OpenGLFrameBuffer* framebuffer, Unloved::Viewport* viewport);
    GLint CreateQuadVAO();
    void GaussianBlur(OpenGLFrameBuffer& srcFrameBuffer, OpenGLFrameBuffer& dstFrameBuffer, const std::string& srcAttachmentName, const std::string& dstAttachmentName, BlitRect srcRect, BlitRect dstRect, int blurRadius, int passCount);

	uint32_t GetTileCount();
	uint32_t GetTileCountX();
	uint32_t GetTileCountY();

    // TIDY ME
    inline bool g_flipNormalMapY = false;
    inline void FlipNormalMapY() {
        g_flipNormalMapY = !g_flipNormalMapY;
    }
    inline bool ShouldFlipNormalMapY() {
        return g_flipNormalMapY;
    }
    // TIDY ME

    // Profiling
    const std::string& GetZoneNames();
    const std::string& GetZoneGPUTimings();
    const std::string& GetZoneCPUTimings();
    const std::string& GetTotalGPUTime();
    const std::string& GetTotalCPUTime();
    float GetTotalGPUTimeFloat();
}
