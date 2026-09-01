#include "EditorSession.h"

#include "Unloved/EditorSession/UI/EditorDialogs.h"
#include "EditorHierarchy.h"
#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "Unloved/EditorSession/UI/EditorInputElements.h"
#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "EditorMenuBar.h"
#include "EditorObjectOptions.h"
#include "EditorInspector.h"
#include "EditorPlacement.h"
#include "EditorToolbar.h"
#include "Unloved/EditorSession/Interaction/EditorPointSequences.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/EditorSession/BoneMask/EditorBoneMask.h"
#include "Unloved/EditorSession/Ragdoll/EditorRagdoll.h"
#include "Unloved/EditorSession/UI/EditorUI.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/EditorSession/Interaction/EditorVisibility.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Enum.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

namespace Unloved::EditorSession {
    namespace {
        bool g_isActive = false;
        EditorRenderMode g_renderMode = EditorRenderMode::PBR;
        std::optional<RagdollAsset> g_pendingRagdollTest;
        uint64_t g_ragdollTestAnimatorInstanceId = 0;
        uint32_t g_ragdollTestAnimationLayerIndex = 0;
        uint64_t g_ragdollTestSkinnedGameObjectId = 0;
        uint64_t g_ragdollTestRagdollId = 0;
        std::string g_ragdollTestAnimationName;

        constexpr const char* RAGDOLL_TEST_HOUSE_NAME = "RagdollTestScene";

        void DeactivateEditor() {
            Workspace::Close();
            SetActive(false);
        }

        void DestroyRagdollTest() {
            const bool removedSkinnedGameObject = g_ragdollTestSkinnedGameObjectId != 0 &&
                                                  World::RemoveObjectById(g_ragdollTestSkinnedGameObjectId);
            if (!removedSkinnedGameObject && g_ragdollTestRagdollId != 0) {
                Hell::Physics::MarkRagdollForRemoval(g_ragdollTestRagdollId);
            }

            if (g_ragdollTestAnimatorInstanceId != 0) {
                Animator::RemoveAnimatorInstance(g_ragdollTestAnimatorInstanceId);
            }

            g_ragdollTestAnimatorInstanceId = 0;
            g_ragdollTestAnimationLayerIndex = 0;
            g_ragdollTestSkinnedGameObjectId = 0;
            g_ragdollTestRagdollId = 0;
            g_ragdollTestAnimationName.clear();
        }

        SkinnedGameObject* GetRagdollTestSkinnedGameObject() {
            if (g_ragdollTestSkinnedGameObjectId == 0) return nullptr;

            SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(g_ragdollTestSkinnedGameObjectId);
            if (!skinnedGameObject) {
                g_ragdollTestSkinnedGameObjectId = 0;
            }
            return skinnedGameObject;
        }

        Ragdoll* GetRagdollTestRagdoll() {
            if (g_ragdollTestRagdollId == 0) return nullptr;

            Ragdoll* ragdoll = Hell::Physics::GetRagdollById(g_ragdollTestRagdollId);
            if (!ragdoll) {
                g_ragdollTestRagdollId = 0;
            }
            return ragdoll;
        }

        bool HasUsableSkinnedModel(const RagdollAsset& asset) {
            if (asset.skinnedModelName.empty()) return false;

            SkinnedModel* skinnedModel = Hell::ResourceManager::GetSkinnedModelByName(asset.skinnedModelName);
            return skinnedModel && skinnedModel->GetNodeCount() > 0 && skinnedModel->GetBoneCount() > 0;
        }

