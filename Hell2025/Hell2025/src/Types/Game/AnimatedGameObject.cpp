#include "AnimatedGameObject.h"
#include "AssetManagement/AssetManager.h"
#include "Core/Game.h"
#include "HellLogging.h"
#include "Input/Input.h"
#include "Physics/Physics.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "UniqueID.h"
#include "Util.h"

#include <unordered_set>
#include <Ragdoll/RagdollManager.h>

AnimatedGameObject::AnimatedGameObject(uint64_t id) {
    m_objectId = id;
}


void AnimatedGameObject::SetMeshWoundMaskTextureIndex(const std::string& meshName, int32_t woundMaskTextureIndex) {
    std::vector<uint32_t>& meshIndices = m_skinnedModel->GetMeshIndices();

    for (int i = 0; i < meshIndices.size(); i++) {
        uint32_t meshIndex = meshIndices[i];
        SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(meshIndex);
        if (skinnedMesh && skinnedMesh->name == meshName) {
            m_woundMaskTextureIndices[i] = woundMaskTextureIndex;
            return;
        }
    }
}


void AnimatedGameObject::EnableRendering() {
    m_renderingEnabled = true;
}


void AnimatedGameObject::DisableRendering() {
    m_renderingEnabled = false;
}


void AnimatedGameObject::SetRagdoll(const std::string& ragdollName, float ragdollTotalWeight) {
    m_ragdollId = Physics::CreateRagdollByName(ragdollName, ragdollTotalWeight);
}


void AnimatedGameObject::UpdateRenderItems() {
    if (!m_skinnedModel) return;

    int meshCount = m_meshRenderingEntries.size();

    m_deformingRenderItems.clear();
    m_nonDeformingRenderItems.clear();
    m_nonDeformingRenderItemsDepthPeeledTransparent.clear();

    int skinnedVertexCount = 0;
    int skippedVertexCount = 0;

    for (int i = 0; i < meshCount; i++) {
        if (m_meshRenderingEntries[i].drawingEnabled) {
            RenderItem renderItem;
            SkinnedMesh* mesh = AssetManager::GetSkinnedMeshByIndex(m_meshRenderingEntries[i].meshIndex);

            Material* material = AssetManager::GetMaterialByIndex(m_meshRenderingEntries[i].materialIndex);
            renderItem.baseColorTextureIndex = material->m_basecolor;
            renderItem.rmaTextureIndex = material->m_rma;
            renderItem.normalMapTextureIndex = material->m_normal;
            renderItem.modelMatrix = GetModelMatrix();
            renderItem.inverseModelMatrix = glm::inverse(GetModelMatrix());
            renderItem.meshIndex = m_skinnedModel->GetMeshIndices()[i];
            renderItem.ignoredViewportIndex = m_ignoredViewportIndex;
            renderItem.exclusiveViewportIndex = m_exclusiveViewportIndex;
            renderItem.furLength = m_meshRenderingEntries[i].furLength;
            renderItem.furUVScale = m_meshRenderingEntries[i].furUVScale;
            renderItem.furShellDistanceAttenuation = m_meshRenderingEntries[i].furShellDistanceAttenuation;
            renderItem.woundMaskTexutreIndex = m_woundMaskTextureIndices[i];
            renderItem.blockScreenSpaceBloodDecals = (int)true;
            Util::PackUint64(m_objectId, renderItem.objectIdLowerBit, renderItem.objectIdUpperBit);

            if (m_woundMaskTextureIndices[i] != -1) {
                Material* wouldMaterial = AssetManager::GetMaterialByIndex(m_meshRenderingEntries[i].woundMaterialIndex);
                renderItem.woundBaseColorTextureIndex = wouldMaterial->m_basecolor;
                renderItem.woundNormalMapTextureIndex = wouldMaterial->m_normal;
                renderItem.woundRmaTextureIndex = wouldMaterial->m_rma;
            }

            // Put it where it belongs
            if (mesh->requiresSkinning) {
                m_deformingRenderItems.push_back(renderItem);
                skinnedVertexCount += mesh->vertexCount;
            }
            else {
                skippedVertexCount += mesh->vertexCount;

                // Update the model matrix to include the animated bone transform
                int boneIndex = mesh->nonDeformingBoneIndex;
                const int boneMatrixCount = static_cast<int>(m_boneSkinningMatrices.size());
                if (boneIndex >= 0 && boneIndex < boneMatrixCount) {
                    renderItem.modelMatrix = GetModelMatrix() * m_boneSkinningMatrices[boneIndex];
                }
                else {
                    renderItem.modelMatrix = GetModelMatrix();
                }
                renderItem.inverseModelMatrix = glm::inverse(renderItem.modelMatrix);


				if (mesh->name == "Magazine_low" ||
                    mesh->name == "Magazine_low2") {
                    m_nonDeformingRenderItemsDepthPeeledTransparent.push_back(renderItem);

                } else {
                    m_nonDeformingRenderItems.push_back(renderItem);
                }
            }
        }
    }
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
    Ragdoll* ragdoll = Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) return;
    if (!m_skinnedModel) return;

    int nodeCount = m_skinnedModel->m_nodes.size();
    m_animator.m_globalBlendedNodeTransforms.resize(nodeCount);

    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::string NodeName = m_skinnedModel->m_nodes[i].name;
        glm::mat4 nodeTransformation = glm::mat4(1);
        nodeTransformation = m_skinnedModel->m_nodes[i].inverseBindTransform;
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_animator.m_globalBlendedNodeTransforms[parentIndex];
        glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;

        for (int j = 0; j < ragdoll->m_correspondingBoneNames.size(); j++) {
            if (ragdoll->m_correspondingBoneNames[j] == NodeName) {
                RigidDynamic* rigidDynamic = Physics::GetRigidDynamicById(ragdoll->m_rigidDynamicIds[j]);
                PxRigidDynamic* pxRigidDynamic = rigidDynamic->GetPxRigidDynamic();
                GlobalTransformation = Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
            }
        }

        m_animator.m_globalBlendedNodeTransforms[i] = GlobalTransformation;
    }
}


