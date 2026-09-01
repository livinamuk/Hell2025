#pragma once
#include "Hell/Math/AABB.h"
#include "Hell/ResourceManagement/Types/SkinnedModel.h"

#include "Unloved/Objects/ObjectEnums.h"
#include "Unloved/Objects/Renderables/AnimatedMeshNodes.h"
#include "Unloved/Objects/Renderables/BoneSegment.h"

#include <map>
#include <string>

struct PhysicsFilterData;
struct Ragdoll;
struct RagdollAsset;

namespace Unloved {

struct SkinnedGameObject {
    enum class AnimationMode { BINDPOSE, ANIMATION, RAGDOLL_V2 };

    SkinnedGameObject() = default;
    SkinnedGameObject(uint64_t id);
    SkinnedGameObject(const SkinnedGameObject&) = delete;
    SkinnedGameObject& operator=(const SkinnedGameObject&) = delete;
    SkinnedGameObject(SkinnedGameObject&&) noexcept = default;
    SkinnedGameObject& operator=(SkinnedGameObject&&) noexcept = default;
    ~SkinnedGameObject() = default;

    void CleanUp();
    void UpdateRenderItems();
    void FinalizeAnimation();
    void SetName(std::string name);
    void SetAnimatorInstanceId(uint64_t animatorInstanceId)                           { m_animatorInstanceId = animatorInstanceId; }
    void SetOwnerObjectId(uint64_t objectId)                                           { m_ownerObjectId = objectId; }
    void SetSkinnedModel(const std::string& skinnedModelName, const std::string& presetName = UNDEFINED_STRING);
    void SetSkinnedModel(const std::string& skinnedModelName, SkinnedModelPreset preset);
    void SetScale(float scale);
    void SetPosition(glm::vec3 position);
    void SetRotationX(float rotation);
    void SetRotationY(float rotation);
    void SetRotationZ(float rotation);
    void SetAnimationModeToAnimated();
    void SetAnimationModeToBindPose();
    void SetAnimationModeToRagdoll();
    void SetBlendingModeByMeshName(const std::string& meshName, BlendingMode blendingMode);
    void SetMeshMaterialByMeshName(const std::string& meshName, const std::string& materialName);
    void SetMeshMaterialByMeshIndex(int meshIndex, const std::string& materialName);
    void SetMeshWoundMaterialByMeshName(const std::string& meshName, const std::string& textureName);
    void SetMeshWoundMaskArrayIndex(const std::string& meshName, int32_t woundMaskArrayIndex);
    void SetAllMeshMaterials(const std::string& materialName);
    void SetAllMeshBlendingModes(BlendingMode blendingMode);
    void SetExcludeFromVulkanTLAS(bool exclude);
    void SetViewWeapon(bool viewWeapon)                                               { m_isViewWeapon = viewWeapon; }

    void EnableModelMatrixOverride();
    void SetCameraMatrix(const glm::mat4& matrix);
    void DrawBones(int exclusiveViewportIndex = -1);
    void DrawBoneTangentVectors(float size = 0.1f, int exclusiveViewportIndex = -1);
    void SetExclusiveViewportIndex(int index);
    void SetIgnoredViewportIndex(int index);
    void PrintNodeNames();
    void PrintMeshNames();
    uint64_t CreateRagdoll(const std::string& ragdollName, const PhysicsFilterData& filterData);
    uint64_t CreateRagdoll(const RagdollAsset& asset, const PhysicsFilterData& filterData);
    void RemoveRagdoll();

    bool HasRagdoll() const                                                               { return m_ragdollId != 0; }
    void EnableRendering();
    void DisableRendering();

    void EnableShadows() { m_castsShadows = true; }
    void DisableShadows() { m_castsShadows = false; }

    const glm::mat4 GetModelMatrix();
    glm::mat4 GetNodeWorldMatrix(const std::string& nodeName);
    glm::vec3 GetNodeWorldPosition(const std::string& nodeName);
    glm::mat4 GetNodeModelSpaceMatrix(const std::string& nodeName);
    const glm::mat4& GetLocalBindTransformByNodeName(const std::string& name);
    const uint32_t GetVerteXCount();

    int32_t GetBoneIndex(const std::string& boneName);
    int32_t GetNodeIndex(const std::string& nodeName);

    bool CastsShadows() const                                                         { return m_castsShadows; }
    bool IsViewWeapon() const                                                         { return m_isViewWeapon; }

