#include "AnimatedGameObject.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Systems/DirtyTracker/DirtyTracker.h"
#include "Unloved/Debug/DebugDraw.h"

#include <iostream>
#include <limits>
#include <cmath>

namespace Unloved {

AnimatedGameObject::AnimatedGameObject(uint64_t id) {
    m_objectId = id;
}

void AnimatedGameObject::UpdateRenderItems() {
    m_animatedMeshNodes.UpdateRenderItems(GetModelMatrix(), m_animationState.boneSkinningMatrices);
}

const uint32_t AnimatedGameObject::GetVerteXCount() {
    if (m_skinnedModel) {
        return m_skinnedModel->GetVertexCount();
    }
    else {
        return 0;
    }
}

void AnimatedGameObject::UpdateBoneTransformsFromRagdoll() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) return;
    if (!m_skinnedModel) return;

    int nodeCount = m_skinnedModel->m_nodes.size();
    m_animationState.globalNodeTransforms.resize(nodeCount);

    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::string NodeName = m_skinnedModel->m_nodes[i].name;
        glm::mat4 nodeTransformation = glm::mat4(1);
        nodeTransformation = m_skinnedModel->m_nodes[i].inverseBindTransform;
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_animationState.globalNodeTransforms[parentIndex];
        glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;

        for (int j = 0; j < ragdoll->m_markerBoneNames.size(); j++) {
            if (ragdoll->m_markerBoneNames[j] == NodeName) {
                PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[j];
                GlobalTransformation = Hell::Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
            }
        }

        m_animationState.globalNodeTransforms[i] = GlobalTransformation;
    }
}


void AnimatedGameObject::Update(float deltaTime) {
    if (!m_skinnedModel) return;

    if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        UpdateBoneTransformsFromRagdoll();
    }
    else {
        if (m_animationMode == AnimationMode::BINDPOSE) {
            //m_animationLayerOLD.ClearAllAnimationStates();
            //m_animator.ClearAllAnimations();
        }

        Animator::Update(m_animationState, deltaTime);
        //m_globalBlendedNodeTransforms = m_animator.m_globalBlendedNodeTransforms;
    }

    Animator::UpdateBoneSkinningMatrices(m_animationState);

    // If it has a ragdoll then sink the ragdoll rigids to the animated pose
    if (m_animationMode == AnimationMode::BINDPOSE || m_animationMode == AnimationMode::ANIMATION) {
        SyncRagdollToAnimation();
    }

    UpdateDirtyBounds();
}

void AnimatedGameObject::ComputeBoneSegments() {
    m_boneSegments.clear();

    for (int boneIndex = 0; boneIndex < m_skinnedModel->m_boneNodeIndices.size(); boneIndex++) {
        int nodeIndex = m_skinnedModel->m_boneNodeIndices[boneIndex];
        if (nodeIndex < 0 || nodeIndex >= m_animationState.globalNodeTransforms.size()) {
            continue;
        }

        int parentNodeIndex = m_skinnedModel->m_nodes[nodeIndex].parentIndex;
        int parentBoneNodeIndex = -1;

        // Walk past helper nodes until another skinning bone is found
        while (parentNodeIndex >= 0 && parentNodeIndex < m_skinnedModel->m_nodes.size()) {
            const std::string& parentNodeName = m_skinnedModel->m_nodes[parentNodeIndex].name;
            if (m_skinnedModel->BoneExists(parentNodeName)) {
                parentBoneNodeIndex = parentNodeIndex;
                break;
            }

            parentNodeIndex = m_skinnedModel->m_nodes[parentNodeIndex].parentIndex;
        }

        if (parentBoneNodeIndex == -1 || parentBoneNodeIndex >= m_animationState.globalNodeTransforms.size()) {
            continue;
        }

        const glm::mat4& boneWorldMatrix = m_animationState.globalNodeTransforms[nodeIndex];
        const glm::mat4& parentBoneWorldMatrix = m_animationState.globalNodeTransforms[parentBoneNodeIndex];

        BoneSegment& segment = m_boneSegments.emplace_back();
        segment.boneName = m_skinnedModel->m_nodes[nodeIndex].name;
        segment.start = GetModelMatrix() * boneWorldMatrix * glm::vec4(0, 0, 0, 1);
        segment.end = GetModelMatrix() * parentBoneWorldMatrix * glm::vec4(0, 0, 0, 1);
    }
}

