#include "EditorToolbar.h"

#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "EditorSession.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#include "Hell/Common/Enum.h"

#include "Unloved/Debug/Debug.h"

#include <array>
#include <cstdint>

namespace Unloved::EditorSession::Toolbar {
    namespace {
        constexpr int32_t RENDER_MODE_COUNT = 3;
        constexpr int32_t SCULPT_TOOL_COUNT = 4;
        constexpr int32_t TERRAIN_TOOL_COUNT = 3;
        constexpr int32_t MAX_VIEWPORT_COUNT = 4;

        const std::array<const char*, RENDER_MODE_COUNT> RENDER_MODE_ICON_TEXTURE_NAMES = {
            "render_pbr_button",
            "render_base_color_button",
            "render_normals_button"
        };

        const std::array<EditorRenderMode, RENDER_MODE_COUNT> RENDER_MODES = {
            EditorRenderMode::PBR,
            EditorRenderMode::SOLID_COLOR,
            EditorRenderMode::NORMALS
        };

        const std::array<const char*, SCULPT_TOOL_COUNT> SCULPT_ICON_TEXTURE_NAMES = {
            "height_add",
            "height_flat",
            "height_slope",
            "height_smooth"
        };

        const std::array<HeightMapEditor::Tool, SCULPT_TOOL_COUNT> SCULPT_TOOLS = {
            HeightMapEditor::Tool::ADD,
            HeightMapEditor::Tool::FLAT,
            HeightMapEditor::Tool::SLOPE,
            HeightMapEditor::Tool::SMOOTH
        };

        const std::array<const char*, TERRAIN_TOOL_COUNT> TERRAIN_ICON_TEXTURE_NAMES = {
            "texture_paint",
            "texture_spray",
            "autoshader"
        };

        const std::array<HeightMapEditor::Tool, TERRAIN_TOOL_COUNT> TERRAIN_TOOLS = {
            HeightMapEditor::Tool::TEXTURE_PAINT,
            HeightMapEditor::Tool::TEXTURE_SPRAY,
            HeightMapEditor::Tool::AUTO_SHADER
        };

        EditorButton g_mapModeButton;
        std::array<EditorButton, RENDER_MODE_COUNT> g_renderModeButtons;
        std::array<EditorButton, SCULPT_TOOL_COUNT> g_sculptButtons;
        std::array<EditorButton, TERRAIN_TOOL_COUNT> g_terrainButtons;
        EditorButton g_terrainLayersButton;
        bool g_wantsMouseCapture = false;

        bool IsMapEditorVisible() {
            return Unloved::EditorSession::HasMode() && Unloved::EditorSession::GetMode() == EditorSessionMode::MAP;
        }

        void SelectHeightMapTool(HeightMapEditor::Tool tool) {
            HeightMapEditor::SetTool(tool);
            Debug::BlitQuickDebugMessage(Hell::Enum::ToString(HeightMapEditor::GetTool()));
        }

        const EditorViewportRegion* GetToolbarViewport() {
            for (int32_t viewportIndex = 0; viewportIndex < MAX_VIEWPORT_COUNT; viewportIndex++) {
                const EditorViewportRegion* region = Layout::GetViewportRegionByIndex(viewportIndex);
                if (region && region->visible) {
                    return region;
                }
            }
            return nullptr;
        }

        void ResetButton(EditorButton& button) {
            button.visible = false;
            button.hovered = false;
            button.selected = false;
        }

        void UpdateButton(EditorButton& button, bool visible, int32_t& slot, const EditorViewportRegion* region, const EditorToolbarStyle& style, const glm::ivec2& mousePosition, bool allowInput) {
            button.visible = visible;
            if (visible) {
                button.rect = { region->rect.x + style.viewportPadding, region->rect.y + style.viewportPadding + style.labelHeight + style.buttonGap + slot * (style.buttonSize + style.buttonGap), style.buttonSize, style.buttonSize };
                slot++;
            }

            Buttons::Update(button, mousePosition, allowInput);
            g_wantsMouseCapture = g_wantsMouseCapture || button.hovered;
        }
    }