void AnimatedGameObject::UpdateBoneTransformsFromRagdollV2() {
    RagdollV2* ragdoll = RagdollManager::GetRagdollV2ById(m_ragdollV2Id);
    if (!ragdoll) return;
    if (!m_skinnedModel) return;

    int nodeCount = m_skinnedModel->m_nodes.size();
    m_animator.m_globalBlendedNodeTransforms.resize(nodeCount);

    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::string NodeName = m_skinnedModel->m_nodes[i].name;
        glm::mat4 nodeTransformation = glm::mat4(1);
        nodeTransformation = m_skinnedModel->m_nodes[i].inverseBindTransform;
        unsigned int parentIndex = m_skinnedModel->m_nodes[i].parentIndex;
        glm::mat4 ParentTransformation = (parentIndex == -1) ? glm::mat4(1) : m_animator.m_globalBlendedNodeTransforms[parentIndex];
        glm::mat4 GlobalTransformation = ParentTransformation * nodeTransformation;

        for (int j = 0; j < ragdoll->m_markerBoneNames.size(); j++) {
            if (ragdoll->m_markerBoneNames[j] == NodeName) {
                PxRigidDynamic* pxRigidDynamic = ragdoll->m_pxRigidDynamics[j];
                GlobalTransformation = Physics::PxMat44ToGlmMat4(pxRigidDynamic->getGlobalPose());
            }
        }

        m_animator.m_globalBlendedNodeTransforms[i] = GlobalTransformation;
    }
}


