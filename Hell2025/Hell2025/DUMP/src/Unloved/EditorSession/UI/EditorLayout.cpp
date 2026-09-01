#include "EditorLayout.h"

#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace Unloved::EditorSession::Layout {
    namespace {
        constexpr int32_t UNSET_PANEL_HEIGHT = -1;

        int32_t g_fileMenuHeight = GetStyle().layout.defaultFileMenuHeight;
        int32_t g_hierarchyWidth = GetStyle().layout.defaultPanelWidth;
        int32_t g_propertiesWidth = GetStyle().layout.defaultPanelWidth;
        int32_t g_propertiesContentHeight = 0;
        int32_t g_toolsContentHeight = 0;
        int32_t g_propertiesHeight = UNSET_PANEL_HEIGHT;
        int32_t g_toolsHeight = UNSET_PANEL_HEIGHT;
        int32_t g_brushesHeight = UNSET_PANEL_HEIGHT;
        int32_t g_layoutWidth = 1;
        int32_t g_layoutHeight = 1;
        float g_viewportSplitX = 0.5f;
        float g_viewportSplitY = 0.5f;
        EditorViewportLayout g_viewportLayout = EditorViewportLayout::SINGLE;

        enum class Divider {
            NONE,
            HIERARCHY_RIGHT,
            PROPERTIES_LEFT,
            PROPERTIES_BOTTOM,
            TOOLS_BOTTOM,
            BRUSHES_BOTTOM,
            VIEWPORT_VERTICAL,
            VIEWPORT_HORIZONTAL,
            VIEWPORT_BOTH
        };

        Divider g_hoveredDivider = Divider::NONE;
        Divider g_activeDivider = Divider::NONE;
        glm::ivec2 g_dividerGrabOffset = glm::ivec2(0);
        bool g_dividerCursorActive = false;

        EditorPanel CreatePanel(EditorPanelEdge edges, const glm::vec4& backgroundColor, bool drawBackground = true) {
            EditorPanel panel;
            panel.edges = edges;
            panel.backgroundColor = backgroundColor;
            panel.borderColor = GetStyle().colors.border;
            panel.borderThickness = 1;
            panel.drawBackground = drawBackground;
            return panel;
        }

        EditorPanel g_fileMenuPanel = CreatePanel(EditorPanelEdge::BOTTOM, GetStyle().colors.controlBackground);
        EditorPanel g_hierarchyPanel = CreatePanel(EditorPanelEdge::RIGHT, GetStyle().colors.panelBackground);
        EditorPanel g_viewportsPanel = CreatePanel(EditorPanelEdge::NONE, glm::vec4(0.0f), false);
        EditorPanel g_propertiesPanel = CreatePanel(EditorPanelEdge::LEFT, GetStyle().colors.panelBackground);
        EditorPanel g_toolsPanel = CreatePanel(EditorPanelEdge::TOP | EditorPanelEdge::LEFT, GetStyle().colors.panelBackground);
        EditorPanel g_brushesPanel = CreatePanel(EditorPanelEdge::TOP | EditorPanelEdge::LEFT, GetStyle().colors.panelBackground);
        EditorPanel g_materialsPanel = CreatePanel(EditorPanelEdge::TOP | EditorPanelEdge::LEFT, GetStyle().colors.panelBackground);
        std::array<EditorViewportRegion, 4> g_viewportRegions;

        int32_t GetViewportSplitOffset(int32_t extent, float split) {
            if (extent <= 0) return 0;
            const int32_t minimumSize = std::min(GetStyle().layout.minimumViewportRegionSize, extent / 2);
            return std::clamp(static_cast<int32_t>(std::round(static_cast<float>(extent) * split)), minimumSize, extent - minimumSize);
        }

        int32_t GetRequiredViewportWidth() {
            const int32_t columnCount = g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR ? 2 : 1;
            return std::max(GetStyle().layout.minimumViewportsWidth, columnCount * GetStyle().layout.minimumViewportRegionSize);
        }

        void SetViewportRegion(uint32_t index, const EditorRect& rect) {
            EditorViewportRegion& region = g_viewportRegions[index];
            region.rect = rect;
            region.visible = rect.HasArea();

            const float inverseWidth = 1.0f / static_cast<float>(g_layoutWidth);
            const float inverseHeight = 1.0f / static_cast<float>(g_layoutHeight);
            region.normalizedPosition.x = static_cast<float>(rect.x) * inverseWidth;
            region.normalizedPosition.y = static_cast<float>(rect.y) * inverseHeight;
            region.normalizedSize.x = static_cast<float>(rect.width) * inverseWidth;
            region.normalizedSize.y = static_cast<float>(rect.height) * inverseHeight;
        }

        void UpdateViewportRegions() {
            for (EditorViewportRegion& region : g_viewportRegions) {
                region = {};
            }

            const EditorRect& rect = g_viewportsPanel.rect;
            if (g_viewportLayout == EditorViewportLayout::SINGLE) {
                SetViewportRegion(0, rect);
                return;
            }

            const int32_t leftWidth = GetViewportSplitOffset(rect.width, g_viewportSplitX);
            const int32_t rightWidth = rect.width - leftWidth;

            if (g_viewportLayout == EditorViewportLayout::LEFT_RIGHT) {
                SetViewportRegion(0, { rect.x, rect.y, leftWidth, rect.height });
                SetViewportRegion(1, { rect.x + leftWidth, rect.y, rightWidth, rect.height });
                return;
            }

            const int32_t topHeight = GetViewportSplitOffset(rect.height, g_viewportSplitY);
            const int32_t bottomHeight = rect.height - topHeight;
            if (g_viewportLayout == EditorViewportLayout::TOP_BOTTOM) {
                SetViewportRegion(0, { rect.x, rect.y, rect.width, topHeight });
                SetViewportRegion(1, { rect.x, rect.y + topHeight, rect.width, bottomHeight });
                return;
            }

            SetViewportRegion(0, { rect.x, rect.y, leftWidth, topHeight });
            SetViewportRegion(1, { rect.x + leftWidth, rect.y, rightWidth, topHeight });
            SetViewportRegion(2, { rect.x, rect.y + topHeight, leftWidth, bottomHeight });
            SetViewportRegion(3, { rect.x + leftWidth, rect.y + topHeight, rightWidth, bottomHeight });
        }

        EditorPanel* GetPanelById(EditorPanelId panelId) {
            switch (panelId) {
                case EditorPanelId::FILE_MENU:  return &g_fileMenuPanel;
                case EditorPanelId::HIERARCHY:  return &g_hierarchyPanel;
                case EditorPanelId::VIEWPORTS:  return &g_viewportsPanel;
                case EditorPanelId::PROPERTIES: return &g_propertiesPanel;
                case EditorPanelId::TOOLS:      return &g_toolsPanel;
                case EditorPanelId::MATERIALS:  return &g_materialsPanel;
                case EditorPanelId::BRUSHES:    return &g_brushesPanel;
            }
            return nullptr;
        }

        EditorRect GetPanelHeadingRect(const EditorPanel& panel) {
            return { panel.rect.x, panel.rect.y, panel.rect.width, std::min(GetStyle().layout.panelHeadingHeight, panel.rect.height) };
        }

        EditorRect GetPanelContentRect(const EditorPanel& panel) {
            const int32_t headingHeight = std::min(GetStyle().layout.panelHeadingHeight, panel.rect.height);
            return { panel.rect.x, panel.rect.y + headingHeight, panel.rect.width, panel.rect.height - headingHeight };
        }

        void DrawPanelHeadingBackground(const EditorPanel& panel) {
            if (!panel.visible || !panel.rect.HasArea()) return;
            UI::DrawSolidRect(GetPanelHeadingRect(panel), GetStyle().colors.controlBackground);
        }

        void DrawPanelHeadingText(const EditorPanel& panel, const char* heading) {
            if (!panel.visible || !panel.rect.HasArea()) return;

            const EditorStyle& style = GetStyle();
            const EditorRect headingRect = GetPanelHeadingRect(panel);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + heading, style.font.name, glm::ivec2(headingRect.x + style.layout.headingTextPadding, headingRect.y + headingRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);
        }

        bool IsNearVerticalDivider(const glm::ivec2& mousePosition, int32_t dividerX, const EditorRect& bounds) {
            return bounds.HasArea() && mousePosition.y >= bounds.y && mousePosition.y < bounds.Bottom() && std::abs(mousePosition.x - dividerX) <= GetStyle().layout.dividerHitRadius;
        }

        bool IsNearHorizontalDivider(const glm::ivec2& mousePosition, int32_t dividerY, const EditorRect& bounds) {
            return bounds.HasArea() && mousePosition.x >= bounds.x && mousePosition.x < bounds.Right() && std::abs(mousePosition.y - dividerY) <= GetStyle().layout.dividerHitRadius;
        }

        Divider FindHoveredDivider(const glm::ivec2& mousePosition) {
            // Side panel dividers
            if (IsNearVerticalDivider(mousePosition, g_hierarchyPanel.rect.Right(), g_hierarchyPanel.rect)) {
                return Divider::HIERARCHY_RIGHT;
            }
            if (IsNearVerticalDivider(mousePosition, g_propertiesPanel.rect.x, g_propertiesPanel.rect)) {
                return Divider::PROPERTIES_LEFT;
            }
            if (IsNearVerticalDivider(mousePosition, g_toolsPanel.rect.x, g_toolsPanel.rect)) {
                return Divider::PROPERTIES_LEFT;
            }
            if (IsNearVerticalDivider(mousePosition, g_brushesPanel.rect.x, g_brushesPanel.rect)) {
                return Divider::PROPERTIES_LEFT;
            }
            if (IsNearVerticalDivider(mousePosition, g_materialsPanel.rect.x, g_materialsPanel.rect)) {
                return Divider::PROPERTIES_LEFT;
            }

            const EditorRect rightPanelBounds = { g_propertiesPanel.rect.x, g_propertiesPanel.rect.y, g_propertiesPanel.rect.width, g_layoutHeight - g_propertiesPanel.rect.y };
            if ((g_toolsPanel.visible || g_brushesPanel.visible || g_materialsPanel.visible) && IsNearHorizontalDivider(mousePosition, g_propertiesPanel.rect.Bottom(), rightPanelBounds)) {
                return Divider::PROPERTIES_BOTTOM;
            }
            if (g_toolsPanel.visible && (g_brushesPanel.visible || g_materialsPanel.visible) && IsNearHorizontalDivider(mousePosition, g_toolsPanel.rect.Bottom(), rightPanelBounds)) {
                return Divider::TOOLS_BOTTOM;
            }
            if (g_brushesPanel.visible && g_materialsPanel.visible && IsNearHorizontalDivider(mousePosition, g_brushesPanel.rect.Bottom(), rightPanelBounds)) {
                return Divider::BRUSHES_BOTTOM;
            }

            // Viewport dividers
            const bool verticalSplitVisible = g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR;
            const bool horizontalSplitVisible = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM || g_viewportLayout == EditorViewportLayout::FOUR;
            const int32_t horizontalSplitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
            const bool verticalSplitHovered = verticalSplitVisible && IsNearVerticalDivider(mousePosition, g_viewportRegions[1].rect.x, g_viewportsPanel.rect);
            const bool horizontalSplitHovered = horizontalSplitVisible && IsNearHorizontalDivider(mousePosition, horizontalSplitY, g_viewportsPanel.rect);

            if (verticalSplitHovered && horizontalSplitHovered) {
                return Divider::VIEWPORT_BOTH;
            }
            if (verticalSplitHovered) {
                return Divider::VIEWPORT_VERTICAL;
            }
            if (horizontalSplitHovered) {
                return Divider::VIEWPORT_HORIZONTAL;
            }

            return Divider::NONE;
        }

        void ResizeHierarchyPanel(int32_t mouseX) {
            const int32_t maximumWidth = std::max(0, g_layoutWidth - g_propertiesPanel.rect.width - GetRequiredViewportWidth());
            const int32_t minimumWidth = std::min(GetStyle().layout.minimumSidePanelWidth, maximumWidth);
            g_hierarchyWidth = std::clamp(mouseX, minimumWidth, maximumWidth);
        }

        void ResizePropertiesPanel(int32_t mouseX) {
            const int32_t maximumWidth = std::max(0, g_layoutWidth - g_hierarchyPanel.rect.width - GetRequiredViewportWidth());
            const int32_t minimumWidth = std::min(GetStyle().layout.minimumSidePanelWidth, maximumWidth);
            g_propertiesWidth = std::clamp(g_layoutWidth - mouseX, minimumWidth, maximumWidth);
        }

        void ResizePropertiesHeight(int32_t mouseY) {
            const int32_t headingHeight = GetStyle().layout.panelHeadingHeight;
            const int32_t workspaceHeight = g_layoutHeight - g_fileMenuPanel.rect.height;
            const int32_t panelsBelow = static_cast<int32_t>(g_toolsPanel.visible) + static_cast<int32_t>(g_brushesPanel.visible) + static_cast<int32_t>(g_materialsPanel.visible);
            const int32_t maximumHeight = std::max(0, workspaceHeight - panelsBelow * headingHeight);
            const int32_t minimumHeight = std::min(headingHeight, maximumHeight);
            g_propertiesHeight = std::clamp(mouseY - g_fileMenuPanel.rect.height, minimumHeight, maximumHeight);
        }

        void ResizeToolsHeight(int32_t mouseY) {
            const int32_t headingHeight = GetStyle().layout.panelHeadingHeight;
            const int32_t availableHeight = g_layoutHeight - g_toolsPanel.rect.y;
            const int32_t panelsBelow = static_cast<int32_t>(g_brushesPanel.visible) + static_cast<int32_t>(g_materialsPanel.visible);
            const int32_t maximumHeight = std::max(0, availableHeight - panelsBelow * headingHeight);
            const int32_t minimumHeight = std::min(headingHeight, maximumHeight);
            g_toolsHeight = std::clamp(mouseY - g_toolsPanel.rect.y, minimumHeight, maximumHeight);
        }

        void ResizeBrushesHeight(int32_t mouseY) {
            const int32_t headingHeight = GetStyle().layout.panelHeadingHeight;
            const int32_t availableHeight = g_layoutHeight - g_brushesPanel.rect.y;
            const int32_t maximumHeight = std::max(0, availableHeight - headingHeight);
            const int32_t minimumHeight = std::min(headingHeight, maximumHeight);
            g_brushesHeight = std::clamp(mouseY - g_brushesPanel.rect.y, minimumHeight, maximumHeight);
        }

        void ResizeViewportVerticalSplit(int32_t mouseX) {
            if (g_viewportsPanel.rect.width <= 0) return;
            const int32_t minimumSize = std::min(GetStyle().layout.minimumViewportRegionSize, g_viewportsPanel.rect.width / 2);
            const int32_t splitOffset = std::clamp(mouseX - g_viewportsPanel.rect.x, minimumSize, g_viewportsPanel.rect.width - minimumSize);
            g_viewportSplitX = static_cast<float>(splitOffset) / static_cast<float>(g_viewportsPanel.rect.width);
        }

        void ResizeViewportHorizontalSplit(int32_t mouseY) {
            if (g_viewportsPanel.rect.height <= 0) return;
            const int32_t minimumSize = std::min(GetStyle().layout.minimumViewportRegionSize, g_viewportsPanel.rect.height / 2);
            const int32_t splitOffset = std::clamp(mouseY - g_viewportsPanel.rect.y, minimumSize, g_viewportsPanel.rect.height - minimumSize);
            g_viewportSplitY = static_cast<float>(splitOffset) / static_cast<float>(g_viewportsPanel.rect.height);
        }

        void ResizeActiveDivider(const glm::ivec2& mousePosition) {
            const glm::ivec2 dividerPosition = mousePosition - g_dividerGrabOffset;
            switch (g_activeDivider) {
                case Divider::HIERARCHY_RIGHT:
                    ResizeHierarchyPanel(dividerPosition.x);
                    break;
                case Divider::PROPERTIES_LEFT:
                    ResizePropertiesPanel(dividerPosition.x);
                    break;
                case Divider::PROPERTIES_BOTTOM:
                    ResizePropertiesHeight(dividerPosition.y);
                    break;
                case Divider::TOOLS_BOTTOM:
                    ResizeToolsHeight(dividerPosition.y);
                    break;
                case Divider::BRUSHES_BOTTOM:
                    ResizeBrushesHeight(dividerPosition.y);
                    break;
                case Divider::VIEWPORT_VERTICAL:
                    ResizeViewportVerticalSplit(dividerPosition.x);
                    break;
                case Divider::VIEWPORT_HORIZONTAL:
                    ResizeViewportHorizontalSplit(dividerPosition.y);
                    break;
                case Divider::VIEWPORT_BOTH:
                    ResizeViewportVerticalSplit(dividerPosition.x);
                    ResizeViewportHorizontalSplit(dividerPosition.y);
                    break;
                default:
                    break;
            }
        }

        void BeginDividerDrag(const glm::ivec2& mousePosition) {
            g_activeDivider = g_hoveredDivider;
            g_dividerGrabOffset = glm::ivec2(0);

            if (g_activeDivider == Divider::HIERARCHY_RIGHT || g_activeDivider == Divider::PROPERTIES_LEFT) {
                g_hierarchyWidth = g_hierarchyPanel.rect.width;
                g_propertiesWidth = g_propertiesPanel.rect.width;
            }

            if (g_activeDivider == Divider::PROPERTIES_BOTTOM) {
                g_propertiesHeight = g_propertiesPanel.rect.height;
            }
            else if (g_activeDivider == Divider::TOOLS_BOTTOM) {
                g_toolsHeight = g_toolsPanel.rect.height;
            }
            else if (g_activeDivider == Divider::BRUSHES_BOTTOM) {
                g_brushesHeight = g_brushesPanel.rect.height;
            }

            if (g_activeDivider == Divider::HIERARCHY_RIGHT) {
                g_dividerGrabOffset.x = mousePosition.x - g_hierarchyPanel.rect.Right();
            }
            else if (g_activeDivider == Divider::PROPERTIES_LEFT) {
                g_dividerGrabOffset.x = mousePosition.x - g_propertiesPanel.rect.x;
            }
            else if (g_activeDivider == Divider::PROPERTIES_BOTTOM) {
                g_dividerGrabOffset.y = mousePosition.y - g_propertiesPanel.rect.Bottom();
            }
            else if (g_activeDivider == Divider::TOOLS_BOTTOM) {
                g_dividerGrabOffset.y = mousePosition.y - g_toolsPanel.rect.Bottom();
            }
            else if (g_activeDivider == Divider::BRUSHES_BOTTOM) {
                g_dividerGrabOffset.y = mousePosition.y - g_brushesPanel.rect.Bottom();
            }
            else if (g_activeDivider == Divider::VIEWPORT_VERTICAL) {
                g_dividerGrabOffset.x = mousePosition.x - g_viewportRegions[1].rect.x;
            }
            else if (g_activeDivider == Divider::VIEWPORT_HORIZONTAL) {
                const int32_t splitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
                g_dividerGrabOffset.y = mousePosition.y - splitY;
            }
            else if (g_activeDivider == Divider::VIEWPORT_BOTH) {
                g_dividerGrabOffset = mousePosition - glm::ivec2(g_viewportRegions[1].rect.x, g_viewportRegions[2].rect.y);
            }
        }

        void ApplyDividerCursor() {
            const Divider divider = g_activeDivider != Divider::NONE ? g_activeDivider : g_hoveredDivider;
            if (divider == Divider::NONE) {
                if (g_dividerCursorActive) {
                    Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
                }

                g_dividerCursorActive = false;
                return;
            }

            if (divider == Divider::VIEWPORT_BOTH) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_CROSSHAIR);
            }
            else if (divider == Divider::PROPERTIES_BOTTOM || divider == Divider::TOOLS_BOTTOM || divider == Divider::BRUSHES_BOTTOM || divider == Divider::VIEWPORT_HORIZONTAL) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_VRESIZE);
            }
            else {
                Hell::BackEnd::SetCursor(HELL_CURSOR_HRESIZE);
            }

            g_dividerCursorActive = true;
        }
    }

    void Update() {
        const EditorLayoutStyle& style = GetStyle().layout;
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        g_layoutWidth = std::max(1, static_cast<int32_t>(resolution.x));
        g_layoutHeight = std::max(1, static_cast<int32_t>(resolution.y));

        // Calculate panel dimensions
        const int32_t fileMenuHeight = std::clamp(g_fileMenuHeight, 0, g_layoutHeight);
        const int32_t availableSideWidth = std::max(0, g_layoutWidth - GetRequiredViewportWidth());
        const int32_t requestedSideWidth = std::max(0, g_hierarchyWidth) + std::max(0, g_propertiesWidth);
        const float sideScale = requestedSideWidth > availableSideWidth && requestedSideWidth > 0
            ? static_cast<float>(availableSideWidth) / static_cast<float>(requestedSideWidth)
            : 1.0f;
        const int32_t hierarchyWidth = static_cast<int32_t>(std::round(static_cast<float>(std::max(0, g_hierarchyWidth)) * sideScale));
        const int32_t propertiesWidth = std::min(static_cast<int32_t>(std::round(static_cast<float>(std::max(0, g_propertiesWidth)) * sideScale)), g_layoutWidth - hierarchyWidth);
        const int32_t viewportWidth = std::max(0, g_layoutWidth - hierarchyWidth - propertiesWidth);
        const int32_t workspaceHeight = g_layoutHeight - fileMenuHeight;
        const bool hasLowerPanels = g_toolsPanel.visible || g_brushesPanel.visible || g_materialsPanel.visible;
        const int32_t lowerHeadingHeight = (g_toolsPanel.visible ? style.panelHeadingHeight : 0) + (g_brushesPanel.visible ? style.panelHeadingHeight : 0) + (g_materialsPanel.visible ? style.panelHeadingHeight : 0);
        const int32_t maximumPropertiesHeight = std::max(0, workspaceHeight - lowerHeadingHeight);
        const int32_t minimumPropertiesHeight = std::min(style.panelHeadingHeight, maximumPropertiesHeight);
        const int32_t automaticPropertiesHeight = std::min(style.panelHeadingHeight + g_propertiesContentHeight + style.panelGap, maximumPropertiesHeight);
        const int32_t propertiesHeight = !hasLowerPanels ? workspaceHeight : g_propertiesHeight == UNSET_PANEL_HEIGHT ? automaticPropertiesHeight : std::clamp(g_propertiesHeight, minimumPropertiesHeight, maximumPropertiesHeight);
        const int32_t remainingHeight = workspaceHeight - propertiesHeight;
        const int32_t panelsBelowTools = static_cast<int32_t>(g_brushesPanel.visible) + static_cast<int32_t>(g_materialsPanel.visible);
        const int32_t maximumToolsHeight = std::max(0, remainingHeight - std::min(remainingHeight, panelsBelowTools * style.panelHeadingHeight));
        const int32_t minimumToolsHeight = std::min(style.panelHeadingHeight, maximumToolsHeight);
        const int32_t automaticToolsHeight = std::min(style.panelHeadingHeight + g_toolsContentHeight + style.panelGap, maximumToolsHeight);
        const int32_t toolsHeight = !g_toolsPanel.visible ? 0 : panelsBelowTools == 0 ? remainingHeight : g_toolsHeight == UNSET_PANEL_HEIGHT ? automaticToolsHeight : std::clamp(g_toolsHeight, minimumToolsHeight, maximumToolsHeight);
        const int32_t galleryHeight = remainingHeight - toolsHeight;
        const int32_t minimumSplitPanelHeight = std::min(style.panelHeadingHeight, galleryHeight / 2);
        const int32_t automaticBrushesHeight = galleryHeight - std::clamp(galleryHeight / 3, minimumSplitPanelHeight, galleryHeight - minimumSplitPanelHeight);
        const int32_t brushesHeight = !g_brushesPanel.visible ? 0 : !g_materialsPanel.visible ? galleryHeight : g_brushesHeight == UNSET_PANEL_HEIGHT ? automaticBrushesHeight : std::clamp(g_brushesHeight, minimumSplitPanelHeight, galleryHeight - minimumSplitPanelHeight);
        const int32_t materialsHeight = g_materialsPanel.visible ? galleryHeight - brushesHeight : 0;
        const int32_t rightPanelX = hierarchyWidth + viewportWidth;

        // Position panels
        g_fileMenuPanel.rect = { 0, 0, g_layoutWidth, fileMenuHeight };
        g_hierarchyPanel.rect = { 0, fileMenuHeight, hierarchyWidth, workspaceHeight };
        g_viewportsPanel.rect = { hierarchyWidth, fileMenuHeight, viewportWidth, workspaceHeight };
        g_propertiesPanel.rect = { rightPanelX, fileMenuHeight, propertiesWidth, propertiesHeight };
        g_toolsPanel.rect = g_toolsPanel.visible ? EditorRect{ rightPanelX, fileMenuHeight + propertiesHeight, propertiesWidth, toolsHeight } : EditorRect{};
        g_brushesPanel.rect = g_brushesPanel.visible ? EditorRect{ rightPanelX, fileMenuHeight + propertiesHeight + toolsHeight, propertiesWidth, brushesHeight } : EditorRect{};
        g_materialsPanel.rect = g_materialsPanel.visible ? EditorRect{ rightPanelX, fileMenuHeight + propertiesHeight + toolsHeight + brushesHeight, propertiesWidth, materialsHeight } : EditorRect{};

        UpdateViewportRegions();
    }

    void UpdateDividerInput(bool allowInput) {
        if (!Hell::BackEnd::WindowHasFocus()) {
            CancelInteraction();
            return;
        }

        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();

        if (!Hell::Input::LeftMouseDown()) {
            g_activeDivider = Divider::NONE;
        }

        g_hoveredDivider = g_activeDivider == Divider::NONE && allowInput ? FindHoveredDivider(mousePosition) : g_activeDivider;
        if (allowInput && Hell::Input::LeftMousePressed() && g_hoveredDivider != Divider::NONE) {
            BeginDividerDrag(mousePosition);
        }

        if (g_activeDivider != Divider::NONE && Hell::Input::LeftMouseDown()) {
            ResizeActiveDivider(mousePosition);
            Update();
        }

        ApplyDividerCursor();
    }

    void RenderBackgrounds() {
        // Panel backgrounds
        UI::DrawPanelBackground(g_fileMenuPanel);
        UI::DrawPanelBackground(g_hierarchyPanel);
        UI::DrawPanelBackground(g_viewportsPanel);
        UI::DrawPanelBackground(g_propertiesPanel);
        UI::DrawPanelBackground(g_toolsPanel);
        UI::DrawPanelBackground(g_brushesPanel);
        UI::DrawPanelBackground(g_materialsPanel);

        // Panel headings
        DrawPanelHeadingBackground(g_hierarchyPanel);
        DrawPanelHeadingBackground(g_propertiesPanel);
        DrawPanelHeadingBackground(g_toolsPanel);
        DrawPanelHeadingBackground(g_brushesPanel);
        DrawPanelHeadingBackground(g_materialsPanel);
    }

    void RenderOverlay() {
        // Heading labels
        DrawPanelHeadingText(g_hierarchyPanel, "Hierarchy");
        DrawPanelHeadingText(g_propertiesPanel, "Properties");
        DrawPanelHeadingText(g_toolsPanel, "Tools");
        DrawPanelHeadingText(g_brushesPanel, "Brushes");
        DrawPanelHeadingText(g_materialsPanel, "Materials");

        // Panel borders
        UI::DrawPanelEdges(g_fileMenuPanel);
        UI::DrawPanelEdges(g_hierarchyPanel);
        UI::DrawPanelEdges(g_viewportsPanel);
        UI::DrawPanelEdges(g_propertiesPanel);
        UI::DrawPanelEdges(g_toolsPanel);
        UI::DrawPanelEdges(g_brushesPanel);
        UI::DrawPanelEdges(g_materialsPanel);

        // Viewport dividers
        if (g_viewportLayout == EditorViewportLayout::LEFT_RIGHT || g_viewportLayout == EditorViewportLayout::FOUR) {
            const int32_t splitX = g_viewportRegions[1].rect.x;
            UI::DrawLine(glm::vec2(splitX, g_viewportsPanel.rect.y), glm::vec2(splitX, g_viewportsPanel.rect.Bottom()), GetStyle().colors.border);
        }
        if (g_viewportLayout == EditorViewportLayout::TOP_BOTTOM || g_viewportLayout == EditorViewportLayout::FOUR) {
            const int32_t splitY = g_viewportLayout == EditorViewportLayout::TOP_BOTTOM ? g_viewportRegions[1].rect.y : g_viewportRegions[2].rect.y;
            UI::DrawLine(glm::vec2(g_viewportsPanel.rect.x, splitY), glm::vec2(g_viewportsPanel.rect.Right(), splitY), GetStyle().colors.border);
        }
    }

    void SetFileMenuHeight(int32_t height) {
        g_fileMenuHeight = std::max(0, height);
    }

    void SetHierarchyWidth(int32_t width) {
        g_hierarchyWidth = std::max(0, width);
    }

    void SetPropertiesWidth(int32_t width) {
        g_propertiesWidth = std::max(0, width);
    }

    void SetPropertiesContentHeight(int32_t height) {
        g_propertiesContentHeight = std::max(0, height);
    }

    void SetToolsContentHeight(int32_t height) {
        g_toolsContentHeight = std::max(0, height);
    }

    void SetToolsVisible(bool visible) {
        if (g_toolsPanel.visible == visible) return;
        g_toolsPanel.visible = visible;
        Update();
    }

    void SetBrushesVisible(bool visible) {
        if (g_brushesPanel.visible == visible) return;
        g_brushesPanel.visible = visible;
        Update();
    }

    void SetMaterialsVisible(bool visible) {
        if (g_materialsPanel.visible == visible) return;
        g_materialsPanel.visible = visible;
        Update();
    }

    void SetViewportLayout(EditorViewportLayout layout) {
        g_viewportLayout = layout;
        CancelInteraction();
    }

    void CancelInteraction() {
        g_hoveredDivider = Divider::NONE;
        g_activeDivider = Divider::NONE;
        g_dividerGrabOffset = glm::ivec2(0);
        if (g_dividerCursorActive) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        g_dividerCursorActive = false;
    }

    void SetPanelEdges(EditorPanelId panelId, EditorPanelEdge edges) {
        if (EditorPanel* panel = GetPanelById(panelId)) {
            panel->edges = edges;
        }
    }

    int32_t GetFileMenuHeight() {
        return g_fileMenuHeight;
    }

    int32_t GetHierarchyWidth() {
        return g_hierarchyWidth;
    }

    int32_t GetPropertiesWidth() {
        return g_propertiesWidth;
    }

    uint32_t GetViewportCount() {
        if (g_viewportLayout == EditorViewportLayout::SINGLE) {
            return 1;
        }
        if (g_viewportLayout == EditorViewportLayout::FOUR) {
            return 4;
        }

        return 2;
    }

    EditorViewportLayout GetViewportLayout() {
        return g_viewportLayout;
    }

    bool WantsMouseCapture() {
        return g_hoveredDivider != Divider::NONE || g_activeDivider != Divider::NONE;
    }

    const EditorPanel& GetFileMenuPanel() {
        return g_fileMenuPanel;
    }

    const EditorPanel& GetHierarchyPanel() {
        return g_hierarchyPanel;
    }

    const EditorPanel& GetViewportsPanel() {
        return g_viewportsPanel;
    }

    const EditorPanel& GetPropertiesPanel() {
        return g_propertiesPanel;
    }

    const EditorPanel& GetToolsPanel() {
        return g_toolsPanel;
    }

    const EditorPanel& GetBrushesPanel() {
        return g_brushesPanel;
    }

    const EditorPanel& GetMaterialsPanel() {
        return g_materialsPanel;
    }

    EditorRect GetHierarchyContentRect() {
        return GetPanelContentRect(g_hierarchyPanel);
    }

    EditorRect GetPropertiesContentRect() {
        return GetPanelContentRect(g_propertiesPanel);
    }

    EditorRect GetToolsContentRect() {
        return GetPanelContentRect(g_toolsPanel);
    }

    EditorRect GetBrushesContentRect() {
        return GetPanelContentRect(g_brushesPanel);
    }

    EditorRect GetMaterialsContentRect() {
        return GetPanelContentRect(g_materialsPanel);
    }

    const EditorViewportRegion* GetViewportRegionByIndex(uint32_t index) {
        return index < g_viewportRegions.size() ? &g_viewportRegions[index] : nullptr;
    }
}
