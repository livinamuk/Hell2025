#include "Unloved/EditorSession/EditorInspector.h"
#include "EditorInspectorInternal.h"

#include "Unloved/EditorSession/HeightMap/EditorHeightMap.h"
#include "Unloved/EditorSession/EditorObjectOptions.h"
#include "Unloved/EditorSession/Interaction/EditorSelection.h"
#include "Unloved/EditorSession/UI/EditorStyle.h"
#include "Unloved/EditorSession/UI/EditorUI.h"
#include "Unloved/EditorSession/Core/EditorWorkspace.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Unloved::EditorSession::Inspector::Internal {

    void ApplyWeatherBoardMaterialDefaults(Unloved::Wall* wall, const std::string& materialName) {
        const ObjectOptions::WeatherBoardMaterialSettings* settings = ObjectOptions::GetWeatherBoardMaterialSettings(materialName);
        if (!wall || !settings) return;

        wall->SetWeatherBoardMaterial(settings->materialName, settings->boardCount, settings->startIndex, settings->endIndex, settings->textureOffsetU, settings->textureOffsetV);
    }
}

namespace Unloved::EditorSession::Inspector {
    namespace {
        constexpr int32_t ROWS_PER_SCROLL = 1;
        const std::vector<std::string> TERRAIN_MATERIALS = { "Grass", "RockFace", "Sand", "DirtRoad" };
        const std::array<uint8_t, 4> TERRAIN_MATERIAL_INDICES = { 0, 2, 3, 1 };

        struct BrushItem {
            HeightMapEditor::BrushType type = HeightMapEditor::BrushType::CIRCLE_0;
            std::string name;
            std::string textureName;
        };

        EditorScrollBar g_scrollBar;

        const std::array<BrushItem, static_cast<size_t>(HeightMapEditor::BrushType::COUNT)>& GetBrushItems() {
            static const std::array<BrushItem, static_cast<size_t>(HeightMapEditor::BrushType::COUNT)> BRUSH_ITEMS = [] {
                std::array<BrushItem, static_cast<size_t>(HeightMapEditor::BrushType::COUNT)> items;
                for (size_t i = 0; i < items.size(); i++) {
                    BrushItem& item = items[i];
                    item.type = static_cast<HeightMapEditor::BrushType>(i);
                    item.name = HeightMapEditor::GetBrushTypeName(item.type);
                    item.textureName = HeightMapEditor::GetBrushTextureName(item.type);
                }
                return items;
            }();
            return BRUSH_ITEMS;
        }

        const std::string& GetTerrainMaterialName(uint8_t materialIndex) {
            for (size_t i = 0; i < TERRAIN_MATERIAL_INDICES.size(); i++) {
                if (TERRAIN_MATERIAL_INDICES[i] == materialIndex) {
                    return TERRAIN_MATERIALS[i];
                }
            }
            return TERRAIN_MATERIALS.front();
        }

        void SetTerrainMaterial(const std::string& materialName) {
            const auto found = std::find(TERRAIN_MATERIALS.begin(), TERRAIN_MATERIALS.end(), materialName);
            if (found == TERRAIN_MATERIALS.end()) return;
            HeightMapEditor::SetSelectedTerrainMaterial(TERRAIN_MATERIAL_INDICES[found - TERRAIN_MATERIALS.begin()]);
        }

        void SetWallMaterial(Wall* wall, bool weatherBoards, const std::string& materialName) {
            if (weatherBoards) {
                Internal::ApplyWeatherBoardMaterialDefaults(wall, materialName);
            }
            else {
                wall->SetMaterial(materialName);
            }
        }

        std::string GetBaseColorTextureName(const std::string& materialName) {
            Material* material = Hell::ResourceManager::GetMaterialByName(materialName);
            if (!material) return "";

            Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(material->m_basecolor);
            return texture ? texture->GetFileName() : "";
        }
    }

