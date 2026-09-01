#include "EditorInspector.h"
#include "Unloved/EditorSession/Inspector/EditorInspectorInternal.h"

#include "Unloved/EditorSession/UI/EditorDialogs.h"
#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "EditorHierarchy.h"
#include "Unloved/EditorSession/UI/EditorInputElements.h"
#include "EditorObjectOptions.h"
#include "Unloved/EditorSession/Interaction/EditorPointSequences.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/EditorSession/BoneMask/EditorBoneMask.h"
#include "Unloved/EditorSession/Ragdoll/EditorRagdoll.h"
#include "EditorSession.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/Common/Enum.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Spawns/HouseLocation.h"
#include "Unloved/Objects/Spawns/SpawnPoint.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <cstdio>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>

namespace Unloved::EditorSession::Inspector {
    namespace {
        constexpr const char* NO_SKINNED_MODEL = "None";
        constexpr const char* NO_RAGDOLL_TEST_OPTION = "None";

        bool AnimationMatchesSkinnedModel(const std::string& animationName, const std::string& skinnedModelName) {
            return !skinnedModelName.empty() && animationName.starts_with(skinnedModelName);
        }

        void SetEditorName(uint64_t objectId, const std::string& editorName) {
            if (!World::SetEditorNameById(objectId, editorName)) {
                Dialog::Open("Name '" + editorName + "' Taken");
            }
        }

        void SetEditorPosition(uint64_t objectId, const glm::vec3& position) {
            if (World::SetPositionById(objectId, position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetEditorYaw(uint64_t objectId, float rotation) {
            const glm::vec3 fullRotation = glm::vec3(0.0f, rotation, 0.0f);
            if (World::SetRotationById(objectId, fullRotation)) {
                Gizmo::SetRotation(fullRotation);
            }
        }

        void SetEditorRotation(uint64_t objectId, const glm::vec3& rotation) {
            if (World::SetRotationById(objectId, rotation)) {
                Gizmo::SetRotation(rotation);
            }
        }

        void SetSpawnPointRotation(uint64_t objectId, const glm::vec2& rotation) {
            SetEditorRotation(objectId, glm::vec3(rotation, 0.0f));
        }

        void SetSpawnPointFromPlayerCamera(uint64_t objectId) {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player) return;

            SetEditorPosition(objectId, player->GetCamera().GetPosition());
            SetEditorRotation(objectId, player->GetCamera().GetEulerRotation());
        }

        void TestSpawnPoint(SpawnPoint* spawnPoint) {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player || !spawnPoint) return;

            player->SetFootPosition(spawnPoint->GetPosition() - glm::vec3(0.0f, 1.6f, 0.0f));
            player->GetCamera().SetEulerRotation(spawnPoint->GetCameraEuler());
            Unloved::EditorSession::Close();
        }

        void SetMermaidShopViewFromPlayerCamera(Mermaid* mermaid) {
            Player* player = Session::GetLocalPlayerByViewportIndex(0);
            if (!player || !mermaid) return;

            mermaid->SetShopTeleportPosition(player->GetCameraPosition());
            mermaid->SetShopTeleportEuler(player->GetCamera().GetEulerRotation());
        }

        void SetWallType(Wall* wall, const std::string& wallType, const std::string& materialName) {
            const WallType selectedType = Hell::Enum::FromString(wallType, WallType::UNDEFINED);
            wall->SetWallType(selectedType);

            const std::vector<std::string>& interiorMaterials = ObjectOptions::GetInteriorMaterials();
            const std::vector<std::string>& weatherBoardMaterials = ObjectOptions::GetWeatherBoardMaterials();

            if (selectedType == WallType::WEATHER_BOARDS && std::find(weatherBoardMaterials.begin(), weatherBoardMaterials.end(), materialName) == weatherBoardMaterials.end()) {
                Internal::ApplyWeatherBoardMaterialDefaults(wall, weatherBoardMaterials.front());
            }
            if (selectedType == WallType::INTERIOR && std::find(interiorMaterials.begin(), interiorMaterials.end(), materialName) == interiorMaterials.end()) {
                wall->SetMaterial(interiorMaterials.front());
            }
        }

        void SetWorkspaceMapName(const std::string& name) {
            if (!Workspace::SetMapName(name)) {
                Dialog::Open("Name '" + name + "' Taken");
            }
        }

        void SetWorkspaceHouseName(const std::string& name) {
            if (!Workspace::SetHouseName(name)) {
                Dialog::Open("Name '" + name + "' Taken");
            }
        }

        void RevertMapFromDisk() {
            if (Workspace::RevertMap()) {
                Hierarchy::Refresh();
            }
        }

        void RevertHouseFromDisk() {
            if (Workspace::RevertHouse()) {
                Hierarchy::Refresh();
            }
        }

        void SetWorldPlaneRotation(WorldPlane* worldPlane, const glm::vec3& rotation) {
            if (worldPlane->SetRotation(rotation)) {
                Gizmo::SetRotation(worldPlane->GetRotation());
            }
        }

        void SetWorldPlanePoint(WorldPlane* worldPlane, int32_t pointIndex, const glm::vec3& position) {
            if (worldPlane->SetPointPosition(pointIndex, position)) {
                Gizmo::SetPosition(worldPlane->GetWorldSpaceCenter());
            }
        }

        void SetSequencePointPosition(uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType, const glm::vec3& position) {
            if (PointSequences::SetPointPosition(objectId, pointIndex, handleType, position)) {
                glm::vec3 actualPosition;
                if (PointSequences::GetPointPosition(objectId, pointIndex, handleType, actualPosition)) {
                    Gizmo::SetPosition(actualPosition);
                }
            }
        }

        void SetWallSegmentPoint(Wall* wall, int32_t pointIndex, const glm::vec3& position, const glm::vec3& otherPosition) {
            if (wall->UpdatePointPosition(pointIndex, position)) {
                Gizmo::SetPosition((position + otherPosition) * 0.5f);
            }
        }

        void AddNameProperty(InputElements::PropertyList& properties, uint64_t objectId, std::string& editorName) {
            properties.String(objectId, "Name", editorName, [objectId, editorName = &editorName] { SetEditorName(objectId, *editorName); });
        }

        void AddPositionProperty(InputElements::PropertyList& properties, uint64_t objectId, glm::vec3& position) {
            properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetEditorPosition(objectId, *position); });
        }

