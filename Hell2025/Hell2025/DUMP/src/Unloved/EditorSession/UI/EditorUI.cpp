#include "EditorUI.h"

#include "EditorStyle.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <cmath>
#include <glm/geometric.hpp>

// UI coordinates
namespace Unloved::EditorSession::Coordinates {

    glm::ivec2 WindowToUI(const glm::ivec2& windowPosition) {
        const glm::uvec2 resolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
        const int32_t windowWidth = std::max(1, Hell::BackEnd::GetCurrentWindowWidth());
        const int32_t windowHeight = std::max(1, Hell::BackEnd::GetCurrentWindowHeight());

        return glm::ivec2(static_cast<int32_t>(static_cast<float>(windowPosition.x) * static_cast<float>(resolution.x) / static_cast<float>(windowWidth)), static_cast<int32_t>(static_cast<float>(windowPosition.y) * static_cast<float>(resolution.y) / static_cast<float>(windowHeight)));
    }

    glm::ivec2 GetMousePositionUI() {
        // Hidden fly cursor does not touch editor UI
        if (Viewports::IsFlyMode()) return glm::ivec2(-1);
        return WindowToUI(glm::ivec2(Hell::Input::GetMouseX(), Hell::Input::GetMouseY()));
    }
}

namespace Unloved::EditorSession::UI {

    void DrawSolidRect(const EditorRect& rect, const glm::vec4& color) {
        if (!rect.HasArea() || color.a <= 0.0f) return;

        UIBackEnd::BlitTexture(UICanvas::NATIVE, "White", glm::ivec2(rect.x, rect.y), Alignment::TOP_LEFT, color, glm::ivec2(rect.width, rect.height));
    }

    void DrawBorder(const EditorRect& rect, const glm::vec4& color, int32_t thickness) {
        if (thickness <= 0) return;

        DrawSolidRect({ rect.x, rect.y, rect.width, thickness }, color);
        DrawSolidRect({ rect.x, rect.Bottom() - thickness, rect.width, thickness }, color);
        DrawSolidRect({ rect.x, rect.y, thickness, rect.height }, color);
        DrawSolidRect({ rect.Right() - thickness, rect.y, thickness, rect.height }, color);
    }

    void DrawLine(const glm::vec2& begin, const glm::vec2& end, const glm::vec4& color, float thickness) {
        const glm::vec2 delta = end - begin;
        const float length = glm::length(delta);
        if (length <= 0.0f || thickness <= 0.0f || color.a <= 0.0f) return;

        const glm::vec2 midpoint = (begin + end) * 0.5f;
        const glm::ivec2 location = glm::ivec2(static_cast<int32_t>(std::round(midpoint.x)), static_cast<int32_t>(std::round(midpoint.y)));
        const glm::ivec2 size = glm::ivec2(std::max(1, static_cast<int32_t>(std::round(length))), std::max(1, static_cast<int32_t>(std::round(thickness))));
        const float rotation = std::atan2(delta.y, delta.x);

        UIBackEnd::BlitTexture(UICanvas::NATIVE, "White", location, Alignment::CENTERED, color, size, TextureFilter::NEAREST, rotation);
    }

    void DrawPanelBackground(const EditorPanel& panel) {
        if (!panel.visible || !panel.drawBackground) return;
        DrawSolidRect(panel.rect, panel.backgroundColor);
    }

    void DrawPanelEdges(const EditorPanel& panel) {
        if (!panel.visible || !panel.rect.HasArea() || panel.borderThickness <= 0) return;

        const int32_t thickness = panel.borderThickness;
        if (HasPanelEdge(panel.edges, EditorPanelEdge::TOP)) {
            DrawSolidRect({ panel.rect.x, panel.rect.y, panel.rect.width, thickness }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::BOTTOM)) {
            DrawSolidRect({ panel.rect.x, panel.rect.Bottom() - thickness, panel.rect.width, thickness }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::LEFT)) {
            DrawSolidRect({ panel.rect.x, panel.rect.y, thickness, panel.rect.height }, panel.borderColor);
        }
        if (HasPanelEdge(panel.edges, EditorPanelEdge::RIGHT)) {
            DrawSolidRect({ panel.rect.Right() - thickness, panel.rect.y, thickness, panel.rect.height }, panel.borderColor);
        }
    }

    void DrawPanel(const EditorPanel& panel) {
        DrawPanelBackground(panel);
        DrawPanelEdges(panel);
    }
}

namespace Unloved::EditorSession::Buttons {

    void Update(EditorButton& button, const glm::ivec2& mousePosition, bool allowInput) {
        button.hovered = button.visible && button.rect.Contains(mousePosition);

        if (allowInput && button.hovered && Hell::Input::LeftMousePressed() && button.onPressed) {
            button.onPressed();
        }
    }

