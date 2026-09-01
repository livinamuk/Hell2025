#pragma once

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Unloved::EditorSession {

    enum class EditorSessionMode : uint8_t {
        HOUSE,
        MAP,
        RAGDOLL,
        BONE_MASK
    };

    enum class EditorRenderMode : uint8_t {
        PBR,
        SOLID_COLOR,
        NORMALS
    };

    enum class EditorSelectionMode : uint8_t {
        OBJECT,
        VERTEX
    };

    enum class EditorObjectMode : uint8_t {
        OBJECT,
        VERTEX,
        VERTEX_AND_OBJECT
    };

    struct EditorRect {
        int32_t x = 0;
        int32_t y = 0;
        int32_t width = 0;
        int32_t height = 0;

        int32_t Right() const  { return x + width; }
        int32_t Bottom() const { return y + height; }
        bool HasArea() const   { return width > 0 && height > 0; }
        bool Contains(const glm::ivec2& point) const {
            return point.x >= x && point.x < Right() && point.y >= y && point.y < Bottom();
        }
    };

    enum class EditorPanelEdge : uint8_t {
        NONE   = 0,
        TOP    = 1 << 0,
        BOTTOM = 1 << 1,
        LEFT   = 1 << 2,
        RIGHT  = 1 << 3,
        ALL    = 0x0F
    };

    constexpr EditorPanelEdge operator|(EditorPanelEdge lhs, EditorPanelEdge rhs) {
        return static_cast<EditorPanelEdge>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    constexpr bool HasPanelEdge(EditorPanelEdge edges, EditorPanelEdge edge) {
        return (static_cast<uint8_t>(edges) & static_cast<uint8_t>(edge)) != 0;
    }

    enum class EditorPanelId : uint8_t {
        FILE_MENU,
        HIERARCHY,
        VIEWPORTS,
        PROPERTIES,
        TOOLS,
        MATERIALS,
        BRUSHES
    };

    enum class EditorViewportLayout : uint8_t {
        SINGLE,
        LEFT_RIGHT,
        TOP_BOTTOM,
        FOUR
    };

    enum class EditorViewportMode {
        PERSPECTIVE,
        ORTHOGRAPHIC,
        TOP,
        BOTTOM,
        FRONT,
        BACK,
        LEFT,
        RIGHT
    };

    constexpr bool UsesOrthographicProjection(EditorViewportMode mode) {
        return mode != EditorViewportMode::PERSPECTIVE;
    }

    struct EditorPanel {
        EditorRect rect;
        EditorPanelEdge edges = EditorPanelEdge::ALL;
        glm::vec4 backgroundColor = glm::vec4(0.0f);
        glm::vec4 borderColor = glm::vec4(1.0f);
        int32_t borderThickness = 1;
        bool drawBackground = true;
        bool visible = true;
    };

    struct EditorViewportRegion {
        EditorRect rect;
        glm::vec2 normalizedPosition = glm::vec2(0.0f);
        glm::vec2 normalizedSize = glm::vec2(0.0f);
        bool visible = false;
    };
}
