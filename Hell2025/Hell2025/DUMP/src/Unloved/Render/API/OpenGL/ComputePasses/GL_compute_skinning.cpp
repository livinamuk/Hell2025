#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Hell/Input.h"
#include "Unloved/Debug/Debug.h"

namespace OpenGL::Renderer {

    void ComputeSkinningPass() {
        ProfilerOpenGLZoneFunction();

        const std::vector<SkinningDispatchGroup>& skinningDispatchGroups = Unloved::RenderDataManager::GetSkinningDispatchGroups();
        const std::vector<SkinningJob>& skinningJobs = Unloved::RenderDataManager::GetSkinningJobs();
        const std::vector<SkinningMorphJob>& skinningMorphJobs = Unloved::RenderDataManager::GetSkinningMorphJobs();
        const std::vector<SkinningMorphTarget>& skinningMorphTargets = Unloved::RenderDataManager::GetSkinningMorphTargets();
        const std::vector<glm::mat4>& skinningTransforms = Unloved::RenderDataManager::GetSkinningTransforms();
        const std::vector<glm::mat4>& previousSkinningTransforms = Unloved::RenderDataManager::GetPreviousSkinningTransforms();

        uint32_t totalVertexCount = Unloved::RenderDataManager::GetRequiredSkinnedVertexCount();

        if (skinningDispatchGroups.empty()) return;
        if (skinningJobs.empty()) return;
        if (skinningMorphJobs.size() != skinningJobs.size()) return;
        if (skinningTransforms.empty()) return;
        if (previousSkinningTransforms.size() != skinningTransforms.size()) return;

        // Calculate total amount of vertices to skin and allocate space
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        OpenGLMeshBuffer& glMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("AssetGeometry");

        // Make sure there is enough space allocated on the GPU to store them all
        OpenGL::BackEnd::AllocateSkinnedVertexBufferSpace(totalVertexCount);

        UpdateSSBO("SkinningDispatchGroups", skinningDispatchGroups.size() * sizeof(SkinningDispatchGroup), skinningDispatchGroups.data());
        UpdateSSBO("SkinningJobs", skinningJobs.size() * sizeof(SkinningJob), skinningJobs.data());
        UpdateSSBO("SkinningMorphJobs", skinningMorphJobs.size() * sizeof(SkinningMorphJob), skinningMorphJobs.data());
        if (!skinningMorphTargets.empty()) {
            UpdateSSBO("SkinningMorphTargets", skinningMorphTargets.size() * sizeof(SkinningMorphTarget), skinningMorphTargets.data());
        }
        UpdateSSBO("SkinningTransforms", skinningTransforms.size() * sizeof(glm::mat4), skinningTransforms.data());
        UpdateSSBO("PreviousSkinningTransforms", previousSkinningTransforms.size() * sizeof(glm::mat4), previousSkinningTransforms.data());

        BindSSBO(SSBO_IDX_SKINNING_OUTPUT_VERTICES, BackEnd::GetSkinnedVertexDataVBO());
        BindSSBO(SSBO_IDX_SKINNING_INPUT_VERTICES, glMeshBuffer.GetVBO());
        BindSSBO(SSBO_IDX_SKINNING_ANIMATED_TRANSFORMS, "SkinningTransforms");
        BindSSBO(SSBO_IDX_SKINNING_WEIGHTS, glMeshBuffer.GetVertexWeightSSBO());
        BindSSBO(SSBO_IDX_SKINNING_JOBS, "SkinningJobs");
        BindSSBO(SSBO_IDX_SKINNING_DISPATCH_GROUPS, "SkinningDispatchGroups");
        BindSSBO(SSBO_IDX_SKINNING_PREVIOUS_TRANSFORMS, "PreviousSkinningTransforms");
        BindSSBO(SSBO_IDX_SKINNING_PREVIOUS_POSITIONS, BackEnd::GetPreviousSkinnedPositionBuffer());
        BindSSBO(SSBO_IDX_SKINNING_MORPH_JOBS, "SkinningMorphJobs");
        BindSSBO(SSBO_IDX_SKINNING_MORPH_TARGETS, "SkinningMorphTargets");
        BindSSBO(SSBO_IDX_SKINNING_MORPH_DELTAS, glMeshBuffer.GetMorphDeltaSSBO());

        BindShader("ComputeSkinning");
        DispatchCompute(skinningDispatchGroups.size(), 1, 1);

        glMemoryBarrier(GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
    }
}