        void LaunchRagdollTest(RagdollAsset asset) {
            DestroyRagdollTest();
            RagdollEditor::CleanUpSkinnedModelPreview();
            DeactivateEditor();
            World::LoadSingleHouse(RAGDOLL_TEST_HOUSE_NAME);
            Session::RespawnPlayers();

            const uint64_t standaloneObjectId = GetNextObjectId(ObjectType::RAGDOLL_STANDALONE);
            PhysicsFilterData filterData;
            filterData.raycastGroup = RaycastGroup::RAYCAST_ENABLED;
            filterData.collisionGroup = CollisionGroup::RAGDOLL_ENEMY;
            filterData.collidesWith = CollisionGroup(ENVIROMENT_OBSTACLE | CHARACTER_CONTROLLER | RAGDOLL_ENEMY);

            uint64_t ragdollId = 0;
            if (HasUsableSkinnedModel(asset)) {
                const uint64_t skinnedGameObjectId = World::CreateSkinnedGameObject();
                const uint64_t animatorInstanceId = Animator::CreateAnimatorInstance();
                SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(skinnedGameObjectId);
                AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(animatorInstanceId);
                if (!skinnedGameObject || !animatorInstance) {
                    if (skinnedGameObjectId != 0) World::RemoveObjectById(skinnedGameObjectId);
                    if (animatorInstanceId != 0) Animator::RemoveAnimatorInstance(animatorInstanceId);
                    Logging::Error() << "EditorSession::LaunchRagdollTest() failed to create the skinned game object or animator instance";
                    return;
                }

                animatorInstance->RegisterSkinnedModels({ asset.skinnedModelName });
                const uint32_t animationLayerIndex = animatorInstance->CreateAnimationLayer();
                skinnedGameObject->SetAnimatorInstanceId(animatorInstanceId);

                skinnedGameObject->SetOwnerObjectId(standaloneObjectId);
                skinnedGameObject->SetName("Ragdoll Test");
                if (asset.testMaterialPresetName.empty()) {
                    skinnedGameObject->SetSkinnedModel(asset.skinnedModelName);
                }
                else {
                    skinnedGameObject->SetSkinnedModel(asset.skinnedModelName, asset.testMaterialPresetName);
                }
                skinnedGameObject->SetPosition(glm::vec3(0.0f));
                skinnedGameObject->SetRotationX(0.0f);
                skinnedGameObject->SetRotationY(0.0f);
                skinnedGameObject->SetRotationZ(0.0f);
                skinnedGameObject->SetScale(asset.skinnedModelScale);
                if (asset.testAnimationName.empty()) {
                    skinnedGameObject->SetAnimationModeToBindPose();
                }
                else {
                    skinnedGameObject->SetAnimationModeToAnimated();
                    animatorInstance->PlayAndLoopAnimation(animationLayerIndex, asset.testAnimationName, 1.0f);
                    animatorInstance->RestartAnimation();
                }

                ragdollId = skinnedGameObject->CreateRagdoll(asset, filterData);
                if (ragdollId != 0) {
                    g_ragdollTestAnimatorInstanceId = animatorInstanceId;
                    g_ragdollTestAnimationLayerIndex = animationLayerIndex;
                    g_ragdollTestSkinnedGameObjectId = skinnedGameObjectId;
                }
                else {
                    World::RemoveObjectById(skinnedGameObjectId);
                    Animator::RemoveAnimatorInstance(animatorInstanceId);
                }
            }
            else {
                ragdollId = Hell::Physics::SpawnRagdoll(
                    glm::vec3(0.0f),
                    glm::vec3(0.0f),
                    asset,
                    standaloneObjectId,
                    filterData
                );
            }

            if (ragdollId == 0) {
                Logging::Error() << "EditorSession::LaunchRagdollTest() failed to create the physics ragdoll";
                return;
            }

            g_ragdollTestRagdollId = ragdollId;
            g_ragdollTestAnimationName = asset.testAnimationName;
            const std::string renderMode = g_ragdollTestSkinnedGameObjectId == 0 ? " (physics shapes only)" : "";
            Debug::BlitQuickDebugMessage("Testing ragdoll '" + asset.name + "'" + renderMode);
        }

        void ResetMapEditor() {
            SetRenderMode(EditorRenderMode::PBR);
            HeightMapEditor::ResetTools();
            Toolbar::Reset();
        }

        bool IsDialogOpen() {
            return Dialog::IsOpen() || FileDialog::IsOpen();
        }

        void DrawGrid() {
            constexpr float DEFAULT_GRID_EXTENT = 20.0f;
            constexpr float GRID_SPACING = 0.5f;
            constexpr float DEFAULT_GRID_HEIGHT = -0.01f;

            float minimumX = -DEFAULT_GRID_EXTENT;
            float maximumX = DEFAULT_GRID_EXTENT;
            float minimumZ = -DEFAULT_GRID_EXTENT;
            float maximumZ = DEFAULT_GRID_EXTENT;
            float gridHeight = DEFAULT_GRID_HEIGHT;

            const bool heightMapMode = Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive();
            if (heightMapMode) {
                const uint32_t chunkWidth = Workspace::GetMapChunkWidth();
                const uint32_t chunkDepth = Workspace::GetMapChunkDepth();
                if (chunkWidth == 0 || chunkDepth == 0) return;

                minimumX = 0.0f;
                maximumX = static_cast<float>(chunkWidth * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
                minimumZ = 0.0f;
                maximumZ = static_cast<float>(chunkDepth * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
                gridHeight = Ocean::GetOceanOriginY();
            }

            for (float x = minimumX; x <= maximumX; x += GRID_SPACING) {
                DebugDraw::DrawLine(glm::vec3(x, gridHeight, minimumZ), glm::vec3(x, gridHeight, maximumZ), GRID_COLOR, true);
            }
            for (float z = minimumZ; z <= maximumZ; z += GRID_SPACING) {
                DebugDraw::DrawLine(glm::vec3(minimumX, gridHeight, z), glm::vec3(maximumX, gridHeight, z), GRID_COLOR, true);
            }

            DebugDraw::DrawLine(glm::vec3(minimumX, gridHeight, 0.0f), glm::vec3(maximumX, gridHeight, 0.0f), WHITE, true);
            DebugDraw::DrawLine(glm::vec3(0.0f, gridHeight, minimumZ), glm::vec3(0.0f, gridHeight, maximumZ), WHITE, true);
        }

        void RefreshNativeLayout() {
            // The editor UI owns the whole native canvas
            UIBackEnd::SetCanvasResolution(UICanvas::NATIVE, static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableWidth())), static_cast<uint32_t>(std::max(1, Hell::BackEnd::GetDrawableHeight())));
            Layout::Update();
        }