    // Sketchy, only used by shark currently
    const glm::vec3& GetPosition() const                                              { return m_transform.position;  }

    SkinnedModel* GetSkinnedModel()                                                   { return m_skinnedModel; }
    Ragdoll* GetRagdoll();
    AnimatedMeshNodes& GetAnimatedMeshNodes()                                         { return m_animatedMeshNodes; }

    bool RenderingEnabled()                                                           { return m_animatedMeshNodes.RenderingEnabled(); }
    const uint64_t& GetObjectId() const                                               { return m_objectId; }
    uint64_t GetAnimatorInstanceId() const                                             { return m_animatorInstanceId; }
    uint64_t GetOwnerObjectId() const                                                 { return m_ownerObjectId ? m_ownerObjectId : m_objectId; }
    uint64_t GetRagdollId() const                                                     { return m_ragdollId; }
    const uint32_t GetBaseTransfromIndex() const                                      { return baseTransformIndex; }
    const uint32_t& GetIgnoredViewportIndex() const                                   { return m_animatedMeshNodes.GetIgnoredViewportIndex(); };
    const uint32_t& GetExclusiveViewportIndex() const                                 { return m_animatedMeshNodes.GetExclusiveViewportIndex(); };
    const glm::vec3 GetScale() const                                                  { return m_transform.scale; }
    const std::vector<RenderItem>& GetDeformingRenderItems() const                    { return m_animatedMeshNodes.m_deformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItems() const                 { return m_animatedMeshNodes.m_nonDeformingRenderItems; }
    const std::vector<RenderItem>& GetNonDeformingRenderItemsDepthPeeledTransparent() { return m_animatedMeshNodes.m_nonDeformingRenderItemsDepthPeeledTransparent; }
    const std::vector<glm::mat4>& GetBoneSkinningMatrices();
    const std::map<std::string, float>& GetMorphTargetWeights();
    const std::vector<glm::mat4>& GetPreviousRenderBoneSkinningMatrices() const        { return m_previousRenderBoneSkinningMatrices; }
    const std::map<std::string, float>& GetPreviousRenderMorphTargetWeights() const    { return m_previousRenderMorphTargetWeights; }
    const glm::mat4& GetPreviousRenderModelMatrix() const                              { return m_previousRenderModelMatrix; }
    bool HasRenderPoseHistory() const                                                  { return m_hasRenderPoseHistory; }
    void CommitRenderPoseHistory();
    const std::string& GetName() const                                                { return m_name; }
    const std::string& GetEditorName() const                                          { return m_name; }
    const glm::mat4 GetModelMatrixOverride() const                                    { return m_modelMatrixOverride; }
    const AABB& GetSkinnedAABB() const                                                { return m_skinnedAABB; }

private:
    void SetSkinnedModelResource(const std::string& name);
    void UpdateBoneTransformsFromRagdoll();
    void SyncRagdollToAnimation();
    void ComputeBoneSegments();
    void CalculateSkinnedAABB();
    void UpdateDirtyBounds();

    AnimationMode m_animationMode = AnimationMode::BINDPOSE;
    SkinnedModel* m_skinnedModel = nullptr;
    Hell::Transform m_transform;
    glm::mat4 m_modelMatrixOverride = glm::mat4(1);
    std::string m_name = "";

    AnimatedMeshNodes m_animatedMeshNodes;

    uint64_t m_objectId = 0;
    uint64_t m_animatorInstanceId = 0;
    uint64_t m_ownerObjectId = 0;
    uint64_t m_ragdollId = 0;
    uint32_t baseTransformIndex = -1;
    bool m_useModelMatrixOverride = false;
    bool m_castsShadows = true;
    bool m_isViewWeapon = false;

    std::vector<BoneSegment> m_boneSegments;
    std::vector<glm::mat4> m_ragdollWorldNodeTransforms;
    std::vector<glm::mat4> m_ragdollBoneSkinningMatrices;
    std::vector<glm::mat4> m_previousRenderBoneSkinningMatrices;
    std::map<std::string, float> m_previousRenderMorphTargetWeights;
    glm::mat4 m_previousRenderModelMatrix = glm::mat4(1.0f);
    bool m_hasRenderPoseHistory = false;
    AABB m_skinnedAABB;
    AABB m_skinnedAABBLastFrame;
    float m_skinnedAABBThreshold = 0.1f;
    float m_skinnedAABBChangeThreshold = 0.01f;
    bool m_hasSkinnedAABBLastFrame = false;
};
}
