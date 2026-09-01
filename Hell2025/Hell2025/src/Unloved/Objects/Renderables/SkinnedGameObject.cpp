#include "SkinnedGameObject.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/Debug/DebugDraw.h"

#include <iostream>
#include <limits>
#include <cmath>

namespace Unloved {

SkinnedGameObject::SkinnedGameObject(uint64_t id) {
    m_objectId = id;
}

void SkinnedGameObject::UpdateRenderItems() {
    m_animatedMeshNodes.UpdateRenderItems(GetModelMatrix(), GetBoneSkinningMatrices());
}

void SkinnedGameObject::CommitRenderPoseHistory() {
    m_previousRenderBoneSkinningMatrices = GetBoneSkinningMatrices();
    m_previousRenderMorphTargetWeights = GetMorphTargetWeights();
    m_previousRenderModelMatrix = GetModelMatrix();
    m_hasRenderPoseHistory = true;
}

const std::vector<glm::mat4>& SkinnedGameObject::GetBoneSkinningMatrices() {
    if (m_animationMode == AnimationMode::BINDPOSE && m_skinnedModel) {
        return m_skinnedModel->GetBindPoseBoneSkinningMatrices();
    }

    if (m_animationMode == AnimationMode::ANIMATION && m_animatorInstanceId && m_skinnedModel) {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (animatorInstance) {
            return animatorInstance->GetBoneSkinningMatrices(m_skinnedModel->GetSkinnedModelId());
        }
    }

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        return m_ragdollBoneSkinningMatrices;
    }

    if (m_skinnedModel) return m_skinnedModel->GetBindPoseBoneSkinningMatrices();

    static const std::vector<glm::mat4> emptyMatrices;
    return emptyMatrices;
}

const std::map<std::string, float>& SkinnedGameObject::GetMorphTargetWeights() {
    if (m_animationMode == AnimationMode::ANIMATION && m_animatorInstanceId && m_skinnedModel) {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (animatorInstance) {
            return animatorInstance->GetMorphTargetWeights(m_skinnedModel->GetSkinnedModelId());
        }
    }

    static const std::map<std::string, float> emptyWeights;
    return emptyWeights;
}

const uint32_t SkinnedGameObject::GetVerteXCount() {
    if (m_skinnedModel) {
        return m_skinnedModel->GetVertexCount();
    }
    else {
        return 0;
    }
}

void SkinnedGameObject::UpdateBoneTransformsFromRagdoll() {
    Ragdoll* ragdoll = GetRagdoll();
    if (!ragdoll) return;
    if (!m_skinnedModel) return;

    int nodeCount = m_skinnedModel->m_nodes.size();
    m_ragdollWorldNodeTransforms.resize(nodeCount);

    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::string NodeName = m_skinnedModel->m_nodes[i].name;
        glm::mat4 nodeTransformation = glm::mat4(1);
        nodeTransformation = m_skinnedModel->m_nodes[i].localBindTransform;
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_ragdollWorldNodeTransforms[parentIndex];
        glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;

        for (int j = 0; j < ragdoll->m_markerBoneNames.size(); j++) {
            if (ragdoll->m_markerBoneNames[j] == NodeName) {
                PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[j];
                GlobalTransformation = Hell::Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
            }
        }

        m_ragdollWorldNodeTransforms[i] = GlobalTransformation;
    }

    int boneCount = m_skinnedModel->GetBoneCount();
    m_ragdollBoneSkinningMatrices.assign(boneCount, glm::mat4(1.0f));

    for (int boneIndex = 0; boneIndex < boneCount; boneIndex++) {
        int nodeIndex = m_skinnedModel->m_boneNodeIndices[boneIndex];
        if (nodeIndex >= 0 && nodeIndex < int(m_ragdollWorldNodeTransforms.size())) {
            m_ragdollBoneSkinningMatrices[boneIndex] = m_ragdollWorldNodeTransforms[nodeIndex] * m_skinnedModel->m_boneOffsets[boneIndex];
        }
    }
}

