#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/RenderDataManager.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {
    using namespace Unloved;


    void ReserveLightAABBSSBOStorage() {
        uint32_t size = Unloved::World::GetLightCount() * sizeof(glm::vec4) * 2;
        OpenGL::ReserveSSBO("LightAABBs", size);
    }

    void RenderWorldPosition(uint32_t lightIndex);
    void ComputeMinMax(uint32_t lightIndex);

    void DrawHouse(OpenGLShader* shader);
    void DrawHeightMap(OpenGLShader* shader, Light* light);
    void DebugDrawLightAABB(uint32_t lightIndex);

    void ComputeLightAABBs() {
    }

    void RenderWorldPosition(uint32_t lightIndex) {
        ProfilerOpenGLZoneFunctionLightGreen();

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("LightAABBPosition");
        if (!shader) return;

        Light* light = Unloved::World::GetLightByIndex(lightIndex);
        if (!light) return;

        OpenGLCubemapFrameBuffer& fbo = OpenGL::ResourceManager::GetCubemapFrameBuffer("LightAABB");
        fbo.Bind();
        fbo.SetViewport();

        OpenGL::BindShader("LightAABBPosition");
        OpenGL::BindSSBO(SSBO_IDX_LIGHTS, "Lights");

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glEnable(GL_DEPTH_TEST);

        glm::vec3 lightPos = light->GetPosition();
        float radius = light->GetRadius();

        OpenGL::SetUniformInt("u_lightIndex", lightIndex);
        OpenGL::SetUniformFloat("u_lightRadius", radius);
        OpenGL::SetUniformVec3("u_lightPosition", lightPos);

        const glm::vec3 faceDirs[6] = {
            { 1,  0,  0}, // +X
            {-1,  0,  0}, // -X
            { 0,  1,  0}, // +Y
            { 0, -1,  0}, // -Y
            { 0,  0,  1}, // +Z
            { 0,  0, -1}  // -Z
        };

        for (int face = 0; face < 6; ++face) {
            fbo.BindFaceByIndex(face);
            fbo.ClearFaceDepth(1.0f);

            // Clear to the far extent of the light radius for this face
            glm::vec3 farPoint = lightPos + (faceDirs[face] * radius);
            fbo.ClearFaceColor(glm::vec4(farPoint, 1.0f));

            OpenGL::SetUniformInt("u_faceIndex", face);
            OpenGL::SetUniformMat4("u_shadowMatrix", light->GetProjectionView(face));

            DrawHouse(shader);
            DrawHeightMap(shader, light);
        }
        glBindVertexArray(0);
    }

    void DrawHouse(OpenGLShader* shader) {
        Logging::Fatal() << "You called the suss function DrawHouse()\n";
        //OpenGLMeshBuffer& houseMeshBuffer = LegacyWorld::GetHouseMeshBuffer().GetGLMeshBuffer();
        //glBindVertexArray(houseMeshBuffer.GetVAO());
        //
        //shader->SetMat4("u_model", glm::mat4(1.0f));
        //
        //for (const HouseRenderItem& renderItem : Unloved::RenderDataManager::GetHouseRenderItems()) {
        //    glDrawElementsBaseVertex(
        //        GL_TRIANGLES,
        //        renderItem.indexCount,
        //        GL_UNSIGNED_INT,
        //        (void*)(sizeof(unsigned int) * renderItem.baseIndex),
        //        renderItem.baseVertex
        //    );
        //}
        //
    }

    void DrawHeightMap(OpenGLShader* shader, Light* light) {
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        if (chunks.empty()) return;

        Hell::MeshBuffer& heightMapMeshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        Transform transform;
        transform.scale = glm::vec3(HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ);
        glm::mat4 modelMatrix = transform.to_mat4();
        glm::mat4 inverseModelMatrix = glm::inverse(modelMatrix);

        OpenGL::SetUniformMat4("u_model", modelMatrix);

        glBindVertexArray(glHeightMapMeshBuffer.GetVAO());

        for (HeightMapChunk& chunk : chunks) {

            // Skip any chunks that don't intersect the light radius
            AABB chunkAABB(chunk.aabbMin, chunk.aabbMax);
            if (!chunkAABB.IntersectsSphere(light->GetPosition(), light->GetRadius())) {
                continue;
            }

            Mesh* mesh = heightMapMeshBuffer.GetMeshById(chunk.meshId);
            if (!mesh) continue;

            int indexCount = mesh->indexCount;
            int baseVertex = mesh->baseVertex;
            int baseIndex = mesh->baseIndex;
            void* indexOffset = (GLvoid*)(baseIndex * sizeof(GLuint));
            int instanceCount = 1;
            glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indexOffset, instanceCount, baseVertex, 0);
        }
    }

    uint32_t FloatToUint(float f) {
        uint32_t u;
        memcpy(&u, &f, sizeof(float));
        return (u & 0x80000000) ? ~u : u | 0x80000000;
    }

    void ComputeMinMax(uint32_t lightIndex) {
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("LightAABBMinMax");
        OpenGLSSBO* ssbo = OpenGL::ResourceManager::GetSSBOPtr("LightAABBs");
        Light* light = Unloved::World::GetLightByIndex(lightIndex);

        if (!shader) return;
        if (!ssbo) return;
        if (!light) return;

        OpenGLCubemapFrameBuffer& fbo = OpenGL::ResourceManager::GetCubemapFrameBuffer("LightAABB");

        unsigned int minBits = 0xFFFFFFFF;
        unsigned int maxBits = 0;

        // Reset with flipped bits
        struct { uint32_t x, y, z, w; } minU, maxU;
        minU.x = maxU.x = FloatToUint(light->GetPosition().x);
        minU.y = maxU.y = FloatToUint(light->GetPosition().y);
        minU.z = maxU.z = FloatToUint(light->GetPosition().z);
        minU.w = maxU.w = 0;

        size_t baseOffset = lightIndex * sizeof(glm::vec4) * 2;
        glNamedBufferSubData(ssbo->GetHandle(), baseOffset, sizeof(minU), &minU);
        glNamedBufferSubData(ssbo->GetHandle(), baseOffset + sizeof(glm::vec4), sizeof(maxU), &maxU);

        OpenGL::BindShader("LightAABBMinMax");
        OpenGL::SetUniformInt("u_lightIndex", lightIndex);
        OpenGL::SetUniformInt("u_resolution", fbo.GetSize());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, fbo.GetColorHandle());
        OpenGL::SetUniformInt("u_WorldPosCubemap", 0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_LIGHT_AABB_OUTPUT, ssbo->GetHandle());

        uint32_t numGroups = fbo.GetSize() / 16;
        //OpenGL::DispatchCompute(numGroups, numGroups, 1);
        OpenGL::DispatchCompute(1, 1, 1);

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

	void DebugDrawLightAABB(uint32_t lightIndex) {
		static GLuint vao = 0;
		if (vao == 0) {
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
		gBuffer->Bind();
		gBuffer->DrawBuffer("Lighting");

		OpenGL::BindShader("DebugLightAABB");

		OpenGL::BindSSBO(SSBO_IDX_LIGHT_AABB_DEBUG_LIGHTS, "Lights");
		OpenGL::BindSSBO(SSBO_IDX_LIGHT_AABB_DEBUG_BOUNDS, "LightAABBs");

		glBindVertexArray(vao);

		for (int i = 0; i < 4; i++) {
			Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
			if (viewport->IsVisible()) {
				OpenGL::Renderer::SetViewport(gBuffer, viewport);
				OpenGL::SetUniformMat4("u_projectionView", Unloved::RenderDataManager::GetViewportData()[i].projectionView);
                OpenGL::SetUniformInt("u_lightIndex", lightIndex);
                glDrawArrays(GL_LINE_STRIP, 0, 16);
			}
		}

        DebugDraw::DrawPoint(Unloved::World::GetLightByIndex(lightIndex)->GetPosition(), YELLOW);
	}
}
