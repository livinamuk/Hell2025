#include "../GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "Renderer/RenderDataManager.h"
#include "Core/Game.h" // remove me when u can
#include "World/World.h" // remove me when u can
#include "Util.h"

// TODO
struct SkinningCommand {
    uint32_t vertexCount;
    uint32_t baseInputVertex;
    uint32_t baseOutputVertex;
    uint32_t baseTransformIndex;
};

namespace OpenGLRenderer {

    void ComputeSkinningPass() {
        ProfilerOpenGLZoneFunction();

        OpenGLShader* shader = GetShader("ComputeSkinning");
        OpenGLSSBO* skinningTransformsSSBO = GetSSBO("SkinningTransforms");

        if (!shader) return;
        if (!skinningTransformsSSBO) return;

        // Calculate total amount of vertices to skin and allocate space
        uint32_t totalVertexCount = 0;
        for (const RenderItem& renderItem : RenderDataManager::GetSkinnedRenderItems()) {
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(renderItem.meshIndex);
            if (!mesh) continue;

            totalVertexCount += mesh->vertexCount;
        }

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGLBackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        // Skin
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, OpenGLBackEnd::GetSkinnedVertexDataVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, OpenGLBackEnd::GetWeightedVertexDataVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skinningTransformsSSBO->GetHandle());
        
        shader->Bind();

        const std::vector<glm::mat4>& skinningTransforms = RenderDataManager::GetSkinningTransforms();
        const void* skinningTransformData = skinningTransforms.empty() ? nullptr : skinningTransforms.data();
        skinningTransformsSSBO->Update(skinningTransforms.size() * sizeof(glm::mat4), skinningTransformData);

        for (const RenderItem& renderItem : RenderDataManager::GetSkinnedRenderItems()) {
            uint32_t meshIndex = renderItem.meshIndex;
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
            if (!mesh || mesh->vertexCount == 0) {
                continue;
            }

            const int32_t baseTransformIndex = renderItem.baseSkinningTransformIndex;
            if (baseTransformIndex < 0 || static_cast<size_t>(baseTransformIndex) >= skinningTransforms.size()) {
                continue;
            }

            uint64_t objectId = 0;
            Util::UnpackUint64(renderItem.objectIdLowerBit, renderItem.objectIdUpperBit, objectId);
            AnimatedGameObject* animatedGameObject = World::GetAnimatedGameObjectByObjectId(objectId);
            if (!animatedGameObject) {
                continue;
            }

            const size_t boneCount = animatedGameObject->GetBoneSkinningMatrices().size();
            if (boneCount == 0 || static_cast<size_t>(baseTransformIndex) + boneCount > skinningTransforms.size()) {
                continue;
            }

            shader->SetInt("vertexCount", mesh->vertexCount);
            shader->SetInt("baseInputVertex", mesh->baseVertexGlobal);
            shader->SetInt("baseOutputVertex", renderItem.baseSkinnedVertex);
            shader->SetInt("baseTransformIndex", baseTransformIndex);

            GLuint workgroupSize = 128;
            GLuint groupCountX = (mesh->vertexCount + workgroupSize - 1) / workgroupSize;
            glDispatchCompute(groupCountX, 1, 1);
        }

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