void AnimatedGameObject::Update(float deltaTime) {
    if (!m_skinnedModel) return;

    if (m_animationMode == AnimationMode::RAGDOLL) {
        UpdateBoneTransformsFromRagdoll();
    }
    else if (m_animationMode == AnimationMode::RAGDOLL_V2) {
        UpdateBoneTransformsFromRagdollV2();
    }
    else {
        if (m_animationMode == AnimationMode::BINDPOSE) {
            //m_animationLayerOLD.ClearAllAnimationStates();
            //m_animator.ClearAllAnimations();
        }

        m_animator.UpdateAnimations(deltaTime);
        //m_globalBlendedNodeTransforms = m_animator.m_globalBlendedNodeTransforms;
    }

    m_boneSkinningMatrices.clear();

    for (uint32_t i = 0; i < m_skinnedModel->GetBoneCount(); i++) {
        m_boneSkinningMatrices.push_back(glm::mat4(1));
    }

    // Compute local bone matrices
    int boneCount = m_skinnedModel->GetBoneCount();
    m_boneSkinningMatrices.resize(boneCount);

    const int nodeTransformCount = (int)m_animator.m_globalBlendedNodeTransforms.size();
    const int boneNodeIndexCount = (int)m_skinnedModel->m_boneNodeIndices.size();
    const int boneOffsetCount = (int)m_skinnedModel->m_boneOffsets.size();

    for (int b = 0; b < boneCount; ++b) {
        if (b >= boneNodeIndexCount || b >= boneOffsetCount) {
            continue;
        }

        int nodeIdx = m_skinnedModel->m_boneNodeIndices[b];
        if (nodeIdx < 0 || nodeIdx >= nodeTransformCount) {
            continue;
        }

        m_boneSkinningMatrices[b] = m_animator.m_globalBlendedNodeTransforms[nodeIdx] * m_skinnedModel->m_boneOffsets[b];
    }

    // If it has a ragdoll
    if (m_animationMode == AnimationMode::BINDPOSE ||
        m_animationMode == AnimationMode::ANIMATION) {

        Ragdoll* ragdoll = Physics::GetRagdollById(m_ragdollId);
        if (ragdoll) {
            ragdoll->SetRigidGlobalPosesFromAnimatedGameObject(this);
        }

        if (m_ragdollV2Id != 0) {
            RagdollV2* ragdollV2 = RagdollManager::GetRagdollV2ById(m_ragdollV2Id);
            if (ragdollV2) {
                ragdollV2->SetRigidGlobalPosesFromAnimatedGameObject(this);
            }
        }
    }
    //if (m_animationMode != AnimationMode::RAGDOLL_V2) {
    //
    //
    //
    //
    //}

    Ragdoll* ragdoll = Physics::GetRagdollById(m_ragdollId);
    if (ragdoll && false) {
        for (int i = 0; i < ragdoll->m_components.joints.size(); i++) {

            JointComponent& joint = ragdoll->m_components.joints[i];
            D6Joint* d6Joint = Physics::GetD6JointById(ragdoll->m_d6JointIds[i]);
            PxD6Joint* pxD6Joint = d6Joint->GetPxD6Joint();

            // Linear spring
            joint.limit_linearStiffness = 10000;
            joint.limit_linearDampening = 1000000;
            joint.drive_angularStiffness = 10000;
            joint.drive_angularDamping = 1000000;

            const PxSpring linearSpring = PxSpring(joint.limit_linearStiffness, joint.limit_linearDampening);

            if (joint.limit.x > -1) {
                const PxJointLinearLimitPair limitX = PxJointLinearLimitPair(-joint.limit.x, joint.limit.x, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eX, limitX);
            }

            if (joint.limit.y > -1) {
                const PxJointLinearLimitPair limitY = PxJointLinearLimitPair(-joint.limit.y, joint.limit.y, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eY, limitY);
            }

            if (joint.limit.z > -1) {
                const PxJointLinearLimitPair limitZ = PxJointLinearLimitPair(-joint.limit.z, joint.limit.z, linearSpring);
                pxD6Joint->setLinearLimit(PxD6Axis::eZ, limitZ);
            }

            const PxSpring angularSpring = PxSpring(joint.drive_angularStiffness, joint.drive_angularDamping);
            const PxJointAngularLimitPair twistLimit = PxJointAngularLimitPair(-joint.twist, joint.twist, angularSpring);
            const PxJointLimitCone swingLimit = PxJointLimitCone(joint.swing1, joint.swing2, angularSpring);

            pxD6Joint->setTwistLimit(twistLimit);
            pxD6Joint->setSwingLimit(swingLimit);
        }

    }
}


