#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Logging.h"


// remove me
#include "Unloved/Session/Session.h"
#include "Hell/Physics/Physics.h"
#include "World/LegacyWorld.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <execution>
// remove me

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Input.h"
namespace Input = Hell::Input;

/*
"DepthPeeledTransparency"] = OpenGLFrameBuffer("DepthPeeledTransparency", resolutions.gBuffer);
"DepthPeeledTransparency"].CreateAttachment("Color", GL_RGBA8);
"DepthPeeledTransparency"].CreateAttachment("ViewspaceDepthPrevious", GL_R32F);
"DepthPeeledTransparency"].CreateAttachment("Composite", GL_R32F);
"DepthPeeledTransparency"].CreateAttachment("Composite", GL_RGBA8);
"DecalPainting"].CreateDepthAttachment(GL_DEPTH32F_STENCIL8);
*/

namespace OpenGL::Renderer {

	void P90MagColor();
	void P90MagComposite();


	void DepthPeeledTransparencyPass() {
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		P90MagColor();
	}

	void P90MagColor() {
		ProfilerOpenGLZoneFunction();

		const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
		const std::vector<RenderItem>& renderItems = Unloved::RenderDataManager::GetNonDeformingSkinnedMeshRenderItemsDepthPeeledTransparent();

		OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
		OpenGLFrameBuffer* depthPeeledTransparencyFbo = OpenGL::ResourceManager::GetFrameBufferPtr("DepthPeeledTransparency");
		OpenGLFrameBuffer* miscFullSizeFbo = OpenGL::ResourceManager::GetFrameBufferPtr("MiscFullSize"); // Has gbuffer viewspace depth in here
		OpenGLShader* depthPeelDepthShader = OpenGL::ResourceManager::GetShaderPtr("DepthPeeledTransparencyDepth");
		OpenGLShader* depthPeelColorShader = OpenGL::ResourceManager::GetShaderPtr("DepthPeeledTransparencyColor");

		if (!gBuffer) return;
		if (!depthPeeledTransparencyFbo) return;
		if (!miscFullSizeFbo) return;
		if (!depthPeelDepthShader) return;
		if (!depthPeelColorShader) return;

		// Begin by copying the gbuffer viewspace depth into the depth peeling fbo.
		OpenGL::BlitFrameBuffer(miscFullSizeFbo, depthPeeledTransparencyFbo, "ViewspaceDepth", "ViewspaceDepthPrevious", GL_COLOR_BUFFER_BIT, GL_NEAREST);

		depthPeeledTransparencyFbo->Bind();
		depthPeeledTransparencyFbo->ClearAttachment("Composite", 0.0f, 0.0f, 0.0f, 0.0f);
		depthPeeledTransparencyFbo->DrawBuffers({ "Composite", });


		OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");


        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");
        glBindVertexArray(meshBuffer.GetVAO());
        glBindBuffer(GL_ARRAY_BUFFER, meshBuffer.GetVBO());
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshBuffer.GetEBO());

		//glBindVertexArray(OpenGL::BackEnd::GetWeightedVertexDataVAO());
		//glBindBuffer(GL_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataVBO());
		//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, OpenGL::BackEnd::GetWeightedVertexDataEBO());

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (!viewport->IsVisible()) continue;

			OpenGL::Renderer::SetViewport(gBuffer, viewport);

			OpenGL::BindShader("DepthPeeledTransparencyDepth");
			glm::mat4 jitterMatrix = viewportData[i].jitteredProjectionViewReverseZ * viewportData[i].inverseProjectionViewReverseZ;
			OpenGL::SetUniformMat4("u_projectionView", jitterMatrix * viewportData[i].projectionView);
			OpenGL::SetUniformMat4("u_view", viewportData[i].view);

			OpenGL::BindShader("DepthPeeledTransparencyColor");
            OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
			OpenGL::SetUniformMat4("u_projectionView", jitterMatrix * viewportData[i].projectionView);
			OpenGL::SetUniformMat4("u_view", viewportData[i].view);

			// PEEL
			static int peelCount = 1;
			int maxPeelCount = 10;

			//glDisable(GL_CULL_FACE);