    static void RenderBrushGallery(const EditorRect& rect) {
        const EditorStyle& style = GetStyle();
        const auto& brushItems = GetBrushItems();
        if (!rect.HasArea() || brushItems.empty()) return;

        const int32_t availableWidth = std::max(0, rect.width - style.gallery.contentPadding * 2 - style.gallery.itemGap - style.gallery.scrollBarWidth);
        const int32_t brushWidth = availableWidth / style.gallery.columnCount;
        const int32_t availableHeight = std::max(0, rect.height - style.gallery.contentPadding * 2);
        if (brushWidth <= 0 || availableHeight <= style.gallery.labelHeight) return;

        const int32_t brushHeight = std::max(style.gallery.labelHeight + style.gallery.brushLabelPadding, std::min(brushWidth, availableHeight));
        const int32_t rowHeight = brushHeight + style.gallery.itemGap;
        const int32_t rowCount = (static_cast<int32_t>(brushItems.size()) + style.gallery.columnCount - 1) / style.gallery.columnCount;
        const int32_t visibleRowCount = std::max(1, (availableHeight + style.gallery.itemGap) / rowHeight);
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const bool mouseInPanel = rect.Contains(mousePosition);

        // Scroll the gallery
        if (mouseInPanel) {
            if (Hell::Input::MouseWheelUp()) {
                g_scrollBar.value -= ROWS_PER_SCROLL;
            }
            else if (Hell::Input::MouseWheelDown()) {
                g_scrollBar.value += ROWS_PER_SCROLL;
            }
        }

        const EditorRect scrollBarRect = { rect.Right() - style.gallery.scrollBarWidth, rect.y, style.gallery.scrollBarWidth, rect.height };
        ScrollBar::Update(g_scrollBar, scrollBarRect, rowCount, visibleRowCount, mouseInPanel);

        for (int32_t i = 0; i < static_cast<int32_t>(brushItems.size()); i++) {
            const int32_t row = i / style.gallery.columnCount;
            const int32_t visibleRow = row - g_scrollBar.value;
            if (visibleRow < 0 || visibleRow >= visibleRowCount) continue;

            // Calculate the brush and label bounds
            const int32_t column = i % style.gallery.columnCount;
            const EditorRect brushRect = { rect.x + style.gallery.contentPadding + column * (brushWidth + style.gallery.itemGap), rect.y + style.gallery.contentPadding + visibleRow * rowHeight, brushWidth, brushHeight };
            const EditorRect labelRect = { brushRect.x, brushRect.Bottom() - style.gallery.labelHeight, brushRect.width, style.gallery.labelHeight };

            // Resolve the brush state
            const BrushItem& brush = brushItems[i];
            const bool hovered = !ScrollBar::WantsMouseCapture(g_scrollBar) && brushRect.Contains(mousePosition);
            const bool selected = brush.type == HeightMapEditor::GetBrushType();

            // Brush background
            UI::DrawSolidRect(brushRect, style.colors.controlBackground);

            // Brush preview
            if (Hell::ResourceManager::GetTextureBindlessIndexByName(brush.textureName) != -1) {
                UIBackEnd::BlitTexture(UICanvas::NATIVE, brush.textureName, glm::ivec2(brushRect.x, brushRect.y), Alignment::TOP_LEFT, glm::vec4(1.0f), glm::ivec2(brushRect.width, brushRect.height), TextureFilter::LINEAR, 0.0f, brushRect.x, brushRect.y, brushRect.Right(), brushRect.Bottom());
            }

            // Hover overlay
            if (hovered) {
                UI::DrawSolidRect(brushRect, style.colors.galleryHover);
            }

            // Brush label
            UI::DrawSolidRect(labelRect, style.colors.controlBackground);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + brush.name, style.font.name, glm::ivec2(labelRect.x + labelRect.width / 2, labelRect.y + labelRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, labelRect.x, labelRect.y, labelRect.Right(), labelRect.Bottom());

            // Selection outline
            if (selected) {
                UI::DrawBorder(brushRect, style.colors.text, style.gallery.selectionBorderThickness);
            }

            // Interaction
            if (hovered) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }

            const bool brushPressed = hovered && Hell::Input::LeftMousePressed();
            if (brushPressed) {
                HeightMapEditor::SetBrushType(brush.type);
            }
        }