void SkinnedGameObject::FinalizeAnimation() {
    if (!m_skinnedModel) return;

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        UpdateBoneTransformsFromRagdoll();
    }

    // If it has a ragdoll then sink the ragdoll rigids to the animated pose
    if (m_animationMode == AnimationMode::BINDPOSE || m_animationMode == AnimationMode::ANIMATION) {
        SyncRagdollToAnimation();
    }

    UpdateDirtyBounds();
}

void SkinnedGameObject::ComputeBoneSegments() {
    m_boneSegments.clear();

    for (int boneIndex = 0; boneIndex < m_skinnedModel->m_boneNodeIndices.size(); boneIndex++) {
        int nodeIndex = m_skinnedModel->m_boneNodeIndices[boneIndex];
        if (nodeIndex < 0 || nodeIndex >= int(m_skinnedModel->m_nodes.size())) {
            continue;
        }

        int parentNodeIndex = m_skinnedModel->m_nodes[nodeIndex].parentIndex;
        int parentBoneNodeIndex = -1;

        // Walk past helper nodes until another skinning bone is found
        while (parentNodeIndex >= 0 && parentNodeIndex < int(m_skinnedModel->m_nodes.size())) {
            const std::string& parentNodeName = m_skinnedModel->m_nodes[parentNodeIndex].name;
            if (m_skinnedModel->BoneExists(parentNodeName)) {
                parentBoneNodeIndex = parentNodeIndex;
                break;
            }

            parentNodeIndex = m_skinnedModel->m_nodes[parentNodeIndex].parentIndex;
        }

        if (parentBoneNodeIndex == -1) {
            continue;
        }

        BoneSegment& segment = m_boneSegments.emplace_back();
        segment.boneName = m_skinnedModel->m_nodes[nodeIndex].name;
        segment.start = GetNodeWorldPosition(m_skinnedModel->m_nodes[nodeIndex].name);
        segment.end = GetNodeWorldPosition(m_skinnedModel->m_nodes[parentBoneNodeIndex].name);
    }
}

void SkinnedGameObject::CalculateSkinnedAABB() {
    glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    // If there is a ragdoll, use each rigid as the body bounds
    if (Ragdoll* ragdoll = GetRagdoll()) {
        for (const AABB& aabb : ragdoll->GetWorldSpaceAABBs()) {
            boundsMin = glm::min(boundsMin, aabb.GetBoundsMin());
            boundsMax = glm::max(boundsMax, aabb.GetBoundsMax());
        }

        boundsMin -= glm::vec3(m_skinnedAABBThreshold);
        boundsMax += glm::vec3(m_skinnedAABBThreshold);
        m_skinnedAABB = AABB(boundsMin, boundsMax);
        return;
    }
    else {
        ComputeBoneSegments();

        // Min/Max of all bone segments
        for (const BoneSegment& boneSegment : m_boneSegments) {
            boundsMin = glm::min(boundsMin, boneSegment.start);
            boundsMax = glm::max(boundsMax, boneSegment.start);
            boundsMin = glm::min(boundsMin, boneSegment.end);
            boundsMax = glm::max(boundsMax, boneSegment.end);
        }

        // Inflate bone bounds
        boundsMin -= glm::vec3(m_skinnedAABBThreshold);
        boundsMax += glm::vec3(m_skinnedAABBThreshold);
    }

    // Add visible non deforming mesh bounds
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    const glm::mat4 modelMatrix = GetModelMatrix();
    const std::vector<glm::mat4>& boneSkinningMatrices = GetBoneSkinningMatrices();

    for (const AnimatedMeshNode& node : m_animatedMeshNodes.GetNodes()) {
        if (node.blendingMode == BlendingMode::DO_NOT_RENDER) {
            continue;
        }

        if (node.deforming) {
            continue;
        }

        Mesh* mesh = meshBuffer.GetMeshById(node.meshId);
        Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(node.meshId);
        if (!mesh || !metadata) {
            continue;
        }

        int boneIndex = metadata->nonDeformingBoneIndex;
        if (boneIndex < 0 || boneIndex >= int(boneSkinningMatrices.size())) {
            continue;
        }

        const glm::mat4 finalWorldMatrix = modelMatrix * boneSkinningMatrices[boneIndex];
        const glm::vec3 localCenter = 0.5f * (mesh->aabbMin + mesh->aabbMax);
        const glm::vec3 localExtents = 0.5f * (mesh->aabbMax - mesh->aabbMin);
        const glm::vec3 worldCenter = glm::vec3(finalWorldMatrix * glm::vec4(localCenter, 1.0f));

        const glm::vec3 col0 = glm::vec3(finalWorldMatrix[0]);
        const glm::vec3 col1 = glm::vec3(finalWorldMatrix[1]);
        const glm::vec3 col2 = glm::vec3(finalWorldMatrix[2]);

        const glm::vec3 worldExtents = glm::abs(col0) * localExtents.x +
                                       glm::abs(col1) * localExtents.y +
                                       glm::abs(col2) * localExtents.z;

        boundsMin = glm::min(boundsMin, worldCenter - worldExtents);
        boundsMax = glm::max(boundsMax, worldCenter + worldExtents);
    }

    m_skinnedAABB = AABB(boundsMin, boundsMax);
}