        void HandleMenuAction(MenuBar::EditorMenuAction action) {
            switch (action) {
                case MenuBar::EditorMenuAction::NEW_FILE:
                    FileDialog::New(Workspace::GetMode());
                    break;
                case MenuBar::EditorMenuAction::OPEN_FILE: {
                    std::string selectedFile = Workspace::GetName();
                    if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) selectedFile = RagdollEditor::GetSourcePath();
                    if (Workspace::GetMode() == EditorSessionMode::BONE_MASK) selectedFile = BoneMaskEditor::GetSourcePath();
                    FileDialog::Open(Workspace::GetMode(), selectedFile);
                    break;
                }
                case MenuBar::EditorMenuAction::IMPORT_RAG:
                    if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) FileDialog::ImportRagdoll("");
                    break;
                case MenuBar::EditorMenuAction::SAVE:
                    if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
                        std::string error;
                        if (!Workspace::SaveRagdoll(error)) Dialog::Open(error.empty() ? "Failed to save ragdoll" : error);
                    }
                    else if (Workspace::GetMode() == EditorSessionMode::BONE_MASK) {
                        std::string error;
                        if (!Workspace::SaveBoneMask(error)) Dialog::Open(error.empty() ? "Failed to save bone mask" : error);
                    }
                    else {
                        Workspace::Save();
                    }
                    break;
                case MenuBar::EditorMenuAction::SAVE_AS:
                    if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
                        FileDialog::SaveRagdollAs(RagdollEditor::GetName());
                    }
                    break;
                case MenuBar::EditorMenuAction::CLOSE_EDITOR: Close(); break;
                case MenuBar::EditorMenuAction::VIEWPORT_SINGLE:     Layout::SetViewportLayout(EditorViewportLayout::SINGLE);     break;
                case MenuBar::EditorMenuAction::VIEWPORT_LEFT_RIGHT: Layout::SetViewportLayout(EditorViewportLayout::LEFT_RIGHT); break;
                case MenuBar::EditorMenuAction::VIEWPORT_TOP_BOTTOM: Layout::SetViewportLayout(EditorViewportLayout::TOP_BOTTOM); break;
                case MenuBar::EditorMenuAction::VIEWPORT_FOUR:       Layout::SetViewportLayout(EditorViewportLayout::FOUR);       break;
                default: break;
            }
        }

        bool OpenWorkspaceFile(const std::string& fileName) {
            std::string currentFile = Workspace::GetName();
            if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) currentFile = RagdollEditor::GetSourcePath();
            if (Workspace::GetMode() == EditorSessionMode::BONE_MASK) currentFile = BoneMaskEditor::GetSourcePath();
            if (fileName == currentFile) return true;

            bool opened = false;
            std::string error;
            switch (Workspace::GetMode()) {
                case EditorSessionMode::HOUSE:     opened = Workspace::OpenHouse(fileName); break;
                case EditorSessionMode::MAP:       opened = Workspace::OpenMap(fileName); break;
                case EditorSessionMode::RAGDOLL:   opened = Workspace::OpenRagdoll(fileName, error); break;
                case EditorSessionMode::BONE_MASK: opened = Workspace::OpenBoneMask(fileName, error); break;
            }

            if (!opened) {
                Dialog::Open(error.empty() ? "Failed to open '" + fileName + "'" : error);
                return false;
            }

            if (Workspace::IsWorldBacked()) {
                Visibility::Clear();
            }
            if (Workspace::GetMode() == EditorSessionMode::MAP) {
                ResetMapEditor();
            }
            SetActive(true);

            if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
                const size_t markerCount = RagdollEditor::GetAsset().markers.size();
                Debug::BlitQuickDebugMessage("Opened '" + Workspace::GetName() + "' with " + std::to_string(markerCount) + (markerCount == 1 ? " marker" : " markers"));
            }
            return true;
        }

        bool ImportLegacyRagdollFile(const std::string& fileName) {
            std::string error;
            if (!Workspace::ImportRagdoll(fileName, error)) {
                Dialog::Open(error.empty() ? "Failed to import '" + fileName + "'" : error);
                return false;
            }

            SetActive(true);
            const size_t markerCount = RagdollEditor::GetAsset().markers.size();
            const size_t warningCount = RagdollEditor::GetImportWarnings().size();
            const std::string warningMessage = warningCount == 0 ? "" : ", " + std::to_string(warningCount) + (warningCount == 1 ? " warning" : " warnings");
            Debug::BlitQuickDebugMessage("Imported '" + Workspace::GetName() + "' with " + std::to_string(markerCount) + (markerCount == 1 ? " marker" : " markers") + warningMessage);
            return true;
        }

        bool NewWorkspaceFile(const std::string& fileName, const std::string& skinnedModelName) {
            bool created = false;
            std::string error;
            switch (Workspace::GetMode()) {
                case EditorSessionMode::HOUSE:     created = Workspace::NewHouse(fileName); break;
                case EditorSessionMode::MAP:       created = Workspace::NewMap(fileName); break;
                case EditorSessionMode::RAGDOLL:   created = Workspace::NewRagdoll(fileName, skinnedModelName, error); break;
                case EditorSessionMode::BONE_MASK: created = Workspace::NewBoneMask(fileName, skinnedModelName, error); break;
            }

            if (!created) {
                Dialog::Open(error.empty() ? "Failed to create '" + fileName + "'" : error);
                return false;
            }
            if (Workspace::IsWorldBacked()) {
                Visibility::Clear();
            }
            if (Workspace::GetMode() == EditorSessionMode::MAP) {
                ResetMapEditor();
            }
            SetActive(true);
            return true;
        }
    }

    void Init() {
        InitPlacementTools();
        Toolbar::Init();
        ResetMapEditor();
        Selection::Reset();
        Viewports::Init();
        MenuBar::Init();
        Hierarchy::Init();
    }

    void Open() {
        Workspace::Discard();
        SetActive(true);
    }

    void Open(EditorSessionMode mode) {
        MenuBar::SetMode(mode);
        if (IsActive()) {
            if (Workspace::HasMode() && Workspace::GetMode() == mode) return;
            Close();
        }

        // Asset editors use an empty world
        if (mode == EditorSessionMode::RAGDOLL || mode == EditorSessionMode::BONE_MASK) {
            if (mode == EditorSessionMode::RAGDOLL) {
                Session::KeepOnlyFirstLocalPlayer();
            }
            DestroyRagdollTest();
            World::ResetWorld();
            Viewports::PrepareInitialRagdollView();
        }

        if (!Workspace::Open(mode)) {
            if (mode == EditorSessionMode::MAP) {
                ResetMapEditor();
            }
            SetActive(true);
            FileDialog::Open(mode, "");
            return;
        }

        if (mode == EditorSessionMode::MAP) {
            ResetMapEditor();

            // Put presents back at their authored transforms
            for (GenericObject& genericObject : World::GetGenericObjects()) {
                const GenericObjectType type = genericObject.GetType();
                if (type == GenericObjectType::CHRISTMAS_PRESENT_SMALL || type == GenericObjectType::CHRISTMAS_PRESENT_LARGE) {
                    genericObject.ResetPhysics();
                }
            }
        }

        SetActive(true);
    }

    void Close() {
        if (!IsActive()) return;

        g_pendingRagdollTest.reset();
        const bool hadWorldWorkspace = Workspace::IsWorldBacked();
        const bool hadDisposableWorkspace = Workspace::HasMode() && !hadWorldWorkspace;
        DeactivateEditor();
        if (hadDisposableWorkspace) {
            World::ResetWorld();
            return;
        }
        if (!hadWorldWorkspace) return;

        // Push authored transforms back into PhysX before gameplay reads them
        for (GenericObject& genericObject : World::GetGenericObjects()) {
            genericObject.ResetPhysics();
        }
        for (PickUp& pickUp : World::GetPickUps()) {
            if (pickUp.GetRespawnState()) {
                pickUp.Respawn();
            }
        }
        for (Ladder& ladder : World::GetLadders()) {
            ladder.Reset();
        }

        // Doors always return to their authored start state when gameplay resumes
        for (Door& door : World::GetDoors()) {
            door.Reset();
        }
    }

    void SetActive(bool active) {
        const bool worldBacked = Workspace::IsWorldBacked();

        // No interaction survives crossing the editor boundary
        Selection::Reset();
        InputElements::Reset();
        Dialog::Close();
        FileDialog::Close();
        Placement::Cancel();

        if (!active) {
            Viewports::CancelNavigation();
            Gizmo::SetVisible(true);
        }

        g_isActive = active;

        if (g_isActive && worldBacked) {
            for (Kangaroo& kangaroo : World::GetKangaroos()) {
                kangaroo.Respawn();
            }

            // Respawn authored pickups and delete player dropped items
            const auto pickUpIds = World::GetPickUps().ids();
            for (uint64_t objectId : pickUpIds) {
                PickUp* pickUp = World::GetPickUpByObjectId(objectId);
                if (pickUp->GetRespawnState()) {
                    pickUp->Respawn();
                }
                else {
                    World::RemoveObjectById(objectId);
                }
            }
        }

        if (worldBacked) {
            WorldBVH::MarkStaticSceneBvhDirty();
        }

        if (g_isActive) {
            Hell::Input::ShowCursor();
        }
        else {
            Hell::Input::DisableCursor();
        }

        MenuBar::Close();
        Layout::CancelInteraction();
        Gizmo::CancelInteraction();

        if (!g_isActive) {
            UIBackEnd::ClearCanvas(UICanvas::NATIVE);
            return;
        }

        // Refresh scene backed UI only when entering the editor
        Layout::SetToolsVisible(Inspector::HasTools());
        Layout::SetBrushesVisible(Inspector::HasBrushes());
        Layout::SetMaterialsVisible(Inspector::HasMaterials());
        if (!Workspace::HasMode()) {
            Layout::SetPropertiesContentHeight(0);
        }
        Hierarchy::Refresh();
        Gizmo::SetPosition(glm::vec3(0.0f));
        Gizmo::SetRotation(glm::vec3(0.0f));
        Gizmo::SetSourceObjectOffeset(glm::vec3(0.0f));
        RefreshNativeLayout();
        Viewports::ApplyInitialView();
        MenuBar::RefreshLayout();
    }

    void SetRenderMode(EditorRenderMode renderMode) {
        g_renderMode = renderMode;
    }

    void Update() {
        if (!IsActive()) return;

        if (g_pendingRagdollTest) {
            RagdollAsset asset = std::move(*g_pendingRagdollTest);
            g_pendingRagdollTest.reset();
            LaunchRagdollTest(std::move(asset));
            return;
        }

        // Handle completed file dialogs
        const std::string newFileName = FileDialog::ConsumeNewFileName();
        if (!newFileName.empty()) {
            const std::string newSkinnedModelName = FileDialog::ConsumeNewSkinnedModelName();
            NewWorkspaceFile(newFileName, newSkinnedModelName);
            return;
        }

        const std::string selectedFile = FileDialog::ConsumeSelectedFile();
        if (!selectedFile.empty()) {
            OpenWorkspaceFile(selectedFile);
            return;
        }

        const std::string importedRagdoll = FileDialog::ConsumeImportedRagdoll();
        if (!importedRagdoll.empty()) {
            ImportLegacyRagdollFile(importedRagdoll);
            return;
        }

        const std::string ragdollSaveAsName = FileDialog::ConsumeRagdollSaveAsName();
        if (!ragdollSaveAsName.empty()) {
            std::string error;
            if (!Workspace::SaveRagdollAs(ragdollSaveAsName, error)) {
                Dialog::Open(error.empty() ? "Failed to save ragdoll as '" + ragdollSaveAsName + "'" : error);
            }
            return;
        }

        // Handle editor hotkeys
        const bool allowHotkeys = !WantsKeyboardCapture();
        if (Workspace::IsWorldBacked() && allowHotkeys && Hell::Input::KeyPressed(HELL_KEY_TAB)) {
            const EditorSelectionMode previousMode = Selection::GetMode();
            const EditorSelectionMode mode = Selection::GetMode() == EditorSelectionMode::OBJECT ? EditorSelectionMode::VERTEX : EditorSelectionMode::OBJECT;
            Selection::SetMode(mode);
            if (Selection::GetMode() == previousMode) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            }
            Debug::BlitQuickDebugMessage(Hell::Enum::ToString(Selection::GetMode()));
        }

        if (allowHotkeys && Hell::Input::KeyPressed(HELL_KEY_V)) {
            switch (Layout::GetViewportLayout()) {
                case EditorViewportLayout::SINGLE:     Layout::SetViewportLayout(EditorViewportLayout::LEFT_RIGHT); break;
                case EditorViewportLayout::LEFT_RIGHT: Layout::SetViewportLayout(EditorViewportLayout::TOP_BOTTOM); break;
                case EditorViewportLayout::TOP_BOTTOM: Layout::SetViewportLayout(EditorViewportLayout::FOUR);       break;
                case EditorViewportLayout::FOUR:       Layout::SetViewportLayout(EditorViewportLayout::SINGLE);     break;
            }
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            Debug::BlitQuickDebugMessage(Hell::Enum::ToString(Layout::GetViewportLayout()));
        }

        // Update the grid and menu
        DrawGrid();
        if (Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
            RagdollEditor::DrawSkeleton();
            RagdollEditor::DrawJointLimits();
        }
        if (Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::BONE_MASK) BoneMaskEditor::DrawSkeleton();
        RefreshNativeLayout();
        if (IsDialogOpen()) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            return;
        }
        MenuBar::Update();
        HandleMenuAction(MenuBar::ConsumeAction());
        if (IsDialogOpen()) return;

        // Stop before workspace UI
        if (!Workspace::HasMode()) {
            MenuBar::ConsumePlacementTool();
            return;
        }
        if (Workspace::IsWorldBacked()) {
            Placement::Begin(MenuBar::ConsumePlacementTool());
        }
        else {
            MenuBar::ConsumePlacementTool();
        }

        if (!IsActive()) return;

        // Update workspace panels
        Layout::Update();
        Layout::UpdateDividerInput(!MenuBar::WantsMouseCapture());
        Hierarchy::Update(!Placement::IsActive() && !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture());
        Toolbar::Update(!Placement::IsActive() && !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture() && !Hierarchy::WantsMouseCapture());
        Layout::SetToolsVisible(Inspector::HasTools());
        Layout::SetBrushesVisible(Inspector::HasBrushes());
        Layout::SetMaterialsVisible(Inspector::HasMaterials());

        if (!Layout::WantsMouseCapture() && WantsMouseCapture()) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }
    }

    void Render() {
        if (!IsActive()) return;

        const bool nameInputDialogOpen = FileDialog::IsNameInputOpen();
        InputElements::BeginFrame(!Dialog::IsOpen() && (!FileDialog::IsOpen() || nameInputDialogOpen));
        Layout::RenderBackgrounds();
        Hierarchy::Render();
        if (!nameInputDialogOpen) {
            Inspector::RenderProperties(Layout::GetPropertiesContentRect());
            Layout::SetPropertiesContentHeight(InputElements::GetLastRenderedHeight());
            Inspector::RenderTools(Layout::GetToolsContentRect());
            Layout::SetToolsContentHeight(InputElements::GetLastRenderedHeight());
            Inspector::RenderBrushes(Layout::GetBrushesContentRect());
            Inspector::RenderMaterials(Layout::GetMaterialsContentRect());
            InputElements::EndFrame();
        }
        Layout::RenderOverlay();
        Viewports::RenderLabels();
        Toolbar::Render();
        MenuBar::Render();
        FileDialog::Render();

        // Name input ends after its dialog renders
        if (nameInputDialogOpen) {
            InputElements::EndFrame();
        }
        Dialog::Render();

        // Submit preview after inspector edits
        if (Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
            RagdollEditor::SubmitRenderItems();
        }
    }

    bool RequestRagdollTest(std::string& error) {
        if (!IsActive() || !Workspace::HasMode() || Workspace::GetMode() != EditorSessionMode::RAGDOLL || !RagdollEditor::HasDocument()) {
            error = "Open a ragdoll before testing";
            return false;
        }

        const RagdollAsset& asset = RagdollEditor::GetAsset();
        if (asset.markers.empty()) {
            error = "Create at least one shape before testing";
            return false;
        }

        g_pendingRagdollTest = asset;
        error.clear();
        return true;
    }

    void SimulateRagdollTest() {
        Ragdoll* ragdoll = GetRagdollTestRagdoll();
        if (!ragdoll) return;

        if (SkinnedGameObject* skinnedGameObject = GetRagdollTestSkinnedGameObject()) {
            skinnedGameObject->SetAnimationModeToRagdoll();
        }
        else {
            ragdoll->EnableSimulation();
        }
    }

    void SetRagdollTestToBindPose() {
        Ragdoll* ragdoll = GetRagdollTestRagdoll();
        if (!ragdoll) return;

        ragdoll->DisableSimulation();
        if (SkinnedGameObject* skinnedGameObject = GetRagdollTestSkinnedGameObject()) {
            AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(g_ragdollTestAnimatorInstanceId);
            if (animatorInstance) {
                animatorInstance->SetAnimationLayerWeight(g_ragdollTestAnimationLayerIndex, 0.0f);
                animatorInstance->Update(0.0f);
            }
            skinnedGameObject->SetAnimationModeToBindPose();
        }
        else {
            ragdoll->SetToInitialPose();
        }
    }

    void SetRagdollTestToTestAnimation() {
        Ragdoll* ragdoll = GetRagdollTestRagdoll();
        if (!ragdoll) return;

        ragdoll->DisableSimulation();
        if (SkinnedGameObject* skinnedGameObject = GetRagdollTestSkinnedGameObject()) {
            skinnedGameObject->SetAnimationModeToAnimated();
            AnimatorInstance* animatorInstance = Animator::GetAnimatorInstanceByObjectId(g_ragdollTestAnimatorInstanceId);
            if (animatorInstance && !g_ragdollTestAnimationName.empty()) {
                animatorInstance->SetAnimationLayerWeight(g_ragdollTestAnimationLayerIndex, 1.0f);
                animatorInstance->PlayAndLoopAnimation(g_ragdollTestAnimationLayerIndex, g_ragdollTestAnimationName, 1.0f);
                animatorInstance->RestartAnimation();
            }
        }
        else {
            ragdoll->SetToInitialPose();
        }
    }

    void ElevateRagdollTest() {
        Ragdoll* ragdoll = GetRagdollTestRagdoll();
        if (!ragdoll) return;

        const glm::vec3 elevatedPosition(0.0f, 0.5f, 0.0f);
        if (SkinnedGameObject* skinnedGameObject = GetRagdollTestSkinnedGameObject()) {
            skinnedGameObject->SetPosition(elevatedPosition);
        }
        else {
            ragdoll->SetSpawnPosition(elevatedPosition);
        }
    }

    void UpdateViewportInput() {
        if (!IsActive()) return;

        Viewports::Update();
        if (!Workspace::HasMode()) return;

        // UI input wins before the viewport or gizmo sees it
        const bool allowMouseInput = !IsDialogOpen() && !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture() && !Hierarchy::WantsMouseCapture() && !Toolbar::WantsMouseCapture();
        const bool allowKeyboardInput = !IsDialogOpen() && !MenuBar::WantsKeyboardCapture() && !InputElements::WantsKeyboardCapture();

        Viewports::UpdateInput(allowKeyboardInput, allowMouseInput);

        // Ragdoll input does not touch World
        if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
            if (allowKeyboardInput && Hell::Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
                std::string error;
                if (!RequestRagdollTest(error)) Dialog::Open(error);
                return;
            }

            if (allowKeyboardInput && RagdollEditor::HasSelectedMarker() && (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE) || Hell::Input::KeyPressed(HELL_KEY_DELETE))) {
                std::string error;
                if (RagdollEditor::DeleteSelectedMarker(error)) {
                    Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                    Hierarchy::RefreshRagdollMarkers();
                }
                else if (!error.empty()) {
                    Dialog::Open(error);
                }
                return;
            }

            RagdollEditor::UpdateInput(allowKeyboardInput, allowMouseInput && !Viewports::IsFlyMode());
            return;
        }

        // Placement owns the click so the gizmo and selection never see it
        if (Placement::IsActive()) {
            Placement::Update(allowKeyboardInput && !Viewports::IsFlyMode(), allowMouseInput && !Viewports::IsFlyMode());
            Gizmo::SetVisible(false);
            Hell::BackEnd::SetCursor(Placement::IsActive() && allowMouseInput && !Viewports::IsFlyMode() && Viewports::GetHoveredViewportIndex() >= 0 ? HELL_CURSOR_CROSSHAIR : HELL_CURSOR_ARROW);
            return;
        }

        // Handle object hotkeys
        const bool controlDown = Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        const EditorObjectMode selectedObjectMode = ObjectOptions::GetEditorMode(Selection::GetSelectedObjectId());
        const bool canDuplicateSelection = Selection::GetMode() == EditorSelectionMode::OBJECT || selectedObjectMode == EditorObjectMode::VERTEX;
        if (allowKeyboardInput && controlDown && Selection::HasObjectSelection() && canDuplicateSelection && !Selection::HasSelectedWallSegment() && Hell::Input::KeyPressed(HELL_KEY_D)) {
            const uint64_t objectId = World::DuplicateObjectById(Selection::GetSelectedObjectId());
            if (objectId != 0) {
                WorldBVH::MarkStaticSceneBvhDirty();
                Hierarchy::Refresh();
                Selection::SelectObject(objectId);
            }
        }

        if (allowKeyboardInput && Selection::HasObjectSelection() && Hell::Input::KeyPressed(HELL_KEY_H)) {
            uint64_t objectId = Selection::GetSelectedObjectId();
            if (Selection::HasSelectedWallSegment()) {
                Wall* wall = World::GetWallByObjectId(objectId);
                const int32_t segmentIndex = Selection::GetSelectedWallSegmentIndex();
                if (wall && segmentIndex >= 0 && segmentIndex < static_cast<int32_t>(wall->GetWallSegments().size())) {
                    objectId = wall->GetWallSegments()[segmentIndex].GetObjectId();
                }
            }
            if (Visibility::Hide(objectId)) {
                Selection::ClearSelection();
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
            }
        }

        if (allowKeyboardInput && Hell::Input::KeyPressed(HELL_KEY_U) && Visibility::UnhideAll()) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        }

        // Height map tools own viewport input
        const bool heightMapMode = GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive();
        HeightMapEditor::Update(heightMapMode && allowMouseInput && !Viewports::IsFlyMode());
        if (heightMapMode) {
            Gizmo::SetVisible(false);
            return;
        }

        // Handle point editing hotkeys
        if (allowKeyboardInput && Selection::HasObjectSelection() && Hell::Input::KeyPressed(HELL_KEY_INSERT)) {
            const uint64_t objectId = Selection::GetSelectedObjectId();
            if (Selection::AddPoint()) {
                Hierarchy::RefreshObjectChildren(objectId);
            }
        }

        if (allowKeyboardInput && Selection::HasObjectSelection() && (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE) || Hell::Input::KeyPressed(HELL_KEY_DELETE))) {
            const uint64_t objectId = Selection::GetSelectedObjectId();
            const bool pointSelected = Selection::HasSelectedPoint() && ObjectOptions::GetEditorMode(objectId) != EditorObjectMode::VERTEX;
            if (Selection::DeleteSelected()) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
                if (pointSelected) {
                    Hierarchy::RefreshObjectChildren(objectId);
                }
                else {
                    Hierarchy::RemoveObject(objectId);
                }
            }
        }

        // Update the gizmo and selection
        const bool vertexMode = Selection::GetMode() == EditorSelectionMode::VERTEX;
        const bool showGizmo = !Viewports::IsFlyMode() && Selection::HasObjectSelection() && !Selection::HasSelectedWallSegment() && (!vertexMode || Selection::HasSelectedPoint());
        Gizmo::SetVisible(showGizmo);
        Gizmo::Update(showGizmo && allowKeyboardInput && allowMouseInput);
        const bool pointClicked = PointSequences::UpdateInput(allowMouseInput);
        Selection::Update(allowMouseInput && !pointClicked);
        PointSequences::Draw();
    }

    bool IsActive() {
        return g_isActive;
    }

    bool IsInactive() {
        return !g_isActive;
    }

    bool IsHeightMapEditorActive() {
        return IsActive() && Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive();
    }

    bool HasMode() {
        return Workspace::HasMode();
    }

    EditorSessionMode GetMode() {
        return Workspace::GetMode();
    }

    EditorRenderMode GetRenderMode() {
        return g_renderMode;
    }

    bool WantsMouseCapture() {
        if (!IsActive()) return false;
        if (!Workspace::HasMode()) return true;
        if (IsDialogOpen()) return true;
        if (MenuBar::WantsMouseCapture()) return true;
        if (Layout::WantsMouseCapture()) return true;
        if (Hierarchy::WantsMouseCapture()) return true;
        if (Toolbar::WantsMouseCapture()) return true;
        if (Viewports::IsPanning() || Viewports::IsOrbiting() || Viewports::IsFlyMode()) return true;
        if (Gizmo::HasHover() || Gizmo::GetAction() == GizmoAction::DRAGGING) return true;

        // Mouse inside a viewport belongs to the scene
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        for (uint32_t i = 0; i < 4; i++) {
            const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(i);
            if (region && region->visible && region->rect.Contains(mousePosition)) {
                return false;
            }
        }
        return true;
    }

    bool WantsKeyboardCapture() {
        return IsActive() && (!Workspace::HasMode() || IsDialogOpen() || Placement::IsActive() || MenuBar::WantsKeyboardCapture() || InputElements::WantsKeyboardCapture() || Viewports::IsFlyMode());
    }
}