			if (Input::KeyPressed(HELL_KEY_LEFT)) {
				peelCount--;
				peelCount = std::clamp(peelCount, 1, maxPeelCount);
				std::cout << "peelCount: " << peelCount << "\n";
			}
			if (Input::KeyPressed(HELL_KEY_RIGHT)) {
				peelCount++;
				peelCount = std::clamp(peelCount, 1, maxPeelCount);
				std::cout << "peelCount: " << peelCount << "\n";
			}

			for (int j = 0; j < peelCount; j++) {

				// Fill the depth buffer of this peel layer
				{
					OpenGL::BlitFrameBufferDepth(gBuffer, depthPeeledTransparencyFbo);

					depthPeeledTransparencyFbo->Bind();
					depthPeeledTransparencyFbo->ClearAttachmentR("ViewspaceDepth", 0.0f);
					depthPeeledTransparencyFbo->DrawBuffers({ "ViewspaceDepth" });

					OpenGL::BindShader("DepthPeeledTransparencyDepth");
					OpenGL::BindImageTexture(0, depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("ViewspaceDepthPrevious"), GL_READ_ONLY, GL_R32F);

					glDepthFunc(GL_LESS);

					for (const RenderItem& renderItem : renderItems) {
						Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
						if (!mesh) continue;

						OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
						glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);

					}
				}
				// Re-render GL_EQUAL against the depth buffer
				{
					glDepthFunc(GL_EQUAL);


					depthPeeledTransparencyFbo->Bind();
					depthPeeledTransparencyFbo->ClearAttachment("Color", 0.0f, 0.0f, 0.0f, 0.0f);
					depthPeeledTransparencyFbo->DrawBuffers({ "Color", "ViewspaceDepthPrevious"});


					OpenGL::BindShader("DepthPeeledTransparencyColor");
                    OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");
					OpenGL::BindImageTexture(4, depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("ViewspaceDepth"), GL_READ_ONLY, GL_R32F);

					Material* material = Hell::ResourceManager::GetMaterialByName("Plastic");
					glActiveTexture(GL_TEXTURE3);
					glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor)->GetGLTexture().GetHandle());
					glActiveTexture(GL_TEXTURE4);
					glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_normal)->GetGLTexture().GetHandle());
					glActiveTexture(GL_TEXTURE5);
					glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(material->m_rma)->GetGLTexture().GetHandle());


					OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
					glActiveTexture(GL_TEXTURE6);
					glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentHandleByName("Lighting"));
					glActiveTexture(GL_TEXTURE7);
					glBindTexture(GL_TEXTURE_2D, gBuffer->GetDepthAttachmentHandle());

					for (const RenderItem& renderItem : renderItems) {
						Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
						if (!mesh) continue;

						Material* renderItemMaterial = Hell::ResourceManager::GetMaterialByIndex(renderItem.materialIndex);
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItemMaterial->m_basecolor)->GetGLTexture().GetHandle());
						glActiveTexture(GL_TEXTURE1);
						glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItemMaterial->m_normal)->GetGLTexture().GetHandle());
						glActiveTexture(GL_TEXTURE2);
						glBindTexture(GL_TEXTURE_2D, Hell::ResourceManager::GetTextureByBindlessIndex(renderItemMaterial->m_rma)->GetGLTexture().GetHandle());

						OpenGL::SetUniformMat4("u_model", renderItem.modelMatrix);
						OpenGL::SetUniformMat4("u_inverseModel", renderItem.inverseModelMatrix);
						glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);

					}
				}

				P90MagComposite();
			}
		}
	}

	void
		P90MagComposite() {

		OpenGLFrameBuffer* depthPeeledTransparencyFbo = OpenGL::ResourceManager::GetFrameBufferPtr("DepthPeeledTransparency");
		OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
		OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("DepthPeeledTransparencyComposite");

		if (!depthPeeledTransparencyFbo) return;
		if (!gBuffer) return;
		if (!shader) return;

		OpenGL::BindShader("DepthPeeledTransparencyComposite");
		OpenGL::BindImageTexture(0, gBuffer->GetColorAttachmentHandleByName("Lighting"), GL_READ_WRITE, GL_RGBA16F);
		OpenGL::BindImageTexture(1 , depthPeeledTransparencyFbo->GetColorAttachmentHandleByName("Color"), GL_READ_ONLY, GL_RGBA16F);

		int width = gBuffer->GetWidth();
		int height = gBuffer->GetHeight();

		OpenGL::DispatchCompute((width + 15) / 16, (height + 15) / 16, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}