    void Render(const EditorButton& button) {
        if (!button.visible || !button.iconTextureName) return;

        // Draw button background
        const char* backgroundTextureName = button.hovered || button.selected ? "button_empty_hover" : "button_empty";
        UIBackEnd::BlitTexture(UICanvas::NATIVE, backgroundTextureName, glm::ivec2(button.rect.x, button.rect.y), Alignment::TOP_LEFT, WHITE, glm::ivec2(button.rect.width, button.rect.height), TextureFilter::NEAREST);

        // Draw button icon
        UIBackEnd::BlitTexture(UICanvas::NATIVE, button.iconTextureName, glm::ivec2(button.rect.x + button.rect.width / 2, button.rect.y + button.rect.height / 2), Alignment::CENTERED, WHITE, glm::ivec2(button.rect.width, button.rect.height), TextureFilter::NEAREST);
    }
}

namespace Unloved::EditorSession::ScrollBar {
    namespace {
        int32_t GetMaximumValue(int32_t contentSize, int32_t visibleSize) {
            return std::max(0, contentSize - visibleSize);
        }

        void RefreshThumb(EditorScrollBar& scrollBar, int32_t contentSize, int32_t visibleSize) {
            const int32_t maximumValue = GetMaximumValue(contentSize, visibleSize);
            const float visibleRatio = static_cast<float>(visibleSize) / static_cast<float>(contentSize);
            const int32_t thumbHeight = std::clamp(static_cast<int32_t>(std::round(static_cast<float>(scrollBar.trackRect.height) * visibleRatio)), std::min(GetStyle().scrollBar.minimumThumbSize, scrollBar.trackRect.height), scrollBar.trackRect.height);
            const int32_t thumbTravel = scrollBar.trackRect.height - thumbHeight;
            const float scrollRatio = maximumValue > 0 ? static_cast<float>(scrollBar.value) / static_cast<float>(maximumValue) : 0.0f;

            scrollBar.thumbRect = { scrollBar.trackRect.x, scrollBar.trackRect.y + static_cast<int32_t>(std::round(static_cast<float>(thumbTravel) * scrollRatio)), scrollBar.trackRect.width, thumbHeight };
        }
    }

    void Update(EditorScrollBar& scrollBar, const EditorRect& rect, int32_t contentSize, int32_t visibleSize, bool allowInput) {
        scrollBar.trackRect = rect;
        scrollBar.visible = rect.HasArea() && visibleSize > 0 && contentSize > visibleSize;

        // Reset hidden scroll bars
        if (!scrollBar.visible) {
            scrollBar.value = 0;
            scrollBar.hovered = false;
            scrollBar.dragging = false;
            return;
        }

        const int32_t maximumValue = GetMaximumValue(contentSize, visibleSize);
        scrollBar.value = std::clamp(scrollBar.value, 0, maximumValue);
        RefreshThumb(scrollBar, contentSize, visibleSize);

        // Handle pointer input
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        if (!Hell::Input::LeftMouseDown()) {
            scrollBar.dragging = false;
        }

        scrollBar.hovered = allowInput && scrollBar.trackRect.Contains(mousePosition);
        const bool thumbPressed = allowInput && Hell::Input::LeftMousePressed() && scrollBar.thumbRect.Contains(mousePosition);
        if (thumbPressed) {
            scrollBar.dragging = true;
            scrollBar.dragOffset = mousePosition.y - scrollBar.thumbRect.y;
        }
        else {
            const bool trackPressed = allowInput && Hell::Input::LeftMousePressed() && scrollBar.trackRect.Contains(mousePosition);
            if (trackPressed) {
                scrollBar.value += mousePosition.y < scrollBar.thumbRect.y ? -visibleSize : visibleSize;
                scrollBar.value = std::clamp(scrollBar.value, 0, maximumValue);
                RefreshThumb(scrollBar, contentSize, visibleSize);
            }
        }

        if (!allowInput || !scrollBar.dragging || !Hell::Input::LeftMouseDown()) return;

        // Drag the thumb
        const int32_t thumbTravel = scrollBar.trackRect.height - scrollBar.thumbRect.height;
        if (thumbTravel <= 0) return;

        const int32_t thumbOffset = std::clamp(mousePosition.y - scrollBar.dragOffset - scrollBar.trackRect.y, 0, thumbTravel);
        scrollBar.value = static_cast<int32_t>(std::round(static_cast<float>(thumbOffset) / static_cast<float>(thumbTravel) * static_cast<float>(maximumValue)));
        RefreshThumb(scrollBar, contentSize, visibleSize);
    }

    void Render(const EditorScrollBar& scrollBar) {
        if (!scrollBar.visible) return;

        const EditorColors& colors = GetStyle().colors;
        UI::DrawSolidRect(scrollBar.trackRect, colors.scrollTrack);
        UI::DrawSolidRect(scrollBar.thumbRect, scrollBar.hovered || scrollBar.dragging ? colors.scrollThumbHover : colors.scrollThumb);
    }

    bool WantsMouseCapture(const EditorScrollBar& scrollBar) {
        return scrollBar.visible && (scrollBar.hovered || scrollBar.dragging);
    }
}