        void AddYawProperty(InputElements::PropertyList& properties, uint64_t objectId, float& rotation) {
            properties.Float(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetEditorYaw(objectId, *rotation); });
        }

        void AddEulerRotationProperty(InputElements::PropertyList& properties, uint64_t objectId, glm::vec3& rotation) {
            properties.Vec3(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetEditorRotation(objectId, *rotation); });
        }

        std::vector<std::string> GetSkinnedModelNames() {
            std::vector<std::string> names = { NO_SKINNED_MODEL };
            std::vector<std::string> modelNames = RagdollEditor::GetAvailableSkinnedModelNames();
            names.insert(names.end(), modelNames.begin(), modelNames.end());
            return names;
        }

        void SetRagdollSkinnedModel(const std::string& selectedName) {
            std::string modelName = selectedName;
            if (modelName == NO_SKINNED_MODEL) {
                modelName.clear();
            }

            if (RagdollEditor::SetSkinnedModelName(modelName)) {
                const std::string& testAnimationName = RagdollEditor::GetAsset().testAnimationName;
                if (!testAnimationName.empty() && !AnimationMatchesSkinnedModel(testAnimationName, modelName)) {
                    RagdollEditor::SetTestAnimationName({});
                }
                Hierarchy::Refresh();
            }
        }

        std::string FormatRagdollVec3(const glm::vec3& value) {
            char text[96];
            std::snprintf(text, sizeof(text), "%.3f, %.3f, %.3f", value.x, value.y, value.z);
            return text;
        }

        std::string FormatRagdollFloat(float value) {
            char text[48];
            std::snprintf(text, sizeof(text), "%.4f", value);
            return text;
        }

        void SetSelectedRagdollMassOverride(bool enabled) {
            std::string error;
            if (!RagdollEditor::SetSelectedMarkerMassOverrideEnabled(enabled, error) && !error.empty()) Dialog::Open(error);
        }

        void SetSelectedRagdollMass(float mass) {
            std::string error;
            if (!RagdollEditor::SetSelectedMarkerMass(mass, error) && !error.empty()) Dialog::Open(error);
        }

        void DistributeRagdollMass() {
            std::string error;
            if (!RagdollEditor::DistributeMassByVolume(error) && !error.empty()) Dialog::Open(error);
        }

        void RetargetRagdollMarkersToCurrentBindPose() {
            std::string error;
            if (!RagdollEditor::RetargetMarkersToCurrentBindPose(error) && !error.empty()) Dialog::Open(error);
        }

        void RetargetRagdollMarkersToCurrentBindPosePreserveLimitAxes() {
            std::string error;
            if (!RagdollEditor::RetargetMarkersToCurrentBindPosePreserveLimitAxes(error) && !error.empty()) Dialog::Open(error);
        }

        void RevertRagdollFromDisk() {
            std::string error;
            if (!RagdollEditor::RevertFromDisk(error) && !error.empty()) Dialog::Open(error);
        }

        const char* GetRagdollShapeName(RagdollShapeType type) {
            switch (type) {
                case RagdollShapeType::BOX:         return "Box";
                case RagdollShapeType::SPHERE:      return "Sphere";
                case RagdollShapeType::CAPSULE:     return "Capsule";
                case RagdollShapeType::CONVEX_HULL: return "Convex Hull (Imported)";
            }

            return "Unknown";
        }

        std::vector<std::string> GetRagdollShapeNames(const RagdollMarkerAsset& marker) {
            std::vector<std::string> names = {
                "Box",
                "Sphere",
                "Capsule"
            };

            const bool hasImportedConvexHull = !marker.shape.convexVertices.empty() && !marker.shape.convexIndices.empty();
            if (marker.shape.type == RagdollShapeType::CONVEX_HULL || hasImportedConvexHull) {
                names.push_back("Convex Hull (Imported)");
            }

            return names;
        }

        void SetSelectedRagdollShape(const std::string& shapeName) {
            if (shapeName == "Box") {
                RagdollEditor::SetSelectedMarkerShapeType(RagdollShapeType::BOX);
            }
            else if (shapeName == "Sphere") {
                RagdollEditor::SetSelectedMarkerShapeType(RagdollShapeType::SPHERE);
            }
            else if (shapeName == "Capsule") {
                RagdollEditor::SetSelectedMarkerShapeType(RagdollShapeType::CAPSULE);
            }
            else if (shapeName == "Convex Hull (Imported)") {
                RagdollEditor::SetSelectedMarkerShapeType(RagdollShapeType::CONVEX_HULL);
            }
        }

        const RagdollMarkerAsset* GetRagdollMarkerById(RagdollMarkerId markerId) {
            for (const RagdollMarkerAsset& marker : RagdollEditor::GetAsset().markers) {
                if (marker.id == markerId) return &marker;
            }

            return nullptr;
        }

        std::string GetRagdollMarkerLabel(RagdollMarkerId markerId) {
            const RagdollMarkerAsset* marker = GetRagdollMarkerById(markerId);
            if (!marker) return "Marker " + std::to_string(markerId);
            if (!marker->name.empty()) return marker->name;
            if (!marker->boneName.empty()) return marker->boneName;
            return "Marker " + std::to_string(markerId);
        }

        const RagdollJointAsset* GetRagdollIncomingJoint(RagdollMarkerId markerId) {
            for (const RagdollJointAsset& joint : RagdollEditor::GetAsset().joints) {
                if (joint.childMarkerId == markerId) return &joint;
            }

            return nullptr;
        }

        std::vector<std::string> GetRagdollParentLabels(const std::vector<RagdollEditor::RagdollParentOption>& options) {
            std::vector<std::string> labels;
            labels.reserve(options.size());
            for (const RagdollEditor::RagdollParentOption& option : options) labels.push_back(option.label);
            return labels;
        }

        std::string GetRagdollParentLabel(const std::vector<RagdollEditor::RagdollParentOption>& options, RagdollMarkerId markerId) {
            for (const RagdollEditor::RagdollParentOption& option : options) {
                if (option.markerId == markerId) return option.label;
            }
            return "None";
        }

        RagdollMarkerId GetRagdollParentId(const std::vector<RagdollEditor::RagdollParentOption>& options, const std::string& label) {
            for (const RagdollEditor::RagdollParentOption& option : options) {
                if (option.label == label) return option.markerId;
            }
            return INVALID_RAGDOLL_MARKER_ID;
        }

        InputElements::AxisLimitValue GetRagdollAxisLimitValue(const RagdollAxisLimit& limit) {
            InputElements::AxisLimitValue value;
            value.enabled = limit.motion != RagdollAxisMotion::FREE;
            value.locked = limit.motion == RagdollAxisMotion::LOCKED;
            const float halfRange = limit.motion == RagdollAxisMotion::LIMITED ? limit.limit : 0.0f;
            value.minimumDegrees = glm::degrees(-halfRange);
            value.maximumDegrees = glm::degrees(halfRange);
            return value;
        }

        void SetSelectedRagdollAngularLimit(int32_t axisIndex, const InputElements::AxisLimitValue& value) {
            if (value.locked) {
                RagdollEditor::SetSelectedJointAngularLimit(axisIndex, RagdollAxisMotion::LOCKED, 0.0f);
            }
            else if (value.enabled) {
                const float halfRange = glm::radians((value.maximumDegrees - value.minimumDegrees) * 0.5f);
                RagdollEditor::SetSelectedJointAngularLimit(axisIndex, RagdollAxisMotion::LIMITED, halfRange);
            }
            else {
                RagdollEditor::SetSelectedJointAngularLimit(axisIndex, RagdollAxisMotion::FREE, 0.0f);
            }
        }

        std::vector<std::string> GetRagdollTestAnimationNames(const std::string& skinnedModelName) {
            std::vector<std::string> names;
            for (const auto& animation : Hell::ResourceManager::GetAnimations()) {
                if (AnimationMatchesSkinnedModel(animation.first, skinnedModelName)) {
                    names.push_back(animation.first);
                }
            }
            std::sort(names.begin(), names.end());
            names.insert(names.begin(), NO_RAGDOLL_TEST_OPTION);
            return names;
        }

        std::vector<std::string> GetRagdollTestMaterialPresetNames() {
            std::vector<std::string> names = { NO_RAGDOLL_TEST_OPTION };
            const std::vector<std::string> presetNames = Bible::GetAnimatedMeshNodePresetNames();
            names.insert(names.end(), presetNames.begin(), presetNames.end());
            return names;
        }

        void SetRagdollTestAnimation(const std::string& selectedName) {
            RagdollEditor::SetTestAnimationName(selectedName == NO_RAGDOLL_TEST_OPTION ? std::string{} : selectedName);
        }

        void SetRagdollTestMaterialPreset(const std::string& selectedName) {
            RagdollEditor::SetTestMaterialPresetName(selectedName == NO_RAGDOLL_TEST_OPTION ? std::string{} : selectedName);
        }

        void SetRagdollSkinnedModelVisible(bool visible) {
            if (!RagdollEditor::SetSkinnedModelVisible(visible)) {
                Dialog::Open("Select a skinned model before showing the model preview");
            }
        }

        glm::vec3 GetRagdollMarkerRotation(const RagdollMarkerAsset& marker) {
            glm::vec3 basisX = glm::normalize(glm::vec3(marker.bodyTransform[0]));
            glm::vec3 basisY = glm::vec3(marker.bodyTransform[1]);
            basisY = glm::normalize(basisY - basisX * glm::dot(basisX, basisY));

            glm::mat3 rotation(1.0f);
            rotation[0] = basisX;
            rotation[1] = basisY;
            rotation[2] = glm::cross(basisX, basisY);
            return glm::degrees(glm::eulerAngles(glm::quat_cast(rotation)));
        }

        void RenderRagdollBoneProperties(const EditorRect& rect) {
            InputElements::PropertyList properties;
            if (!RagdollEditor::HasSelectedBone()) {
                properties.Render(rect);
                return;
            }

            const uint64_t propertyId = UINT64_MAX - 1 - static_cast<uint64_t>(RagdollEditor::GetSelectedBoneNodeIndex());
            const std::vector<RagdollEditor::RagdollParentOption> parentOptions = RagdollEditor::GetValidParentOptions(INVALID_RAGDOLL_MARKER_ID);
            const std::vector<std::string> parentLabels = GetRagdollParentLabels(parentOptions);
            std::string parentLabel = GetRagdollParentLabel(parentOptions, RagdollEditor::GetSelectedBoneParentId());

            properties.ReadOnly("Bone", RagdollEditor::GetSelectedBoneName());
            properties.ReadOnly("Path", RagdollEditor::GetSelectedBonePath());
            properties.DropDown(propertyId, "Parent Shape", parentLabels, parentLabel, [parentLabel = &parentLabel, parentOptions = &parentOptions] {
                RagdollEditor::SetSelectedBoneParent(GetRagdollParentId(*parentOptions, *parentLabel));
            });
            properties.Button("Create Shape", [] {
                std::string error;
                if (!RagdollEditor::CreateMarkerForSelectedBone(error)) {
                    Dialog::Open(error);
                    return;
                }
                Hierarchy::RefreshRagdollMarkers();
            });
            properties.Render(rect);
        }

        void RenderBoneMaskBoneProperties(const EditorRect& rect) {
            InputElements::PropertyList properties;
            if (!BoneMaskEditor::HasSelectedBone()) {
                properties.Render(rect);
                return;
            }

            constexpr uint32_t QUICK_SET_BUTTON_COUNT = 6;
            static const char* QUICK_SET_BUTTON_LABELS[QUICK_SET_BUTTON_COUNT] = { "Quick set 0.00", "Quick set 0.01", "Quick set 0.25", "Quick set 0.50", "Quick set 0.75", "Quick set 1.00" };
            static const float QUICK_SET_BUTTON_VALUES[QUICK_SET_BUTTON_COUNT] = { 0.0f, 0.01f, 0.25f, 0.5f, 0.75f, 1.0f };
            constexpr uint64_t PROPERTY_ID = UINT64_MAX - 1;
            const float previousWeight = BoneMaskEditor::GetSelectedBoneWeight();
            float weight = previousWeight;
            bool quickSetButtons[QUICK_SET_BUTTON_COUNT] = {};
            bool applyWeightToChildren = false;

            properties.ReadOnly("Bone", BoneMaskEditor::GetSelectedBoneName());
            properties.FloatSlider(PROPERTY_ID, "Weight", weight, 0.0f, 1.0f);
            for (uint32_t i = 0; i < QUICK_SET_BUTTON_COUNT; i++) {
                properties.Button(QUICK_SET_BUTTON_LABELS[i], quickSetButtons[i]);
            }
            properties.Button("Apply To Children", applyWeightToChildren);
            properties.Render(rect);

            for (uint32_t i = 0; i < QUICK_SET_BUTTON_COUNT; i++) {
                if (quickSetButtons[i]) weight = QUICK_SET_BUTTON_VALUES[i];
            }
            if (weight != previousWeight) BoneMaskEditor::SetSelectedBoneWeight(weight);
            if (applyWeightToChildren) BoneMaskEditor::ApplySelectedBoneWeightToChildren();
        }

        void RenderRagdollMarkerProperties(const EditorRect& rect) {
            const RagdollMarkerAsset* marker = RagdollEditor::GetSelectedMarker();
            InputElements::PropertyList properties;
            if (!marker) {
                properties.Render(rect);
                return;
            }

            const RagdollJointAsset* joint = GetRagdollIncomingJoint(marker->id);
            const std::string boneName = !marker->boneName.empty() ? marker->boneName : marker->bonePath;
            const glm::vec3 position = glm::vec3(marker->bodyTransform[3]);
            const glm::vec3 rotation = GetRagdollMarkerRotation(*marker);
            const std::vector<RagdollEditor::RagdollParentOption> parentOptions = RagdollEditor::GetValidParentOptions(marker->id);
            const std::vector<std::string> parentLabels = GetRagdollParentLabels(parentOptions);
            std::string parentLabel = GetRagdollParentLabel(parentOptions, RagdollEditor::GetSelectedMarkerParentId());
            const std::vector<std::string> shapeNames = GetRagdollShapeNames(*marker);
            std::string shapeName = GetRagdollShapeName(marker->shape.type);
            glm::vec3 shapeOffset = marker->shape.offset;
            glm::vec3 shapeRotation = glm::degrees(marker->shape.rotationRadians);
            glm::vec3 dimensions = marker->shape.extents;
            float radius = marker->shape.radius;
            float length = marker->shape.length;
            float mass = marker->rigidBody.mass;
            float linearDamping = marker->rigidBody.linearDamping;
            float angularDamping = marker->rigidBody.angularDamping;
            float friction = marker->rigidBody.friction;
            bool massOverride = marker->rigidBody.massMode == RagdollMassMode::OVERRIDE;
            bool limitsEnabled = joint && joint->limitEnabled;
            glm::vec3 limitFrameRotation = RagdollEditor::GetSelectedJointLimitFrameRotation();
            InputElements::AxisLimitValue twistLimit;
            InputElements::AxisLimitValue swing1Limit;
            InputElements::AxisLimitValue swing2Limit;
            if (joint) {
                twistLimit = GetRagdollAxisLimitValue(joint->angularLimits[0]);
                swing1Limit = GetRagdollAxisLimitValue(joint->angularLimits[1]);
                swing2Limit = GetRagdollAxisLimitValue(joint->angularLimits[2]);
            }

            properties.ReadOnly("Name", GetRagdollMarkerLabel(marker->id));
            properties.ReadOnly("Bone", boneName);
            properties.DropDown(marker->id, "Parent Shape", parentLabels, parentLabel, [parentLabel = &parentLabel, parentOptions = &parentOptions] {
                std::string error;
                if (!RagdollEditor::SetSelectedMarkerParent(GetRagdollParentId(*parentOptions, *parentLabel), error)) Dialog::Open(error);
            });
            properties.DropDown(marker->id, "Shape", shapeNames, shapeName, [shapeName = &shapeName] { SetSelectedRagdollShape(*shapeName); });
            properties.Button("Randomize Color", [] { RagdollEditor::RandomizeSelectedMarkerColor(); });
            properties.ReadOnly("Position", FormatRagdollVec3(position));
            properties.ReadOnly("Rotation", FormatRagdollVec3(rotation));

            if (marker->shape.type != RagdollShapeType::CONVEX_HULL) {
                properties.Vec3(marker->id, "Shape Offset", shapeOffset, [shapeOffset = &shapeOffset] { RagdollEditor::SetSelectedMarkerShapeOffset(*shapeOffset); });
                properties.Vec3(marker->id, "Shape Rotation", shapeRotation, [shapeRotation = &shapeRotation] { RagdollEditor::SetSelectedMarkerShapeRotation(*shapeRotation); });
            }

            switch (marker->shape.type) {
                case RagdollShapeType::BOX:
                    properties.Vec3(marker->id, "Dimensions", dimensions, [dimensions = &dimensions] { RagdollEditor::SetSelectedMarkerBoxDimensions(*dimensions); });
                    break;
                case RagdollShapeType::SPHERE:
                    properties.Float(marker->id, "Radius", radius, [radius = &radius] { RagdollEditor::SetSelectedMarkerRadius(*radius); });
                    break;
                case RagdollShapeType::CAPSULE:
                    properties.Float(marker->id, "Radius", radius, [radius = &radius] { RagdollEditor::SetSelectedMarkerRadius(*radius); });
                    properties.Float(marker->id, "Length", length, [length = &length] { RagdollEditor::SetSelectedMarkerCapsuleLength(*length); });
                    break;
                case RagdollShapeType::CONVEX_HULL:
                    properties.ReadOnly("Vertices", std::to_string(marker->shape.convexVertices.size()));
                    properties.ReadOnly("Triangles", std::to_string(marker->shape.convexIndices.size() / 3));
                    break;
            }

            properties.ReadOnly("Volume (m^3)", FormatRagdollFloat(RagdollEditor::GetSelectedMarkerVolume()));
            properties.CheckBox("Override Mass", massOverride, [massOverride = &massOverride] { SetSelectedRagdollMassOverride(*massOverride); });
            if (massOverride) {
                properties.Float(marker->id, "Mass (kg)", mass, [mass = &mass] { SetSelectedRagdollMass(*mass); });
            }
            else {
                properties.ReadOnly("Mass (kg)", FormatRagdollFloat(mass));
            }
            properties.Float(marker->id, "Linear Damping", linearDamping, [linearDamping = &linearDamping] { RagdollEditor::SetSelectedMarkerLinearDamping(*linearDamping); });
            properties.Float(marker->id, "Angular Damping", angularDamping, [angularDamping = &angularDamping] { RagdollEditor::SetSelectedMarkerAngularDamping(*angularDamping); });
            properties.Float(marker->id, "Friction", friction, [friction = &friction] { RagdollEditor::SetSelectedMarkerFriction(*friction); });

            if (joint) {
                properties.CheckBox("Limits Enabled", limitsEnabled, [limitsEnabled = &limitsEnabled] { RagdollEditor::SetSelectedJointLimitsEnabled(*limitsEnabled); });
                properties.Button("Reset Constraint Frames", [] { RagdollEditor::ResetSelectedJointConstraintFrames(); });
                properties.Vec3(marker->id, "Limit Frame Rotation", limitFrameRotation, [limitFrameRotation = &limitFrameRotation] { RagdollEditor::SetSelectedJointLimitFrameRotation(*limitFrameRotation); });
                properties.AxisLimit(marker->id, "Twist (X)", twistLimit, [twistLimit = &twistLimit] { SetSelectedRagdollAngularLimit(0, *twistLimit); });
                properties.AxisLimit(marker->id, "Swing 1 (Y)", swing1Limit, [swing1Limit = &swing1Limit] { SetSelectedRagdollAngularLimit(1, *swing1Limit); });
                properties.AxisLimit(marker->id, "Swing 2 (Z)", swing2Limit, [swing2Limit = &swing2Limit] { SetSelectedRagdollAngularLimit(2, *swing2Limit); });
            }

            properties.Render(rect);
        }

        void RenderRagdollWorkspaceProperties(const EditorRect& rect) {
            constexpr uint64_t WORKSPACE_PROPERTY_ID = UINT64_MAX;
            InputElements::PropertyList properties;
            if (!RagdollEditor::HasDocument()) {
                properties.Render(rect);
                return;
            }

            const std::vector<std::string> skinnedModelNames = GetSkinnedModelNames();
            const std::vector<std::string> testAnimationNames = GetRagdollTestAnimationNames(RagdollEditor::GetAsset().skinnedModelName);
            const std::vector<std::string> testMaterialPresetNames = GetRagdollTestMaterialPresetNames();
            std::string selectedName = RagdollEditor::GetAsset().skinnedModelName;
            if (selectedName.empty()) {
                selectedName = NO_SKINNED_MODEL;
            }
            std::string testAnimationName = RagdollEditor::GetAsset().testAnimationName;
            if (testAnimationName.empty()) {
                testAnimationName = NO_RAGDOLL_TEST_OPTION;
            }
            std::string testMaterialPresetName = RagdollEditor::GetAsset().testMaterialPresetName;
            if (testMaterialPresetName.empty()) {
                testMaterialPresetName = NO_RAGDOLL_TEST_OPTION;
            }

            bool showSkinnedModel = RagdollEditor::IsSkinnedModelVisible();
            bool showSkeleton = RagdollEditor::IsSkeletonVisible();
            bool alwaysShowLimits = RagdollEditor::AreLimitsAlwaysVisible();
            bool alwaysShowLimitFrames = RagdollEditor::AreLimitFramesAlwaysVisible();
            float limitScale = RagdollEditor::GetLimitScale();
            float limitHandleScale = RagdollEditor::GetLimitHandleScale();
            float skinnedModelScale = RagdollEditor::GetAsset().skinnedModelScale;
            float targetMass = RagdollEditor::GetTargetMass();
            properties.DropDown(WORKSPACE_PROPERTY_ID, "Skinned Model", skinnedModelNames, selectedName, [selectedName = &selectedName] { SetRagdollSkinnedModel(*selectedName); });
            properties.Float(WORKSPACE_PROPERTY_ID, "Skinned Model Scale", skinnedModelScale, [skinnedModelScale = &skinnedModelScale] { RagdollEditor::SetSkinnedModelScale(*skinnedModelScale); });
            properties.DropDown(WORKSPACE_PROPERTY_ID, "Test Animation", testAnimationNames, testAnimationName, [testAnimationName = &testAnimationName] { SetRagdollTestAnimation(*testAnimationName); });
            properties.DropDown(WORKSPACE_PROPERTY_ID, "Bible Preset", testMaterialPresetNames, testMaterialPresetName, [testMaterialPresetName = &testMaterialPresetName] { SetRagdollTestMaterialPreset(*testMaterialPresetName); });
            properties.Float(WORKSPACE_PROPERTY_ID, "Target Total Mass (kg)", targetMass, [targetMass = &targetMass] { RagdollEditor::SetTargetMass(*targetMass); });
            properties.ReadOnly("Current Total Mass (kg)", FormatRagdollFloat(RagdollEditor::GetCurrentMass()));
            properties.CheckBox("Show Skinned Model", showSkinnedModel, [showSkinnedModel = &showSkinnedModel] { SetRagdollSkinnedModelVisible(*showSkinnedModel); });
            properties.CheckBox("Show Skeleton", showSkeleton, [showSkeleton = &showSkeleton] { RagdollEditor::SetSkeletonVisible(*showSkeleton); });
            properties.CheckBox("Always Show Limits", alwaysShowLimits, [alwaysShowLimits = &alwaysShowLimits] { RagdollEditor::SetLimitsAlwaysVisible(*alwaysShowLimits); });
            properties.CheckBox("Always Show Limit Frames", alwaysShowLimitFrames, [alwaysShowLimitFrames = &alwaysShowLimitFrames] { RagdollEditor::SetLimitFramesAlwaysVisible(*alwaysShowLimitFrames); });
            properties.Float(WORKSPACE_PROPERTY_ID, "Limit Scale", limitScale, [limitScale = &limitScale] { RagdollEditor::SetLimitScale(*limitScale); });
            properties.Float(WORKSPACE_PROPERTY_ID, "Limit Handle Scale", limitHandleScale, [limitHandleScale = &limitHandleScale] { RagdollEditor::SetLimitHandleScale(*limitHandleScale); });
            properties.Button("Expand Hierarchy", [] { Hierarchy::SetAllNodesExpanded(true); });
            properties.Button("Collapse Hierarchy", [] { Hierarchy::SetAllNodesExpanded(false); });
            properties.Button("Retarget Markers To Current Bind Pose", RetargetRagdollMarkersToCurrentBindPose);
            properties.Button("Retarget Markers, Preserve Limit Axes", RetargetRagdollMarkersToCurrentBindPosePreserveLimitAxes);
            properties.Button("Distribute Mass By Volume", DistributeRagdollMass);
            properties.Button("Randomize Colors", [] { RagdollEditor::RandomizeMarkerColors(); });
            properties.Button("Revert from disk", RevertRagdollFromDisk);
            properties.Render(rect);
        }

        void RenderWorkspaceProperties(const EditorRect& rect) {
            if (Workspace::GetMode() == EditorSessionMode::BONE_MASK) {
                constexpr uint64_t WORKSPACE_PROPERTY_ID = UINT64_MAX;
                InputElements::PropertyList properties;
                std::vector<std::string> skinnedModelNames = { NO_SKINNED_MODEL };
                std::vector<std::string> availableSkinnedModelNames = BoneMaskEditor::GetAvailableSkinnedModelNames();
                skinnedModelNames.insert(skinnedModelNames.end(), availableSkinnedModelNames.begin(), availableSkinnedModelNames.end());

                std::string selectedName = BoneMaskEditor::GetSkinnedModelName();
                if (selectedName.empty()) selectedName = NO_SKINNED_MODEL;

                properties.DropDown(WORKSPACE_PROPERTY_ID, "Skinned Model", skinnedModelNames, selectedName);
                properties.Render(rect);

                if (selectedName == NO_SKINNED_MODEL) selectedName.clear();
                if (selectedName != BoneMaskEditor::GetSkinnedModelName() && BoneMaskEditor::SetSkinnedModelName(selectedName)) {
                    Hierarchy::Refresh();
                }
                return;
            }

            if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
                RenderRagdollWorkspaceProperties(rect);
                return;
            }

            constexpr uint64_t WORKSPACE_PROPERTY_ID = UINT64_MAX;
            InputElements::PropertyList properties;
            std::string name = Workspace::GetName();
            if (Workspace::GetMode() == EditorSessionMode::MAP) {
                properties.String(WORKSPACE_PROPERTY_ID, "Name", name, [name = &name] { SetWorkspaceMapName(*name); });
                uint32_t chunkWidth = Workspace::GetMapChunkWidth();
                uint32_t chunkDepth = Workspace::GetMapChunkDepth();
                properties.UInt(WORKSPACE_PROPERTY_ID, "Chunk Width", chunkWidth, [&] { Workspace::ResizeMap(chunkWidth, chunkDepth); });
                properties.UInt(WORKSPACE_PROPERTY_ID, "Chunk Depth", chunkDepth, [&] { Workspace::ResizeMap(chunkWidth, chunkDepth); });
                properties.Button("Reset height map", [] { Workspace::ResetHeightMap(); });
                properties.Button("Revert from disk", RevertMapFromDisk);
            }
            else {
                properties.String(WORKSPACE_PROPERTY_ID, "Name", name, [name = &name] { SetWorkspaceHouseName(*name); });
                properties.Button("Revert from disk", RevertHouseFromDisk);
            }
            properties.Render(rect);
        }

        void RenderWorldPlaneProperties(const EditorRect& rect, uint64_t objectId) {
            WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!worldPlane) {
                properties.Render(rect);
                return;
            }

            WorldPlaneCreateInfo& createInfo = worldPlane->GetCreateInfo();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = worldPlane->GetRotation();
            glm::vec3 p0 = worldPlane->GetPlanarQuad().GetPositionP0();
            glm::vec3 p1 = worldPlane->GetPlanarQuad().GetPositionP1();
            glm::vec3 p2 = worldPlane->GetPlanarQuad().GetPositionP2();
            glm::vec3 p3 = worldPlane->GetPlanarQuad().GetPositionP3();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Vec3(objectId, "Rotation", rotation, [worldPlane, rotation = &rotation] { SetWorldPlaneRotation(worldPlane, *rotation); });
            properties.Vec3(objectId, "P0", p0, [worldPlane, p0 = &p0] { SetWorldPlanePoint(worldPlane, 0, *p0); });
            properties.Vec3(objectId, "P1", p1, [worldPlane, p1 = &p1] { SetWorldPlanePoint(worldPlane, 1, *p1); });
            properties.Vec3(objectId, "P2", p2, [worldPlane, p2 = &p2] { SetWorldPlanePoint(worldPlane, 2, *p2); });
            properties.Vec3(objectId, "P3", p3, [worldPlane, p3 = &p3] { SetWorldPlanePoint(worldPlane, 3, *p3); });
            properties.Float(objectId, "Tex Scale", createInfo.textureScale, [&] { worldPlane->SetTextureScale(createInfo.textureScale); });
            properties.Float(objectId, "Tex Offset U", createInfo.textureOffsetU, [&] { worldPlane->SetTextureOffsetU(createInfo.textureOffsetU); });
            properties.Float(objectId, "Tex Offset V", createInfo.textureOffsetV, [&] { worldPlane->SetTextureOffsetV(createInfo.textureOffsetV); });
            properties.CheckBox("Tex Rotate", createInfo.rotateTexture90, [&] { worldPlane->SetRotateTexture90(createInfo.rotateTexture90); });
            properties.Float(objectId, "Roughness Factor", createInfo.roughnessFactor, [&] { worldPlane->SetRoughnessFactor(createInfo.roughnessFactor); });
            properties.Float(objectId, "Metallic Factor", createInfo.metallicFactor, [&] { worldPlane->SetMetallicFactor(createInfo.metallicFactor); });
            properties.Render(rect);
        }

        void RenderChristmasLightsProperties(const EditorRect& rect, uint64_t objectId) {
            ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!christmasLights) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float spacing = christmasLights->GetCreateInfo().spacing;
            float wireRadius = christmasLights->GetCreateInfo().wireRadius;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Float(objectId, "Spacing", spacing, [&] { christmasLights->SetSpacing(spacing); });
            properties.Float(objectId, "Wire Radius", wireRadius, [&] { christmasLights->SetWireRadius(wireRadius); });
            properties.Render(rect);
        }

        void RenderChristmasLightPointProperties(const EditorRect& rect, uint64_t objectId, int32_t pointIndex) {
            ChristmasLightSet* christmasLights = World::GetChristmasLightsByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!christmasLights || pointIndex < 0 || pointIndex >= static_cast<int32_t>(christmasLights->GetCreateInfo().sequencePoints.size())) {
                properties.Render(rect);
                return;
            }

            std::vector<SequencePoint> sequencePoints = christmasLights->GetCreateInfo().sequencePoints;
            SequencePoint& sequencePoint = sequencePoints[pointIndex];
            bool changed = false;

            properties.Vec3(objectId, "Position", sequencePoint.position, [&] { changed = true; });
            if (pointIndex > 0) {
                properties.Float(objectId, "Sag", sequencePoint.customFloat, [&] { changed = true; });
            }
            properties.Render(rect);

            if (changed) {
                christmasLights->UpdateSequencePoints(sequencePoints);
                Gizmo::SetPosition(sequencePoint.position);
            }
        }

        void RenderSequencePointProperties(const EditorRect& rect, uint64_t objectId, int32_t pointIndex, PointSequences::PointHandleType handleType) {
            InputElements::PropertyList properties;
            glm::vec3 position;
            if (!PointSequences::GetPointPosition(objectId, pointIndex, handleType, position)) {
                properties.Render(rect);
                return;
            }

            properties.Vec3(objectId, "Position", position, [objectId, pointIndex, handleType, position = &position] { SetSequencePointPosition(objectId, pointIndex, handleType, *position); });
            properties.Render(rect);
        }

        void SetGizmoToSelectedPoint(uint64_t objectId) {
            glm::vec3 position;
            if (PointSequences::GetPointPosition(objectId, Selection::GetSelectedPointIndex(), Selection::GetSelectedPointHandleType(), position)) {
                Gizmo::SetPosition(position);
            }
        }

        void SetDDGIPosition(uint64_t objectId, const glm::vec3& position) {
            if (World::SetPositionById(objectId, position)) {
                SetGizmoToSelectedPoint(objectId);
            }
        }

        void SetDDGIExtents(DDGIVolume* volume, uint64_t objectId, const glm::vec3& extents) {
            volume->SetExtents(extents);
            SetGizmoToSelectedPoint(objectId);
        }

        void RenderDDGIVolumeProperties(const EditorRect& rect, uint64_t objectId) {
            DDGIVolume* volume = World::GetDDGIVolumeByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!volume) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 extents = volume->GetExtents();
            float probeSpacing = volume->GetProbeSpacing();
            float pointCloudSpacing = volume->GetPointCloudSpacing();

            AddNameProperty(properties, objectId, editorName);
            properties.Vec3(objectId, "Position", position, [objectId, position = &position] { SetDDGIPosition(objectId, *position); });
            properties.Vec3(objectId, "Extents", extents, [volume, objectId, extents = &extents] { SetDDGIExtents(volume, objectId, *extents); });
            properties.Float(objectId, "Probe Spacing", probeSpacing, [&] { volume->SetProbeSpacing(probeSpacing); });
            properties.Float(objectId, "Point Cloud Spacing", pointCloudSpacing, [&] { volume->SetPointCloudSpacing(pointCloudSpacing); });
            properties.Render(rect);
        }

        void RenderDobermannProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderDoorProperties(const EditorRect& rect, uint64_t objectId) {
            Door* door = World::GetDoorByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!door) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> doorTypes = {
                Hell::Enum::ToString(DoorType::STANDARD_A),
                Hell::Enum::ToString(DoorType::STANDARD_B),
                Hell::Enum::ToString(DoorType::STAINED_GLASS),
                Hell::Enum::ToString(DoorType::STAINED_GLASS2)
            };
            static const std::vector<std::string> materialTypes = {
                Hell::Enum::ToString(DoorMaterialType::RESIDENT_EVIL),
                Hell::Enum::ToString(DoorMaterialType::WHITE_PAINT)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            std::string type = Hell::Enum::ToString(door->GetType());
            std::string frontMaterial = Hell::Enum::ToString(door->GetMaterialTypeFront());
            std::string backMaterial = Hell::Enum::ToString(door->GetMaterialTypeBack());
            std::string frameFrontMaterial = Hell::Enum::ToString(door->GetMaterialTypeFrameFront());
            std::string frameBackMaterial = Hell::Enum::ToString(door->GetMaterialTypeFrameBack());
            bool hasDeadLock = door->GetDeadLockState();
            bool hasSill = door->GetSillState();
            bool deadLockedAtStart = door->GetDeadLockedAtInitState();
            bool openAtStart = door->GetOpenAtStartState();
            float maxOpenValue = door->GetCreateInfo().maxOpenValue;
            float floorPlaneTextureScale = door->GetCreateInfo().floorPlaneTextureScale;
            float floorPlaneTextureOffsetU = door->GetCreateInfo().floorPlaneTextureOffsetU;
            float floorPlaneTextureOffsetV = door->GetCreateInfo().floorPlaneTextureOffsetV;
            bool floorPlaneRotateTexture90 = door->GetCreateInfo().floorPlaneRotateTexture90;
            float floorPlaneRoughnessFactor = door->GetCreateInfo().floorPlaneRoughnessFactor;
            float floorPlaneMetallicFactor = door->GetCreateInfo().floorPlaneMetallicFactor;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", doorTypes, type, [&] { door->SetType(Hell::Enum::FromString(type, DoorType::UNDEFINED)); });
            properties.DropDown(objectId, "Front Material", materialTypes, frontMaterial, [&] { door->SetFrontMaterial(Hell::Enum::FromString(frontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Back Material", materialTypes, backMaterial, [&] { door->SetBackMaterial(Hell::Enum::FromString(backMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Front Material", materialTypes, frameFrontMaterial, [&] { door->SetFrameFrontMaterial(Hell::Enum::FromString(frameFrontMaterial, DoorMaterialType::UNDEFINED)); });
            properties.DropDown(objectId, "Frame Back Material", materialTypes, frameBackMaterial, [&] { door->SetFrameBackMaterial(Hell::Enum::FromString(frameBackMaterial, DoorMaterialType::UNDEFINED)); });
            properties.Float(objectId, "Floor Tex Scale", floorPlaneTextureScale, [&] { door->SetFloorPlaneTextureScale(floorPlaneTextureScale); });
            properties.Float(objectId, "Floor Tex Offset U", floorPlaneTextureOffsetU, [&] { door->SetFloorPlaneTextureOffsetU(floorPlaneTextureOffsetU); });
            properties.Float(objectId, "Floor Tex Offset V", floorPlaneTextureOffsetV, [&] { door->SetFloorPlaneTextureOffsetV(floorPlaneTextureOffsetV); });
            properties.CheckBox("Floor Tex Rotate", floorPlaneRotateTexture90, [&] { door->SetFloorPlaneRotateTexture90(floorPlaneRotateTexture90); });
            properties.Float(objectId, "Floor Tex Roughness Factor", floorPlaneRoughnessFactor, [&] { door->SetFloorPlaneRoughnessFactor(floorPlaneRoughnessFactor); });
            properties.Float(objectId, "Floor Tex Metallic Factor", floorPlaneMetallicFactor, [&] { door->SetFloorPlaneMetallicFactor(floorPlaneMetallicFactor); });
            properties.CheckBox("Has Deadlock", hasDeadLock, [&] { door->SetDeadLockState(hasDeadLock); });
            properties.CheckBox("Deadlocked At Start", deadLockedAtStart, [&] { door->SetDeadLockedAtInitState(deadLockedAtStart); });
            properties.CheckBox("Open At Start", openAtStart, [&] { door->SetOpenAtStartState(openAtStart); });
            properties.Float(objectId, "Max Open", maxOpenValue, [&] { door->SetMaxOpenValue(maxOpenValue); });
            properties.CheckBox("Has Sill", hasSill, [&] { door->SetSillState(hasSill); });
            properties.Render(rect);
        }

        void RenderFireplaceProperties(const EditorRect& rect, uint64_t objectId) {
            Fireplace* fireplace = World::GetFireplaceById(objectId);
            InputElements::PropertyList properties;
            if (!fireplace) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> fireplaceTypes = {
                Hell::Enum::ToString(FireplaceType::DEFAULT),
                Hell::Enum::ToString(FireplaceType::WOOD_STOVE)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            std::string type = Hell::Enum::ToString(fireplace->GetCreateInfo().type);

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", fireplaceTypes, type, [&] { fireplace->SetType(Hell::Enum::FromString(type, FireplaceType::UNDEFINED)); });
            properties.Render(rect);
        }

        void RenderGenericObjectProperties(const EditorRect& rect, uint64_t objectId) {
            GenericObject* genericObject = World::GetGenericObjectById(objectId);
            InputElements::PropertyList properties;
            if (!genericObject) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> genericObjectTypes = {
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_TREE),
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_PRESENT_SMALL),
                Hell::Enum::ToString(GenericObjectType::CHRISTMAS_PRESENT_LARGE),
                Hell::Enum::ToString(GenericObjectType::DRAWERS_SMALL),
                Hell::Enum::ToString(GenericObjectType::DRAWERS_LARGE),
                Hell::Enum::ToString(GenericObjectType::TOILET),
                Hell::Enum::ToString(GenericObjectType::COUCH),
                Hell::Enum::ToString(GenericObjectType::BATHROOM_BASIN),
                Hell::Enum::ToString(GenericObjectType::BATHROOM_CABINET),
                Hell::Enum::ToString(GenericObjectType::CHAIR_RE),
                Hell::Enum::ToString(GenericObjectType::CHAIR_SPINDLE_BACK),
                Hell::Enum::ToString(GenericObjectType::DEER_HEAD),
                Hell::Enum::ToString(GenericObjectType::MERMAID_ROCK),
                Hell::Enum::ToString(GenericObjectType::PLANT_BLACKBERRIES),
                Hell::Enum::ToString(GenericObjectType::PLANT_TREE),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL2),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL3),
                Hell::Enum::ToString(GenericObjectType::TEST_MODEL4)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = genericObject->GetScale();
            std::string type = Hell::Enum::ToString(genericObject->GetType());

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { genericObject->SetScale(scale); });
            properties.DropDown(objectId, "Type", genericObjectTypes, type, [&] { genericObject->SetType(Hell::Enum::FromString(type, GenericObjectType::UNDEFINED)); });
            properties.Render(rect);
        }

        void RenderGenericAnimatedObjectProperties(const EditorRect& rect, uint64_t objectId) {
            GenericAnimatedObject* genericAnimatedObject = World::GetGenericAnimatedObjectById(objectId);
            InputElements::PropertyList properties;
            if (!genericAnimatedObject) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> genericAnimatedObjectTypes = {
                Hell::Enum::ToString(GenericAnimatedObjectType::RAT_KING)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            float scale = genericAnimatedObject->GetScale();
            std::string type = Hell::Enum::ToString(genericAnimatedObject->GetType());

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Float(objectId, "Scale", scale, [&] { genericAnimatedObject->SetScale(scale); });
            properties.DropDown(objectId, "Type", genericAnimatedObjectTypes, type, [&] { genericAnimatedObject->SetType(Hell::Enum::FromString(type, GenericAnimatedObjectType::UNDEFINED)); });
            properties.Render(rect);
        }

        void RenderJettyProperties(const EditorRect& rect, uint64_t objectId) {
            Jetty* jetty = World::GetJettyById(objectId);
            InputElements::PropertyList properties;
            if (!jetty) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = jetty->GetScale();
            uint32_t boardCount = jetty->GetBoardCount();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { jetty->SetScale(scale); });
            properties.UInt(objectId, "Board Count", boardCount, [&] { jetty->SetBoardCount(boardCount); });
            properties.Render(rect);
        }

        void RenderHouseLocationProperties(const EditorRect& rect, uint64_t objectId) {
            HouseLocation* houseLocation = World::GetHouseLocationByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!houseLocation) {
                properties.Render(rect);
                return;
            }

            const HouseLocationCreateInfo& createInfo = houseLocation->GetCreateInfo();
            const std::vector<std::string>& houseNames = ObjectOptions::GetHouseNames();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = createInfo.rotation;
            bool randomHouse = createInfo.randomHouse;
            std::string houseName = createInfo.houseName;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.CheckBox("Random House", randomHouse, [&] { houseLocation->SetRandomHouse(randomHouse); });
            if (!randomHouse) {
                properties.DropDown(objectId, "House", houseNames, houseName, [&] { houseLocation->SetHouseName(houseName); });
            }
            properties.Render(rect);
        }

        void RenderLadderProperties(const EditorRect& rect, uint64_t objectId) {
            Ladder* ladder = World::GetLadderByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!ladder) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderLightProperties(const EditorRect& rect, uint64_t objectId) {
            Light* light = World::GetLightByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!light) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> lightTypes = {
                Hell::Enum::ToString(LightType::HANGING_LIGHT),
                Hell::Enum::ToString(LightType::WALL_LAMP)
            };
            static const std::vector<std::string> iesProfileTypes = Hell::Enum::GetNames<IESProfileType>();

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 color = light->GetColor();
            glm::vec3 forward = light->GetForward();
            float radius = light->GetRadius();
            float strength = light->GetStrength();
            float iesExposure = light->GetIESExposure();
            float twist = light->GetTwist();
            std::string type = Hell::Enum::ToString(light->GetType());
            std::string iesProfileType = Hell::Enum::ToString(light->GetIESProfileType());

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.DropDown(objectId, "Type", lightTypes, type, [&] { light->SetType(Hell::Enum::FromString(type, LightType::HANGING_LIGHT)); });
            properties.Vec3(objectId, "Color", color, [&] { light->SetColor(color); });
            properties.Float(objectId, "Radius", radius, [&] { light->SetRadius(radius); });
            properties.Float(objectId, "Strength", strength, [&] { light->SetStrength(strength); });
            properties.DropDown(objectId, "IES Profile", iesProfileTypes, iesProfileType, [&] { light->SetIESProfileType(Hell::Enum::FromString(iesProfileType, IESProfileType::NONE)); });
            if (light->GetIESProfileType() != IESProfileType::NONE) {
                properties.Float(objectId, "IES Exposure", iesExposure, [&] { light->SetIESExposure(iesExposure); });
                properties.Vec3(objectId, "Forward", forward, [&] { light->SetForward(forward); });
                properties.Float(objectId, "Twist", twist, [&] { light->SetTwist(twist); });
            }
            properties.Render(rect);
        }

        void RenderMermaidProperties(const EditorRect& rect, uint64_t objectId) {
            Mermaid* mermaid = World::GetMermaidByObjectId(objectId);
            if (!mermaid) return;

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            glm::vec3 shopTeleportPosition = mermaid->GetShopTeleportPosition();
            glm::vec3 shopTeleportEuler = mermaid->GetShopTeleportEuler();
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Shop View Position", shopTeleportPosition, [&] { mermaid->SetShopTeleportPosition(shopTeleportPosition); });
            properties.Vec3(objectId, "Shop View Euler", shopTeleportEuler, [&] { mermaid->SetShopTeleportEuler(shopTeleportEuler); });
            properties.Button("Set shop view from player camera", [mermaid] { SetMermaidShopViewFromPlayerCamera(mermaid); });
            properties.Render(rect);
        }

        void RenderPickUpProperties(const EditorRect& rect, uint64_t objectId) {
            PickUp* pickUp = World::GetPickUpByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!pickUp) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            bool respawn = pickUp->GetRespawnState();
            bool disablePhysicsAtSpawn = pickUp->GetDisabledPhysicsAtSpawnState();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.CheckBox("Respawn", respawn, [&] { pickUp->SetRespawnState(respawn); });
            properties.CheckBox("Starts Frozen", disablePhysicsAtSpawn, [&] { pickUp->SetDisabledPhysicsAtSpawnState(disablePhysicsAtSpawn); });
            properties.Render(rect);
        }

        void RenderPictureFrameProperties(const EditorRect& rect, uint64_t objectId) {
            PictureFrame* pictureFrame = World::GetPictureFrameByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!pictureFrame) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> pictureFrameTypes = {
                Hell::Enum::ToString(PictureFrameType::BIG_LANDSCAPE),
                Hell::Enum::ToString(PictureFrameType::TALL_THIN),
                Hell::Enum::ToString(PictureFrameType::REGULAR_PORTRAIT),
                Hell::Enum::ToString(PictureFrameType::REGULAR_LANDSCAPE)
            };

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            glm::vec3 scale = pictureFrame->GetScale();
            std::string type = Hell::Enum::ToString(pictureFrame->GetType());
            bool useRandom = pictureFrame->GetCreateInfo().useRandom;
            std::string materialName = pictureFrame->GetCreateInfo().materialName;
            const std::vector<std::string> materialNames = HouseBuilder::GetLargePictureFrameMaterialNames();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { pictureFrame->SetScale(scale); });
            properties.DropDown(objectId, "Type", pictureFrameTypes, type, [&] { pictureFrame->SetType(Hell::Enum::FromString(type, PictureFrameType::UNDEFINED)); });
            if (pictureFrame->GetType() == PictureFrameType::BIG_LANDSCAPE) {
                properties.CheckBox("Random Material", useRandom, [&] { pictureFrame->SetUseRandom(useRandom); });
                if (!useRandom) {
                    properties.DropDown(objectId, "Material", materialNames, materialName, [&] { pictureFrame->SetMaterialName(materialName); });
                }
            }
            properties.Render(rect);
        }

        void RenderPianoProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderSpawnPointProperties(const EditorRect& rect, uint64_t objectId) {
            const ObjectType objectType = GetObjectIdType(objectId);
            SpawnPoint* spawnPoint = objectType == ObjectType::SPAWN_POINT_CAMPAIGN ? World::GetSpawnPointCampaignByObjectId(objectId) : World::GetSpawnPointDeathMatchByObjectId(objectId);
            if (!spawnPoint) return;

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = spawnPoint->GetPosition();
            glm::vec2 rotation = glm::vec2(spawnPoint->GetRotation());
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Vec2(objectId, "Rotation", rotation, [objectId, rotation = &rotation] { SetSpawnPointRotation(objectId, *rotation); });
            properties.Button("Set from player camera", [objectId] { SetSpawnPointFromPlayerCamera(objectId); });
            properties.Button("Test Spawn", [spawnPoint] { TestSpawnPoint(spawnPoint); });
            properties.Render(rect);
        }

        void RenderStaircaseProperties(const EditorRect& rect, uint64_t objectId) {
            Staircase* staircase = World::GetStaircaseByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!staircase) {
                properties.Render(rect);
                return;
            }

            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            glm::vec3 scale = staircase->GetScale();
            uint32_t stepCount = staircase->GetStepCount();

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Vec3(objectId, "Scale", scale, [&] { staircase->SetScale(scale); });
            properties.UInt(objectId, "Step Count", stepCount, [&] { staircase->SetStepCount(stepCount); });
            properties.Render(rect);
        }

        void RenderWallProperties(const EditorRect& rect, uint64_t objectId) {
            Wall* wall = World::GetWallByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!wall) {
                properties.Render(rect);
                return;
            }

            static const std::vector<std::string> wallTypes = {
                Hell::Enum::ToString(WallType::INTERIOR),
                Hell::Enum::ToString(WallType::WEATHER_BOARDS)
            };

            const WallCreateInfo& createInfo = wall->GetCreateInfo();
            const std::vector<std::string>& interiorMaterials = ObjectOptions::GetInteriorMaterials();
            const std::vector<std::string>& weatherBoardMaterials = ObjectOptions::GetWeatherBoardMaterials();
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            std::string materialName = createInfo.materialName;
            std::string wallType = Hell::Enum::ToString(createInfo.wallType);
            float textureScale = createInfo.textureScale;
            float textureOffsetU = createInfo.textureOffsetU;
            float textureOffsetV = createInfo.textureOffsetV;
            float roughnessFactor = createInfo.roughnessFactor;
            float metallicFactor = createInfo.metallicFactor;
            std::string weatherBoardStopMaterialName = createInfo.weatherBoardStopMaterialName;
            uint32_t weatherBoardTextureBoardCount = createInfo.weatherBoardTextureBoardCount;
            uint32_t weatherBoardStartIndex = createInfo.weatherBoardStartIndex;
            uint32_t weatherBoardEndIndex = createInfo.weatherBoardEndIndex;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.DropDown(objectId, "Type", wallTypes, wallType, [wall, wallType = &wallType, materialName = &materialName] { SetWallType(wall, *wallType, *materialName); });

            if (createInfo.wallType == WallType::WEATHER_BOARDS) {
                properties.UInt(objectId, "Texture Boards", weatherBoardTextureBoardCount, [&] { wall->SetWeatherBoardTextureBoardCount(weatherBoardTextureBoardCount); });
                properties.UInt(objectId, "Start Index", weatherBoardStartIndex, [&] { wall->SetWeatherBoardStartIndex(weatherBoardStartIndex); });
                properties.UInt(objectId, "End Index", weatherBoardEndIndex, [&] { wall->SetWeatherBoardEndIndex(weatherBoardEndIndex); });
                properties.Float(objectId, "Tex Offset U", textureOffsetU, [&] { wall->SetTextureOffsetU(textureOffsetU); });
                properties.Float(objectId, "Tex Offset V", textureOffsetV, [&] { wall->SetTextureOffsetV(textureOffsetV); });
                properties.DropDown(objectId, "Stop Material", weatherBoardMaterials, weatherBoardStopMaterialName, [&] { wall->SetWeatherBoardStopMaterial(weatherBoardStopMaterialName); });
            }
            else {
                properties.Float(objectId, "Tex Scale", textureScale, [&] { wall->SetTextureScale(textureScale); });
                properties.Float(objectId, "Tex Offset U", textureOffsetU, [&] { wall->SetTextureOffsetU(textureOffsetU); });
                properties.Float(objectId, "Tex Offset V", textureOffsetV, [&] { wall->SetTextureOffsetV(textureOffsetV); });
            }
            properties.Float(objectId, "Roughness Factor", roughnessFactor, [&] { wall->SetRoughnessFactor(roughnessFactor); });
            properties.Float(objectId, "Metallic Factor", metallicFactor, [&] { wall->SetMetallicFactor(metallicFactor); });
            properties.Render(rect);
        }

        void RenderWallSegmentProperties(const EditorRect& rect, uint64_t objectId, int32_t segmentIndex) {
            Wall* wall = World::GetWallByObjectId(objectId);
            InputElements::PropertyList properties;
            if (!wall || segmentIndex < 0 || segmentIndex + 1 >= static_cast<int32_t>(wall->GetCreateInfo().sequencePoints.size())) {
                properties.Render(rect);
                return;
            }

            const int32_t startIndex = wall->GetCreateInfo().useReversePointOrder ? segmentIndex + 1 : segmentIndex;
            const int32_t endIndex = wall->GetCreateInfo().useReversePointOrder ? segmentIndex : segmentIndex + 1;
            glm::vec3 startPosition = wall->GetPointByIndex(startIndex);
            glm::vec3 endPosition = wall->GetPointByIndex(endIndex);
            float startHeight = wall->GetPointHeightByIndex(startIndex);
            float endHeight = wall->GetPointHeightByIndex(endIndex);
            bool weatherBoardStop = wall->GetCreateInfo().sequencePoints[segmentIndex].customBool;

            properties.Vec3(objectId, "Start", startPosition, [wall, startIndex, startPosition = &startPosition, endPosition = &endPosition] { SetWallSegmentPoint(wall, startIndex, *startPosition, *endPosition); });
            properties.Float(objectId, "Start Height", startHeight, [&] { wall->SetPointHeight(startIndex, startHeight); });
            properties.Vec3(objectId, "End", endPosition, [wall, endIndex, startPosition = &startPosition, endPosition = &endPosition] { SetWallSegmentPoint(wall, endIndex, *endPosition, *startPosition); });
            properties.Float(objectId, "End Height", endHeight, [&] { wall->SetPointHeight(endIndex, endHeight); });
            if (wall->IsWeatherBoards()) {
                properties.CheckBox("Weatherboard Stop", weatherBoardStop, [&] { wall->SetPointCustomBool(segmentIndex, weatherBoardStop); });
            }
            properties.Render(rect);
        }

        void RenderWindowProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            float rotation = World::GetRotationById(objectId).y;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddYawProperty(properties, objectId, rotation);
            properties.Render(rect);
        }

        void RenderNameOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            InputElements::PropertyList properties;
            AddNameProperty(properties, objectId, editorName);
            properties.Render(rect);
        }

        void RenderPositionOnlyProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            properties.Render(rect);
        }

        void RenderDefaultProperties(const EditorRect& rect, uint64_t objectId) {
            std::string editorName = World::GetEditorNameById(objectId);
            glm::vec3 position = World::GetPositionById(objectId);
            glm::vec3 rotation = World::GetRotationById(objectId);
            static glm::vec2 dummyVec2(0.0f);
            static float dummyFloat = 0.0f;
            InputElements::PropertyList properties;

            AddNameProperty(properties, objectId, editorName);
            AddPositionProperty(properties, objectId, position);
            AddEulerRotationProperty(properties, objectId, rotation);

            // Delete these when Vec2 and float have real properties
            properties.Vec2(objectId, "Vec2", dummyVec2);
            properties.Float(objectId, "Float", dummyFloat);
            properties.Render(rect);
        }
    }

    void RenderProperties(const EditorRect& rect) {
        if (!Workspace::HasMode()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        if (Selection::HasWorkspaceSelection()) {
            RenderWorkspaceProperties(rect);
            return;
        }

        if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
            if (RagdollEditor::HasSelectedBone()) RenderRagdollBoneProperties(rect);
            else RenderRagdollMarkerProperties(rect);
            return;
        }

        if (Workspace::GetMode() == EditorSessionMode::BONE_MASK) {
            RenderBoneMaskBoneProperties(rect);
            return;
        }

        if (!Selection::HasSelection()) {
            InputElements::PropertyList properties;
            properties.Render(rect);
            return;
        }

        const uint64_t objectId = Selection::GetSelectedObjectId();
        const ObjectType objectType = GetObjectIdType(objectId);

        if (objectType == ObjectType::DDGI_VOLUME) {
            RenderDDGIVolumeProperties(rect, objectId);
            return;
        }

        if (objectType == ObjectType::PLANAR_QUAD_OBJECT) {
            Internal::RenderPlanarQuadProperties(rect, objectId);
            return;
        }

        if (objectType == ObjectType::POINT_PAIR_OBJECT) {
            Internal::RenderPointPairProperties(rect, objectId);
            return;
        }

        if (Selection::HasSelectedPoint()) {
            if (objectType == ObjectType::CHRISTMAS_LIGHTS) {
                RenderChristmasLightPointProperties(rect, objectId, Selection::GetSelectedPointIndex());
            }
            else {
                RenderSequencePointProperties(rect, objectId, Selection::GetSelectedPointIndex(), Selection::GetSelectedPointHandleType());
            }
            return;
        }

        if (Selection::HasSelectedWallSegment()) {
            RenderWallSegmentProperties(rect, objectId, Selection::GetSelectedWallSegmentIndex());
            return;
        }

        switch (objectType) {
            case ObjectType::CHRISTMAS_LIGHTS:        RenderChristmasLightsProperties(rect, objectId);       break;
            case ObjectType::DOBERMANN:               RenderDobermannProperties(rect, objectId);             break;
            case ObjectType::DOOR:                    RenderDoorProperties(rect, objectId);                  break;
            case ObjectType::FIREPLACE:               RenderFireplaceProperties(rect, objectId);             break;
            case ObjectType::GENERIC_ANIMATED_OBJECT: RenderGenericAnimatedObjectProperties(rect, objectId); break;
            case ObjectType::GENERIC_OBJECT:          RenderGenericObjectProperties(rect, objectId);         break;
            case ObjectType::HOUSE_LOCATION:          RenderHouseLocationProperties(rect, objectId);         break;
            case ObjectType::JETTY:                   RenderJettyProperties(rect, objectId);                 break;
            case ObjectType::LADDER:                  RenderLadderProperties(rect, objectId);                break;
            case ObjectType::LIGHT:                   RenderLightProperties(rect, objectId);                 break;
            case ObjectType::MERMAID:                 RenderMermaidProperties(rect, objectId);               break;
            case ObjectType::PIANO:                   RenderPianoProperties(rect, objectId);                 break;
            case ObjectType::PICK_UP:                 RenderPickUpProperties(rect, objectId);                break;
            case ObjectType::PICTURE_FRAME:           RenderPictureFrameProperties(rect, objectId);          break;
            case ObjectType::SPAWN_POINT_CAMPAIGN:    RenderSpawnPointProperties(rect, objectId);            break;
            case ObjectType::SPAWN_POINT_DEATHMATCH:  RenderSpawnPointProperties(rect, objectId);            break;
            case ObjectType::STAIRCASE:               RenderStaircaseProperties(rect, objectId);             break;
            case ObjectType::WALL:                    RenderWallProperties(rect, objectId);                  break;
            case ObjectType::WINDOW:                  RenderWindowProperties(rect, objectId);                break;
            case ObjectType::WORLD_PLANE:             RenderWorldPlaneProperties(rect, objectId);            break;
            case ObjectType::SKINNED_GAME_OBJECT:    RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::FENCE:                   RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::POWER_POLE_SET:          RenderPositionOnlyProperties(rect, objectId);          break;
            case ObjectType::SHARK:                   RenderPositionOnlyProperties(rect, objectId);          break;
            default:                                  RenderDefaultProperties(rect, objectId);               break;
        }
    }

    bool HasTools() {
        if (!Workspace::HasMode()) return false;
        return Workspace::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive();
    }

    void RenderTools(const EditorRect& rect) {
        if (!HasTools()) return;

        constexpr uint64_t TOOLS_PROPERTY_ID = UINT64_MAX - 1;
        float brushSize = HeightMapEditor::GetBrushSize();
        float brushStrength = HeightMapEditor::GetBrushStrength();
        float targetHeight = HeightMapEditor::GetTargetHeight();
        float brushRotation = HeightMapEditor::GetBrushRotation();
        float brushGamma = HeightMapEditor::GetBrushGamma();

        InputElements::PropertyList properties;
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Brush Size", brushSize, 0.5f, 32.0f, [&] { HeightMapEditor::SetBrushSize(brushSize); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Strength", brushStrength, 1.0f, 100.0f, [&] { HeightMapEditor::SetBrushStrength(brushStrength); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Target Height", targetHeight, 0.0f, 40.0f, [&] { HeightMapEditor::SetTargetHeight(targetHeight); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Rotation", brushRotation, -180.0f, 180.0f, [&] { HeightMapEditor::SetBrushRotation(brushRotation); });
        properties.FloatSlider(TOOLS_PROPERTY_ID, "Gamma", brushGamma, 0.1f, 2.0f, [&] { HeightMapEditor::SetBrushGamma(brushGamma); });
        properties.Render(rect);
    }
}