void AnimatedGameObject::CalculateSkinnedAABB() {
    glm::vec3 boundsMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 boundsMax = glm::vec3(std::numeric_limits<float>::lowest());

    // If there is a ragdoll, use each rigid as the body bounds
    if (Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId)) {
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
        if (boneIndex < 0 || boneIndex >= m_animationState.boneSkinningMatrices.size()) {
            continue;
        }

        const glm::mat4 finalWorldMatrix = modelMatrix * m_animationState.boneSkinningMatrices[boneIndex];
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

void AnimatedGameObject::UpdateDirtyBounds() {
    if (!RenderingEnabled()) {
        return;
    }

    if (!CastsShadows()) {
        return;
    }

    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);

    if (ragdoll) {
        ragdoll->UpdateWorldSpaceAABBs(m_skinnedAABBChangeThreshold);
    }

    CalculateSkinnedAABB();

    const glm::vec3& boundsMin = m_skinnedAABB.GetBoundsMin();
    const glm::vec3& boundsMax = m_skinnedAABB.GetBoundsMax();
    const glm::vec3& boundsMinLastFrame = m_skinnedAABBLastFrame.GetBoundsMin();
    const glm::vec3& boundsMaxLastFrame = m_skinnedAABBLastFrame.GetBoundsMax();

    bool aabbChanged = false;

    // Force dirty on the first valid frame
    if (!m_hasSkinnedAABBLastFrame) {
        aabbChanged = true;
    }
    else if (ragdoll && ragdoll->IsDirty()) {
        aabbChanged = true;
    }
    else {
        // Check if the bounds moved enough to matter
        if (std::abs(boundsMin.x - boundsMinLastFrame.x) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMin.y - boundsMinLastFrame.y) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMin.z - boundsMinLastFrame.z) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMax.x - boundsMaxLastFrame.x) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMax.y - boundsMaxLastFrame.y) > m_skinnedAABBChangeThreshold ||
            std::abs(boundsMax.z - boundsMaxLastFrame.z) > m_skinnedAABBChangeThreshold) {
            aabbChanged = true;
        }
    }

    if (aabbChanged) {
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

        DirtyTracker::AddDynamicDirtyBounds(dirtyBounds);
    }

    m_skinnedAABBLastFrame = m_skinnedAABB;
    m_hasSkinnedAABBLastFrame = true;

    //DebugDraw::DrawAABB(GetSkinnedAABB(), aabbChanged ? GREEN : RED);
}

void AnimatedGameObject::SyncRagdollToAnimation() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
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

        glm::mat4 boneWorldMatrix = GetModelMatrix() * GetAnimatedTransformByNodeIndex(nodeIndex);
        PxTransform pxTransform = PxTransform(Hell::Physics::GlmMat4ToPxMat44(boneWorldMatrix));

        pxRigidDynamic->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        pxRigidDynamic->setGlobalPose(pxTransform);
    }
}

void AnimatedGameObject::CleanUp() {
    Hell::Physics::MarkRagdollForRemoval(m_ragdollId);
}

void AnimatedGameObject::SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    m_animatedMeshNodes.SetMeshWoundMaskArrayIndex(meshName, woundMaskTextureIndex);
}

void AnimatedGameObject::SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode) {
    m_animatedMeshNodes.SetBlendingModeByMeshName(meshName, blendingMode);
}

void AnimatedGameObject::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshName(meshName, materialName);
}

void AnimatedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    m_animatedMeshNodes.SetMeshMaterialByMeshIndex(meshIndex, materialName);
}

void AnimatedGameObject::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    m_animatedMeshNodes.SetMeshWoundMaterialByMeshName(meshName, textureName);
}

void AnimatedGameObject::SetAllMeshMaterials(const std::string& materialName) {
    m_animatedMeshNodes.SetAllMeshMaterials(materialName);
}

void AnimatedGameObject::SetAllMeshBlendingModes(BlendingMode blendingMode) {
    m_animatedMeshNodes.SetAllMeshBlendingModes(blendingMode);
}

void AnimatedGameObject::SetExcludeFromVulkanTLAS(bool exclude) {
    m_animatedMeshNodes.SetExcludeFromVulkanTLAS(exclude);
}

void AnimatedGameObject::SetExclusiveViewportIndex(int index) {
    m_animatedMeshNodes.SetExclusiveViewportIndex(index);
}

void AnimatedGameObject::SetIgnoredViewportIndex(int index) {
    m_animatedMeshNodes.SetIgnoredViewportIndex(index);
}

void AnimatedGameObject::EnableRendering() {
    m_animatedMeshNodes.EnableRendering();
}

void AnimatedGameObject::DisableRendering() {
    m_animatedMeshNodes.DisableRendering();
}


const glm::mat4& AnimatedGameObject::GetInverseBindTransformByBoneName(const std::string& name) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel) return identity;

    // Name exists?
    auto it = m_skinnedModel->m_nodeMapping.find(name);
    if (it == m_skinnedModel->m_nodeMapping.end()) return identity;

    unsigned int index = it->second;

    // Index in range
    if (index >= m_skinnedModel->m_nodes.size()) return identity;

    return m_skinnedModel->m_nodes[index].inverseBindTransform;
}


void AnimatedGameObject::SetAnimationModeToBindPose() {
    m_animationMode = AnimationMode::BINDPOSE;
    Animator::ClearAllAnimations(m_animationState);
}


void AnimatedGameObject::SetAnimationModeToRagdoll() {
    Ragdoll* ragdoll = Hell::Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) {
        Logging::Error() << "AnimatedGameObject::SetAnimationModeToRagdoll() failed because m_ragdollId [" << m_ragdollId << "] was not found in the RagdollManager";
        return;
    }

    if (m_animationMode != AnimationMode::RAGDOLL_V2) {
        m_animationMode = AnimationMode::RAGDOLL_V2;
        Animator::ClearAllAnimations(m_animationState);
        ragdoll->EnableSimulation();
    }
}


void AnimatedGameObject::SetAnimationModeToAnimated() {
    m_animationMode = AnimationMode::ANIMATION;
    Animator::ClearAllAnimations(m_animationState);
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    Animator::PlayAnimation(m_animationState, layerName, animationName, speed, false);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    Animator::PlayAnimation(m_animationState, layerName, animationName, speed, true);
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAnimation(layerName, animationNames[rand], speed);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAndLoopAnimation(layerName, animationNames[rand], speed);
}

void AnimatedGameObject::CrossFadeAnimation(const std::string& layerName, const std::string& animationName, float fadeDuration, float speed, bool loop) {
    Animator::CrossFade(m_animationState, layerName, animationName, fadeDuration, speed, loop);
}

void AnimatedGameObject::FadeOutAnimationLayer(const std::string& layerName, float fadeDuration) {
    Animator::FadeOutLayer(m_animationState, layerName, fadeDuration);
}

void AnimatedGameObject::SetAnimationLayerWeight(const std::string& layerName, float weight) {
    Animator::SetLayerWeight(m_animationState, layerName, weight);
}

void AnimatedGameObject::SetAnimationLayerBlendMode(const std::string& layerName, AnimationBlendMode blendMode) {
    Animator::SetLayerBlendMode(m_animationState, layerName, blendMode);
}

