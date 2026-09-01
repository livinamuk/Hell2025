#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "World/LegacyWorld.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include <vector>
#include <set>
#include <tuple>
#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>


namespace OpenGL::Renderer {

    std::vector<glm::vec2> GenerateOutlineOffsets(int lineThickness = 1) {
        std::vector<glm::vec2> offsets;
        for (int y = -lineThickness; y <= lineThickness; y++) {
            for (int x = -lineThickness; x <= lineThickness; x++) {
                // Only include the outer perimeter of the square ring
                if (abs(x) == lineThickness || abs(y) == lineThickness) {
                    offsets.emplace_back(x, y);
                }
            }
        }
        //std::cout << "OLD count: " << offsets.size() << "\n";

        // Spherical approach
        offsets.clear();
        float radius = static_cast<float>(lineThickness);
        float quality = 1.0f;

        int numSamples = static_cast<int>(2 * HELL_PI * radius * quality);
        numSamples = std::max(8, numSamples);

        for (int i = 0; i < numSamples; i++) {
            float angle = (2.0f * HELL_PI * i) / static_cast<float>(numSamples);

            float x =(std::round(cos(angle) * radius));
            float y =(std::round(sin(angle) * radius));

            offsets.emplace_back(x, y);
        }

        //std::cout << "NEW count: " << offsets.size() << "\n";

        // Spherical appraoch with removed duplicates
        offsets.clear();
        struct ivec2_less {
            bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
                return std::tie(a.x, a.y) < std::tie(b.x, b.y);
            }
        };
        std::set<glm::ivec2, ivec2_less> unique_integer_offsets;

        for (int i = 0; i < numSamples; i++) {
            float angle = (2.0f * HELL_PI * i) / static_cast<float>(numSamples);

            // Generate points and round them to the nearest integer
            int x = static_cast<int>(std::round(cos(angle) * radius));
            int y = static_cast<int>(std::round(sin(angle) * radius));

            // Insert into the set (duplicates are automatically discarded)
            unique_integer_offsets.insert({ x, y });
        }

        // Create the final vec2 vector for the result
        offsets.reserve(unique_integer_offsets.size());

        // Convert the unique ivec2s from the set back into vec2s
        for (const glm::ivec2& p : unique_integer_offsets) {
            offsets.emplace_back(static_cast<float>(p.x), static_cast<float>(p.y));
        }

        //std::cout << "SUPER count: " << offsets.size() << "\n";
        return offsets;
    }



    void OutlinePass() {
        std::string gBufferName = (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) ? "GBuffer" : "GBuffer";
        OpenGLFrameBuffer& gBuffer = OpenGL::ResourceManager::GetFrameBuffer(gBufferName);

        OpenGLFrameBuffer* outlineFBO = OpenGL::ResourceManager::GetFrameBufferPtr("Outline");
        OpenGLShader* maskShader = OpenGL::ResourceManager::GetShaderPtr("OutlineMask");
        OpenGLShader* outlineShader = OpenGL::ResourceManager::GetShaderPtr("Outline");
        OpenGLShader* compositeShader = OpenGL::ResourceManager::GetShaderPtr("OutlineComposite");

        // Compute offsets given the outline width
        const int outlineWidth = 3;
        static std::vector<glm::vec2> offsets = GenerateOutlineOffsets(outlineWidth);

        //Setup
        //outlineFBO->BindDepthAttachmentFrom(*gBuffer);
        outlineFBO->Bind();
        outlineFBO->ClearAttachmentI("Mask", 0);
        outlineFBO->ClearAttachmentI("Result", 0);

        OpenGLRasterizerState state;
        state.depthTestEnabled = false;
        state.depthMask = false;
        state.blendEnable = false;
        state.cullfaceEnable = false;
        OpenGL::RasterizerStateManager::SetRasterizerState(state);

        const std::vector<RenderItem>& sceneRenderItems = Unloved::RenderDataManager::GetSceneRenderItems();
        const std::vector<uint32_t>& outlineRenderItemIndices = Unloved::RenderDataManager::GetRenderItemIndicesOutline();
        const std::vector<uint32_t>& outlineRenderItemIndicesPhysicsShapes = Unloved::RenderDataManager::GetRenderItemIndicesOutlinePhysicsShapes();
        const std::vector<uint32_t>& outlineRenderItemIndicesProcedural = Unloved::RenderDataManager::GetRenderItemIndicesOutlineProcedural();
        const std::vector<uint32_t>& outlineRenderItemIndicesSkinned = Unloved::RenderDataManager::GetRenderItemIndicesOutlineSkinned();

        // For each viewport
        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(&gBuffer, viewport);

            // Render the mask (by drawing all the mesh into it)
            glDrawBuffer(outlineFBO->GetColorAttachmentSlotByName("Mask"));
            OpenGL::BindShader("OutlineMask");
            OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
            OpenGL::SetUniformInt("u_viewportIndex", i);

            glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
            for (uint32_t renderItemIndex : outlineRenderItemIndices) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;
                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            }

            glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("Procedural").GetVAO());
            for (uint32_t renderItemIndex : outlineRenderItemIndicesProcedural) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("Procedural").GetMeshById(renderItem.meshId);
                if (!mesh) continue;
                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            }

            glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry").GetVAO());
            for (uint32_t renderItemIndex : outlineRenderItemIndicesPhysicsShapes) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("PhysicsShapeGeometry").GetMeshById(renderItem.meshId);
                if (!mesh) continue;
                glDrawElementsBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), mesh->baseVertex);
            }

            glBindVertexArray(OpenGL::BackEnd::GetSkinnedVertexDataVAO());
            for (uint32_t renderItemIndex : outlineRenderItemIndicesSkinned) {
                const RenderItem& renderItem = sceneRenderItems[renderItemIndex];
                OpenGL::SetUniformMat4("u_modelMatrix", renderItem.modelMatrix);
                glDrawElementsBaseVertex(GL_TRIANGLES, renderItem.indexCount, GL_UNSIGNED_INT, (GLvoid*)(renderItem.baseIndex * sizeof(GLuint)), renderItem.baseVertex);
            }

            // Render the outline (by drawing an instanced quad offset many times)
            OpenGL::BindShader("Outline");
            OpenGL::SetUniformVec2Array("u_offsets", offsets);
            int instanceCount = offsets.size();
            Model* primitives = Hell::ResourceManager::GetModelByName("Primitives");
            if (!primitives || primitives->GetMeshIndices().empty()) return;
            if (primitives->GetMeshCount() == 0) return;

            uint32_t meshId = primitives->GetMeshIndices()[0];
            Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
            if (!mesh) return;

            glBindVertexArray(OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry").GetVAO());
            glDrawBuffer(outlineFBO->GetColorAttachmentSlotByName("Result"));
            glBindTextureUnit(1, outlineFBO->GetColorAttachmentHandleByName("Mask"));
            glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), instanceCount, mesh->baseVertex);
        }

        // Composite the outline
        glBindImageTexture(0, gBuffer.GetColorAttachmentHandleByName("Lighting"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);
        glBindImageTexture(1, outlineFBO->GetColorAttachmentHandleByName("Mask"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
        glBindImageTexture(2, outlineFBO->GetColorAttachmentHandleByName("Result"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
        OpenGL::BindShader("OutlineComposite");
        OpenGL::DispatchCompute(gBuffer.GetWidth() / 16, gBuffer.GetHeight() / 16, 1);

        // Clean Up
        glBlendEquation(GL_FUNC_ADD);
        glBindVertexArray(0);
    }
}
