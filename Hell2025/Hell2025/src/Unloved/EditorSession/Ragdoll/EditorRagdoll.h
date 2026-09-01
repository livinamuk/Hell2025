#pragma once

#include "Hell/Physics/Ragdoll/RagdollAsset.h"
#include "Unloved/Render/RendererTypes.h"

#include <string>
#include <vector>

struct SkinnedModel;

namespace Unloved::EditorSession::RagdollEditor {

    enum class RagdollEditMode {
        NONE,
        LIMIT,
        SHAPE_TRANSLATE,
        SHAPE_ROTATE,
        LIMIT_FRAME_TRANSLATE,
        LIMIT_FRAME_ROTATE
    };

    struct RagdollParentOption {
        RagdollMarkerId markerId = INVALID_RAGDOLL_MARKER_ID;
        std::string label;
    };

    bool New(const std::string& name, std::string& error);
    bool OpenNative(const std::string& path, std::string& error);
    bool ImportLegacy(const std::string& path, std::string& error);
    bool Save(std::string& error);
    bool SaveAs(const std::string& name, std::string& error);
    bool RevertFromDisk(std::string& error);
    bool SetSkinnedModelPresetName(const std::string& name);
    bool SetSkinnedModelScale(float scale);
    bool RetargetMarkersToCurrentBindPose(std::string& error);
    bool RetargetMarkersToCurrentBindPosePreserveLimitAxes(std::string& error);
    bool SetTestAnimationName(const std::string& name);
    bool SetTargetMass(float mass);
    bool DistributeMassByVolume(std::string& error);
    bool RandomizeMarkerColors();
    bool RandomizeSelectedMarkerColor();
    bool SetSelectedMarkerMassOverrideEnabled(bool enabled, std::string& error);
    bool SetSelectedMarkerMass(float mass, std::string& error);
    bool SetSelectedMarkerLinearDamping(float damping);
    bool SetSelectedMarkerAngularDamping(float damping);
    bool SetSelectedMarkerFriction(float friction);
    bool SetSelectedMarkerShapeType(RagdollShapeType type);
    bool SetSelectedMarkerBoxDimensions(const glm::vec3& dimensions);
    bool SetSelectedMarkerRadius(float radius);
    bool SetSelectedMarkerCapsuleLength(float length);
    bool SetSelectedMarkerShapeOffset(const glm::vec3& offset);
    bool SetSelectedMarkerShapeRotation(const glm::vec3& rotationDegrees);
    bool CreateMarkerForSelectedBone(std::string& error);
    bool DeleteSelectedMarker(std::string& error);
    bool SetSelectedBoneParent(RagdollMarkerId parentMarkerId);
    bool SetSelectedMarkerParent(RagdollMarkerId parentMarkerId, std::string& error);
    bool SetSelectedJointLimitsEnabled(bool enabled);
    bool SetSelectedJointAngularLimit(int32_t axisIndex, RagdollAxisMotion motion, float halfRange);
    bool SetSelectedJointLimitFrameRotation(const glm::vec3& rotationDegrees);
    bool ResetSelectedJointConstraintFrames();
    bool SetSkinnedModelVisible(bool visible);
    void SetSkeletonVisible(bool visible);
    void SetLimitsAlwaysVisible(bool visible);
    void SetLimitFramesAlwaysVisible(bool visible);
    void SetLimitScale(float scale);
    void SetLimitHandleScale(float scale);
    void Reset();
    void DrawSkeleton();
    void DrawJointLimits();
    void UpdateInput(bool allowKeyboardInput, bool allowMouseInput);
    void SubmitRenderItems();
    void SelectBone(int32_t nodeIndex);
    void SelectMarker(RagdollMarkerId markerId);
    void ClearSelection();

    bool HasDocument();
    bool HasSelectedMarker();
    bool HasSelectedBone();
    bool CanSelectBone(int32_t nodeIndex);
    bool IsBoneNodeSelected(int32_t nodeIndex);
    bool IsDirty();
    bool IsSkinnedModelVisible();
    bool IsSkeletonVisible();
    RagdollEditMode GetEditMode();
    bool AreLimitsAlwaysVisible();
    bool AreLimitFramesAlwaysVisible();
    float GetLimitScale();
    float GetLimitHandleScale();
    float GetTargetMass();
    float GetCurrentMass();
    float GetSelectedMarkerVolume();
    glm::vec3 GetSelectedJointLimitFrameRotation();
    int32_t GetSelectedBoneNodeIndex();
    RagdollMarkerId GetSelectedBoneParentId();
    RagdollMarkerId GetSelectedMarkerParentId();
    const RagdollAsset& GetAsset();
    std::vector<RagdollParentOption> GetValidParentOptions(RagdollMarkerId childMarkerId);
    const RagdollMarkerAsset* GetSelectedMarker();
    const std::vector<RenderItem>& GetRenderItems();
    const SkinnedModel* GetSkinnedModel();
    RagdollMarkerId GetSelectedMarkerId();
    std::string GetSelectedBoneName();
    std::string GetSelectedBonePath();
    const std::string& GetName();
    const std::string& GetSourcePath();
    const std::vector<std::string>& GetImportWarnings();
}