        ScrollBar::Render(g_scrollBar);
    }

    static void RenderMaterialGallery(const EditorRect& rect, const std::vector<std::string>& materialNames, const std::string& selectedMaterialName, const std::function<void(const std::string&)>& onSelect) {
        if (!rect.HasArea() || materialNames.empty()) return;

        const EditorStyle& style = GetStyle();

        // Calculate the gallery layout
        const int32_t availableWidth = std::max(0, rect.width - style.gallery.contentPadding * 2 - style.gallery.itemGap);
        const int32_t materialWidth = availableWidth / style.gallery.columnCount;
        const int32_t materialHeight = std::max(style.gallery.minimumItemHeight, materialWidth * 2 / 3);
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();

        for (int32_t i = 0; i < static_cast<int32_t>(materialNames.size()); i++) {
            // Calculate the material and label bounds
            const int32_t column = i % style.gallery.columnCount;
            const int32_t row = i / style.gallery.columnCount;
            const EditorRect materialRect = { rect.x + style.gallery.contentPadding + column * (materialWidth + style.gallery.itemGap), rect.y + style.gallery.contentPadding + row * (materialHeight + style.gallery.itemGap), materialWidth, materialHeight };
            const EditorRect labelRect = { materialRect.x, materialRect.Bottom() - style.gallery.labelHeight, materialRect.width, style.gallery.labelHeight };

            // Resolve the material state
            const std::string& materialName = materialNames[i];
            const std::string textureName = GetBaseColorTextureName(materialName);
            const bool hovered = materialRect.Contains(mousePosition);
            const bool selected = materialName == selectedMaterialName;

            // Material background
            UI::DrawSolidRect(materialRect, style.colors.controlBackground);

            // Albedo preview
            if (!textureName.empty()) {
                UIBackEnd::BlitTexture(UICanvas::NATIVE, textureName, glm::ivec2(materialRect.x, materialRect.y), Alignment::TOP_LEFT, glm::vec4(1.0f), glm::ivec2(materialRect.width, materialRect.height), TextureFilter::LINEAR, 0.0f, materialRect.x, materialRect.y, materialRect.Right(), materialRect.Bottom());
            }

            // Hover overlay
            if (hovered) {
                UI::DrawSolidRect(materialRect, style.colors.galleryHover);
            }

            // Material label
            UI::DrawSolidRect(labelRect, style.colors.controlBackground);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + materialName, style.font.name, glm::ivec2(labelRect.x + labelRect.width / 2, labelRect.y + labelRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, labelRect.x, labelRect.y, labelRect.Right(), labelRect.Bottom());

            // Selection outline
            if (selected) {
                UI::DrawBorder(materialRect, style.colors.text, style.gallery.selectionBorderThickness);
            }

            // Interaction
            if (hovered) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }

            const bool materialPressed = hovered && Hell::Input::LeftMousePressed();
            if (materialPressed && onSelect) {
                onSelect(materialName);
            }
        }
    }

    static void RenderTerrainMaterials(const EditorRect& rect) {
        RenderMaterialGallery(rect, TERRAIN_MATERIALS, GetTerrainMaterialName(HeightMapEditor::GetSelectedTerrainMaterial()), SetTerrainMaterial);
    }

    static void RenderWorldPlaneMaterials(const EditorRect& rect, uint64_t objectId) {
        WorldPlane* worldPlane = World::GetWorldPlaneByObjectId(objectId);
        if (!worldPlane) return;
        RenderMaterialGallery(rect, ObjectOptions::GetInteriorMaterials(), worldPlane->GetCreateInfo().materialName, [worldPlane](const std::string& materialName) { worldPlane->SetMaterial(materialName); });
    }

    static void RenderDoorFloorMaterials(const EditorRect& rect, uint64_t objectId) {
        Door* door = World::GetDoorByObjectId(objectId);
        if (!door) return;
        RenderMaterialGallery(rect, ObjectOptions::GetInteriorMaterials(), door->GetCreateInfo().floorPlaneMaterialName, [door](const std::string& materialName) { door->SetFloorPlaneMaterial(materialName); });
    }

    static void RenderWallMaterials(const EditorRect& rect, uint64_t objectId) {
        Wall* wall = World::GetWallByObjectId(objectId);
        if (!wall) return;

        const bool weatherBoards = wall->GetCreateInfo().wallType == WallType::WEATHER_BOARDS;
        const std::vector<std::string>& materialNames = weatherBoards ? ObjectOptions::GetWeatherBoardMaterials() : ObjectOptions::GetInteriorMaterials();
        RenderMaterialGallery(rect, materialNames, wall->GetCreateInfo().materialName, [wall, weatherBoards](const std::string& materialName) { SetWallMaterial(wall, weatherBoards, materialName); });
    }

    bool HasBrushes() {
        return HasTools();
    }

    void RenderBrushes(const EditorRect& rect) {
        if (HasBrushes()) {
            RenderBrushGallery(rect);
        }
    }

    bool HasMaterials() {
        if (!Workspace::IsWorldBacked()) return false;
        if (Workspace::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive()) return true;
        if (!Selection::HasObjectSelection() || Selection::HasSelectedPoint() || Selection::HasSelectedWallSegment()) return false;

        const ObjectType objectType = GetObjectIdType(Selection::GetSelectedObjectId());
        return objectType == ObjectType::DOOR || objectType == ObjectType::WALL || objectType == ObjectType::WORLD_PLANE;
    }

    void RenderMaterials(const EditorRect& rect) {
        if (!HasMaterials()) return;

        if (Workspace::GetMode() == EditorSessionMode::MAP && HeightMapEditor::IsActive()) {
            RenderTerrainMaterials(rect);
            return;
        }

        const uint64_t objectId = Selection::GetSelectedObjectId();
        switch (GetObjectIdType(objectId)) {
            case ObjectType::WORLD_PLANE: RenderWorldPlaneMaterials(rect, objectId); break;
            case ObjectType::DOOR:        RenderDoorFloorMaterials(rect, objectId); break;
            case ObjectType::WALL:        RenderWallMaterials(rect, objectId); break;
            default: break;
        }
    }
}
