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
#include "Unloved/EditorSession/Ragdoll/EditorRagdollTest.h"
#include "Unloved/EditorSession/UI/EditorUI.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/EditorSession/Interaction/EditorVisibility.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Enum.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <string>

namespace Unloved::EditorSession {
    namespace {
        bool g_isActive = false;
        EditorRenderMode g_renderMode = EditorRenderMode::PBR;

        void DeactivateEditor() {
            Workspace::Close();
            SetActive(false);
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
                    switch (Workspace::GetMode()) {
                        case EditorSessionMode::HOUSE:     FileDialog::New(NewFileDialogType::NEW_HOUSE); break;
                        case EditorSessionMode::MAP:       FileDialog::New(NewFileDialogType::NEW_MAP); break;
                        case EditorSessionMode::RAGDOLL:   FileDialog::New(NewFileDialogType::NEW_RAGDOLL); break;
                        case EditorSessionMode::BONE_MASK: FileDialog::New(NewFileDialogType::NEW_BONE_MASK); break;
                    }
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

    }

    void CreateNewFile(NewFileDialogType type, const std::string& fileName) {
        bool created = false;
        std::string error;
        switch (type) {
            case NewFileDialogType::NEW_HOUSE:     created = Workspace::NewHouse(fileName); break;
            case NewFileDialogType::NEW_MAP:       created = Workspace::NewMap(fileName); break;
            case NewFileDialogType::NEW_RAGDOLL:   created = Workspace::NewRagdoll(fileName, error); break;
            case NewFileDialogType::NEW_BONE_MASK: created = Workspace::NewBoneMask(fileName, error); break;
            case NewFileDialogType::SAVE_RAGDOLL_AS:
            case NewFileDialogType::NONE: return;
        }

        if (!created) {
            Dialog::Open(error.empty() ? "Failed to create '" + fileName + "'" : error);
            return;
        }
        if (Workspace::IsWorldBacked()) Visibility::Clear();
        if (type == NewFileDialogType::NEW_MAP) ResetMapEditor();
        SetActive(true);
    }

    void SaveRagdollAs(const std::string& fileName) {
        std::string error;
        if (!Workspace::SaveRagdollAs(fileName, error)) {
            Dialog::Open(error.empty() ? "Failed to save ragdoll as '" + fileName + "'" : error);
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
            RagdollTest::Stop();
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

        // Handle editor hotkeys
        const bool allowHotkeys = !WantsKeyboardCapture();
        if (allowHotkeys && Workspace::HasMode() && Workspace::GetMode() == EditorSessionMode::RAGDOLL && Hell::Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
            if (!RagdollEditor::HasDocument()) {
                Dialog::Open("Open a ragdoll before testing");
                return;
            }
            const RagdollAsset& asset = RagdollEditor::GetAsset();
            if (asset.markers.empty()) {
                Dialog::Open("Create at least one shape before testing");
                return;
            }
            RagdollTest::Start(asset);
            return;
        }

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

    void UpdateViewportInput() {
        if (!IsActive()) return;

        Viewports::Update();
        if (!Workspace::HasMode()) return;

        // UI input wins before the viewport or gizmo sees it
        const bool allowMouseInput = !IsDialogOpen() && !MenuBar::WantsMouseCapture() && !Layout::WantsMouseCapture() && !Hierarchy::WantsMouseCapture() && !Toolbar::WantsMouseCapture();
        const bool allowKeyboardInput = !IsDialogOpen() && !MenuBar::WantsKeyboardCapture() && !InputElements::WantsKeyboardCapture();

        Viewports::UpdateInput(allowKeyboardInput, allowMouseInput);

        if (Workspace::GetMode() == EditorSessionMode::RAGDOLL) {
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
