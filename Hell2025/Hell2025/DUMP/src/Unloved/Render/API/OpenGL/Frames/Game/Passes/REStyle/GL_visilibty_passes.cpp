#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

namespace OpenGL::Renderer {

    void VisibilityPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLMeshBuffer& meshBufferProcedural = OpenGL::ResourceManager::GetMeshBuffer("Procedural");
        OpenGLMeshBuffer& meshBufferAssets = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("Visibility");

        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_VERTEX_BUFFER_PROCEDURAL | STENCIL_BIT_WORLD_LIGHTING;

        glBindVertexArray(meshBufferProcedural.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.procedural, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_WORLD_LIGHTING;

        glBindVertexArray(meshBufferAssets.GetVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.standard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingStandard, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_VIEW_WEAPON_LIGHTING;
        MultiDrawPerViewportRE(fbo, drawInfoSet.viewWeaponStandard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingViewWeaponStandard, state);
    }

    void VisibilityHeightMapPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("VisibilityHeightMap");

        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGLFrameBuffer& worldFbo = OpenGL::ResourceManager::GetFrameBuffer("World");
        Hell::TextureArray* displacementBuffer = Hell::ResourceManager::GetTextureArrayPtr("TerrainDisplacement");
        OpenGL::BindTextureUnit(5, worldFbo.GetColorAttachmentHandleByName("HeightMap"));
        if (displacementBuffer) OpenGL::BindTextureUnit(6, displacementBuffer->GetHandle());

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilRef = STENCIL_VERTEX_BUFFER_HEIGHT_MAP | STENCIL_BIT_WORLD_LIGHTING;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        glBindVertexArray(meshBuffer.GetVAO());
        MultiDrawPatchesPerViewportRE(fbo, drawInfoSet.heightMap, state);
    }

    void VisibilityAlphaDiscardPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        static int frameCount = 0;
        frameCount++;

        OpenGL::BindShader("VisibilityAlphaDiscard");
        OpenGL::SetUniformUInt("u_frameCount", frameCount);

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_WORLD_LIGHTING;

        glBindVertexArray(meshBuffer.GetVAO());

        MultiDrawPerViewportRE(fbo, drawInfoSet.alphaDiscard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingAlphaDiscard, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_VIEW_WEAPON_LIGHTING;
        MultiDrawPerViewportRE(fbo, drawInfoSet.viewWeaponAlphaDiscard, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingViewWeaponAlphaDiscard, state);

        // Hair
        state.stencilRef = STENCIL_VERTEX_BUFFER_ASSET | STENCIL_BIT_HAIR;
        OpenGL::SetUniformBool("u_depthOffset", true);
        MultiDrawPerViewportRE(fbo, drawInfoSet.hair, state);
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedNonDeformingHair, state);
        OpenGL::SetUniformBool("u_depthOffset", false);
    }

    void VisibilitySkinnedPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("Visibility");

        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = true;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_WORLD_LIGHTING;

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedStandard, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_VIEW_WEAPON_LIGHTING;
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedViewWeaponStandard, state);
    }

    void VisibilitySkinnedHairPass() {
        ProfilerOpenGLZoneFunction();
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        OpenGLFrameBuffer& fbo = OpenGL::ResourceManager::GetFrameBuffer("GBuffer");
        fbo.Bind();
        fbo.DrawBuffers({ "Visibility" });

        OpenGL::BindShader("VisibilityAlphaDiscard");

        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");

        OpenGLRasterizerState state;
        state.depthTestEnabled = true;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        state.depthMask = true;
        state.colorMask = true;
        state.depthFunc = GL_GREATER;

        state.stencilTestEnabled = true;
        state.stencilFunc = GL_ALWAYS;
        state.stencilReadMask = 0xFF;
        state.stencilWriteMask = 0xFF;
        state.stencilFailOp = GL_KEEP;
        state.stencilDepthFailOp = GL_KEEP;
        state.stencilPassOp = GL_REPLACE;

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_WORLD_LIGHTING;

        glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedAlphaDiscard, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_VIEW_WEAPON_LIGHTING;
        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedViewWeaponAlphaDiscard, state);

        state.stencilRef = STENCIL_VERTEX_BUFFER_SKINNED | STENCIL_BIT_HAIR;

        OpenGL::SetUniformBool("u_depthOffset", true);

        MultiDrawPerViewportRE(fbo, drawInfoSet.skinnedHair, state);

        OpenGL::SetUniformBool("u_depthOffset", false);
    }
}