void AnimatedGameObject::SetAnimationLayerNodeWeights(const std::string& layerName, const std::vector<float>& nodeWeights) {
    Animator::SetLayerNodeWeights(m_animationState, layerName, nodeWeights);
}


const glm::mat4 AnimatedGameObject::GetModelMatrix() {
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


bool AnimatedGameObject::IsAllAnimationsComplete() {
    return Animator::AllAnimationsComplete(m_animationState);
}


void AnimatedGameObject::SetName(std::string name) {
    m_name = name;
}


void AnimatedGameObject::SetSkinnedModel(const std::string& name, const std::string& presetName) {
    SkinnedModel* ptr = Hell::ResourceManager::GetSkinnedModelByName(name);
    if (ptr) {
        m_skinnedModel = ptr;
        Animator::SetSkinnedModel(m_animationState, ptr);
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }

    if (presetName != UNDEFINED_STRING) {
        Bible::ConfigureAnimatedMeshNodes(m_objectId, &m_animatedMeshNodes, presetName);
    }
    else {
        m_animatedMeshNodes.SetSkinnedModel(m_objectId, name);
    }
}


const glm::mat4& AnimatedGameObject::GetAnimatedTransformByNodeIndex(int32_t nodeIndex) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel || nodeIndex < 0 || nodeIndex >= m_animationState.globalNodeTransforms.size()) {
        return identity;
    }

    return m_animationState.globalNodeTransforms[nodeIndex];
}


const glm::mat4& AnimatedGameObject::GetAnimatedTransformByBoneName(const std::string& name) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel) return identity;

    auto it = m_skinnedModel->m_nodeMapping.find(name);
    if (it == m_skinnedModel->m_nodeMapping.end()) {
        //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() failed to find '" << name << "'\n";
        return identity;
    }

    int index = it->second;
    if (index < 0 || index >= int(m_animationState.globalNodeTransforms.size())) {
        //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() '" << name << "' index " << index << " out of range of " << m_animator.m_globalBlendedNodeTransforms.size() << "\n";
        return identity;
    }

    return m_animationState.globalNodeTransforms[index];
}


void AnimatedGameObject::SetScale(float scale) {
    m_transform.scale = glm::vec3(scale);
}

void AnimatedGameObject::SetPosition(glm::vec3 position) {
    m_transform.position = position;
}


void AnimatedGameObject::SetRotationX(float rotation) {
    m_transform.rotation.x = rotation;
}


void AnimatedGameObject::SetRotationY(float rotation) {
    m_transform.rotation.y = rotation;
}

void AnimatedGameObject::SetRotationZ(float rotation) {
    m_transform.rotation.z = rotation;
}

void AnimatedGameObject::PrintNodeNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::cout << "-" << i << " " << m_skinnedModel->m_nodes[i].name << "\n";
    }
}

void AnimatedGameObject::PrintMeshNames() {
    m_animatedMeshNodes.PrintMeshNames();
}


void AnimatedGameObject::EnableModelMatrixOverride() {
    m_useModelMatrixOverride = true;
}


void AnimatedGameObject::SetCameraMatrix(const glm::mat4& matrix) {
    m_modelMatrixOverride = matrix;
}


const uint32_t AnimatedGameObject::GetAnimationFrameNumber(const std::string& animationLayerName) {
    return Animator::GetAnimationFrameNumber(m_animationState, animationLayerName);
}


bool AnimatedGameObject::AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber) {
    return Animator::AnimationIsPastFrameNumber(m_animationState, animationLayerName, frameNumber);
}


