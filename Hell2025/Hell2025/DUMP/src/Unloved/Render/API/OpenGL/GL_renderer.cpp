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
#include "Unloved/Render/RendererConstants.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/UI/TextBlitter.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Legacy/Timer.hpp"

#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/Systems/ShadowMaps/ShadowMapManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Render/API/OpenGL/Types/GL_texture_readback.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <unordered_map>




namespace OpenGL::Renderer {
    using namespace Unloved;

    std::vector<float> g_shadowCascadeLevels{ 5.0f, 10.0f, 20.0f, 40.0f };
    const glm::vec3 g_lightDir = glm::normalize(glm::vec3(20.0f, 50, 20.0f));
    unsigned int g_lightFBO;
    unsigned int g_lightDepthMaps;
    constexpr unsigned int g_depthMapResolution = 4096;

    GLuint g_emptyVao = 0;
    std::unordered_map<std::string, GLuint> g_cachedTextureHandles;

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
        OpenGL::ResourceManager::GetShadowMapById(flashlightShadowMapsId) = OpenGLShadowMap("FlashlightShadowMaps", FLASHLIGHT_SHADOWMAP_SIZE, FLASHLIGHT_SHADOWMAP_SIZE, MAX_SHADOWED_SPOT_LIGHTS);

        CreateFramebuffers();
        CreateSSBOs();
        CreateTextureArrays();
        CreateShaders();

        InitSSBOs();

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

        OpenGLRasterizerState* grassPassRE = OpenGL::RasterizerStateManager::CreateRasterizerState("GrassPass_RE");
        grassPassRE->depthTestEnabled = true;
        grassPassRE->blendEnable = false;
        grassPassRE->cullfaceEnable = true;
        grassPassRE->depthMask = true;
        grassPassRE->depthFunc = GL_GREATER;
        grassPassRE->stencilTestEnabled = true;
        grassPassRE->stencilFunc = GL_ALWAYS;
        grassPassRE->stencilRef = STENCIL_BIT_GRASS | STENCIL_BIT_WORLD_LIGHTING;
        grassPassRE->stencilReadMask = 0xFF;
        grassPassRE->stencilWriteMask = 0xFF;
        grassPassRE->stencilFailOp = GL_KEEP;
        grassPassRE->stencilDepthFailOp = GL_KEEP;
        grassPassRE->stencilPassOp = GL_REPLACE;

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

        OpenGLRasterizerState* skybox = OpenGL::RasterizerStateManager::CreateRasterizerState("SkyBox");
        skybox->depthTestEnabled = false;
        skybox->blendEnable = false;
        skybox->cullfaceEnable = false;
        skybox->depthMask = false;
        skybox->depthFunc = GL_GREATER;

        // Allocate shadow map array memory
        OpenGLShadowCubeMapArray& hiResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("HiRes");
        hiResShadowMapArray.Init(ShadowMapManager::GetShadowMapHiResMaxCount(), ShadowMapManager::GetShadowMapHiResResolution());

        OpenGLShadowCubeMapArray& lowResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("LowRes");
        lowResShadowMapArray.Init(ShadowMapManager::GetShadowMapLowResMaxCount(), ShadowMapManager::GetShadowMapLowResResolution());

        OpenGLShadowCubeMapArray& staticHiResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("StaticHiRes");
        staticHiResShadowMapArray.Init(ShadowMapManager::GetShadowMapHiResMaxCount(), ShadowMapManager::GetShadowMapHiResResolution());

        OpenGLShadowCubeMapArray& staticLowResShadowMapArray = OpenGL::ResourceManager::CreateShadowCubeMapArray("StaticLowRes");
        staticLowResShadowMapArray.Init(ShadowMapManager::GetShadowMapLowResMaxCount(), ShadowMapManager::GetShadowMapLowResResolution());

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

        // Upload materials
        std::vector<Material>& materials = Hell::ResourceManager::GetMaterials();
        OpenGL::UpdateSSBO("Materials", materials.size() * sizeof(Material), materials.data());
    }


    void InitSSBOs() {
        //DispatchIndirectCommand command = { 1, 1, 1 };
        //OpenGL::UpdateSSBO("ProbeDispatchArgs", sizeof(DispatchIndirectCommand), &command);

        // HO
        for (int i = 0; i < Ocean::FFT_BAND_COUNT; i++) {
            const std::vector<std::complex<float>>& h0 = Ocean::GetH0(i);
            if (OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("ffth0Band" + std::to_string(i))) {
                ssbo->CopyFrom(h0.data(), sizeof(std::complex<float>) * h0.size());
                Ocean::MarkH0Uploaded(i);
            }
        }

    }

    void PreGameLogicComputePasses() {
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

    void MultiDrawIndirectPatches(const std::vector<DrawIndexedIndirectCommand>& commands) {
        if (commands.size()) {
            g_indirectBuffer.Bind();
            g_indirectBuffer.Update(sizeof(DrawIndexedIndirectCommand) * commands.size(), commands.data());
            glPatchParameteri(GL_PATCH_VERTICES, 3);
            glMultiDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, (GLvoid*)0, (GLsizei)commands.size(), 0);
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
        glViewport(0, 0, Hell::BackEnd::GetDrawableWidth(), Hell::BackEnd::GetDrawableHeight());
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

    void BindEmptyVAO() {
        if (g_emptyVao == 0) glGenVertexArrays(1, &g_emptyVao);
        glBindVertexArray(g_emptyVao);
    }

    void MultiDrawPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
        OpenGL::RasterizerStateManager::SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(&fbo, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirect(drawCommands[i]);
            }
        }
    }

    void MultiDrawPatchesPerViewportRE(OpenGLFrameBuffer& fbo, const std::vector<DrawIndexedIndirectCommand> drawCommands[4], OpenGLRasterizerState& rasterizerState) {
        OpenGL::RasterizerStateManager::SetRasterizerState(rasterizerState);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (viewport->IsVisible()) {
                OpenGL::Renderer::SetViewport(&fbo, viewport);
                OpenGL::SetUniformInt("u_viewportIndex", i);
                MultiDrawIndirectPatches(drawCommands[i]);
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

    void CleanUp() {
        if (g_emptyVao != 0) {
            glDeleteVertexArrays(1, &g_emptyVao);
            g_emptyVao = 0;
        }
    }

    std::vector<float>& GetShadowCascadeLevels() {
        return g_shadowCascadeLevels;
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

    float GetTotalGPUTimeFloat() {
        return ProfilerOpenGLTotalGPUFloat();
    }

    uint32_t GetTileCount() { return Unloved::Renderer::GetTileCount(); }
	uint32_t GetTileCountX() { return Unloved::Renderer::GetTileCountX(); }
	uint32_t GetTileCountY() { return Unloved::Renderer::GetTileCountY(); }
}
