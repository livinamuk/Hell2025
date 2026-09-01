#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <functional>

namespace Unloved::EditorSession {

    namespace Coordinates {

        glm::ivec2 WindowToUI(const glm::ivec2& windowPosition);
        glm::ivec2 GetMousePositionUI();
    }

    struct EditorButton {
        EditorRect rect;
        const char* iconTextureName = nullptr;
        std::function<void()> onPressed;
        bool visible = false;
        bool hovered = false;
        bool selected = false;
    };

    struct EditorScrollBar {
        EditorRect trackRect;
        EditorRect thumbRect;
        int32_t value = 0;
        int32_t dragOffset = 0;
        bool hovered = false;
        bool dragging = false;
        bool visible = false;
    };

    namespace UI {

        void DrawSolidRect(const EditorRect& rect, const glm::vec4& color);
        void DrawBorder(const EditorRect& rect, const glm::vec4& color, int32_t thickness = 1);
        void DrawLine(const glm::vec2& begin, const glm::vec2& end, const glm::vec4& color, float thickness = 1.0f);

        void DrawPanelBackground(const EditorPanel& panel);
        void DrawPanelEdges(const EditorPanel& panel);
        void DrawPanel(const EditorPanel& panel);
    }

    namespace Buttons {

        void Update(EditorButton& button, const glm::ivec2& mousePosition, bool allowInput);
        void Render(const EditorButton& button);
    }

    namespace ScrollBar {

        void Update(EditorScrollBar& scrollBar, const EditorRect& rect, int32_t contentSize, int32_t visibleSize, bool allowInput);
        void Render(const EditorScrollBar& scrollBar);
        bool WantsMouseCapture(const EditorScrollBar& scrollBar);
    }
}
