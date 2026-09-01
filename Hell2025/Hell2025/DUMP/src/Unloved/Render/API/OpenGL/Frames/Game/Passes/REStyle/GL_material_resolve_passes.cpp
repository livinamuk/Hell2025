#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Render/RendererConstants.h"

namespace OpenGL::Renderer {

    void MaterialResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", false);
        OpenGL::SetUniformBool("u_woundMaskEnabled", false);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, meshBuffer.GetVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, meshBuffer.GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveHeightMapPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolveHeightMap");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());
        OpenGL::BindTextureUnit(4, OpenGL::ResourceManager::GetFrameBuffer("World").GetColorAttachmentHandleByName("TerrainControl"));
        OpenGL::BindTextureUnit(5, OpenGL::ResourceManager::GetFrameBuffer("World").GetColorAttachmentHandleByName("HeightMap"));

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");

        int32_t fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        if (fallbackMaterialIndex == -1) fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        if (fallbackMaterialIndex == -1) fallbackMaterialIndex = 0;
        int32_t grassMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Grass");
        int32_t dirtRoadMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("DirtRoad");
        int32_t rockFaceMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("RockFace");
        int32_t sandMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Sand");
        OpenGL::SetUniformInt("u_terrainMaterial0", grassMaterialIndex == -1 ? fallbackMaterialIndex : grassMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial1", dirtRoadMaterialIndex == -1 ? fallbackMaterialIndex : dirtRoadMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial2", rockFaceMaterialIndex == -1 ? fallbackMaterialIndex : rockFaceMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial3", sandMaterialIndex == -1 ? fallbackMaterialIndex : sandMaterialIndex);

        float textureScaling = 1.0f;
        if (Unloved::EditorSession::IsHeightMapEditorActive()) {
            textureScaling = 0.1f;
        }
        OpenGL::SetUniformFloat("u_textureScaling", textureScaling);

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_VERTEX_BUFFER_HEIGHT_MAP;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveSkinnedPass() {
        ProfilerOpenGLZoneFunction();

        Hell::TextureArray* woundMaskArray = Hell::ResourceManager::GetTextureArrayPtr("WoundMasks");

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", true);
        OpenGL::SetUniformBool("u_woundMaskEnabled", woundMaskArray != nullptr);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());
        if (woundMaskArray) OpenGL::BindTextureUnit(2, woundMaskArray->GetHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_PREVIOUS_POSITIONS, OpenGL::BackEnd::GetPreviousSkinnedPositionBuffer());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void MaterialResolveProceduralPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLMeshBuffer& proceduralMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("Procedural");

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("MaterialResolve");
        OpenGL::SetUniformBool("u_hasPreviousSkinnedPositions", false);
        OpenGL::SetUniformBool("u_woundMaskEnabled", false);

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, proceduralMeshBuffer.GetVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, proceduralMeshBuffer.GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_VERTEX_BUFFER_PROCEDURAL;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }

    void HairLightingSkinnedResolvePass() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer& gbufferFbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        gbufferFbo.Bind();
        gbufferFbo.SetViewport();
        gbufferFbo.DrawBuffers({ "Lighting", "BaseColorMetallic", "NormalXYRoughnessMisc", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HairLightingResolve");

        OpenGL::BindImageTexture(0, gbufferFbo.GetColorAttachmentHandleByName("Visibility"), GL_READ_ONLY, GL_RG32UI);
        OpenGL::BindTextureUnit(1, gbufferFbo.GetDepthAttachmentHandle());

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_VERTICES, OpenGL::BackEnd::GetSkinnedVertexDataVBO());
        OpenGL::BindSSBO(SSBO_IDX_MATERIAL_RESOLVE_INDICES, OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetEBO());

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = false;
        state.colorMask = true;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_EQUAL;
        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_HAIR;
        state.stencilReadMask = STENCIL_VERTEX_BUFFER_MASK | STENCIL_BIT_HAIR;
        state.stencilWriteMask = 0x00;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_KEEP;

        OpenGL::RasterizerStateManager::SetRasterizerState(state);
        RenderFullscreenTriangle();
    }
}