void AnimatedGameObject::CleanUp() {
    if (m_ragdollId != 0) {
        Physics::MarkRagdollForRemoval(m_ragdollId);
    }
}


void AnimatedGameObject::SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.materialIndex = AssetManager::GetMaterialIndexByName(materialName);
        }
    }
}


void AnimatedGameObject::SetMeshFurLength(const std::string& meshName, float furLength) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.furLength = furLength;
        }
    }
}


void AnimatedGameObject::SetMeshFurUVScale(const std::string& meshName, float uvScale) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.furUVScale = uvScale;
        }
    }
}


void AnimatedGameObject::SetMeshFurShellDistanceAttenuation(const std::string& meshName, float furShellDistanceAttenuation) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.furShellDistanceAttenuation = furShellDistanceAttenuation;
        }
    }
}


void AnimatedGameObject::SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName) {
    if (!m_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < m_meshRenderingEntries.size()) {
        m_meshRenderingEntries[meshIndex].materialIndex = AssetManager::GetMaterialIndexByName(materialName);
    }
}


void AnimatedGameObject::SetMeshToRenderAsGlassByMeshIndex(const std::string& meshName) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.renderAsGlass = true;
        }
    }
}


void AnimatedGameObject::SetMeshEmissiveColorTextureByMeshName(const std::string& meshName, const std::string& textureName) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.emissiveColorTexutreIndex = AssetManager::GetTextureIndexByName(textureName);
        }
    }
}


void AnimatedGameObject::SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.woundMaterialIndex = AssetManager::GetMaterialIndexByName(textureName);
        }
    }
}


void AnimatedGameObject::SetAdditiveTransform(const std::string& nodeName, const glm::mat4& matrix) {
    m_animator.SetAdditiveTransform(nodeName, matrix);
}


void AnimatedGameObject::PauseAllAnimationLayers() {
    m_animator.PauseAllLayers();
}


void AnimatedGameObject::EnableBlendingByMeshIndex(int meshIndex) {
    if (!m_skinnedModel) {
        return;
    }
    if (meshIndex >= 0 && meshIndex < m_meshRenderingEntries.size()) {
        m_meshRenderingEntries[meshIndex].blendingEnabled = true;
    }
}


void AnimatedGameObject::SetAllMeshMaterials(const std::string& materialName) {
    if (!m_skinnedModel) {
        return;
    }
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        meshRenderingEntry.materialIndex =AssetManager::GetMaterialIndexByName(materialName);
    }
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
    m_animator.ClearAllAnimations();
}


void AnimatedGameObject::SetAnimationModeToRagdoll() {
    Ragdoll* ragdoll = Physics::GetRagdollById(m_ragdollId);
    if (!ragdoll) return;

    if (m_animationMode != AnimationMode::RAGDOLL) {
        m_animationMode = AnimationMode::RAGDOLL;
        m_animator.ClearAllAnimations();
        ragdoll->ActivatePhysics();
    }
}


void AnimatedGameObject::SetAnimationModeToRagdollV2() {
    RagdollV2* ragdoll = RagdollManager::GetRagdollV2ById(m_ragdollV2Id);
    if (!ragdoll) {
        Logging::Error() << "AnimatedGameObject::SetAnimationModeToRagdollV2() failed because m_ragdollId [" << m_ragdollV2Id << "] was not found in the RagdollManager";
        return;
    }

    if (m_animationMode != AnimationMode::RAGDOLL_V2) {
        m_animationMode = AnimationMode::RAGDOLL_V2;
        m_animator.ClearAllAnimations();
        ragdoll->EnableSimulation();
    }
}


void AnimatedGameObject::SetAnimationModeToAnimated() {
    m_animationMode = AnimationMode::ANIMATION;
    m_animator.ClearAllAnimations();
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    m_animator.PlayAnimation(layerName, animationName, speed, false);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, const std::string& animationName, float speed) {
    m_animator.PlayAnimation(layerName, animationName, speed, true);
}


void AnimatedGameObject::PlayAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAnimation(layerName, animationNames[rand], speed);
}


