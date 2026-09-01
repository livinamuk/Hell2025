#include "EditorRagdoll.h"

#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/AssetLoader/LegacyRag/LegacyRagdollImporter.h"
#include "Hell/Audio.h"
#include "Hell/BVH/BVH.h"
#include "Hell/Common/Bit.h"
#include "Hell/Common/Color.h"
#include "Hell/Common/Enum.h"
#include "Hell/Common/Random.h"
#include "Hell/Common/String.h"
#include "Hell/File/File.h"
#include "Hell/Input.h"
#include "Hell/Math/Matrix.h"
#include "Hell/Math/Ray.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Physics/Ragdoll/RagdollMass.h"
#include "Hell/Physics/Ragdoll/RagdollShapeMesh.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Bible/Bible.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
#include <limits>
#include <unordered_map>
#include <utility>

namespace Unloved::EditorSession::RagdollEditor {
    namespace {
        constexpr float MAX_PICK_DISTANCE = 2000.0f;
        constexpr float MIN_SHAPE_RADIUS = 0.001f;
        constexpr float LIMIT_DIRECTION_EPSILON = 0.000001f;
        constexpr float SHAPE_SCALE_EPSILON = 0.000001f;
        constexpr float MIN_ANGULAR_LIMIT = 0.0872664626f;  // 5 degrees
        constexpr float MAX_ANGULAR_LIMIT = 3.1241393611f;  // 179 degrees
        constexpr float DEFAULT_LIMIT_SCALE = 0.15f;
        constexpr float DEFAULT_LIMIT_HANDLE_SCALE = 0.03f;
        constexpr const char* SKINNED_MODEL_PREVIEW_NAME = "Ragdoll Editor Skinned Model Preview";
        const glm::vec4 LIMIT_TWIST_COLOR = glm::vec4(0.24f, 0.80f, 0.42f, 1.0f);
        const glm::vec4 LIMIT_SWING_COLOR = glm::vec4(0.94f, 0.31f, 0.33f, 1.0f);
        constexpr int32_t ANGULAR_LIMIT_HANDLE_SEGMENTS = 32;
        constexpr int32_t ANGULAR_LIMIT_HANDLE_THICKNESS = 3;
        constexpr int32_t ANGULAR_LIMIT_LINE_THICKNESS = 2;
        constexpr int32_t LIMIT_FRAME_AXIS_THICKNESS = 3;
        constexpr int32_t TWIST_LIMIT_SEGMENTS = 20;
        constexpr int32_t SWING_LIMIT_SEGMENTS = 32;
        constexpr std::array<RagdollEditMode, 5> EDIT_MODES = {
            RagdollEditMode::LIMIT,
            RagdollEditMode::SHAPE_TRANSLATE,
            RagdollEditMode::SHAPE_ROTATE,
            RagdollEditMode::LIMIT_FRAME_TRANSLATE,
            RagdollEditMode::LIMIT_FRAME_ROTATE
        };

        struct Document {
            RagdollAsset asset;
            std::string sourcePath;
            std::string legacySourcePath;
            std::vector<std::string> importWarnings;
            float limitScale = DEFAULT_LIMIT_SCALE;
            float limitHandleScale = DEFAULT_LIMIT_HANDLE_SCALE;
            bool showSkeleton = true;
            bool showSkinnedModel = false;
            bool alwaysShowLimits = false;
            bool alwaysShowLimitFrames = false;
            RagdollEditMode editMode = RagdollEditMode::NONE;
            RagdollEditMode lastEditMode = RagdollEditMode::LIMIT;
            bool gizmoStateCaptured = false;
            GizmoMode previousGizmoMode = GizmoMode::TRANSLATE;
            bool previousGizmoLocalAxes = false;
            bool previousGizmoWorldRotationAxes = false;
            bool dirty = false;
            bool loaded = false;
        };

        struct Preview {
            std::vector<RenderItem> renderItems;
            RagdollMarkerId dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
            bool dirty = false;
        };

        struct ShapePoseDrag {
            glm::mat4 initialLocalPose = glm::mat4(1.0f);
            RagdollMarkerId markerId = INVALID_RAGDOLL_MARKER_ID;
            bool active = false;
        };

        enum class AngularLimitEdge : int32_t {
            MINIMUM,
            MAXIMUM
        };

        enum class BindPoseRetargetMode {
            FOLLOW_PARENT,
            PRESERVE_WORLD_LIMIT_AXES
        };

        struct AngularLimitDrag {
            glm::mat4 initialParentFrame = glm::mat4(1.0f);
            glm::mat4 previewParentFrame = glm::mat4(1.0f);
            glm::vec3 origin = glm::vec3(0.0f);
            glm::vec3 axis = glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 initialDirection = glm::vec3(0.0f, 1.0f, 0.0f);
            RagdollMarkerId childMarkerId = INVALID_RAGDOLL_MARKER_ID;
            float initialHalfRange = 0.0f;
            float previewHalfRange = 0.0f;
            float previousRawAngle = 0.0f;
            float dragAngle = 0.0f;
            int32_t axisIndex = -1;
            int32_t viewportIndex = -1;
            AngularLimitEdge edge = AngularLimitEdge::MINIMUM;
            bool active = false;
        };

        struct AngularLimitHandleExclusion {
            std::array<glm::vec3, 6> centers;
            size_t count = 0;
            float radius = 0.0f;
        };

        Document g_document;
        Preview g_preview;
        ShapePoseDrag g_shapePoseDrag;
        AngularLimitDrag g_angularLimitDrag;
        RagdollMarkerId g_selectedMarkerId = INVALID_RAGDOLL_MARKER_ID;
        RagdollMarkerId g_selectedBoneParentMarkerId = INVALID_RAGDOLL_MARKER_ID;
        uint64_t g_previewSkinnedGameObjectId = 0;
        int32_t g_selectedBoneNodeIndex = -1;
        int32_t g_hoveredAngularLimitAxis = -1;
        int32_t g_hoveredAngularLimitEdge = -1;

        void ReleasePreviewMeshes();
        void ResetAngularLimitInteraction();
        void SetEditMode(RagdollEditMode mode);

        void DestroySkinnedModelPreview() {
            if (g_previewSkinnedGameObjectId == 0) return;

            SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_previewSkinnedGameObjectId);
            if (skinnedGameObject && skinnedGameObject->GetName() == SKINNED_MODEL_PREVIEW_NAME) {
                World::RemoveObjectById(g_previewSkinnedGameObjectId);
            }
            g_previewSkinnedGameObjectId = 0;
        }

        bool CreateSkinnedModelPreview() {
            if (!g_document.loaded || g_document.asset.skinnedModelPresetName.empty()) return false;

            g_previewSkinnedGameObjectId = World::CreateSkinnedGameObject();
            SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_previewSkinnedGameObjectId);
            if (!skinnedGameObject) {
                g_previewSkinnedGameObjectId = 0;
                return false;
            }