void SkinnedGameObject::UpdateDirtyBounds() {
    if (!RenderingEnabled()) {
        return;
    }

    if (!CastsShadows()) {
        return;
    }

    Ragdoll* ragdoll = GetRagdoll();

    if (ragdoll) {
        ragdoll->UpdateWorldSpaceAABBs(m_skinnedAABBChangeThreshold);
    }

    CalculateSkinnedAABB();

    const glm::vec3& boundsMin = m_skinnedAABB.GetBoundsMin();
    const glm::vec3& boundsMax = m_skinnedAABB.GetBoundsMax();
    const glm::vec3& boundsMinLastFrame = m_skinnedAABBLastFrame.GetBoundsMin();
    const glm::vec3& boundsMaxLastFrame = m_skinnedAABBLastFrame.GetBoundsMax();

    bool shadowCasterChanged = false;
    bool animationPoseChanged = false;

    if (m_animationMode == AnimationMode::ANIMATION && m_animatorInstanceId) {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (animatorInstance) animationPoseChanged = animatorInstance->PoseChangedThisFrame();
    }

    // Force dirty on the first valid frame
    if (!m_hasSkinnedAABBLastFrame) {
        shadowCasterChanged = true;
    }
    else if (ragdoll && ragdoll->IsDirty()) {
        shadowCasterChanged = true;
    }
    else if (!ragdoll && animationPoseChanged) {
        shadowCasterChanged = true;
    }
    else {
        // Check if the bounds moved enough to matter
        if (std::abs(boundsMin.x - boundsMinLastFrame.x) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMin.y - boundsMinLastFrame.y) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMin.z - boundsMinLastFrame.z) > m_skinnedAABBChangeThreshold ||
                std::abs(boundsMax.x - boundsMaxLastFrame.x) > m_skinnedAABBChangeThreshold ||
                std::abs(boundsMax.y - boundsMaxLastFrame.y) > m_skinnedAABBChangeThreshold ||
                std::abs(boundsMax.z - boundsMaxLastFrame.z) > m_skinnedAABBChangeThreshold) {
            shadowCasterChanged = true;
        }
    }

    if (shadowCasterChanged) {
        DirtyBounds dirtyBounds;
        dirtyBounds.objectId = m_objectId;
        dirtyBounds.boundsMin = boundsMin;
        dirtyBounds.boundsMax = boundsMax;
        dirtyBounds.castShadows = true;

        // Dirty both where the shadow was and where it is now
        if (m_hasSkinnedAABBLastFrame) {
            dirtyBounds.boundsMin = glm::min(boundsMin, boundsMinLastFrame);
            dirtyBounds.boundsMax = glm::max(boundsMax, boundsMaxLastFrame);
        }

        DirtyTracker::AddDirtyBounds(dirtyBounds);
    }

    m_skinnedAABBLastFrame = m_skinnedAABB;
    m_hasSkinnedAABBLastFrame = true;

    //DebugDraw::DrawAABB(GetSkinnedAABB(), shadowCasterChanged ? GREEN : RED);
}