void AnimatedGameObject::PlayAndLoopAnimation(const std::string& layerName, std::vector<std::string>& animationNames, float speed) {
    int rand = std::rand() % animationNames.size();
    PlayAndLoopAnimation(layerName, animationNames[rand], speed);
}


const glm::mat4 AnimatedGameObject::GetModelMatrix() {
    if (m_useModelMatrixOverride) {
        return m_modelMatrixOverride;
    }

    if (m_animationMode == AnimationMode::RAGDOLL || m_animationMode == AnimationMode::RAGDOLL_V2) {
        return glm::mat4(1);
    }
    else {
        return m_transform.to_mat4();
    }
}


bool AnimatedGameObject::IsAllAnimationsComplete() {
    return m_animator.AllAnimationsComplete();
}


void AnimatedGameObject::SetName(std::string name) {
    m_name = name;
}


void AnimatedGameObject::SetSkinnedModel(std::string name) {
    SkinnedModel* ptr = AssetManager::GetSkinnedModelByName(name);
    if (ptr) {
        //std::cout << "SetSkinnedModel() " << name << " mesh count: " << m_skinnedModel->GetMeshCount() << "\n";

        m_skinnedModel = ptr;
        m_meshRenderingEntries.clear();
        m_woundMaskTextureIndices.resize(m_skinnedModel->GetMeshCount());

        int meshCount = m_skinnedModel->GetMeshCount();

        for (int i = 0; i < meshCount; i++) {
            SkinnedMesh* skinnedMesh = AssetManager::GetSkinnedMeshByIndex(m_skinnedModel->GetMeshIndices()[i]);
            MeshRenderingEntry& meshRenderingEntry = m_meshRenderingEntries.emplace_back();
            meshRenderingEntry.meshName = skinnedMesh->name;
            meshRenderingEntry.meshIndex = m_skinnedModel->GetMeshIndices()[i];


            m_woundMaskTextureIndices[i] = -1;
        }

        // Store bone indices
        m_boneMapping.clear();
        for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
            m_boneMapping[m_skinnedModel->m_nodes[i].name] = i;
        }

        m_animator.SetSkinnedModel(name);
    }
    else {
        std::cout << "Could not SetSkinnedModel(name) with name: \"" << name << "\", it does not exist\n";
    }
}


const glm::mat4& AnimatedGameObject::GetAnimatedTransformByNodeIndex(int32_t nodeIndex) {
    const static glm::mat4 identity = glm::mat4(1.0f);

    if (!m_skinnedModel || nodeIndex < 0 || nodeIndex >= m_animator.m_globalBlendedNodeTransforms.size()) {
        return identity;
    }

    return m_animator.m_globalBlendedNodeTransforms[nodeIndex];
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
    if (index < 0 || index >= int(m_animator.m_globalBlendedNodeTransforms.size())) {
        //std::cout << "AnimatedGameObject::GetAnimatedTransformByBoneName() '" << name << "' index " << index << " out of range of " << m_animator.m_globalBlendedNodeTransforms.size() << "\n";
        return identity;
    }

    return m_animator.m_globalBlendedNodeTransforms[index];
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


void AnimatedGameObject::EnableDrawingForAllMesh() {
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        meshRenderingEntry.drawingEnabled = true;
    }
}


void AnimatedGameObject::EnableDrawingForMeshByMeshName(const std::string& meshName) {
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.drawingEnabled = true;
            return;
        }
    }
    //std::cout << "DisableDrawingForMeshByMeshName() called but name " << meshName << " was not found!\n";
}


void AnimatedGameObject::DisableDrawingForMeshByMeshName(const std::string& meshName) {
    for (MeshRenderingEntry& meshRenderingEntry : m_meshRenderingEntries) {
        if (meshRenderingEntry.meshName == meshName) {
            meshRenderingEntry.drawingEnabled = false;
            return;
        }
    }
    //std::cout << "DisableDrawingForMeshByMeshName() called but name " << meshName << " was not found!\n";
}


void AnimatedGameObject::PrintNodeNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_skinnedModel->m_nodes.size(); i++) {
        std::cout << "-" << i << " " << m_skinnedModel->m_nodes[i].name << "\n";
    }
}