            skinnedGameObject->SetName(SKINNED_MODEL_PREVIEW_NAME);
            Bible::ConfigureSkinnedModel(*skinnedGameObject, Hell::Enum::FromString(g_document.asset.skinnedModelPresetName, Bible::SkinnedModelPreset::UNDEFINED));
            SkinnedModel* skinnedModel = skinnedGameObject->GetSkinnedModel();
            if (!skinnedModel || skinnedModel->GetNodeCount() == 0 || skinnedModel->GetBoneCount() == 0) {
                DestroySkinnedModelPreview();
                return false;
            }
            skinnedGameObject->SetPosition(glm::vec3(0.0f));
            skinnedGameObject->SetRotationX(0.0f);
            skinnedGameObject->SetRotationY(0.0f);
            skinnedGameObject->SetRotationZ(0.0f);
            skinnedGameObject->SetScale(g_document.asset.skinnedModelScale);
            skinnedGameObject->SetAnimationModeToBindPose();
            if (!g_document.showSkinnedModel) skinnedGameObject->DisableRendering();
            return true;
        }

        void AdoptDocument(Document&& document) {
            SetEditMode(RagdollEditMode::NONE);
            ResetAngularLimitInteraction();
            DestroySkinnedModelPreview();
            ReleasePreviewMeshes();
            g_document = std::move(document);
            g_preview.dirty = true;
            g_selectedMarkerId = INVALID_RAGDOLL_MARKER_ID;
            g_selectedBoneParentMarkerId = INVALID_RAGDOLL_MARKER_ID;
            g_selectedBoneNodeIndex = -1;
            Gizmo::SetVisible(false);
            CreateSkinnedModelPreview();
        }

        std::string GetPreviewMeshName(const RagdollMarkerAsset& marker) {
            return "EditorRagdoll_" + std::to_string(marker.id) + "_" + marker.name;
        }

        glm::mat4 CreateMarkerRigidPose(const RagdollMarkerAsset& marker);
        glm::mat4 CreateMarkerRestPose(const RagdollMarkerAsset& marker);
        glm::mat4 CreateMarkerModelMatrix(const RagdollMarkerAsset& marker);

        size_t FindMarkerIndex(RagdollMarkerId markerId) {
            for (size_t markerIndex = 0; markerIndex < g_document.asset.markers.size(); markerIndex++) {
                if (g_document.asset.markers[markerIndex].id == markerId) return markerIndex;
            }
            return g_document.asset.markers.size();
        }

        void ReleasePreviewRenderItem(RenderItem& renderItem, Hell::MeshBuffer& meshBuffer) {
            if (renderItem.meshId != 0) {
                if (Mesh* mesh = meshBuffer.GetMeshById(renderItem.meshId)) {
                    Hell::Bvh::DestroyMeshBvh(mesh->meshBvhId);
                    mesh->meshBvhId = 0;
                }
                meshBuffer.RemoveMesh(renderItem.meshId);
            }
            renderItem = {};
        }

        void ReleasePreviewMeshes() {
            if (Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry")) {
                for (RenderItem& renderItem : g_preview.renderItems) {
                    ReleasePreviewRenderItem(renderItem, *meshBuffer);
                }
            }

            g_preview.renderItems.clear();
            g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
        }

        bool PreviewMeshesMissing() {
            if (g_preview.renderItems.size() != g_document.asset.markers.size()) return true;

            Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry");
            if (!meshBuffer) return true;

            for (size_t markerIndex = 0; markerIndex < g_document.asset.markers.size(); markerIndex++) {
                const uint32_t meshId = g_preview.renderItems[markerIndex].meshId;
                if (meshId == 0) continue;

                const Mesh* mesh = meshBuffer->GetMeshById(meshId);
                if (!mesh || mesh->meshBvhId == 0 || mesh->name != GetPreviewMeshName(g_document.asset.markers[markerIndex])) return true;
            }

            return false;
        }

        void UpdatePreviewRenderItemTransform(RenderItem& renderItem, const Mesh& mesh, const glm::mat4& modelMatrix) {
            const glm::vec3 localCenter = (mesh.aabbMin + mesh.aabbMax) * 0.5f;
            const glm::vec3 localExtents = (mesh.aabbMax - mesh.aabbMin) * 0.5f;
            const glm::vec3 worldCenter = glm::vec3(modelMatrix * glm::vec4(localCenter, 1.0f));
            const glm::vec3 worldExtents = glm::abs(glm::vec3(modelMatrix[0])) * localExtents.x + glm::abs(glm::vec3(modelMatrix[1])) * localExtents.y + glm::abs(glm::vec3(modelMatrix[2])) * localExtents.z;

            renderItem.modelMatrix = modelMatrix;
            renderItem.prevModelMatrix = modelMatrix;
            renderItem.inverseModelMatrix = glm::inverse(modelMatrix);
            renderItem.aabbMin = glm::vec4(worldCenter - worldExtents, 0.0f);
            renderItem.aabbMax = glm::vec4(worldCenter + worldExtents, 0.0f);
        }

        RenderItem CreatePreviewRenderItem(const RagdollMarkerAsset& marker, Hell::MeshBuffer& meshBuffer) {
            RagdollShapeMeshData meshData = RagdollShapeMesh::Create(marker.shape);
            if (meshData.vertices.empty() || meshData.indices.empty()) return {};

            const uint32_t meshId = meshBuffer.AddMesh(meshData.vertices, meshData.indices, GetPreviewMeshName(marker));
            Mesh* mesh = meshBuffer.GetMeshById(meshId);
            if (!mesh) return {};

            mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromVertexData(meshData.vertices, meshData.indices);

            RenderItem renderItem{};
            UpdatePreviewRenderItemTransform(renderItem, *mesh, CreateMarkerModelMatrix(marker));
            renderItem.vertexCount = mesh->vertexCount;
            renderItem.indexCount = mesh->indexCount;
            renderItem.baseVertex = mesh->baseVertex;
            renderItem.baseIndex = mesh->baseIndex;
            renderItem.meshId = meshId;
            renderItem.customId = marker.id;
            renderItem.tintColorR = marker.color.r;
            renderItem.tintColorG = marker.color.g;
            renderItem.tintColorB = marker.color.b;
            return renderItem;
        }

        void RebuildPreviewMeshes() {
            ReleasePreviewMeshes();
            g_preview.dirty = false;

            if (!g_document.loaded) return;

            Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry");
            if (!meshBuffer) {
                g_preview.dirty = true;
                return;
            }

            g_preview.renderItems.reserve(g_document.asset.markers.size());

            for (const RagdollMarkerAsset& marker : g_document.asset.markers) {
                g_preview.renderItems.push_back(CreatePreviewRenderItem(marker, *meshBuffer));
            }
        }

        void RebuildPreviewMesh(RagdollMarkerId markerId) {
            if (!g_document.loaded || g_preview.dirty || g_preview.renderItems.size() != g_document.asset.markers.size()) {
                g_preview.dirty = true;
                return;
            }

            Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry");
            const size_t markerIndex = FindMarkerIndex(markerId);
            if (!meshBuffer || markerIndex >= g_document.asset.markers.size()) {
                g_preview.dirty = true;
                return;
            }

            ReleasePreviewRenderItem(g_preview.renderItems[markerIndex], *meshBuffer);
            g_preview.renderItems[markerIndex] = CreatePreviewRenderItem(g_document.asset.markers[markerIndex], *meshBuffer);
            g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
        }

        void SetPreviewMarkerModelMatrix(RagdollMarkerId markerId, const glm::mat4& modelMatrix) {
            const size_t markerIndex = FindMarkerIndex(markerId);
            if (markerIndex >= g_preview.renderItems.size()) return;

            RenderItem& renderItem = g_preview.renderItems[markerIndex];
            Hell::MeshBuffer* meshBuffer = Hell::ResourceManager::GetMeshBufferPtr("PhysicsShapeGeometry");
            const Mesh* mesh = meshBuffer && renderItem.meshId != 0 ? meshBuffer->GetMeshById(renderItem.meshId) : nullptr;
            if (mesh) UpdatePreviewRenderItemTransform(renderItem, *mesh, modelMatrix);
        }

        void EnsurePreviewMeshes() {
            if (g_preview.dirty || PreviewMeshesMissing()) {
                RebuildPreviewMeshes();
                return;
            }
            if (g_preview.dirtyMarkerId != INVALID_RAGDOLL_MARKER_ID) RebuildPreviewMesh(g_preview.dirtyMarkerId);
        }

        RagdollMarkerAsset* FindMarker(RagdollMarkerId markerId) {
            for (RagdollMarkerAsset& marker : g_document.asset.markers) {
                if (marker.id == markerId) return &marker;
            }

            return nullptr;
        }

        RagdollJointAsset* FindIncomingJoint(RagdollMarkerId markerId) {
            for (RagdollJointAsset& joint : g_document.asset.joints) {
                if (joint.childMarkerId == markerId) return &joint;
            }

            return nullptr;
        }

        bool IsShapeEditMode(RagdollEditMode mode) {
            return mode == RagdollEditMode::SHAPE_TRANSLATE || mode == RagdollEditMode::SHAPE_ROTATE;
        }

        bool IsLimitFrameEditMode(RagdollEditMode mode) {
            return mode == RagdollEditMode::LIMIT_FRAME_TRANSLATE ||
                   mode == RagdollEditMode::LIMIT_FRAME_ROTATE;
        }

        bool EditModeUsesGizmo(RagdollEditMode mode) {
            return IsShapeEditMode(mode) || IsLimitFrameEditMode(mode);
        }

        bool IsEditModeApplicable(RagdollEditMode mode) {
            RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
            if (!g_document.loaded || !marker) return mode == RagdollEditMode::NONE;

            if (IsShapeEditMode(mode)) {
                return marker->shape.type != RagdollShapeType::CONVEX_HULL;
            }
            if (mode == RagdollEditMode::LIMIT || IsLimitFrameEditMode(mode)) {
                RagdollJointAsset* joint = FindIncomingJoint(marker->id);
                return joint && FindMarker(joint->parentMarkerId) && FindMarker(joint->childMarkerId);
            }
            return mode == RagdollEditMode::NONE;
        }

        RagdollEditMode ResolveEditMode(RagdollEditMode preferredMode) {
            if (preferredMode != RagdollEditMode::NONE && IsEditModeApplicable(preferredMode)) return preferredMode;
            for (RagdollEditMode mode : EDIT_MODES) {
                if (IsEditModeApplicable(mode)) return mode;
            }
            return RagdollEditMode::NONE;
        }

        RagdollEditMode GetNextEditMode() {
            size_t currentIndex = EDIT_MODES.size() - 1;
            for (size_t index = 0; index < EDIT_MODES.size(); index++) {
                if (EDIT_MODES[index] == g_document.editMode) {
                    currentIndex = index;
                    break;
                }
            }

            for (size_t offset = 1; offset <= EDIT_MODES.size(); offset++) {
                const RagdollEditMode mode = EDIT_MODES[(currentIndex + offset) % EDIT_MODES.size()];
                if (IsEditModeApplicable(mode)) return mode;
            }
            return RagdollEditMode::NONE;
        }

        void SynchronizeShapeDimensions(RagdollShape& shape) {
            switch (shape.type) {
                case RagdollShapeType::BOX: {
                    shape.extents.x = std::max(shape.extents.x, MIN_SHAPE_RADIUS * 2.0f);
                    shape.extents.y = std::max(shape.extents.y, MIN_SHAPE_RADIUS * 2.0f);
                    shape.extents.z = std::max(shape.extents.z, MIN_SHAPE_RADIUS * 2.0f);
                    shape.radius = std::min(shape.extents.x, std::min(shape.extents.y, shape.extents.z)) * 0.5f;
                    shape.length = std::max(shape.extents.x - shape.radius * 2.0f, 0.0f);
                    break;
                }
                case RagdollShapeType::SPHERE:
                    shape.radius = std::max(shape.radius, MIN_SHAPE_RADIUS);
                    shape.length = 0.0f;
                    shape.extents = glm::vec3(shape.radius * 2.0f);
                    break;
                case RagdollShapeType::CAPSULE:
                    shape.radius = std::max(shape.radius, MIN_SHAPE_RADIUS);
                    shape.length = std::max(shape.length, 0.0f);
                    shape.extents = glm::vec3(shape.length + shape.radius * 2.0f, shape.radius * 2.0f, shape.radius * 2.0f);
                    break;
                case RagdollShapeType::CONVEX_HULL:
                    break;
            }
        }

        void MarkShapeChanged(RagdollMarkerId markerId = INVALID_RAGDOLL_MARKER_ID) {
            g_document.dirty = true;
            if (!g_preview.dirty) {
                const RagdollMarkerId changedMarkerId = markerId != INVALID_RAGDOLL_MARKER_ID ? markerId : g_selectedMarkerId;
                if (g_preview.dirtyMarkerId == INVALID_RAGDOLL_MARKER_ID || g_preview.dirtyMarkerId == changedMarkerId) {
                    g_preview.dirtyMarkerId = changedMarkerId;
                }
                else {
                    g_preview.dirty = true;
                    g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
                }
            }
        }

        glm::mat4 CreateRigidPose(const glm::mat4& transform) {
            return Hell::Math::RemoveScaleAndShear(transform);
        }

        glm::mat4 CreateMarkerRigidPose(const RagdollMarkerAsset& marker) {
            return CreateRigidPose(marker.bodyTransform);
        }

        glm::mat4 CreateMarkerRestPose(const RagdollMarkerAsset& marker) {
            return CreateRigidPose(marker.restTransform);
        }

        glm::mat4 CreateMarkerModelMatrix(const RagdollMarkerAsset& marker) {
            // RagdollShapeMesh emits rigid-local coordinates.
            return CreateMarkerRigidPose(marker);
        }

        glm::mat4 CreateShapeLocalPose(const RagdollShape& shape) {
            return glm::translate(glm::mat4(1.0f), shape.offset) * glm::mat4_cast(Hell::Math::EulerXYZToQuaternion(shape.rotationRadians));
        }

        glm::mat4 CreateShapeWorldPose(const RagdollMarkerAsset& marker) {
            return CreateMarkerRigidPose(marker) * CreateShapeLocalPose(marker.shape);
        }

        bool ShapeSupportsPoseEditing(const RagdollShape& shape) {
            return shape.type != RagdollShapeType::CONVEX_HULL;
        }

        glm::mat4 CreateJointAuthoringWorldFrame(const RagdollMarkerAsset& marker, const glm::mat4& localFrame) {
            return CreateMarkerRestPose(marker) * Hell::Math::RemoveScaleAndShear(localFrame);
        }

        void SetJointFrameTranslationsFromWorldPosition(
            const RagdollMarkerAsset& parentMarker,
            const RagdollMarkerAsset& childMarker,
            const glm::vec3& worldPosition,
            glm::mat4& parentFrame,
            glm::mat4& childFrame
        ) {
            const glm::vec3 parentPosition = glm::vec3(glm::inverse(CreateMarkerRestPose(parentMarker)) * glm::vec4(worldPosition, 1.0f));
            const glm::vec3 childPosition = glm::vec3(glm::inverse(CreateMarkerRestPose(childMarker)) * glm::vec4(worldPosition, 1.0f));
            parentFrame[3] = glm::vec4(parentPosition, 1.0f);
            childFrame[3] = glm::vec4(childPosition, 1.0f);
        }

        void AlignJointFrameTranslationsToChildAnchor(
            const RagdollMarkerAsset& parentMarker,
            const RagdollMarkerAsset& childMarker,
            glm::mat4& parentFrame,
            glm::mat4& childFrame
        ) {
            const glm::vec3 worldPosition = glm::vec3(CreateMarkerRestPose(childMarker) * glm::vec4(glm::vec3(childFrame[3]), 1.0f));
            SetJointFrameTranslationsFromWorldPosition(parentMarker, childMarker, worldPosition, parentFrame, childFrame);
        }

        glm::quat GetJointFrameRotation(const glm::mat4& frame) {
            return Hell::Math::ExtractRotation(frame);
        }

        glm::quat GetAngularLimitAxisRotation(int32_t axisIndex) {
            const glm::quat rotateNegativeZ = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, -1.0f));

            if (axisIndex == 1) {
                return rotateNegativeZ;
            }
            if (axisIndex == 2) {
                const glm::quat rotateX = glm::angleAxis(glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
                return glm::normalize(rotateX * rotateNegativeZ);
            }
            return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }

        glm::mat4 GetAngularLimitAxisFrame(const glm::mat4& parentFrame, int32_t axisIndex) {
            return parentFrame * glm::mat4_cast(GetAngularLimitAxisRotation(axisIndex));
        }

        glm::vec3 GetAngularLimitEndpoint(const glm::mat4& parentFrame, int32_t axisIndex, AngularLimitEdge edge, float halfRange, float scale) {
            const glm::mat4 axisFrame = GetAngularLimitAxisFrame(parentFrame, axisIndex);
            const float angle = edge == AngularLimitEdge::MINIMUM ? -halfRange : halfRange;
            const glm::vec3 direction = glm::angleAxis(angle, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::vec3(0.0f, 1.0f, 0.0f);
            return glm::vec3(axisFrame * glm::vec4(direction * scale, 1.0f));
        }

        bool IsAngularLimitDragForJoint(const RagdollJointAsset& joint) {
            return g_angularLimitDrag.active && g_angularLimitDrag.childMarkerId == joint.childMarkerId;
        }

        const glm::mat4& GetDisplayedParentFrame(const RagdollJointAsset& joint) {
            return IsAngularLimitDragForJoint(joint) ? g_angularLimitDrag.previewParentFrame : joint.parentFrame;
        }

        float GetDisplayedAngularLimit(const RagdollJointAsset& joint, int32_t axisIndex) {
            if (IsAngularLimitDragForJoint(joint) && g_angularLimitDrag.axisIndex == axisIndex) {
                return g_angularLimitDrag.previewHalfRange;
            }
            return joint.angularLimits[axisIndex].limit;
        }

        bool ControlIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        }

        bool SetJointParentWorldRotation(RagdollJointAsset& joint, const glm::quat& targetParentWorldRotation) {
            RagdollMarkerAsset* parentMarker = FindMarker(joint.parentMarkerId);
            RagdollMarkerAsset* childMarker = FindMarker(joint.childMarkerId);
            if (!parentMarker || !childMarker) return false;

            const glm::quat parentRigidRotation = Hell::Math::ExtractRotation(CreateMarkerRestPose(*parentMarker));
            const glm::quat childRigidRotation = Hell::Math::ExtractRotation(CreateMarkerRestPose(*childMarker));
            const glm::quat parentWorldRotation = glm::normalize(parentRigidRotation * GetJointFrameRotation(joint.parentFrame));
            const glm::quat childWorldRotation = glm::normalize(childRigidRotation * GetJointFrameRotation(joint.childFrame));
            const glm::quat normalizedTarget = glm::normalize(targetParentWorldRotation);
            if (std::abs(glm::dot(parentWorldRotation, normalizedTarget)) > 0.999999f) return true;

            // Rotate both frames together so their neutral relationship survives
            const glm::quat worldDelta = glm::normalize(normalizedTarget * glm::inverse(parentWorldRotation));
            const glm::quat newParentLocalRotation = glm::normalize(glm::inverse(parentRigidRotation) * normalizedTarget);
            const glm::quat newChildWorldRotation = glm::normalize(worldDelta * childWorldRotation);
            const glm::quat newChildLocalRotation = glm::normalize(glm::inverse(childRigidRotation) * newChildWorldRotation);
            Hell::Math::SetRotationPreserveTranslation(joint.parentFrame, newParentLocalRotation);
            Hell::Math::SetRotationPreserveTranslation(joint.childFrame, newChildLocalRotation);
            g_document.dirty = true;
            return true;
        }

        void DrawLimitFrameAxes(const glm::mat4& frame, float scale) {
            const glm::vec3 position = glm::vec3(frame[3]);
            DebugDraw::DrawLine(position, position + glm::normalize(glm::vec3(frame[0])) * scale, RED, false, LIMIT_FRAME_AXIS_THICKNESS);
            DebugDraw::DrawLine(position, position + glm::normalize(glm::vec3(frame[1])) * scale, GREEN, false, LIMIT_FRAME_AXIS_THICKNESS);
            DebugDraw::DrawLine(position, position + glm::normalize(glm::vec3(frame[2])) * scale, BLUE, false, LIMIT_FRAME_AXIS_THICKNESS);
        }

        void FinishShapePoseDrag() {
            if (!g_shapePoseDrag.active) return;
            MarkShapeChanged(g_shapePoseDrag.markerId);
            g_shapePoseDrag = {};
        }

        void CaptureGizmoState() {
            if (g_document.gizmoStateCaptured) return;
            g_document.previousGizmoMode = Gizmo::GetMode();
            g_document.previousGizmoLocalAxes = Gizmo::UsesLocalAxes();
            g_document.previousGizmoWorldRotationAxes = Gizmo::UsesWorldRotationAxes();
            g_document.gizmoStateCaptured = true;
        }

        void RestoreGizmoState() {
            if (!g_document.gizmoStateCaptured) return;
            Gizmo::SetVisible(false);
            Gizmo::SetMode(g_document.previousGizmoMode);
            Gizmo::SetLocalAxes(g_document.previousGizmoLocalAxes);
            Gizmo::SetWorldRotationAxes(g_document.previousGizmoWorldRotationAxes);
            g_document.gizmoStateCaptured = false;
        }

        void SetEditMode(RagdollEditMode mode) {
            if (mode != RagdollEditMode::NONE && !IsEditModeApplicable(mode)) mode = RagdollEditMode::NONE;
            if (g_document.editMode == mode) return;

            FinishShapePoseDrag();
            ResetAngularLimitInteraction();
            if (EditModeUsesGizmo(mode)) CaptureGizmoState();

            Gizmo::CancelInteraction();
            Gizmo::SetVisible(false);
            g_document.editMode = mode;
            if (mode != RagdollEditMode::NONE) {
                g_document.lastEditMode = mode;
            }
            else {
                RestoreGizmoState();
            }
        }

        void CycleEditMode() {
            SetEditMode(GetNextEditMode());
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Debug::BlitQuickDebugMessage("Ragdoll Edit Mode: " + Hell::Enum::ToString(g_document.editMode));
        }

        void PreviewShapePose(const RagdollMarkerAsset& marker) {
            if (!g_shapePoseDrag.active || g_shapePoseDrag.markerId != marker.id) return;

            const glm::mat4 targetLocalPose = CreateShapeLocalPose(marker.shape);
            const glm::mat4 localCorrection = targetLocalPose * glm::inverse(g_shapePoseDrag.initialLocalPose);
            SetPreviewMarkerModelMatrix(marker.id, CreateMarkerModelMatrix(marker) * localCorrection);
        }

        void UpdateShapePoseFromGizmo(RagdollMarkerAsset& marker) {
            if (g_document.editMode == RagdollEditMode::SHAPE_TRANSLATE) {
                marker.shape.offset = glm::vec3(glm::inverse(CreateMarkerRigidPose(marker)) * glm::vec4(Gizmo::GetPosition(), 1.0f));
            }
            else if (g_document.editMode == RagdollEditMode::SHAPE_ROTATE) {
                const glm::quat markerWorldRotation = Hell::Math::ExtractRotation(CreateMarkerRigidPose(marker));
                const glm::quat localRotation = glm::normalize(glm::inverse(markerWorldRotation) * Gizmo::GetRotationQuaternion());
                marker.shape.rotationRadians = Hell::Math::NearestEulerEquivalent(localRotation, marker.shape.rotationRadians);
            }

            g_document.dirty = true;
            PreviewShapePose(marker);
        }

        bool UpdateShapeGizmo(bool allowInput) {
            RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
            if (!IsShapeEditMode(g_document.editMode) || !marker || !ShapeSupportsPoseEditing(marker->shape) || Viewports::IsFlyMode()) {
                if (IsShapeEditMode(g_document.editMode)) FinishShapePoseDrag();
                Gizmo::SetVisible(false);
                return false;
            }

            EnsurePreviewMeshes();
            const GizmoMode gizmoMode = g_document.editMode == RagdollEditMode::SHAPE_TRANSLATE ? GizmoMode::TRANSLATE : GizmoMode::ROTATE;
            Gizmo::SetMode(gizmoMode);
            Gizmo::SetLocalAxes(false);
            Gizmo::SetWorldRotationAxes(false);
            Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
            Gizmo::SetVisible(true);

            if (Gizmo::GetAction() != GizmoAction::DRAGGING) {
                const glm::mat4 shapeWorldPose = CreateShapeWorldPose(*marker);
                Gizmo::SetPosition(glm::vec3(shapeWorldPose[3]));
                Gizmo::SetRotation(Hell::Math::ExtractRotation(shapeWorldPose));
            }

            const bool wasDragging = Gizmo::GetAction() == GizmoAction::DRAGGING;
            Gizmo::Update(allowInput, false);
            const bool isDragging = Gizmo::GetAction() == GizmoAction::DRAGGING;

            if (!wasDragging && isDragging) {
                g_shapePoseDrag = {};
                g_shapePoseDrag.initialLocalPose = CreateShapeLocalPose(marker->shape);
                g_shapePoseDrag.markerId = marker->id;
                g_shapePoseDrag.active = true;
            }

            if (isDragging || wasDragging) {
                UpdateShapePoseFromGizmo(*marker);
            }
            if (wasDragging && !isDragging) {
                FinishShapePoseDrag();
            }

            return Gizmo::HasHover() || isDragging || wasDragging;
        }

        bool SetJointWorldPosition(RagdollJointAsset& joint, const glm::vec3& worldPosition) {
            RagdollMarkerAsset* parentMarker = FindMarker(joint.parentMarkerId);
            RagdollMarkerAsset* childMarker = FindMarker(joint.childMarkerId);
            if (!parentMarker || !childMarker) return false;

            const glm::vec3 previousParentPosition(joint.parentFrame[3]);
            const glm::vec3 previousChildPosition(joint.childFrame[3]);

            SetJointFrameTranslationsFromWorldPosition(*parentMarker, *childMarker, worldPosition, joint.parentFrame, joint.childFrame);
            if (previousParentPosition == glm::vec3(joint.parentFrame[3]) && previousChildPosition == glm::vec3(joint.childFrame[3])) return true;

            g_document.dirty = true;
            return true;
        }

        bool UpdateLimitFrameGizmo(bool allowInput) {
            RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
            RagdollMarkerAsset* parentMarker = joint ? FindMarker(joint->parentMarkerId) : nullptr;
            if (!IsLimitFrameEditMode(g_document.editMode) || !joint || !parentMarker || Viewports::IsFlyMode()) {
                Gizmo::SetVisible(false);
                return false;
            }

            const GizmoMode gizmoMode = g_document.editMode == RagdollEditMode::LIMIT_FRAME_TRANSLATE ? GizmoMode::TRANSLATE : GizmoMode::ROTATE;
            Gizmo::SetMode(gizmoMode);
            Gizmo::SetLocalAxes(false);
            Gizmo::SetWorldRotationAxes(false);
            Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
            Gizmo::SetVisible(true);

            if (Gizmo::GetAction() != GizmoAction::DRAGGING) {
                const glm::mat4 parentWorldFrame = CreateJointAuthoringWorldFrame(*parentMarker, joint->parentFrame);
                Gizmo::SetPosition(glm::vec3(parentWorldFrame[3]));
                Gizmo::SetRotation(Hell::Math::ExtractRotation(parentWorldFrame));
            }

            const bool wasDragging = Gizmo::GetAction() == GizmoAction::DRAGGING;
            Gizmo::Update(allowInput, false);
            if (wasDragging || Gizmo::GetAction() == GizmoAction::DRAGGING) {
                if (g_document.editMode == RagdollEditMode::LIMIT_FRAME_TRANSLATE) {
                    SetJointWorldPosition(*joint, Gizmo::GetPosition());
                }
                else {
                    SetJointParentWorldRotation(*joint, Gizmo::GetRotationQuaternion());
                }
            }

            return Gizmo::HasHover() || Gizmo::GetAction() == GizmoAction::DRAGGING;
        }

        bool IsLimited(const RagdollAxisLimit& limit) {
            return limit.motion == RagdollAxisMotion::LIMITED && limit.limit > 0.0f;
        }

        glm::vec3 TransformLimitPoint(const glm::mat4& frame, const glm::vec3& point, float scale) {
            return glm::vec3(frame * glm::vec4(point * scale, 1.0f));
        }

        void DrawWorldLimitLine(const glm::vec3& begin, const glm::vec3& end, const glm::vec4& color, const AngularLimitHandleExclusion* exclusion = nullptr) {
            if (!exclusion || exclusion->count == 0 || exclusion->radius <= 0.0f) {
                DebugDraw::DrawLine(begin, end, color, false, ANGULAR_LIMIT_LINE_THICKNESS);
                return;
            }

            const glm::vec3 segment = end - begin;
            const float segmentLengthSquared = glm::dot(segment, segment);
            if (segmentLengthSquared <= LIMIT_DIRECTION_EPSILON) return;

            std::array<glm::vec2, 6> clippedIntervals;
            size_t intervalCount = 0;
            const float radiusSquared = exclusion->radius * exclusion->radius;

            for (size_t centerIndex = 0; centerIndex < exclusion->count; centerIndex++) {
                const glm::vec3 centerToBegin = begin - exclusion->centers[centerIndex];
                const float projectedDistance = glm::dot(centerToBegin, segment);
                const float discriminant = projectedDistance * projectedDistance - segmentLengthSquared * (glm::dot(centerToBegin, centerToBegin) - radiusSquared);
                if (discriminant < 0.0f) continue;

                const float root = std::sqrt(discriminant);
                const float intervalBegin = std::max(0.0f, (-projectedDistance - root) / segmentLengthSquared);
                const float intervalEnd = std::min(1.0f, (-projectedDistance + root) / segmentLengthSquared);
                if (intervalBegin >= intervalEnd) continue;

                clippedIntervals[intervalCount++] = glm::vec2(intervalBegin, intervalEnd);
            }

            if (intervalCount == 0) {
                DebugDraw::DrawLine(begin, end, color, false, ANGULAR_LIMIT_LINE_THICKNESS);
                return;
            }

            std::sort(clippedIntervals.begin(), clippedIntervals.begin() + intervalCount, [](const glm::vec2& a, const glm::vec2& b) {
                return a.x < b.x;
            });

            float visibleBegin = 0.0f;
            for (size_t intervalIndex = 0; intervalIndex < intervalCount; intervalIndex++) {
                const glm::vec2& interval = clippedIntervals[intervalIndex];
                if (interval.x > visibleBegin) {
                    DebugDraw::DrawLine(glm::mix(begin, end, visibleBegin), glm::mix(begin, end, interval.x), color, false, ANGULAR_LIMIT_LINE_THICKNESS);
                }
                visibleBegin = std::max(visibleBegin, interval.y);
            }

            if (visibleBegin < 1.0f) {
                DebugDraw::DrawLine(glm::mix(begin, end, visibleBegin), end, color, false, ANGULAR_LIMIT_LINE_THICKNESS);
            }
        }

        void DrawLimitLine(const glm::mat4& frame, const glm::vec3& begin, const glm::vec3& end, float scale, const glm::vec4& color, const AngularLimitHandleExclusion* exclusion = nullptr) {
            DrawWorldLimitLine(TransformLimitPoint(frame, begin, scale), TransformLimitPoint(frame, end, scale), color, exclusion);
        }

        void DrawAngularArc(const glm::mat4& frame, float limit, float scale, const glm::mat3& localRotation, const glm::vec4& color, const AngularLimitHandleExclusion* exclusion) {
            const auto pointAt = [&](float alpha) {
                const float angle = glm::mix(-limit, limit, alpha);
                return localRotation * glm::vec3(0.0f, std::cos(angle), std::sin(angle));
            };

            const glm::vec3 origin(0.0f);
            glm::vec3 previous = pointAt(0.0f);
            DrawLimitLine(frame, origin, previous, scale, WHITE, exclusion);

            for (int32_t segmentIndex = 1; segmentIndex < TWIST_LIMIT_SEGMENTS; segmentIndex++) {
                const float alpha = static_cast<float>(segmentIndex) / static_cast<float>(TWIST_LIMIT_SEGMENTS - 1);
                const glm::vec3 point = pointAt(alpha);
                DrawLimitLine(frame, previous, point, scale, color, exclusion);
                previous = point;
            }

            DrawLimitLine(frame, previous, origin, scale, WHITE, exclusion);
        }

        glm::vec3 GetSwingConePoint(float angle, float swing1, float swing2) {
            glm::vec3 axis(0.0f, swing2 * std::sin(angle), swing1 * std::cos(angle));
            const float amount = glm::length(axis);
            if (amount <= LIMIT_DIRECTION_EPSILON) return glm::vec3(1.0f, 0.0f, 0.0f);

            axis /= amount;
            return glm::vec3(std::cos(amount), std::sin(amount) * axis.y, std::sin(amount) * axis.z);
        }

        void DrawSwingCone(const glm::mat4& frame, float swing1, float swing2, float scale, const AngularLimitHandleExclusion* exclusion) {
            const float angleStep = glm::two_pi<float>() / static_cast<float>(SWING_LIMIT_SEGMENTS);
            const glm::vec3 first = GetSwingConePoint(0.0f, swing1, swing2);
            glm::vec3 previous = first;

            for (int32_t segmentIndex = 1; segmentIndex < SWING_LIMIT_SEGMENTS; segmentIndex++) {
                const glm::vec3 point = GetSwingConePoint(angleStep * static_cast<float>(segmentIndex), swing1, swing2);
                DrawLimitLine(frame, previous, point, scale, LIMIT_SWING_COLOR, exclusion);
                previous = point;
            }
            DrawLimitLine(frame, previous, first, scale, LIMIT_SWING_COLOR, exclusion);

            for (int32_t spokeIndex = 0; spokeIndex < 4; spokeIndex++) {
                const float angle = glm::half_pi<float>() * static_cast<float>(spokeIndex);
                DrawLimitLine(frame, glm::vec3(0.0f), GetSwingConePoint(angle, swing1, swing2), scale, WHITE, exclusion);
            }
        }

        void DrawCurrentJointDirections(const RagdollJointAsset& joint, const glm::mat4& parentFrame, const glm::mat4& childFrame, float scale, const AngularLimitHandleExclusion* exclusion) {
            const bool twistLimited = IsLimited(joint.angularLimits[0]);
            const bool swingLimited = IsLimited(joint.angularLimits[1]) || IsLimited(joint.angularLimits[2]);
            const glm::vec3 childFramePosition = glm::vec3(childFrame[3]);

            if (twistLimited) {
                const glm::vec3 parentAxis = glm::vec3(parentFrame[0]);
                const glm::vec3 childAxis = glm::vec3(childFrame[1]);
                const float parentAxisLengthSquared = glm::dot(parentAxis, parentAxis);

                if (parentAxisLengthSquared > LIMIT_DIRECTION_EPSILON) {
                    const glm::vec3 projectedAxis = childAxis - glm::dot(childAxis, parentAxis) * parentAxis / parentAxisLengthSquared;
                    const float projectedAxisLengthSquared = glm::dot(projectedAxis, projectedAxis);

                    if (projectedAxisLengthSquared > LIMIT_DIRECTION_EPSILON) {
                        DrawWorldLimitLine(childFramePosition, childFramePosition + glm::normalize(projectedAxis) * scale, LIMIT_TWIST_COLOR, exclusion);
                    }
                }
            }

            if (swingLimited) {
                const glm::vec3 childAxis = glm::vec3(childFrame[0]);
                const float childAxisLengthSquared = glm::dot(childAxis, childAxis);

                if (childAxisLengthSquared > LIMIT_DIRECTION_EPSILON) {
                    DrawWorldLimitLine(childFramePosition, childFramePosition + glm::normalize(childAxis) * scale, LIMIT_SWING_COLOR, exclusion);
                }
            }
        }

        bool ProjectAngularLimitHandle(Viewport& viewport, uint32_t viewportIndex, const glm::vec3& worldPosition, glm::vec2& viewportPosition, float& depth) {
            const glm::vec4 viewPosition = Viewports::GetViewMatrix(viewportIndex) * glm::vec4(worldPosition, 1.0f);
            const glm::vec4 clipPosition = viewport.GetProjectionMatrix() * viewPosition;
            if (std::abs(clipPosition.w) <= LIMIT_DIRECTION_EPSILON || (!viewport.IsOrthographic() && clipPosition.w <= 0.0f)) return false;

            const glm::vec3 ndc = glm::vec3(clipPosition) / clipPosition.w;
            if (ndc.z < -1.0f || ndc.z > 1.0f) return false;

            const float width = static_cast<float>(viewport.GetRightPixel() - viewport.GetLeftPixel());
            const float height = static_cast<float>(viewport.GetBottomPixel() - viewport.GetTopPixel());
            viewportPosition.x = (ndc.x * 0.5f + 0.5f) * width;
            viewportPosition.y = (0.5f - ndc.y * 0.5f) * height;
            depth = std::abs(viewPosition.z);
            return true;
        }

        bool GetAngularLimitHandleScreenBounds(Viewport& viewport, uint32_t viewportIndex, const glm::vec3& center, float worldRadius, glm::vec2& screenCenter, float& screenRadius, float& depth) {
            const EditorCamera* camera = Viewports::GetCameraByIndex(viewportIndex);
            if (!camera || !ProjectAngularLimitHandle(viewport, viewportIndex, center, screenCenter, depth)) return false;

            glm::vec2 radiusPoint;
            float radiusPointDepth = 0.0f;
            if (!ProjectAngularLimitHandle(viewport, viewportIndex, center + camera->GetRight() * worldRadius, radiusPoint, radiusPointDepth)) return false;

            screenRadius = glm::length(radiusPoint - screenCenter);
            return screenRadius > 0.0f;
        }

        void DrawAngularLimitHandleRing(const glm::vec3& center, float radius, const glm::vec4& color, int32_t viewportIndex) {
            const EditorCamera* camera = Viewports::GetCameraByIndex(viewportIndex);
            if (!camera || radius <= 0.0f) return;

            const glm::vec3 right = camera->GetRight();
            const glm::vec3 up = camera->GetUp();
            const float angleStep = glm::two_pi<float>() / static_cast<float>(ANGULAR_LIMIT_HANDLE_SEGMENTS);
            glm::vec3 previous = center + right * radius;

            for (int32_t segmentIndex = 1; segmentIndex <= ANGULAR_LIMIT_HANDLE_SEGMENTS; segmentIndex++) {
                const float angle = angleStep * static_cast<float>(segmentIndex);
                const glm::vec3 point = center + radius * (right * std::cos(angle) + up * std::sin(angle));
                DebugDraw::DrawLine(previous, point, color, false, ANGULAR_LIMIT_HANDLE_THICKNESS, viewportIndex);
                previous = point;
            }
        }

        void DrawAngularLimitHandles(const RagdollJointAsset& joint, const glm::mat4& parentFrame, float scale) {
            if (g_document.editMode != RagdollEditMode::LIMIT || scale <= 0.0f) return;

            std::vector<Viewport>& viewports = ViewportManager::GetViewports();
            const float handleRadius = scale * g_document.limitHandleScale;
            if (handleRadius <= 0.0f) return;

            for (int32_t axisIndex = 0; axisIndex < static_cast<int32_t>(joint.angularLimits.size()); axisIndex++) {
                if (!IsLimited(joint.angularLimits[axisIndex])) continue;

                const float halfRange = GetDisplayedAngularLimit(joint, axisIndex);
                for (int32_t edgeIndex = 0; edgeIndex < 2; edgeIndex++) {
                    const AngularLimitEdge edge = static_cast<AngularLimitEdge>(edgeIndex);
                    const glm::vec3 endpoint = GetAngularLimitEndpoint(parentFrame, axisIndex, edge, halfRange, scale);
                    const bool active = g_angularLimitDrag.active && g_angularLimitDrag.axisIndex == axisIndex && static_cast<int32_t>(g_angularLimitDrag.edge) == edgeIndex;
                    const bool hovered = g_hoveredAngularLimitAxis == axisIndex && g_hoveredAngularLimitEdge == edgeIndex;
                    const glm::vec4 handleColor = active || hovered ? WHITE : (axisIndex == 0 ? LIMIT_TWIST_COLOR : LIMIT_SWING_COLOR);

                    for (Viewport& viewport : viewports) {
                        if (!viewport.IsVisible()) continue;
                        DrawAngularLimitHandleRing(endpoint, handleRadius, handleColor, static_cast<int32_t>(viewport.GetViewportIndex()));
                    }
                }
            }
        }

        void DrawJointLimit(const RagdollJointAsset& joint, bool selected, bool drawLimits, bool drawLimitFrame) {
            const RagdollMarkerAsset* parentMarker = FindMarker(joint.parentMarkerId);
            const RagdollMarkerAsset* childMarker = FindMarker(joint.childMarkerId);
            if (!parentMarker || !childMarker) return;

            const float scale = g_document.limitScale;
            if (scale <= 0.0f) return;

            const glm::mat4 parentFrame = CreateJointAuthoringWorldFrame(*parentMarker, GetDisplayedParentFrame(joint));
            if (drawLimitFrame) {
                DrawLimitFrameAxes(parentFrame, scale);
            }

            if (!drawLimits || !joint.limitEnabled) return;

            const bool twistLimited = IsLimited(joint.angularLimits[0]);
            const bool swing1Limited = IsLimited(joint.angularLimits[1]);
            const bool swing2Limited = IsLimited(joint.angularLimits[2]);
            if (!twistLimited && !swing1Limited && !swing2Limited) return;

            const glm::mat4 childFrame = CreateJointAuthoringWorldFrame(*childMarker, joint.childFrame);
            const float twist = GetDisplayedAngularLimit(joint, 0);
            const float swing1 = GetDisplayedAngularLimit(joint, 1);
            const float swing2 = GetDisplayedAngularLimit(joint, 2);

            AngularLimitHandleExclusion handleExclusion;
            const AngularLimitHandleExclusion* exclusion = nullptr;
            if (selected && g_document.editMode == RagdollEditMode::LIMIT && g_document.limitHandleScale > 0.0f) {
                handleExclusion.radius = scale * g_document.limitHandleScale;
                for (int32_t axisIndex = 0; axisIndex < static_cast<int32_t>(joint.angularLimits.size()); axisIndex++) {
                    if (!IsLimited(joint.angularLimits[axisIndex])) continue;

                    const float halfRange = GetDisplayedAngularLimit(joint, axisIndex);
                    for (int32_t edgeIndex = 0; edgeIndex < 2; edgeIndex++) {
                        const AngularLimitEdge edge = static_cast<AngularLimitEdge>(edgeIndex);
                        handleExclusion.centers[handleExclusion.count++] = GetAngularLimitEndpoint(parentFrame, axisIndex, edge, halfRange, scale);
                    }
                }
                if (handleExclusion.count > 0) exclusion = &handleExclusion;
            }

            if (twistLimited) {
                DrawAngularArc(parentFrame, twist, scale, glm::mat3(1.0f), LIMIT_TWIST_COLOR, exclusion);
            }

            if (swing1Limited && swing2Limited) {
                DrawSwingCone(parentFrame, swing1, swing2, scale, exclusion);
            }
            else if (swing1Limited) {
                const glm::mat3 rotation = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, -1.0f)));
                DrawAngularArc(parentFrame, swing1, scale, rotation, LIMIT_SWING_COLOR, exclusion);
            }
            else if (swing2Limited) {
                const glm::mat4 rotationX = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(1.0f, 0.0f, 0.0f));
                const glm::mat4 rotationNegativeZ = glm::rotate(glm::mat4(1.0f), glm::half_pi<float>(), glm::vec3(0.0f, 0.0f, -1.0f));
                DrawAngularArc(parentFrame, swing2, scale, glm::mat3(rotationX * rotationNegativeZ), LIMIT_SWING_COLOR, exclusion);
            }

            if (selected) {
                DrawCurrentJointDirections(joint, parentFrame, childFrame, scale, exclusion);
                DrawAngularLimitHandles(joint, parentFrame, scale);
            }
        }

        void ResetAngularLimitInteraction() {
            g_angularLimitDrag = {};
            g_hoveredAngularLimitAxis = -1;
            g_hoveredAngularLimitEdge = -1;
        }

        void CommitAngularLimitDrag() {
            if (!g_angularLimitDrag.active) return;

            RagdollJointAsset* joint = FindIncomingJoint(g_angularLimitDrag.childMarkerId);
            if (joint && g_angularLimitDrag.axisIndex >= 0 && g_angularLimitDrag.axisIndex < static_cast<int32_t>(joint->angularLimits.size())) {
                joint->parentFrame = g_angularLimitDrag.previewParentFrame;
                joint->angularLimits[g_angularLimitDrag.axisIndex].limit = g_angularLimitDrag.previewHalfRange;
                g_document.dirty = true;
            }

            ResetAngularLimitInteraction();
        }

        bool BeginAngularLimitDrag(RagdollJointAsset& joint, const RagdollMarkerAsset& parentMarker, int32_t axisIndex, AngularLimitEdge edge, int32_t viewportIndex, const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
            const glm::mat4 parentWorldFrame = CreateJointAuthoringWorldFrame(parentMarker, joint.parentFrame);
            const glm::mat4 axisFrame = GetAngularLimitAxisFrame(parentWorldFrame, axisIndex);
            const glm::vec3 origin = glm::vec3(axisFrame[3]);
            const glm::vec3 axis = glm::normalize(glm::vec3(axisFrame[0]));
            glm::vec3 intersection;
            if (!Hell::Ray::IntersectRayPlane(rayOrigin, rayDirection, origin, axis, intersection, LIMIT_DIRECTION_EPSILON)) return false;

            const glm::vec3 direction = intersection - origin;
            const float directionLengthSquared = glm::dot(direction, direction);
            if (directionLengthSquared <= LIMIT_DIRECTION_EPSILON) return false;

            g_angularLimitDrag = {};
            g_angularLimitDrag.initialParentFrame = joint.parentFrame;
            g_angularLimitDrag.previewParentFrame = joint.parentFrame;
            g_angularLimitDrag.origin = origin;
            g_angularLimitDrag.axis = axis;
            g_angularLimitDrag.initialDirection = direction / std::sqrt(directionLengthSquared);
            g_angularLimitDrag.childMarkerId = joint.childMarkerId;
            g_angularLimitDrag.initialHalfRange = joint.angularLimits[axisIndex].limit;
            g_angularLimitDrag.previewHalfRange = joint.angularLimits[axisIndex].limit;
            g_angularLimitDrag.axisIndex = axisIndex;
            g_angularLimitDrag.viewportIndex = viewportIndex;
            g_angularLimitDrag.edge = edge;
            g_angularLimitDrag.active = true;
            return true;
        }

        void UpdateAngularLimitDrag(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) {
            glm::vec3 intersection;
            if (!Hell::Ray::IntersectRayPlane(rayOrigin, rayDirection, g_angularLimitDrag.origin, g_angularLimitDrag.axis, intersection, LIMIT_DIRECTION_EPSILON)) return;

            const glm::vec3 direction = intersection - g_angularLimitDrag.origin;
            const float directionLengthSquared = glm::dot(direction, direction);
            if (directionLengthSquared <= LIMIT_DIRECTION_EPSILON) return;

            const glm::vec3 currentDirection = direction / std::sqrt(directionLengthSquared);
            const float rawAngle = std::atan2(
                glm::dot(g_angularLimitDrag.axis, glm::cross(g_angularLimitDrag.initialDirection, currentDirection)),
                glm::dot(g_angularLimitDrag.initialDirection, currentDirection)
            );
            g_angularLimitDrag.dragAngle += Hell::Math::WrapRadians(rawAngle - g_angularLimitDrag.previousRawAngle);
            g_angularLimitDrag.previousRawAngle = rawAngle;

            const float edgeDirection = g_angularLimitDrag.edge == AngularLimitEdge::MINIMUM ? -1.0f : 1.0f;
            g_angularLimitDrag.previewParentFrame = g_angularLimitDrag.initialParentFrame;

            if (ControlIsDown()) {
                float frameOffset = g_angularLimitDrag.dragAngle * 0.5f;
                const float unclampedRange = g_angularLimitDrag.initialHalfRange + edgeDirection * frameOffset;
                g_angularLimitDrag.previewHalfRange = std::clamp(unclampedRange, MIN_ANGULAR_LIMIT, MAX_ANGULAR_LIMIT);
                frameOffset = (g_angularLimitDrag.previewHalfRange - g_angularLimitDrag.initialHalfRange) / edgeDirection;

                const glm::quat axisRotation = GetAngularLimitAxisRotation(g_angularLimitDrag.axisIndex);
                const glm::quat frameRotation = axisRotation * glm::angleAxis(frameOffset, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::inverse(axisRotation);
                const glm::quat parentRotation = GetJointFrameRotation(g_angularLimitDrag.initialParentFrame);
                Hell::Math::SetRotationPreserveTranslation(g_angularLimitDrag.previewParentFrame, glm::normalize(parentRotation * frameRotation));
            }
            else {
                const float range = g_angularLimitDrag.initialHalfRange + edgeDirection * g_angularLimitDrag.dragAngle;
                g_angularLimitDrag.previewHalfRange = std::clamp(range, MIN_ANGULAR_LIMIT, MAX_ANGULAR_LIMIT);
            }
        }

        bool UpdateAngularLimitHandles(bool allowInput) {
            g_hoveredAngularLimitAxis = -1;
            g_hoveredAngularLimitEdge = -1;

            if (g_angularLimitDrag.active) {
                RagdollJointAsset* joint = FindIncomingJoint(g_angularLimitDrag.childMarkerId);
                if (!joint || joint->childMarkerId != g_selectedMarkerId) {
                    ResetAngularLimitInteraction();
                    return false;
                }

                if (!Hell::Input::LeftMouseDown()) {
                    CommitAngularLimitDrag();
                    return true;
                }

                if (allowInput && g_angularLimitDrag.viewportIndex >= 0) {
                    const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(g_angularLimitDrag.viewportIndex);
                    const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(g_angularLimitDrag.viewportIndex);
                    UpdateAngularLimitDrag(rayOrigin, glm::normalize(rayDirection));
                }
                return true;
            }

            if (!allowInput || !g_document.loaded || g_document.editMode != RagdollEditMode::LIMIT) return false;

            RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
            RagdollMarkerAsset* parentMarker = joint ? FindMarker(joint->parentMarkerId) : nullptr;
            if (!joint || !parentMarker || !joint->limitEnabled || g_document.limitScale <= 0.0f) return false;

            const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
            if (viewportIndex < 0) return false;

            Viewport* viewport = ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport) return false;

            const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(viewportIndex);
            const glm::vec3 rayDirection = glm::normalize(Viewports::GetMouseRayDirection(viewportIndex));
            const glm::mat4 parentWorldFrame = CreateJointAuthoringWorldFrame(*parentMarker, joint->parentFrame);
            const glm::vec2 mousePosition = viewport->GetLocalMouseCoords();
            const float handleRadius = g_document.limitScale * g_document.limitHandleScale;
            if (handleRadius <= 0.0f) return false;

            float nearestScreenDistanceSquared = std::numeric_limits<float>::max();
            float nearestDepth = std::numeric_limits<float>::max();

            for (int32_t axisIndex = 0; axisIndex < static_cast<int32_t>(joint->angularLimits.size()); axisIndex++) {
                if (!IsLimited(joint->angularLimits[axisIndex])) continue;

                for (int32_t edgeIndex = 0; edgeIndex < 2; edgeIndex++) {
                    const AngularLimitEdge edge = static_cast<AngularLimitEdge>(edgeIndex);
                    const glm::vec3 endpoint = GetAngularLimitEndpoint(parentWorldFrame, axisIndex, edge, joint->angularLimits[axisIndex].limit, g_document.limitScale);
                    glm::vec2 handlePosition;
                    float handleScreenRadius = 0.0f;
                    float depth = 0.0f;
                    if (!GetAngularLimitHandleScreenBounds(*viewport, static_cast<uint32_t>(viewportIndex), endpoint, handleRadius, handlePosition, handleScreenRadius, depth)) continue;

                    const glm::vec2 handleToMouse = mousePosition - handlePosition;
                    const float screenDistanceSquared = glm::dot(handleToMouse, handleToMouse);
                    const float hitRadius = handleScreenRadius + static_cast<float>(ANGULAR_LIMIT_HANDLE_THICKNESS) * 0.5f;
                    if (screenDistanceSquared > hitRadius * hitRadius) continue;
                    if (screenDistanceSquared > nearestScreenDistanceSquared || (screenDistanceSquared == nearestScreenDistanceSquared && depth >= nearestDepth)) continue;

                    nearestScreenDistanceSquared = screenDistanceSquared;
                    nearestDepth = depth;
                    g_hoveredAngularLimitAxis = axisIndex;
                    g_hoveredAngularLimitEdge = edgeIndex;
                }
            }

            if (g_hoveredAngularLimitAxis < 0) return false;
            if (!Hell::Input::LeftMousePressed()) return true;

            return BeginAngularLimitDrag(
                *joint,
                *parentMarker,
                g_hoveredAngularLimitAxis,
                static_cast<AngularLimitEdge>(g_hoveredAngularLimitEdge),
                viewportIndex,
                rayOrigin,
                rayDirection
            );
        }

        int32_t GetNearestBoneParentIndex(const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            int32_t parentIndex = skinnedModel.m_nodes[nodeIndex].parentIndex;

            // Skip helper transforms between bones
            for (size_t depth = 0; depth < skinnedModel.m_nodes.size() && parentIndex >= 0; depth++) {
                if (parentIndex >= static_cast<int32_t>(skinnedModel.m_nodes.size())) return -1;

                const Node& parentNode = skinnedModel.m_nodes[parentIndex];
                if (skinnedModel.m_boneMapping.find(parentNode.name) != skinnedModel.m_boneMapping.end()) return parentIndex;
                parentIndex = parentNode.parentIndex;
            }

            return -1;
        }

        bool IsWeaponBoneOrDescendant(const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            for (size_t depth = 0; depth < skinnedModel.m_nodes.size() && nodeIndex >= 0; depth++) {
                if (nodeIndex >= static_cast<int32_t>(skinnedModel.m_nodes.size())) return false;

                const Node& node = skinnedModel.m_nodes[nodeIndex];
                if (node.name == "weapon") return true;
                nodeIndex = node.parentIndex;
            }

            return false;
        }

        std::vector<glm::mat4> BuildBindPoseTransforms(const SkinnedModel& skinnedModel) {
            std::vector<glm::mat4> globalTransforms(skinnedModel.m_nodes.size(), glm::mat4(1.0f));

            // Build every node so helper transforms are preserved
            for (size_t nodeIndex = 0; nodeIndex < skinnedModel.m_nodes.size(); nodeIndex++) {
                const Node& node = skinnedModel.m_nodes[nodeIndex];
                if (node.parentIndex >= 0 && node.parentIndex < static_cast<int32_t>(nodeIndex)) {
                    globalTransforms[nodeIndex] = globalTransforms[node.parentIndex] * node.localBindTransform;
                }
                else {
                    globalTransforms[nodeIndex] = node.localBindTransform;
                }
            }

            // Match the SkinnedGameObject's uniform asset scale while keeping
            // authoring transforms rigid for marker and joint creation.
            for (glm::mat4& transform : globalTransforms) {
                const glm::vec3 scaledPosition = glm::vec3(transform[3]) * g_document.asset.skinnedModelScale;
                transform = Hell::Math::RemoveScaleAndShear(transform);
                transform[3] = glm::vec4(scaledPosition, 1.0f);
            }

            return globalTransforms;
        }

        bool IsSelectableBoneNode(const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(skinnedModel.m_nodes.size())) return false;
            if (IsWeaponBoneOrDescendant(skinnedModel, nodeIndex)) return false;
            return std::find(skinnedModel.m_boneNodeIndices.begin(), skinnedModel.m_boneNodeIndices.end(), nodeIndex) != skinnedModel.m_boneNodeIndices.end();
        }

        std::string GetBonePath(const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            std::vector<std::string> names;
            for (size_t depth = 0; depth < skinnedModel.m_nodes.size() && nodeIndex >= 0; depth++) {
                if (nodeIndex >= static_cast<int32_t>(skinnedModel.m_nodes.size())) return {};
                names.push_back(skinnedModel.m_nodes[nodeIndex].name);
                nodeIndex = skinnedModel.m_nodes[nodeIndex].parentIndex;
            }

            std::string path;
            for (auto name = names.rbegin(); name != names.rend(); ++name) {
                if (!path.empty()) path += '|';
                path += *name;
            }
            return path;
        }

        const RagdollMarkerAsset* FindMarkerInAsset(const RagdollAsset& asset, RagdollMarkerId markerId) {
            for (const RagdollMarkerAsset& marker : asset.markers) {
                if (marker.id == markerId) return &marker;
            }
            return nullptr;
        }

        const RagdollJointAsset* FindIncomingJointInAsset(const RagdollAsset& asset, RagdollMarkerId markerId) {
            for (const RagdollJointAsset& joint : asset.joints) {
                if (joint.childMarkerId == markerId) return &joint;
            }
            return nullptr;
        }

        RagdollMarkerId FindBoundMarkerId(const RagdollAsset& asset, const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(skinnedModel.m_nodes.size())) return INVALID_RAGDOLL_MARKER_ID;

            const Node& node = skinnedModel.m_nodes[nodeIndex];
            const std::string bonePath = GetBonePath(skinnedModel, nodeIndex);
            for (const RagdollMarkerAsset& marker : asset.markers) {
                if (!marker.bonePath.empty() && marker.bonePath == bonePath) return marker.id;
            }
            for (const RagdollMarkerAsset& marker : asset.markers) {
                if (!marker.boneName.empty() && marker.boneName == node.name) return marker.id;
            }
            return INVALID_RAGDOLL_MARKER_ID;
        }

        int32_t FindBoundBoneNodeIndex(const RagdollMarkerAsset& marker, const SkinnedModel& skinnedModel) {
            if (!marker.bonePath.empty()) {
                for (size_t nodeIndex = 0; nodeIndex < skinnedModel.m_nodes.size(); nodeIndex++) {
                    if (GetBonePath(skinnedModel, static_cast<int32_t>(nodeIndex)) == marker.bonePath) {
                        return static_cast<int32_t>(nodeIndex);
                    }
                }
            }

            if (!marker.boneName.empty()) {
                for (size_t nodeIndex = 0; nodeIndex < skinnedModel.m_nodes.size(); nodeIndex++) {
                    if (skinnedModel.m_nodes[nodeIndex].name == marker.boneName) {
                        return static_cast<int32_t>(nodeIndex);
                    }
                }
            }

            return -1;
        }

        bool IsFiniteMatrix(const glm::mat4& matrix) {
            for (int32_t column = 0; column < 4; column++) {
                for (int32_t row = 0; row < 4; row++) {
                    if (!std::isfinite(matrix[column][row])) return false;
                }
            }
            return true;
        }

        RagdollMarkerId FindNearestAncestorMarkerId(const RagdollAsset& asset, const SkinnedModel& skinnedModel, int32_t nodeIndex) {
            nodeIndex = GetNearestBoneParentIndex(skinnedModel, nodeIndex);
            for (size_t depth = 0; depth < skinnedModel.m_nodes.size() && nodeIndex >= 0; depth++) {
                const RagdollMarkerId markerId = FindBoundMarkerId(asset, skinnedModel, nodeIndex);
                if (markerId != INVALID_RAGDOLL_MARKER_ID) return markerId;
                nodeIndex = GetNearestBoneParentIndex(skinnedModel, nodeIndex);
            }
            return INVALID_RAGDOLL_MARKER_ID;
        }

        void MakeCapsuleShape(RagdollShape& shape, const glm::vec3& localDirection) {
            const float segmentLength = glm::length(localDirection);
            shape.type = RagdollShapeType::CAPSULE;
            shape.offset = localDirection * 0.5f;
            const glm::quat rotation = Hell::Math::RotationFromTo(glm::vec3(1.0f, 0.0f, 0.0f), localDirection);
            shape.rotationRadians = Hell::Math::QuaternionToEulerXYZ(rotation);
            shape.length = segmentLength;
            shape.radius = std::max(segmentLength * 0.1f, MIN_SHAPE_RADIUS);
            SynchronizeShapeDimensions(shape);
        }

        void InferMarkerShape(const SkinnedModel& skinnedModel, int32_t nodeIndex, const std::vector<glm::mat4>& globalTransforms, RagdollShape& shape) {
            std::vector<int32_t> childBoneIndices;
            for (int32_t candidateIndex : skinnedModel.m_boneNodeIndices) {
                if (candidateIndex >= 0 && candidateIndex < static_cast<int32_t>(globalTransforms.size()) && GetNearestBoneParentIndex(skinnedModel, candidateIndex) == nodeIndex) {
                    childBoneIndices.push_back(candidateIndex);
                }
            }

            const glm::mat4 inverseBoneTransform = glm::inverse(globalTransforms[nodeIndex]);
            if (childBoneIndices.size() == 1) {
                const glm::vec3 localDirection = glm::vec3(inverseBoneTransform * glm::vec4(glm::vec3(globalTransforms[childBoneIndices.front()][3]), 1.0f));
                if (glm::dot(localDirection, localDirection) > SHAPE_SCALE_EPSILON) {
                    MakeCapsuleShape(shape, localDirection);
                    return;
                }
            }

            if (childBoneIndices.size() > 1) {
                glm::vec3 minimum(0.0f);
                glm::vec3 maximum(0.0f);
                for (int32_t childIndex : childBoneIndices) {
                    const glm::vec3 localPosition = glm::vec3(inverseBoneTransform * glm::vec4(glm::vec3(globalTransforms[childIndex][3]), 1.0f));
                    minimum = glm::min(minimum, localPosition);
                    maximum = glm::max(maximum, localPosition);
                }

                const float minimumThickness = 0.01f;
                shape.type = RagdollShapeType::BOX;
                shape.offset = (minimum + maximum) * 0.5f;
                shape.extents = glm::max(maximum - minimum, glm::vec3(minimumThickness));
                SynchronizeShapeDimensions(shape);
                return;
            }

            const int32_t parentIndex = GetNearestBoneParentIndex(skinnedModel, nodeIndex);
            if (parentIndex >= 0 && parentIndex < static_cast<int32_t>(globalTransforms.size())) {
                const glm::vec3 localDirection = glm::vec3(inverseBoneTransform * glm::vec4(glm::vec3(globalTransforms[parentIndex][3]), 1.0f));
                if (glm::dot(localDirection, localDirection) > SHAPE_SCALE_EPSILON) {
                    MakeCapsuleShape(shape, localDirection);
                    return;
                }
            }

            float nearestSegmentLength = std::numeric_limits<float>::max();
            for (int32_t candidateIndex : skinnedModel.m_boneNodeIndices) {
                const int32_t candidateParentIndex = GetNearestBoneParentIndex(skinnedModel, candidateIndex);
                if (candidateIndex < 0 || candidateIndex >= static_cast<int32_t>(globalTransforms.size()) || candidateParentIndex < 0 || candidateParentIndex >= static_cast<int32_t>(globalTransforms.size())) continue;
                const float length = glm::distance(glm::vec3(globalTransforms[candidateIndex][3]), glm::vec3(globalTransforms[candidateParentIndex][3]));
                if (length > SHAPE_SCALE_EPSILON) nearestSegmentLength = std::min(nearestSegmentLength, length);
            }

            shape.type = RagdollShapeType::SPHERE;
            shape.radius = nearestSegmentLength < std::numeric_limits<float>::max() ? nearestSegmentLength * 0.1f : 1.0f;
            SynchronizeShapeDimensions(shape);
        }

        RagdollMarkerId AllocateMarkerId(const RagdollAsset& asset) {
            RagdollMarkerId maximumId = INVALID_RAGDOLL_MARKER_ID;
            for (const RagdollMarkerAsset& marker : asset.markers) maximumId = std::max(maximumId, marker.id);
            if (maximumId == std::numeric_limits<RagdollMarkerId>::max()) return INVALID_RAGDOLL_MARKER_ID;
            return maximumId + 1;
        }

        RagdollJointAsset CreateJointAsset(const RagdollMarkerAsset& parentMarker, const RagdollMarkerAsset& childMarker) {
            RagdollJointAsset joint;
            joint.name = parentMarker.name + "_to_" + childMarker.name;
            joint.parentMarkerId = parentMarker.id;
            joint.childMarkerId = childMarker.id;
            joint.childFrame = glm::mat4(1.0f);

            const glm::vec3 absoluteOffset = glm::abs(childMarker.shape.offset);
            int32_t mainAxisIndex = 0;
            if (absoluteOffset.y > absoluteOffset.x) mainAxisIndex = 1;
            if (absoluteOffset.z > absoluteOffset[mainAxisIndex]) mainAxisIndex = 2;
            if (childMarker.shape.offset[mainAxisIndex] < 0.0f) {
                const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
                const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
                glm::quat flip = mainAxisIndex == 0
                    ? glm::angleAxis(glm::pi<float>(), yAxis)
                    : glm::angleAxis(glm::pi<float>(), xAxis);
                flip *= glm::angleAxis(glm::pi<float>(), mainAxisIndex == 0 ? xAxis : yAxis);
                joint.childFrame *= glm::mat4_cast(glm::normalize(flip));
            }

            joint.parentFrame = glm::inverse(parentMarker.restTransform) * childMarker.restTransform * joint.childFrame;
            AlignJointFrameTranslationsToChildAnchor(parentMarker, childMarker, joint.parentFrame, joint.childFrame);
            for (RagdollAxisLimit& angularLimit : joint.angularLimits) angularLimit.motion = RagdollAxisMotion::FREE;
            return joint;
        }

        bool WouldCreateParentCycle(const RagdollAsset& asset, RagdollMarkerId childMarkerId, RagdollMarkerId parentMarkerId) {
            RagdollMarkerId ancestorId = parentMarkerId;
            for (size_t depth = 0; depth <= asset.markers.size() && ancestorId != INVALID_RAGDOLL_MARKER_ID; depth++) {
                if (ancestorId == childMarkerId) return true;
                const RagdollJointAsset* incomingJoint = FindIncomingJointInAsset(asset, ancestorId);
                ancestorId = incomingJoint ? incomingJoint->parentMarkerId : INVALID_RAGDOLL_MARKER_ID;
            }
            return false;
        }

        std::string GetMarkerOptionBaseLabel(const RagdollMarkerAsset& marker) {
            if (!marker.name.empty()) return marker.name;
            if (!marker.boneName.empty()) return marker.boneName;
            return "Shape " + std::to_string(marker.id);
        }

        void SelectBoneNode(int32_t nodeIndex) {
            const SkinnedModel* skinnedModel = GetSkinnedModel();
            if (!skinnedModel || !IsSelectableBoneNode(*skinnedModel, nodeIndex)) {
                ClearSelection();
                return;
            }

            const RagdollMarkerId markerId = FindBoundMarkerId(g_document.asset, *skinnedModel, nodeIndex);
            if (markerId != INVALID_RAGDOLL_MARKER_ID) {
                SelectMarker(markerId);
                return;
            }

            if (g_selectedMarkerId == INVALID_RAGDOLL_MARKER_ID && g_selectedBoneNodeIndex == nodeIndex) return;

            FinishShapePoseDrag();
            ResetAngularLimitInteraction();
            Gizmo::CancelInteraction();
            g_selectedMarkerId = INVALID_RAGDOLL_MARKER_ID;
            g_selectedBoneNodeIndex = nodeIndex;
            g_selectedBoneParentMarkerId = FindNearestAncestorMarkerId(g_document.asset, *skinnedModel, nodeIndex);
            SetEditMode(RagdollEditMode::NONE);
        }
    }

    bool New(const std::string& name, std::string& error) {
        Document newDocument;
        newDocument.asset.name = name;
        newDocument.sourcePath = "res/ragdolls/" + name + ".ragdoll";
        newDocument.loaded = true;

        if (!Hell::AssetLoader::SaveRagdollAsset(newDocument.sourcePath, newDocument.asset, error)) return false;

        AdoptDocument(std::move(newDocument));
        error.clear();
        return true;
    }

    bool OpenNative(const std::string& path, std::string& error) {
        Document openedDocument;
        if (!Hell::AssetLoader::LoadRagdollAsset(path, openedDocument.asset, error)) return false;
        if (!RagdollMass::Recalculate(openedDocument.asset, error)) {
            error = "Failed to open '" + path + "': " + error;
            return false;
        }

        openedDocument.sourcePath = path;
        openedDocument.dirty = false;
        openedDocument.loaded = true;
        AdoptDocument(std::move(openedDocument));
        error.clear();
        return true;
    }

    bool ImportLegacy(const std::string& path, std::string& error) {
        Document importedDocument;
        std::string importError;

        // Keep the current document if import fails
        if (!Hell::AssetLoader::ImportLegacyRagdollAsset(path, importedDocument.asset, importedDocument.importWarnings, importError)) {
            error = std::move(importError);
            return false;
        }

        importedDocument.sourcePath = "res/ragdolls/" + Hell::File::GetName(path) + ".ragdoll";
        importedDocument.legacySourcePath = path;
        importedDocument.dirty = true;
        importedDocument.loaded = true;
        AdoptDocument(std::move(importedDocument));
        error.clear();
        return true;
    }

    bool Save(std::string& error) {
        if (!g_document.loaded) {
            error = "No ragdoll is open";
            return false;
        }
        RagdollAsset savedAsset = g_document.asset;
        if (!RagdollMass::Recalculate(savedAsset, error)) return false;
        if (!Hell::AssetLoader::SaveRagdollAsset(g_document.sourcePath, savedAsset, error)) return false;

        g_document.asset = std::move(savedAsset);
        g_document.dirty = false;
        error.clear();
        return true;
    }

    bool SaveAs(const std::string& name, std::string& error) {
        if (!g_document.loaded) {
            error = "No ragdoll is open";
            return false;
        }
        if (name.empty()) {
            error = "Enter a ragdoll name";
            return false;
        }
        if (name == "." || name == ".." ||
            name.front() == ' ' || name.back() == ' ' || name.back() == '.' ||
            name.find_first_of("<>:\"/\\|?*") != std::string::npos) {
            error = "Enter a ragdoll name without a path or invalid filename characters";
            return false;
        }
        if (Hell::String::ToLower(Hell::File::GetExtension(name)) == "ragdoll") {
            error = "Enter the ragdoll name without the .ragdoll extension";
            return false;
        }

        const std::string path = "res/ragdolls/" + name + ".ragdoll";
        if (Hell::File::Exists(path)) {
            error = "Ragdoll '" + name + "' already exists";
            return false;
        }
        RagdollAsset savedAsset = g_document.asset;
        savedAsset.name = name;
        if (!RagdollMass::Recalculate(savedAsset, error)) return false;
        if (!Hell::AssetLoader::SaveRagdollAsset(path, savedAsset, error)) return false;

        g_document.asset = std::move(savedAsset);
        g_document.sourcePath = path;
        g_document.legacySourcePath.clear();
        g_document.importWarnings.clear();
        g_document.dirty = false;
        error.clear();
        return true;
    }

    bool RevertFromDisk(std::string& error) {
        if (!g_document.loaded) {
            error = "No ragdoll is open";
            return false;
        }

        const std::string sourcePath = g_document.sourcePath;
        const std::string legacySourcePath = g_document.legacySourcePath;
        bool reverted = false;
        if (!sourcePath.empty() && Hell::File::Exists(sourcePath)) {
            reverted = OpenNative(sourcePath, error);
        }
        else if (!legacySourcePath.empty() && Hell::File::Exists(legacySourcePath)) {
            reverted = ImportLegacy(legacySourcePath, error);
        }
        else {
            error = "The ragdoll source file no longer exists";
            return false;
        }

        if (!reverted) return false;

        Debug::BlitQuickDebugMessage("Ragdoll reverted");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        error.clear();
        return true;
    }

    bool SetSkinnedModelPresetName(const std::string& name) {
        if (!g_document.loaded) return false;
        if (g_document.asset.skinnedModelPresetName == name) return true;

        DestroySkinnedModelPreview();
        g_document.asset.skinnedModelPresetName = name;
        g_document.asset.testAnimationName.clear();
        if (name.empty()) g_document.showSkinnedModel = false;

        // Native skeleton signatures are not defined yet
        g_document.asset.skeletonSignature = 0;
        ClearSelection();
        g_document.dirty = true;
        CreateSkinnedModelPreview();
        return true;
    }

    bool SetSkinnedModelScale(float scale) {
        if (!g_document.loaded || !std::isfinite(scale) || scale <= 0.0f) return false;
        if (g_document.asset.skinnedModelScale == scale) return true;

        g_document.asset.skinnedModelScale = scale;
        g_document.dirty = true;
        SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_previewSkinnedGameObjectId);
        if (skinnedGameObject) skinnedGameObject->SetScale(scale);
        return true;
    }

    static bool RetargetMarkersToCurrentBindPoseWithMode(BindPoseRetargetMode mode, std::string& error) {
        if (!g_document.loaded) {
            error = "No ragdoll is open";
            return false;
        }

        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel || skinnedModel->m_nodes.empty() || skinnedModel->m_boneNodeIndices.empty()) {
            error = "Select an available skinned model before retargeting";
            return false;
        }
        if (g_document.asset.markers.empty()) {
            error = "The ragdoll has no markers to retarget";
            return false;
        }

        FinishShapePoseDrag();
        ResetAngularLimitInteraction();
        Gizmo::CancelInteraction();

        const std::vector<glm::mat4> physicalBindTransforms = BuildBindPoseTransforms(*skinnedModel);
        RagdollAsset updatedAsset = g_document.asset;
        std::vector<glm::quat> preservedWorldLimitRotations;
        if (mode == BindPoseRetargetMode::PRESERVE_WORLD_LIMIT_AXES) {
            preservedWorldLimitRotations.reserve(updatedAsset.joints.size());
            for (const RagdollJointAsset& joint : updatedAsset.joints) {
                const RagdollMarkerAsset* parentMarker = FindMarkerInAsset(updatedAsset, joint.parentMarkerId);
                if (!parentMarker) {
                    error = "Joint '" + joint.name + "' references a missing parent marker";
                    return false;
                }

                const glm::mat4 worldFrame = CreateJointAuthoringWorldFrame(*parentMarker, joint.parentFrame);
                if (!IsFiniteMatrix(worldFrame)) {
                    error = "Joint '" + joint.name + "' has an invalid limit frame";
                    return false;
                }
                preservedWorldLimitRotations.push_back(Hell::Math::ExtractRotation(worldFrame));
            }
        }

        for (RagdollMarkerAsset& marker : updatedAsset.markers) {
            const int32_t nodeIndex = FindBoundBoneNodeIndex(marker, *skinnedModel);
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(physicalBindTransforms.size())) {
                const std::string markerLabel = !marker.name.empty() ? marker.name : std::to_string(marker.id);
                error = "Marker '" + markerLabel + "' does not resolve to a bone in the current skinned model";
                return false;
            }
            const float inputDeterminant = glm::determinant(marker.inputTransform);
            if (!IsFiniteMatrix(marker.inputTransform) || !std::isfinite(inputDeterminant) || std::abs(inputDeterminant) <= SHAPE_SCALE_EPSILON) {
                const std::string markerLabel = !marker.name.empty() ? marker.name : std::to_string(marker.id);
                error = "Marker '" + markerLabel + "' has a non-invertible input transform";
                return false;
            }

            glm::mat4 newInputTransform = physicalBindTransforms[nodeIndex];
            if (!IsFiniteMatrix(newInputTransform)) {
                error = "The current bind pose contains an invalid transform for bone '" + skinnedModel->m_nodes[nodeIndex].name + "'";
                return false;
            }

            const glm::mat4 bindDelta = newInputTransform * glm::inverse(marker.inputTransform);
            marker.restTransform = bindDelta * marker.restTransform;
            marker.bodyTransform = bindDelta * marker.bodyTransform;
            marker.inputTransform = newInputTransform;
            marker.boneName = skinnedModel->m_nodes[nodeIndex].name;
            marker.bonePath = GetBonePath(*skinnedModel, nodeIndex);
        }

        for (size_t jointIndex = 0; jointIndex < updatedAsset.joints.size(); jointIndex++) {
            RagdollJointAsset& joint = updatedAsset.joints[jointIndex];
            const RagdollMarkerAsset* parentMarker = FindMarkerInAsset(updatedAsset, joint.parentMarkerId);
            const RagdollMarkerAsset* childMarker = FindMarkerInAsset(updatedAsset, joint.childMarkerId);
            if (!parentMarker || !childMarker) {
                error = "Joint '" + joint.name + "' references a missing marker";
                return false;
            }

            const glm::quat parentRestRotation = Hell::Math::ExtractRotation(parentMarker->restTransform);
            const glm::quat childRestRotation = Hell::Math::ExtractRotation(childMarker->restTransform);
            glm::quat worldFrameRotation;
            if (mode == BindPoseRetargetMode::PRESERVE_WORLD_LIMIT_AXES) {
                worldFrameRotation = preservedWorldLimitRotations[jointIndex];
                const glm::quat parentFrameRotation = glm::normalize(glm::inverse(parentRestRotation) * worldFrameRotation);
                Hell::Math::SetRotationPreserveTranslation(joint.parentFrame, parentFrameRotation);
            }
            else {
                // Keep the constraint axes attached to the parent marker.
                const glm::quat parentFrameRotation = Hell::Math::ExtractRotation(joint.parentFrame);
                worldFrameRotation = glm::normalize(parentRestRotation * parentFrameRotation);
            }

            // Both local frames must resolve to the same world-space basis.
            const glm::quat childFrameRotation = glm::normalize(glm::inverse(childRestRotation) * worldFrameRotation);
            Hell::Math::SetRotationPreserveTranslation(joint.childFrame, childFrameRotation);
            AlignJointFrameTranslationsToChildAnchor(*parentMarker, *childMarker, joint.parentFrame, joint.childFrame);
        }

        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        g_preview.dirty = true;
        g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
        error.clear();
        return true;
    }

    bool RetargetMarkersToCurrentBindPose(std::string& error) {
        return RetargetMarkersToCurrentBindPoseWithMode(BindPoseRetargetMode::FOLLOW_PARENT, error);
    }

    bool RetargetMarkersToCurrentBindPosePreserveLimitAxes(std::string& error) {
        return RetargetMarkersToCurrentBindPoseWithMode(BindPoseRetargetMode::PRESERVE_WORLD_LIMIT_AXES, error);
    }

    bool SetTestAnimationName(const std::string& name) {
        if (!g_document.loaded) return false;
        if (g_document.asset.testAnimationName == name) return true;

        g_document.asset.testAnimationName = name;
        g_document.dirty = true;
        return true;
    }

    bool CreateMarkerForSelectedBone(std::string& error) {
        if (!g_document.loaded) {
            error = "No ragdoll is open";
            return false;
        }

        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel || !IsSelectableBoneNode(*skinnedModel, g_selectedBoneNodeIndex)) {
            error = "Select an unassigned skeleton bone";
            return false;
        }
        if (FindBoundMarkerId(g_document.asset, *skinnedModel, g_selectedBoneNodeIndex) != INVALID_RAGDOLL_MARKER_ID) {
            error = "That bone already has a shape";
            return false;
        }

        const std::vector<glm::mat4> globalTransforms = BuildBindPoseTransforms(*skinnedModel);
        if (g_selectedBoneNodeIndex >= static_cast<int32_t>(globalTransforms.size())) {
            error = "The selected bone has no bind-pose transform";
            return false;
        }

        RagdollAsset updatedAsset = g_document.asset;
        const RagdollMarkerId markerId = AllocateMarkerId(updatedAsset);
        if (markerId == INVALID_RAGDOLL_MARKER_ID) {
            error = "The ragdoll has no marker ids remaining";
            return false;
        }

        const Node& node = skinnedModel->m_nodes[g_selectedBoneNodeIndex];
        RagdollMarkerAsset marker;
        marker.id = markerId;
        marker.name = node.name;
        marker.boneName = node.name;
        marker.bonePath = GetBonePath(*skinnedModel, g_selectedBoneNodeIndex);
        marker.inputTransform = globalTransforms[g_selectedBoneNodeIndex];
        marker.restTransform = globalTransforms[g_selectedBoneNodeIndex];
        marker.bodyTransform = globalTransforms[g_selectedBoneNodeIndex];

        const std::array<glm::vec4, 6> MARKER_COLORS = {
            glm::vec4(0.35f, 0.75f, 1.0f, 1.0f),
            glm::vec4(0.45f, 1.0f, 0.55f, 1.0f),
            glm::vec4(1.0f, 0.65f, 0.35f, 1.0f),
            glm::vec4(0.85f, 0.45f, 1.0f, 1.0f),
            glm::vec4(1.0f, 0.9f, 0.35f, 1.0f),
            glm::vec4(0.4f, 1.0f, 0.9f, 1.0f)
        };
        marker.color = MARKER_COLORS[(markerId - 1) % MARKER_COLORS.size()];
        InferMarkerShape(*skinnedModel, g_selectedBoneNodeIndex, globalTransforms, marker.shape);
        updatedAsset.markers.push_back(marker);

        if (g_selectedBoneParentMarkerId != INVALID_RAGDOLL_MARKER_ID) {
            const RagdollMarkerAsset* parentMarker = FindMarkerInAsset(updatedAsset, g_selectedBoneParentMarkerId);
            const RagdollMarkerAsset* childMarker = FindMarkerInAsset(updatedAsset, markerId);
            if (!parentMarker || !childMarker) {
                error = "The selected parent shape is no longer available";
                return false;
            }
            updatedAsset.joints.push_back(CreateJointAsset(*parentMarker, *childMarker));
        }

        if (!RagdollMass::Recalculate(updatedAsset, error)) return false;

        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        g_preview.dirty = true;
        g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
        SelectMarker(markerId);
        error.clear();
        return true;
    }

    bool DeleteSelectedMarker(std::string& error) {
        const RagdollMarkerAsset* selectedMarker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !selectedMarker) {
            error = "Select a ragdoll shape";
            return false;
        }

        const RagdollMarkerId markerId = selectedMarker->id;
        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers.erase(
            std::remove_if(updatedAsset.markers.begin(), updatedAsset.markers.end(), [markerId](const RagdollMarkerAsset& marker) {
                return marker.id == markerId;
            }),
            updatedAsset.markers.end()
        );
        updatedAsset.joints.erase(
            std::remove_if(updatedAsset.joints.begin(), updatedAsset.joints.end(), [markerId](const RagdollJointAsset& joint) {
                return joint.parentMarkerId == markerId || joint.childMarkerId == markerId;
            }),
            updatedAsset.joints.end()
        );

        if (!RagdollMass::Recalculate(updatedAsset, error)) return false;

        SelectMarker(INVALID_RAGDOLL_MARKER_ID);
        ReleasePreviewMeshes();
        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        g_preview.dirty = true;
        g_preview.dirtyMarkerId = INVALID_RAGDOLL_MARKER_ID;
        error.clear();
        return true;
    }

    bool SetSelectedBoneParent(RagdollMarkerId parentMarkerId) {
        if (!HasSelectedBone()) return false;
        if (parentMarkerId != INVALID_RAGDOLL_MARKER_ID && !FindMarker(parentMarkerId)) return false;
        g_selectedBoneParentMarkerId = parentMarkerId;
        return true;
    }

    bool SetSelectedMarkerParent(RagdollMarkerId parentMarkerId, std::string& error) {
        RagdollMarkerAsset* childMarker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !childMarker) {
            error = "Select a ragdoll shape";
            return false;
        }
        if (parentMarkerId == childMarker->id || WouldCreateParentCycle(g_document.asset, childMarker->id, parentMarkerId)) {
            error = "That parent would create a cycle in the ragdoll hierarchy";
            return false;
        }
        if (parentMarkerId != INVALID_RAGDOLL_MARKER_ID && !FindMarker(parentMarkerId)) {
            error = "The selected parent shape is no longer available";
            return false;
        }

        const RagdollJointAsset* currentJoint = FindIncomingJoint(childMarker->id);
        const RagdollMarkerId currentParentId = currentJoint ? currentJoint->parentMarkerId : INVALID_RAGDOLL_MARKER_ID;
        if (currentParentId == parentMarkerId) {
            error.clear();
            return true;
        }

        RagdollAsset updatedAsset = g_document.asset;
        RagdollJointAsset updatedJoint;
        bool preserveJointSettings = false;
        for (const RagdollJointAsset& joint : updatedAsset.joints) {
            if (joint.childMarkerId == childMarker->id) {
                updatedJoint = joint;
                preserveJointSettings = true;
                break;
            }
        }
        updatedAsset.joints.erase(
            std::remove_if(updatedAsset.joints.begin(), updatedAsset.joints.end(), [childMarkerId = childMarker->id](const RagdollJointAsset& joint) {
                return joint.childMarkerId == childMarkerId;
            }),
            updatedAsset.joints.end()
        );

        if (parentMarkerId != INVALID_RAGDOLL_MARKER_ID) {
            const RagdollMarkerAsset* updatedParentMarker = FindMarkerInAsset(updatedAsset, parentMarkerId);
            const RagdollMarkerAsset* updatedChildMarker = FindMarkerInAsset(updatedAsset, childMarker->id);
            if (!updatedParentMarker || !updatedChildMarker) {
                error = "Failed to resolve the new parent joint";
                return false;
            }

            if (preserveJointSettings) {
                updatedJoint.name = updatedParentMarker->name + "_to_" + updatedChildMarker->name;
                updatedJoint.parentMarkerId = parentMarkerId;
                updatedJoint.childMarkerId = updatedChildMarker->id;
                updatedJoint.parentFrame = glm::inverse(updatedParentMarker->restTransform) * updatedChildMarker->restTransform * updatedJoint.childFrame;
                AlignJointFrameTranslationsToChildAnchor(*updatedParentMarker, *updatedChildMarker, updatedJoint.parentFrame, updatedJoint.childFrame);
            }
            else {
                updatedJoint = CreateJointAsset(*updatedParentMarker, *updatedChildMarker);
            }
            updatedAsset.joints.push_back(std::move(updatedJoint));
        }

        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        const RagdollEditMode preferredMode = g_document.editMode != RagdollEditMode::NONE ? g_document.editMode : g_document.lastEditMode;
        SetEditMode(ResolveEditMode(preferredMode));
        error.clear();
        return true;
    }

    bool SetSelectedMarkerShapeType(RagdollShapeType type) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size()) return false;
        if (g_document.asset.markers[markerIndex].shape.type == type) return true;

        if (type == RagdollShapeType::CONVEX_HULL && (g_document.asset.markers[markerIndex].shape.convexVertices.empty() || g_document.asset.markers[markerIndex].shape.convexIndices.empty())) {
            return false;
        }

        FinishShapePoseDrag();

        // Keep the primitive dimensions comparable when changing shape
        RagdollAsset updatedAsset = g_document.asset;
        RagdollShape& shape = updatedAsset.markers[markerIndex].shape;
        SynchronizeShapeDimensions(shape);
        shape.type = type;
        SynchronizeShapeDimensions(shape);
        std::string massError;
        if (!RagdollMass::Recalculate(updatedAsset, massError)) return false;
        g_document.asset = std::move(updatedAsset);
        MarkShapeChanged();
        const RagdollEditMode preferredMode = g_document.editMode != RagdollEditMode::NONE ? g_document.editMode : g_document.lastEditMode;
        SetEditMode(ResolveEditMode(preferredMode));
        return true;
    }

    bool SetSelectedMarkerBoxDimensions(const glm::vec3& dimensions) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size() || g_document.asset.markers[markerIndex].shape.type != RagdollShapeType::BOX) return false;

        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers[markerIndex].shape.extents = dimensions;
        SynchronizeShapeDimensions(updatedAsset.markers[markerIndex].shape);
        std::string massError;
        if (!RagdollMass::Recalculate(updatedAsset, massError)) return false;
        g_document.asset = std::move(updatedAsset);
        MarkShapeChanged();
        return true;
    }

    bool SetSelectedMarkerRadius(float radius) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size()) return false;
        const RagdollShapeType shapeType = g_document.asset.markers[markerIndex].shape.type;
        if (shapeType != RagdollShapeType::SPHERE && shapeType != RagdollShapeType::CAPSULE) return false;

        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers[markerIndex].shape.radius = radius;
        SynchronizeShapeDimensions(updatedAsset.markers[markerIndex].shape);
        std::string massError;
        if (!RagdollMass::Recalculate(updatedAsset, massError)) return false;
        g_document.asset = std::move(updatedAsset);
        MarkShapeChanged();
        return true;
    }

    bool SetSelectedMarkerCapsuleLength(float length) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size() || g_document.asset.markers[markerIndex].shape.type != RagdollShapeType::CAPSULE) return false;

        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers[markerIndex].shape.length = length;
        SynchronizeShapeDimensions(updatedAsset.markers[markerIndex].shape);
        std::string massError;
        if (!RagdollMass::Recalculate(updatedAsset, massError)) return false;
        g_document.asset = std::move(updatedAsset);
        MarkShapeChanged();
        return true;
    }

    bool SetTargetMass(float mass) {
        if (!g_document.loaded || !std::isfinite(mass) || mass <= 0.0f) return false;
        if (g_document.asset.targetMass == mass) return true;
        g_document.asset.targetMass = mass;
        g_document.dirty = true;
        return true;
    }

    bool DistributeMassByVolume(std::string& error) {
        if (!g_document.loaded) return false;

        RagdollAsset distributedAsset = g_document.asset;
        for (RagdollMarkerAsset& marker : distributedAsset.markers) {
            if (!marker.rigidBody.isKinematic) marker.rigidBody.massMode = RagdollMassMode::AUTOMATIC;
        }
        if (!RagdollMass::Recalculate(distributedAsset, error)) return false;

        g_document.asset = std::move(distributedAsset);
        g_document.dirty = true;
        error.clear();
        return true;
    }

    bool RandomizeMarkerColors() {
        if (!g_document.loaded || g_document.asset.markers.empty()) return false;

        const int colorSeed = Hell::Random::Int(0, 1000000);
        const bool canUpdatePreview = !g_preview.dirty && g_preview.renderItems.size() == g_document.asset.markers.size();

        for (size_t markerIndex = 0; markerIndex < g_document.asset.markers.size(); markerIndex++) {
            const glm::vec3 color = Hell::Color::Random(colorSeed + static_cast<int>(markerIndex));
            RagdollMarkerAsset& marker = g_document.asset.markers[markerIndex];
            marker.color = glm::vec4(color, 1.0f);

            if (canUpdatePreview) {
                RenderItem& renderItem = g_preview.renderItems[markerIndex];
                renderItem.tintColorR = color.r;
                renderItem.tintColorG = color.g;
                renderItem.tintColorB = color.b;
            }
        }

        if (!canUpdatePreview) g_preview.dirty = true;
        g_document.dirty = true;
        return true;
    }

    bool RandomizeSelectedMarkerColor() {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size()) return false;

        const glm::vec3 color = Hell::Color::Random(Hell::Random::Int(0, 1000000));
        g_document.asset.markers[markerIndex].color = glm::vec4(color, 1.0f);

        if (!g_preview.dirty && markerIndex < g_preview.renderItems.size()) {
            RenderItem& renderItem = g_preview.renderItems[markerIndex];
            renderItem.tintColorR = color.r;
            renderItem.tintColorG = color.g;
            renderItem.tintColorB = color.b;
        }
        else {
            g_preview.dirty = true;
        }

        g_document.dirty = true;
        return true;
    }

    bool SetSelectedMarkerMassOverrideEnabled(bool enabled, std::string& error) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size()) return false;

        const RagdollMassMode requestedMode = enabled ? RagdollMassMode::OVERRIDE : RagdollMassMode::AUTOMATIC;
        if (g_document.asset.markers[markerIndex].rigidBody.massMode == requestedMode) return true;

        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers[markerIndex].rigidBody.massMode = requestedMode;
        if (!RagdollMass::Recalculate(updatedAsset, error)) return false;

        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        error.clear();
        return true;
    }

    bool SetSelectedMarkerMass(float mass, std::string& error) {
        const size_t markerIndex = FindMarkerIndex(g_selectedMarkerId);
        if (!g_document.loaded || markerIndex >= g_document.asset.markers.size() || !std::isfinite(mass) || mass <= 0.0f) return false;
        if (g_document.asset.markers[markerIndex].rigidBody.massMode != RagdollMassMode::OVERRIDE) return false;
        if (g_document.asset.markers[markerIndex].rigidBody.mass == mass) return true;

        RagdollAsset updatedAsset = g_document.asset;
        updatedAsset.markers[markerIndex].rigidBody.mass = mass;
        if (!RagdollMass::Recalculate(updatedAsset, error)) return false;

        g_document.asset = std::move(updatedAsset);
        g_document.dirty = true;
        error.clear();
        return true;
    }

    bool SetSelectedMarkerLinearDamping(float damping) {
        RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !marker || !std::isfinite(damping) || damping < 0.0f) return false;
        if (marker->rigidBody.linearDamping == damping) return true;
        marker->rigidBody.linearDamping = damping;
        g_document.dirty = true;
        return true;
    }

    bool SetSelectedMarkerAngularDamping(float damping) {
        RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !marker || !std::isfinite(damping) || damping < 0.0f) return false;
        if (marker->rigidBody.angularDamping == damping) return true;
        marker->rigidBody.angularDamping = damping;
        g_document.dirty = true;
        return true;
    }

    bool SetSelectedMarkerFriction(float friction) {
        RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !marker || !std::isfinite(friction) || friction < 0.0f) return false;
        if (marker->rigidBody.friction == friction) return true;
        marker->rigidBody.friction = friction;
        g_document.dirty = true;
        return true;
    }

    bool SetSelectedMarkerShapeOffset(const glm::vec3& offset) {
        RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !marker || !ShapeSupportsPoseEditing(marker->shape)) return false;
        if (!std::isfinite(offset.x) || !std::isfinite(offset.y) || !std::isfinite(offset.z)) return false;
        if (marker->shape.offset.x == offset.x && marker->shape.offset.y == offset.y && marker->shape.offset.z == offset.z) return true;

        FinishShapePoseDrag();
        marker->shape.offset = offset;
        MarkShapeChanged();
        return true;
    }

    bool SetSelectedMarkerShapeRotation(const glm::vec3& rotationDegrees) {
        RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        if (!g_document.loaded || !marker || !ShapeSupportsPoseEditing(marker->shape)) return false;
        if (!std::isfinite(rotationDegrees.x) || !std::isfinite(rotationDegrees.y) || !std::isfinite(rotationDegrees.z)) return false;

        const glm::vec3 rotationRadians = glm::radians(rotationDegrees);
        if (marker->shape.rotationRadians.x == rotationRadians.x && marker->shape.rotationRadians.y == rotationRadians.y && marker->shape.rotationRadians.z == rotationRadians.z) return true;

        FinishShapePoseDrag();
        marker->shape.rotationRadians = rotationRadians;
        MarkShapeChanged();
        return true;
    }

    bool SetSelectedJointLimitsEnabled(bool enabled) {
        RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        if (!g_document.loaded || !joint) return false;
        if (joint->limitEnabled == enabled) return true;

        joint->limitEnabled = enabled;
        g_document.dirty = true;
        return true;
    }

    bool SetSelectedJointAngularLimit(int32_t axisIndex, RagdollAxisMotion motion, float halfRange) {
        ResetAngularLimitInteraction();
        RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        if (!g_document.loaded || !joint) return false;
        if (axisIndex < 0 || axisIndex >= static_cast<int32_t>(joint->angularLimits.size())) return false;
        if (!std::isfinite(halfRange)) return false;

        if (motion == RagdollAxisMotion::LIMITED) {
            if (halfRange <= 0.0f || halfRange > glm::radians(179.0f)) return false;
        }
        else if (motion == RagdollAxisMotion::LOCKED || motion == RagdollAxisMotion::FREE) {
            halfRange = 0.0f;
        }
        else {
            return false;
        }

        RagdollAxisLimit& axisLimit = joint->angularLimits[axisIndex];
        if (axisLimit.motion == motion && axisLimit.limit == halfRange) return true;
        axisLimit.motion = motion;
        axisLimit.limit = halfRange;
        g_document.dirty = true;
        return true;
    }

    bool SetSelectedJointLimitFrameRotation(const glm::vec3& rotationDegrees) {
        ResetAngularLimitInteraction();
        RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        RagdollMarkerAsset* parentMarker = joint ? FindMarker(joint->parentMarkerId) : nullptr;
        if (!g_document.loaded || !joint || !parentMarker) return false;
        if (!std::isfinite(rotationDegrees.x) || !std::isfinite(rotationDegrees.y) || !std::isfinite(rotationDegrees.z)) return false;

        // Inspector rotation is local to the parent rigid
        const glm::quat parentRigidRotation = Hell::Math::ExtractRotation(CreateMarkerRestPose(*parentMarker));
        const glm::quat targetParentLocalRotation = Hell::Math::EulerXYZToQuaternion(glm::radians(rotationDegrees));
        const glm::quat targetParentWorldRotation = glm::normalize(parentRigidRotation * targetParentLocalRotation);
        return SetJointParentWorldRotation(*joint, targetParentWorldRotation);
    }

    bool ResetSelectedJointConstraintFrames() {
        ResetAngularLimitInteraction();
        RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        RagdollMarkerAsset* parentMarker = joint ? FindMarker(joint->parentMarkerId) : nullptr;
        RagdollMarkerAsset* childMarker = joint ? FindMarker(joint->childMarkerId) : nullptr;
        if (!g_document.loaded || !joint || !parentMarker || !childMarker) return false;

        const glm::vec3 childAnchor(joint->childFrame[3]);
        joint->childFrame = glm::translate(glm::mat4(1.0f), childAnchor);

        const glm::vec3 absoluteOffset = glm::abs(childMarker->shape.offset);
        int32_t mainAxisIndex = 0;
        if (absoluteOffset.y > absoluteOffset.x) mainAxisIndex = 1;
        if (absoluteOffset.z > absoluteOffset[mainAxisIndex]) mainAxisIndex = 2;
        if (childMarker->shape.offset[mainAxisIndex] < 0.0f) {
            const glm::vec3 xAxis(1.0f, 0.0f, 0.0f);
            const glm::vec3 yAxis(0.0f, 1.0f, 0.0f);
            glm::quat flip = mainAxisIndex == 0
                ? glm::angleAxis(glm::pi<float>(), yAxis)
                : glm::angleAxis(glm::pi<float>(), xAxis);
            flip *= glm::angleAxis(glm::pi<float>(), mainAxisIndex == 0 ? xAxis : yAxis);
            joint->childFrame *= glm::mat4_cast(glm::normalize(flip));
        }

        joint->parentFrame = glm::inverse(parentMarker->restTransform) * childMarker->restTransform * joint->childFrame;
        AlignJointFrameTranslationsToChildAnchor(*parentMarker, *childMarker, joint->parentFrame, joint->childFrame);
        for (RagdollAxisLimit& angularLimit : joint->angularLimits) {
            angularLimit.motion = RagdollAxisMotion::LIMITED;
            angularLimit.limit = glm::quarter_pi<float>();
        }
        g_document.dirty = true;
        return true;
    }

    void SetSkeletonVisible(bool visible) {
        g_document.showSkeleton = visible;
    }

    bool SetSkinnedModelVisible(bool visible) {
        if (!g_document.loaded || g_document.asset.skinnedModelPresetName.empty()) return false;
        SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_previewSkinnedGameObjectId);
        if (!skinnedGameObject) return false;

        g_document.showSkinnedModel = visible;
        if (visible) skinnedGameObject->EnableRendering();
        else skinnedGameObject->DisableRendering();
        return true;
    }

    void SetLimitsAlwaysVisible(bool visible) {
        g_document.alwaysShowLimits = visible;
    }

    void SetLimitFramesAlwaysVisible(bool visible) {
        g_document.alwaysShowLimitFrames = visible;
    }

    void SetLimitScale(float scale) {
        g_document.limitScale = scale;
    }

    void SetLimitHandleScale(float scale) {
        g_document.limitHandleScale = std::max(0.0f, scale);
    }

    void Reset() {
        SetEditMode(RagdollEditMode::NONE);
        ResetAngularLimitInteraction();
        DestroySkinnedModelPreview();
        ReleasePreviewMeshes();
        g_document = {};
        g_preview = {};
        g_selectedMarkerId = INVALID_RAGDOLL_MARKER_ID;
        g_selectedBoneParentMarkerId = INVALID_RAGDOLL_MARKER_ID;
        g_selectedBoneNodeIndex = -1;
        Gizmo::SetVisible(false);
    }

    void DrawSkeleton() {
        if (!g_document.showSkeleton) return;

        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel) return;

        const std::vector<glm::mat4> globalTransforms = BuildBindPoseTransforms(*skinnedModel);

        // Draw deform bones only
        for (int32_t nodeIndex : skinnedModel->m_boneNodeIndices) {
            if (nodeIndex < 0 || nodeIndex >= static_cast<int32_t>(globalTransforms.size())) continue;

            // Hide weapon bones
            if (IsWeaponBoneOrDescendant(*skinnedModel, nodeIndex)) continue;

            const glm::vec3 position = glm::vec3(globalTransforms[nodeIndex][3]);
            glm::vec4 pointColor = OUTLINE_COLOR;
            const bool selected = nodeIndex == g_selectedBoneNodeIndex || (g_selectedMarkerId != INVALID_RAGDOLL_MARKER_ID && FindBoundMarkerId(g_document.asset, *skinnedModel, nodeIndex) == g_selectedMarkerId);
            if (selected) pointColor = glm::vec4(0.2f, 1.0f, 1.0f, 1.0f);
            DebugDraw::DrawPoint(position, pointColor);

            const int32_t parentIndex = GetNearestBoneParentIndex(*skinnedModel, nodeIndex);
            if (parentIndex < 0 || parentIndex >= static_cast<int32_t>(globalTransforms.size())) continue;

            const glm::vec3 parentPosition = glm::vec3(globalTransforms[parentIndex][3]);
            DebugDraw::DrawLine(position, parentPosition, WHITE);
        }
    }

    void DrawJointLimits() {
        if (!g_document.loaded) return;

        const RagdollJointAsset* selectedJoint = FindIncomingJoint(g_selectedMarkerId);
        const bool showSelectedLimit = g_document.editMode == RagdollEditMode::LIMIT;
        const bool showSelectedLimitFrame = IsLimitFrameEditMode(g_document.editMode);

        for (const RagdollJointAsset& joint : g_document.asset.joints) {
            const bool selected = &joint == selectedJoint;
            const bool drawLimits = g_document.alwaysShowLimits || (selected && showSelectedLimit);
            const bool drawLimitFrame = g_document.alwaysShowLimitFrames || (selected && showSelectedLimitFrame);
            if (drawLimits || drawLimitFrame) {
                DrawJointLimit(joint, selected, drawLimits, drawLimitFrame);
            }
        }
    }

    void SubmitRenderItems() {
        if (!g_document.loaded) return;

        EnsurePreviewMeshes();
        if (g_preview.renderItems.size() != g_document.asset.markers.size()) return;

        for (size_t markerIndex = 0; markerIndex < g_document.asset.markers.size(); markerIndex++) {
            const RagdollMarkerAsset& marker = g_document.asset.markers[markerIndex];
            const RenderItem& renderItem = g_preview.renderItems[markerIndex];
            if (renderItem.meshId == 0) continue;

            RenderDataManager::SubmitRenderItemPhysicsShape(renderItem, marker.id == g_selectedMarkerId);
        }
    }

    void UpdateInput(bool allowKeyboardInput, bool allowMouseInput) {
        if (allowKeyboardInput && g_document.loaded && Hell::Input::KeyPressed(HELL_KEY_1)) {
            const bool visible = !g_document.showSkinnedModel;
            if (SetSkinnedModelVisible(visible)) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                Debug::BlitQuickDebugMessage(std::string("Skinned Model: ") + (visible ? "Visible" : "Hidden"));
            }
        }

        if (allowKeyboardInput && g_document.loaded && Hell::Input::KeyPressed(HELL_KEY_2)) {
            const bool visible = !g_document.showSkeleton;
            SetSkeletonVisible(visible);
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Debug::BlitQuickDebugMessage(std::string("Skeleton: ") + (visible ? "Visible" : "Hidden"));
        }

        if (allowKeyboardInput && g_document.loaded && Hell::Input::KeyPressed(HELL_KEY_3)) {
            const bool visible = !g_document.alwaysShowLimits;
            SetLimitsAlwaysVisible(visible);
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Debug::BlitQuickDebugMessage(std::string("Always Show Limits: ") + (visible ? "On" : "Off"));
        }

        if (allowKeyboardInput && g_document.loaded && Hell::Input::KeyPressed(HELL_KEY_4)) {
            const bool visible = !g_document.alwaysShowLimitFrames;
            SetLimitFramesAlwaysVisible(visible);
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Debug::BlitQuickDebugMessage(std::string("Always Show Limit Frames: ") + (visible ? "On" : "Off"));
        }

        if (allowKeyboardInput && !g_angularLimitDrag.active && Gizmo::GetAction() != GizmoAction::DRAGGING && Hell::Input::KeyPressed(HELL_KEY_TAB)) {
            CycleEditMode();
        }

        bool gizmoOwnsMouse = false;
        switch (g_document.editMode) {
            case RagdollEditMode::LIMIT:
                Gizmo::SetVisible(false);
                gizmoOwnsMouse = UpdateAngularLimitHandles(allowMouseInput);
                break;
            case RagdollEditMode::SHAPE_TRANSLATE:
            case RagdollEditMode::SHAPE_ROTATE:
                gizmoOwnsMouse = UpdateShapeGizmo(allowKeyboardInput && allowMouseInput);
                break;
            case RagdollEditMode::LIMIT_FRAME_TRANSLATE:
            case RagdollEditMode::LIMIT_FRAME_ROTATE:
                gizmoOwnsMouse = UpdateLimitFrameGizmo(allowKeyboardInput && allowMouseInput);
                break;
            case RagdollEditMode::NONE:
                Gizmo::SetVisible(false);
                break;
        }

        if (!allowMouseInput || gizmoOwnsMouse || !g_document.loaded || !Hell::Input::LeftMousePressed()) return;

        const int32_t viewportIndex = Viewports::GetHoveredViewportIndex();
        if (viewportIndex < 0) return;

        Selection::ClearSelection();
        const glm::vec3& rayOrigin = Viewports::GetMouseRayOrigin(viewportIndex);
        const glm::vec3& rayDirection = Viewports::GetMouseRayDirection(viewportIndex);
        const BvhRayResult bvhResult = WorldBVH::ClosestHit(rayOrigin, rayDirection, MAX_PICK_DISTANCE);
        const RagdollMarkerId markerId = bvhResult.hitFound && bvhResult.objectId == 0 ? bvhResult.customId : INVALID_RAGDOLL_MARKER_ID;

        SelectMarker(markerId);
    }

    void SelectBone(int32_t nodeIndex) {
        SelectBoneNode(nodeIndex);
    }

    void SelectMarker(RagdollMarkerId markerId) {
        const RagdollMarkerId selectedMarkerId = markerId != INVALID_RAGDOLL_MARKER_ID && FindMarker(markerId) ? markerId : INVALID_RAGDOLL_MARKER_ID;
        if (g_selectedMarkerId != selectedMarkerId || g_selectedBoneNodeIndex >= 0) {
            FinishShapePoseDrag();
            ResetAngularLimitInteraction();
            Gizmo::CancelInteraction();
            g_selectedMarkerId = selectedMarkerId;
            g_selectedBoneNodeIndex = -1;
            g_selectedBoneParentMarkerId = INVALID_RAGDOLL_MARKER_ID;

            const RagdollEditMode preferredMode = g_document.editMode != RagdollEditMode::NONE ? g_document.editMode : g_document.lastEditMode;
            SetEditMode(ResolveEditMode(preferredMode));
        }
    }

    void ClearSelection() {
        SelectMarker(INVALID_RAGDOLL_MARKER_ID);
    }

    bool HasDocument() {
        return g_document.loaded;
    }

    bool HasSelectedMarker() {
        return GetSelectedMarker() != nullptr;
    }

    bool HasSelectedBone() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        return skinnedModel && IsSelectableBoneNode(*skinnedModel, g_selectedBoneNodeIndex) && FindBoundMarkerId(g_document.asset, *skinnedModel, g_selectedBoneNodeIndex) == INVALID_RAGDOLL_MARKER_ID;
    }

    bool CanSelectBone(int32_t nodeIndex) {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        return skinnedModel && IsSelectableBoneNode(*skinnedModel, nodeIndex);
    }

    bool IsBoneNodeSelected(int32_t nodeIndex) {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!skinnedModel || !IsSelectableBoneNode(*skinnedModel, nodeIndex)) return false;
        if (nodeIndex == g_selectedBoneNodeIndex) return true;
        return g_selectedMarkerId != INVALID_RAGDOLL_MARKER_ID && FindBoundMarkerId(g_document.asset, *skinnedModel, nodeIndex) == g_selectedMarkerId;
    }

    bool IsDirty() {
        return g_document.dirty;
    }

    bool IsSkinnedModelVisible() {
        return g_document.showSkinnedModel;
    }

    bool IsSkeletonVisible() {
        return g_document.showSkeleton;
    }

    RagdollEditMode GetEditMode() {
        return g_document.editMode;
    }

    bool AreLimitsAlwaysVisible() {
        return g_document.alwaysShowLimits;
    }

    bool AreLimitFramesAlwaysVisible() {
        return g_document.alwaysShowLimitFrames;
    }

    float GetLimitScale() {
        return g_document.limitScale;
    }

    float GetLimitHandleScale() {
        return g_document.limitHandleScale;
    }

    float GetTargetMass() {
        return g_document.asset.targetMass;
    }

    float GetCurrentMass() {
        return RagdollMass::ComputeTotalMass(g_document.asset);
    }

    float GetSelectedMarkerVolume() {
        const RagdollMarkerAsset* marker = FindMarker(g_selectedMarkerId);
        return marker ? RagdollMass::ComputeMarkerVolume(*marker) : 0.0f;
    }

    glm::vec3 GetSelectedJointLimitFrameRotation() {
        const RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        if (!joint) return glm::vec3(0.0f);
        return glm::degrees(Hell::Math::QuaternionToEulerXYZ(GetJointFrameRotation(joint->parentFrame)));
    }

    int32_t GetSelectedBoneNodeIndex() {
        return HasSelectedBone() ? g_selectedBoneNodeIndex : -1;
    }

    RagdollMarkerId GetSelectedBoneParentId() {
        return HasSelectedBone() ? g_selectedBoneParentMarkerId : INVALID_RAGDOLL_MARKER_ID;
    }

    RagdollMarkerId GetSelectedMarkerParentId() {
        const RagdollJointAsset* joint = FindIncomingJoint(g_selectedMarkerId);
        return joint ? joint->parentMarkerId : INVALID_RAGDOLL_MARKER_ID;
    }

    const RagdollAsset& GetAsset() {
        return g_document.asset;
    }

    std::vector<RagdollParentOption> GetValidParentOptions(RagdollMarkerId childMarkerId) {
        std::vector<RagdollParentOption> options;
        options.push_back({ INVALID_RAGDOLL_MARKER_ID, "None" });

        std::unordered_map<std::string, size_t> labelCounts;
        labelCounts["None"] = 1;
        for (const RagdollMarkerAsset& marker : g_document.asset.markers) {
            if (marker.id == childMarkerId || WouldCreateParentCycle(g_document.asset, childMarkerId, marker.id)) continue;
            labelCounts[GetMarkerOptionBaseLabel(marker)]++;
        }

        for (const RagdollMarkerAsset& marker : g_document.asset.markers) {
            if (marker.id == childMarkerId || WouldCreateParentCycle(g_document.asset, childMarkerId, marker.id)) continue;

            std::string label = GetMarkerOptionBaseLabel(marker);
            if (labelCounts[label] > 1) label += " (#" + std::to_string(marker.id) + ")";
            options.push_back({ marker.id, std::move(label) });
        }
        return options;
    }

    const RagdollMarkerAsset* GetSelectedMarker() {
        return FindMarker(g_selectedMarkerId);
    }

    const std::vector<RenderItem>& GetRenderItems() {
        EnsurePreviewMeshes();
        return g_preview.renderItems;
    }

    const SkinnedModel* GetSkinnedModel() {
        SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_previewSkinnedGameObjectId);
        return skinnedGameObject ? skinnedGameObject->GetSkinnedModel() : nullptr;
    }

    RagdollMarkerId GetSelectedMarkerId() {
        return g_selectedMarkerId;
    }

    std::string GetSelectedBoneName() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!HasSelectedBone() || !skinnedModel) return {};
        return skinnedModel->m_nodes[g_selectedBoneNodeIndex].name;
    }

    std::string GetSelectedBonePath() {
        const SkinnedModel* skinnedModel = GetSkinnedModel();
        if (!HasSelectedBone() || !skinnedModel) return {};
        return GetBonePath(*skinnedModel, g_selectedBoneNodeIndex);
    }

    const std::string& GetName() {
        return g_document.asset.name;
    }

    const std::string& GetSourcePath() {
        return g_document.sourcePath;
    }

    const std::vector<std::string>& GetImportWarnings() {
        return g_document.importWarnings;
    }
}