void SkinnedGameObject::SyncRagdollToAnimation() {
    Ragdoll* ragdoll = GetRagdoll();
    if (!ragdoll) return;

    for (int i = 0; i < ragdoll->m_markerBoneNames.size(); i++) {
        const std::string& boneName = ragdoll->m_markerBoneNames[i];

        int nodeIndex = GetNodeIndex(boneName);
        if (nodeIndex == -1) {
            continue;
        }

        PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[i];
        if (!pxRigidDynamic) {
            Logging::Error() << "pxRigidDynamic for " << boneName << " is nullptr";
            continue;
        }

        glm::mat4 boneWorldMatrix = GetNodeWorldMatrix(boneName);
        PxTransform pxTransform = PxTransform(Hell::Physics::GlmMat4ToPxMat44(boneWorldMatrix));

        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        pxRigidDynamic->setGlobalPose(pxTransform);
    }
}

void SkinnedGameObject::CleanUp() {
    RemoveRagdoll();
}

void SkinnedGameObject::SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    m_animatedMeshNodes.SetMeshWoundMaskArrayIndex(meshName, woundMaskTextureIndex);
}

void SkinnedGameObject::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    m_animatedMeshNodes.SetBlendingModeByMeshName(meshName, blendingMode);
}

void SkinnedGameObject::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshName(meshName, materialName);
}

void SkinnedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshIndex(meshIndex, materialName);
}

void SkinnedGameObject::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    m_animatedMeshNodes.SetMeshWoundMaterialByMeshName(meshName, textureName);
}

void SkinnedGameObject::SetAllMeshMaterials(const std::string& materialName) {
    m_animatedMeshNodes.SetAllMeshMaterials(materialName);
}

void SkinnedGameObject::SetAllMeshBlendingModes(BlendingMode blendingMode) {
    m_animatedMeshNodes.SetAllMeshBlendingModes(blendingMode);
}

void SkinnedGameObject::SetExcludeFromVulkanTLAS(bool exclude) {
    m_animatedMeshNodes.SetExcludeFromVulkanTLAS(exclude);
}

void SkinnedGameObject::SetExclusiveViewportIndex(int index) {
    m_animatedMeshNodes.SetExclusiveViewportIndex(index);
}

void SkinnedGameObject::SetIgnoredViewportIndex(int index) {
    m_animatedMeshNodes.SetIgnoredViewportIndex(index);
}

void SkinnedGameObject::EnableRendering() {
    m_animatedMeshNodes.EnableRendering();
}

void SkinnedGameObject::DisableRendering() {
    m_animatedMeshNodes.DisableRendering();
}


const glm::mat4& SkinnedGameObject::GetLocalBindTransformByNodeName(const std::string& name) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel) return identity;

    // Name exists?
    auto it = m_skinnedModel->m_nodeMapping.find(name);
    if (it == m_skinnedModel->m_nodeMapping.end()) return identity;

    unsigned int index = it->second;

    // Index in range
    if (index >= m_skinnedModel->m_nodes.size()) return identity;

    return m_skinnedModel->m_nodes[index].localBindTransform;
}

void SkinnedGameObject::SetAnimationModeToBindPose() {
    m_animationMode = AnimationMode::BINDPOSE;
}