void AnimatedGameObject::PrintMeshNames() {
    std::cout << m_skinnedModel->GetName() << "\n";
    for (int i = 0; i < m_meshRenderingEntries.size(); i++) {
        std::cout << "-" << i << " " << m_meshRenderingEntries[i].meshName << "\n";
    }
}


void AnimatedGameObject::EnableModelMatrixOverride() {
    m_useModelMatrixOverride = true;
}


void AnimatedGameObject::SetCameraMatrix(const glm::mat4& matrix) {
    m_modelMatrixOverride = matrix;
}


const uint32_t AnimatedGameObject::GetAnimationFrameNumber(const std::string& animationLayerName) {
    return m_animator.GetAnimationFrameNumber(animationLayerName);
}


bool AnimatedGameObject::AnimationIsPastFrameNumber(const std::string& animationLayerName, int frameNumber) {
    return m_animator.AnimationIsPastFrameNumber(animationLayerName, frameNumber);
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
            const glm::mat4& boneWorldMatrix = m_animator.m_globalBlendedNodeTransforms[i];
            const glm::mat4& parentBoneWorldMatrix = m_animator.m_globalBlendedNodeTransforms[parentIndex];
            glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            glm::vec3 parentPosition = GetModelMatrix() * parentBoneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            Renderer::DrawPoint(position, OUTLINE_COLOR, false, exclusiveViewportIndex);
            Renderer::DrawLine(position, parentPosition, WHITE, false, exclusiveViewportIndex);
        }
    }

    // // To draw all nodes
    // for (const glm::mat4& boneWorldMatrix : m_animationLayer.m_globalBlendedNodeTransforms) {
    //     glm::vec3 position = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    //     Renderer::DrawPoint(position, color, false, exclusiveViewportIndex);
    // }
}


void AnimatedGameObject::DrawBoneTangentVectors(float size, int exclusiveViewportIndex) {
    for (const glm::mat4& boneWorldMatrix : m_animator.m_globalBlendedNodeTransforms) {
        glm::vec3 origin = GetModelMatrix() * boneWorldMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        glm::vec3 right = glm::normalize(glm::vec3(boneWorldMatrix[0]));
        glm::vec3 up = glm::normalize(glm::vec3(boneWorldMatrix[1]));
        glm::vec3 forward = glm::normalize(glm::vec3(boneWorldMatrix[2]));
        Renderer::DrawLine(origin, origin + (forward * size), BLUE, false, exclusiveViewportIndex);
        Renderer::DrawLine(origin, origin + (up * size), GREEN, false, exclusiveViewportIndex);
        Renderer::DrawLine(origin, origin + (right * size), RED, false, exclusiveViewportIndex);
    }
}


void AnimatedGameObject::SetExclusiveViewportIndex(int index) {
    m_exclusiveViewportIndex = index;
}


void AnimatedGameObject::SetIgnoredViewportIndex(int index) {
    m_ignoredViewportIndex = index;
}


bool AnimatedGameObject::AnimationByNameIsComplete(const std::string& name) {
    return m_animator.AnimationIsCompleteAnyLayer(name);
}


const glm::mat4& AnimatedGameObject::GetGlobalBlendedNodeTransfrom(const std::string& nodeName) {
    uint32_t nodeIndex = GetNodeIndex(nodeName);

    if (nodeIndex >= 0 && nodeIndex < (uint32_t)m_animator.m_globalBlendedNodeTransforms.size()) {
        return m_animator.m_globalBlendedNodeTransforms[nodeIndex];
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
    if (nodeIndex == -1 || m_animator.m_globalBlendedNodeTransforms.empty()) {
        return glm::mat4(1.0f);
    }
    else {
        return GetModelMatrix() * m_animator.m_globalBlendedNodeTransforms[nodeIndex];
    }
}


const glm::vec3 AnimatedGameObject::GetBoneWorldPosition(const std::string& boneName) {
    return GetBoneWorldMatrix(boneName)[3];
}


void AnimatedGameObject::SetRagdollV2Id(uint64_t ragdollV2Id) {
    m_ragdollV2Id = ragdollV2Id;
}