    void Init() {
        g_mapModeButton.onPressed = [] {
            const bool active = !HeightMapEditor::IsActive();
            HeightMapEditor::SetActive(active);
            Debug::BlitQuickDebugMessage(active ? "HEIGHT_MAP" : "OBJECT");
        };

        for (int32_t renderModeIndex = 0; renderModeIndex < RENDER_MODE_COUNT; renderModeIndex++) {
            EditorButton& button = g_renderModeButtons[renderModeIndex];
            button.iconTextureName = RENDER_MODE_ICON_TEXTURE_NAMES[renderModeIndex];
            button.onPressed = [renderModeIndex] {
                Unloved::EditorSession::SetRenderMode(RENDER_MODES[renderModeIndex]);
                Debug::BlitQuickDebugMessage(Hell::Enum::ToString(Unloved::EditorSession::GetRenderMode()));
            };
        }

        for (int32_t toolIndex = 0; toolIndex < SCULPT_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_sculptButtons[toolIndex];
            button.iconTextureName = SCULPT_ICON_TEXTURE_NAMES[toolIndex];
            button.onPressed = [toolIndex] { SelectHeightMapTool(SCULPT_TOOLS[toolIndex]); };
        }

        for (int32_t toolIndex = 0; toolIndex < TERRAIN_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_terrainButtons[toolIndex];
            button.iconTextureName = TERRAIN_ICON_TEXTURE_NAMES[toolIndex];
            button.onPressed = [toolIndex] { SelectHeightMapTool(TERRAIN_TOOLS[toolIndex]); };
        }

        g_terrainLayersButton.iconTextureName = "layers";
        g_terrainLayersButton.onPressed = [] {
            HeightMapEditor::SetTerrainLayersOpen(!HeightMapEditor::IsTerrainLayersOpen());
            Debug::BlitQuickDebugMessage("LAYERS");
        };
    }

    void Reset() {
        g_wantsMouseCapture = false;
        ResetButton(g_mapModeButton);
        for (EditorButton& button : g_renderModeButtons) {
            ResetButton(button);
        }
        for (EditorButton& button : g_sculptButtons) {
            ResetButton(button);
        }
        for (EditorButton& button : g_terrainButtons) {
            ResetButton(button);
        }
        ResetButton(g_terrainLayersButton);
    }

    void Update(bool allowInput) {
        const EditorToolbarStyle& style = GetStyle().toolbar;
        const bool mapEditorVisible = IsMapEditorVisible();
        const bool editorVisible = Unloved::EditorSession::HasMode();
        const bool inputEnabled = allowInput && !Viewports::IsFlyMode();
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const EditorViewportRegion* region = GetToolbarViewport();
        const bool viewportVisible = editorVisible && region;
        int32_t slot = 0;
        g_wantsMouseCapture = false;

        // Map editor mode
        g_mapModeButton.iconTextureName = HeightMapEditor::IsActive() ? "object_mode" : "heightmap_mode";
        UpdateButton(g_mapModeButton, viewportVisible && mapEditorVisible, slot, region, style, mousePosition, inputEnabled);
        const bool heightMapToolsVisible = viewportVisible && mapEditorVisible && HeightMapEditor::IsActive();

        // Render modes
        for (int32_t renderModeIndex = 0; renderModeIndex < RENDER_MODE_COUNT; renderModeIndex++) {
            EditorButton& button = g_renderModeButtons[renderModeIndex];
            button.selected = Unloved::EditorSession::GetRenderMode() == RENDER_MODES[renderModeIndex];
            UpdateButton(button, viewportVisible, slot, region, style, mousePosition, inputEnabled);
        }

        // Sculpt tools
        for (int32_t toolIndex = 0; toolIndex < SCULPT_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_sculptButtons[toolIndex];
            button.selected = HeightMapEditor::GetTool() == SCULPT_TOOLS[toolIndex];
            UpdateButton(button, heightMapToolsVisible, slot, region, style, mousePosition, inputEnabled);
        }

        // Terrain paint tools
        for (int32_t toolIndex = 0; toolIndex < TERRAIN_TOOL_COUNT; toolIndex++) {
            EditorButton& button = g_terrainButtons[toolIndex];
            button.selected = HeightMapEditor::GetTool() == TERRAIN_TOOLS[toolIndex];
            UpdateButton(button, heightMapToolsVisible, slot, region, style, mousePosition, inputEnabled);
        }

        // Terrain layers
        g_terrainLayersButton.selected = HeightMapEditor::IsTerrainLayersOpen();
        UpdateButton(g_terrainLayersButton, heightMapToolsVisible, slot, region, style, mousePosition, inputEnabled);
    }

    void Render() {
        if (!Unloved::EditorSession::HasMode()) return;

        Buttons::Render(g_mapModeButton);
        for (const EditorButton& button : g_renderModeButtons) {
            Buttons::Render(button);
        }
        for (const EditorButton& button : g_sculptButtons) {
            Buttons::Render(button);
        }
        for (const EditorButton& button : g_terrainButtons) {
            Buttons::Render(button);
        }
        Buttons::Render(g_terrainLayersButton);
    }

    bool WantsMouseCapture() {
        return g_wantsMouseCapture;
    }
}