void SkinnedGameObject::SetAnimationModeToRagdoll() {
    Ragdoll* ragdoll = GetRagdoll();
    if (!ragdoll) {
        Logging::Error() << "SkinnedGameObject::SetAnimationModeToRagdoll() failed because m_ragdollId [" << m_ragdollId << "] was not found in the RagdollManager";
        return;
    }

    if (m_animationMode != AnimationMode::RAGDOLL_V2) {
        m_animationMode = AnimationMode::RAGDOLL_V2;
        ragdoll->EnableSimulation();
        UpdateBoneTransformsFromRagdoll();
    }
}

void SkinnedGameObject::SetAnimationModeToAnimated() {
    m_animationMode = AnimationMode::ANIMATION;
}

const glm::mat4 SkinnedGameObject::GetModelMatrix() {
    if (m_useModelMatrixOverride) {
        return m_modelMatrixOverride;
    }

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        return glm::mat4(1);
    }
    else {
        return m_transform.to_mat4();
    }
}


void SkinnedGameObject::SetName(std::string name) {
    m_name = name;
}

glm::mat4 SkinnedGameObject::GetNodeWorldMatrix(const std::string& nodeName) {
    return GetModelMatrix() * GetNodeModelSpaceMatrix(nodeName);
}

glm::mat4 SkinnedGameObject::GetNodeModelSpaceMatrix(const std::string& nodeName) {
    if (m_animationMode == AnimationMode::ANIMATION && m_animatorInstanceId) {
        AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(m_animatorInstanceId);
        if (animatorInstance) {
            const int32_t nodeIndex = animatorInstance->GetSkeleton().GetNodeIndex(nodeName);
            const std::vector<glm::mat4>& globalPose = animatorInstance->GetGlobalPose();
            if (nodeIndex >= 0 && nodeIndex < int(globalPose.size())) {
                return globalPose[nodeIndex];
            }
        }
    }

    if (m_animationMode == AnimationMode::RAGDOLL_V2 && m_skinnedModel) {
        const int32_t nodeIndex = m_skinnedModel->GetNodeIndex(nodeName);
        if (nodeIndex >= 0 && nodeIndex < int(m_ragdollWorldNodeTransforms.size())) {
            return m_ragdollWorldNodeTransforms[nodeIndex];
        }
    }

    if (m_skinnedModel) {
        const int32_t nodeIndex = m_skinnedModel->GetNodeIndex(nodeName);
        const std::vector<glm::mat4>& globalBindPoseMatrices = m_skinnedModel->GetGlobalBindPoseMatrices();
        if (nodeIndex >= 0 && nodeIndex < int(globalBindPoseMatrices.size())) {
            return globalBindPoseMatrices[nodeIndex];
        }
    }

    return glm::mat4(1.0f);
}

glm::vec3 SkinnedGameObject::GetNodeWorldPosition(const std::string& nodeName) {
    return GetNodeWorldMatrix(nodeName)[3];
}


void SkinnedGameObject::SetSkinnedModelResource(const std::string& name) {
    SkinnedModel* ptr = Hell::ResourceManager::GetSkinnedModelByName(name);
    if (ptr) {
        m_skinnedModel = ptr;
        m_ragdollWorldNodeTransforms.clear();
        m_ragdollBoneSkinningMatrices.clear();
        m_previousRenderBoneSkinningMatrices.clear();
        m_previousRenderMorphTargetWeights.clear();
        m_previousRenderModelMatrix = GetModelMatrix();
        m_hasRenderPoseHistory = false;
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }
}

void SkinnedGameObject::SetSkinnedModel(const std::string& name) {
    SetSkinnedModelResource(name);
    m_animatedMeshNodes.SetSkinnedModel(m_objectId, name);
}

void SkinnedGameObject::SetScale(float scale) {
    m_transform.scale = glm::vec3(scale);
}

