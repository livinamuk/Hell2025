#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "World/LegacyWorld.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Viewport/ViewportManager.h"


namespace OpenGL::Renderer {


	void PlasticPass() {
		ProfilerOpenGLZoneFunction();

		const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
		const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
		const std::vector<uint32_t>& renderItemIndices = Unloved::RenderDataManager::GetRenderItemIndicesPlastic();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* miscFullSizeFbo = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize");
		OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("Plastic");
        OpenGLShadowCubeMapArray* hiResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("HiRes");
        OpenGLShadowCubeMapArray* lowResShadowMaps = OpenGL::ResourceManager::GetShadowCubeMapArrayPtr("LowRes");

        if (!gBuffer) return;
        if (!shader) return;
        if (!hiResShadowMaps) return;
        if (!lowResShadowMaps) return;

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		OpenGL::BlitFrameBuffer(gBuffer, miscFullSizeFbo, "Lighting", "FinalLightingCopy", GL_COLOR_BUFFER_BIT, GL_NEAREST);

		gBuffer->Bind();
		gBuffer->DrawBuffers({ "Lighting" });

		OpenGL::BindShader("Plastic");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
        OpenGL::SetUniformInt("u_tileXCount", GetTileCountX());

        OpenGL::BindSSBO(SSBO_IDX_LIGHTING_TILE_LIGHTS, "TileLights");

		OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

		glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());

		// Fill the death butter
		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGL::Renderer::SetViewport(gBuffer, viewport);

			OpenGL::BindShader("Plastic");
			OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
			OpenGL::SetUniformMat4("u_view", viewportData[i].view);

			glDepthFunc(GL_GREATER);

			for (uint32_t renderItemIndex : renderItemIndices) {
				const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
				Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
				if (!mesh) continue;

				OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
				glDrawElementsBaseVertex(GL_TRIANGLES,  mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}

		// Bind plastic material
		// It'd be great if you didn't have to blend in these hacky plastic material properties
		// and could derive the result you want directly from the source material
		Material* material = Hell::ResourceManager::GetMaterialByName("Plastic");
		glActiveTexture(GL_TEXTURE7);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE8);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
		glActiveTexture(GL_TEXTURE9);
		glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());

		glActiveTexture(GL_TEXTURE10);
		glBindTexture(GL_TEXTURE_2D, miscFullSizeFbo->GetColorAttachmentHandleByName("FinalLightingCopy"));
		glActiveTexture(GL_TEXTURE11);
		glBindTexture(GL_TEXTURE_2D, gBuffer->GetDepthAttachmentHandle());

        glBindTextureUnit(TEX_IDX_SHADOW_MAP_HI_RES, hiResShadowMaps->GetDepthTexture());
        glBindTextureUnit(TEX_IDX_SHADOW_MAP_LOW_RES, lowResShadowMaps->GetDepthTexture());

		// Now render color
		glEnable(GL_DEPTH_TEST);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGL::Renderer::SetViewport(gBuffer, viewport);

			OpenGL::BindShader("Plastic");
            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);
            OpenGL::SetUniformVec3("u_viewPos", viewportData[i].viewPos);

			glDepthFunc(GL_EQUAL);

			for (uint32_t renderItemIndex : renderItemIndices) {
				const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
				Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
				if (!mesh) continue;

				Material* material = Hell::ResourceManager::GetMaterialByIndex(renderItem.materialIndex);
				glActiveTexture(GL_TEXTURE4);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE5);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
				glActiveTexture(GL_TEXTURE6);
				glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());

				OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
				OpenGL::SetUniformMat4("u_inverseModel", renderItem.inverseModelMatrix);

				glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
			}
		}


		// Clean up
		glDepthFunc(GL_GREATER);
	}

}