void AnimatedGameObject::DrawBones(int exclusiveViewportIndex) {
    if (!m_skinnedModel) return;

    // Traverse the tree
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        glm::mat4 nodeTransformation = glm::mat4(1);
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        std::string& nodeName = m_skinnedModel->m_nodes[i].name;
        std::string& parentNodeName = m_skinnedModel->m_nodes[parentIndex].name;

        if (parentIndex != -1 && m_skinnedModel->BoneExists(nodeName) && m_skinnedModel->BoneExists(parentNodeName)) {
            const glm::mat4& boneWorldMatrix = m_animationState.globalNodeTransforms[i];
            const glm::mat4& parentBoneWorldMatrix = m_animationState.globalNodeTransforms[parentIndex];
            glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec3 parentPosition = GetModelMatrix() * parentBoneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            DebugDraw::DrawPoint(position, OUTLINE_COLOR, false, exclusiveViewportIndex);
            DebugDraw::DrawLine(position, parentPosition, WHITE, false, exclusiveViewportIndex);
        }
    }

    // // To draw all nodes
    // for (const glm::mat4& boneWorldMatrix : m_animationLayer.m_globalBlendedNodeTransforms) {
    //     glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    //     DebugDraw::DrawPoint(position, color, false, exclusiveViewportIndex);
    // }
}


void AnimatedGameObject::DrawBoneTangentVectors(float size, int exclusiveViewportIndex) {
    size *= 0.5f;

    for (const glm::mat4& boneWorldMatrix : m_animationState.globalNodeTransforms) {
        glm::vec3 origin = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 right = glm::normalize(glm::vec3(boneWorldMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(boneWorldMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(boneWorldMatrix[2]));

        DebugDraw::DrawLine(origin, origin + (forward * size), BLUE, false, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (up * size), GREEN, false, exclusiveViewportIndex);
        DebugDraw::DrawLine(origin, origin + (right * size), RED, false, exclusiveViewportIndex);
    }
}


bool AnimatedGameObject::AnimationByNameIsComplete(const std::string& name) {
    return Animator::AnimationIsCompleteAnyLayer(m_animationState, name);
}


const glm::mat4& AnimatedGameObject::GetGlobalBlendedNodeTransfrom(const std::string& nodeName) {
    uint32_t nodeIndex = GetNodeIndex(nodeName);

    if (nodeIndex >= 0 && nodeIndex < (uint32_t)m_animationState.globalNodeTransforms.size()) {
        return m_animationState.globalNodeTransforms[nodeIndex];
    }
    static const glm::mat4 identity = glm::mat4(1.0f);
    return identity;
}


const glm::mat4 AnimatedGameObject::GetBoneWorldMatrixWithBoneOffset(const std::string& boneName) {
    const glm::mat4& globalBlendedNodeTransform = GetGlobalBlendedNodeTransfrom(boneName);
    const glm::mat4& boneOffset = m_skinnedModel->GetBoneOffset(boneName);

    return GetModelMatrix() * globalBlendedNodeTransform * boneOffset;
}


int32_t AnimatedGameObject::GetBoneIndex(const std::string& boneName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetBoneIndex(boneName);
}


int32_t AnimatedGameObject::GetNodeIndex(const std::string& nodeName) {
    if (!m_skinnedModel) return -1;
    return m_skinnedModel->GetNodeIndex(nodeName);
}


const glm::mat4 AnimatedGameObject::GetBoneWorldMatrix(const std::string& boneName) {
    int nodeIndex = GetNodeIndex(boneName);
    if (nodeIndex < 0 || nodeIndex >= int(m_animationState.globalNodeTransforms.size())) {
        return glm::mat4(1.0f);
    }
    else {
        return GetModelMatrix() * m_animationState.globalNodeTransforms[nodeIndex];
    }
}

const glm::vec3 AnimatedGameObject::GetBoneWorldPosition(const std::string& boneName) {
    return GetBoneWorldMatrix(boneName)[3];
}

void AnimatedGameObject::SetRagdollId(uint64_t RagdollId) {
    m_ragdollId = RagdollId;
}

void AnimatedGameObject::SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix) {
    Animator::SetAdditiveTransform(m_animationState, nodeName, matrix);
}

void AnimatedGameObject::PauseAllAnimationLayers() {
    Animator::PauseAllLayers(m_animationState);
}
}