void SkinnedGameObject::SetPosition(glm::vec3 position) {
    m_transform.position = position;
}


void SkinnedGameObject::SetRotationX(float rotation) {
    m_transform.rotation.x = rotation;
}


void SkinnedGameObject::SetRotationY(float rotation) {
    m_transform.rotation.y = rotation;
}

void SkinnedGameObject::SetRotationZ(float rotation) {
    m_transform.rotation.z = rotation;
}

void SkinnedGameObject::PrintNodeNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::cout << "-" << i << " " << m_skinnedModel->m_nodes[i].name << "\n";
    }
}

void SkinnedGameObject::PrintMeshNames() {
    m_animatedMeshNodes.PrintMeshNames();
}


void SkinnedGameObject::EnableModelMatrixOverride() {
    m_useModelMatrixOverride = true;
}


void SkinnedGameObject::SetCameraMatrix(const glm::mat4& matrix) {
    m_modelMatrixOverride = matrix;
}


void SkinnedGameObject::DrawBones(int exclusiveViewportIndex) {
    if (!m_skinnedModel) return;

    // Traverse the tree
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        if (parentIndex < 0) continue;

        const std::string& nodeName = m_skinnedModel->m_nodes[i].name;
        const std::string& parentNodeName = m_skinnedModel->m_nodes[parentIndex].name;
        if (!m_skinnedModel->BoneExists(nodeName)) continue;
        if (!m_skinnedModel->BoneExists(parentNodeName)) continue;

        glm::vec3 position = GetNodeWorldPosition(nodeName);
        glm::vec3 parentPosition = GetNodeWorldPosition(parentNodeName);
        DebugDraw::DrawPoint(position, OUTLINE_COLOR, false, exclusiveViewportIndex);
        DebugDraw::DrawLine(position, parentPosition, WHITE, false, 1, exclusiveViewportIndex);
    }
}


void SkinnedGameObject::DrawBoneTangentVectors(float size, int exclusiveViewportIndex) {
    if (!m_skinnedModel) return;

    size *= 0.5f;

    for (const Node& node : m_skinnedModel->m_nodes) {
        if (!m_skinnedModel->BoneExists(node.name)) continue;

        glm::mat4 boneWorldMatrix = GetNodeWorldMatrix(node.name);
        glm::vec3 origin = boneWorldMatrix[3];
        glm::vec3 right = glm::normalize(glm::vec3(boneWorldMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(boneWorldMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(boneWorldMatrix[2]));

        DebugDraw::DrawLine(origin, origin + (forward * size), BLUE, false, 1, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (up * size), GREEN, false, 1, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (right * size), RED, false, 1, exclusiveViewportIndex);
    }
}


int32_t SkinnedGameObject::GetBoneIndex(const std::string& boneName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetBoneIndex(boneName);
}


int32_t SkinnedGameObject::GetNodeIndex(const std::string& nodeName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetNodeIndex(nodeName);
}


uint64_t SkinnedGameObject::CreateRagdoll(const std::string& ragdollName) {
    const RagdollAsset* asset = Hell::ResourceManager::GetRagdollAssetByName(ragdollName);
    return asset ? CreateRagdoll(*asset) : 0;
}

uint64_t SkinnedGameObject::CreateRagdoll(const RagdollAsset& asset) {
    SetScale(asset.skinnedModelScale);
    m_ragdollId = Hell::Physics::SpawnRagdoll(m_transform.position, m_transform.rotation, asset, GetOwnerObjectId());
    return m_ragdollId;
}

void SkinnedGameObject::RemoveRagdoll() {
    if (m_ragdollId == 0) return;

    Hell::Physics::MarkRagdollForRemoval(m_ragdollId);
    m_ragdollId = 0;
}

Ragdoll* SkinnedGameObject::GetRagdoll() {
    if (m_ragdollId == 0) return nullptr;
    return Hell::Physics::GetRagdollById(m_ragdollId);
}

}
