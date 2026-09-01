#pragma once

#include <cstdint>
#include <glm/vec4.hpp>

namespace Unloved::EditorSession {

    struct EditorColors {
        glm::vec4 text = glm::vec4(0.545098f, 0.541176f, 0.568627f, 1.0f);              // #8b8a91
        glm::vec4 controlBackground = glm::vec4(0.101961f, 0.090196f, 0.121569f, 1.0f); // #1a171f
        glm::vec4 panelBackground = glm::vec4(0.058824f, 0.050980f, 0.070588f, 1.0f);   // #0f0d12
        glm::vec4 hover = glm::vec4(0.141176f, 0.125490f, 0.168627f, 1.0f);             // #24202b
        glm::vec4 selected = glm::vec4 (0.231373f, 0.196078f, 0.286275f, 1.0f);
        glm::vec4 border = glm::vec4(0.42f, 0.40f, 0.46f, 1.0f);
        glm::vec4 separator = glm::vec4(0.24f, 0.23f, 0.27f, 1.0f);
        glm::vec4 overlay = glm::vec4(0.0f, 0.0f, 0.0f, 0.55f);
        glm::vec4 galleryHover = glm::vec4(0.2f, 0.176471f, 0.239216f, 0.35f);
        glm::vec4 sliderHover = glm::vec4(0.305882f, 0.258824f, 0.380392f, 1.0f);
        glm::vec4 scrollTrack = glm::vec4(0.082353f, 0.074510f, 0.094118f, 1.0f);
        glm::vec4 scrollThumb = glm::vec4(0.258824f, 0.243137f, 0.286275f, 1.0f);
        glm::vec4 scrollThumbHover = glm::vec4(0.352941f, 0.333333f, 0.388235f, 1.0f);
    };

    struct EditorFontStyle {
        const char* name = "RobotoMono";
        float scale = 1.0f;
        const char* textColorTag = "[COL=0.545098,0.541176,0.568627,1.0]";
        const char* viewportTextColorTag = "[COL=1.0,1.0,1.0,1.0]";
    };

    struct EditorMenuStyle {
        int32_t barLeftPadding = 10;
        int32_t buttonHorizontalPadding = 10;
        int32_t popupMinimumWidth = 220;
        int32_t popupVerticalPadding = 4;
        int32_t itemHeight = 24;
        int32_t separatorHeight = 9;
        int32_t itemHorizontalPadding = 12;
        int32_t shortcutGap = 32;
        int32_t submenuArrowSize = 8;
    };

    struct EditorModalStyle {
        int32_t windowPadding = 16;
        int32_t titleBarHeight = 26;
        int32_t buttonWidth = 110;
        int32_t buttonHeight = 30;
    };

    struct EditorDialogStyle {
        int32_t windowWidth = 360;
        int32_t windowHeight = 130;
    };

    struct EditorFileDialogStyle {
        int32_t windowWidth = 460;
        int32_t windowHeight = 400;
        int32_t newWindowWidth = 400;
        int32_t newWindowHeight = 170;
        int32_t rowHeight = 26;
        int32_t buttonGap = 8;
        int32_t inputHeight = 46;
        int32_t scrollBarWidth = 8;
    };

    struct EditorHierarchyStyle {
        int32_t rowHeight = 24;
        int32_t scrollBarWidth = 10;
        int32_t scrollBarRightPadding = 7;
        int32_t leftPadding = 8;
        int32_t indentWidth = 14;
        int32_t arrowSize = 8;
        int32_t textGap = 6;
    };

    struct EditorInputStyle {
        int32_t rowHeight = 26;
        int32_t contentPadding = 10;
        int32_t labelFieldGap = 16;
        int32_t fieldPadding = 6;
        int32_t fieldGap = 4;
        int32_t buttonHeight = 30;
        int32_t buttonElementGap = 6;
        int32_t checkBoxSize = 16;
        int32_t dropDownArrowSize = 8;
        int32_t dropDownScrollBarWidth = 8;
        int32_t sliderGrabberWidth = 3;
    };

    struct EditorLayoutStyle {
        int32_t defaultFileMenuHeight = 26;
        int32_t defaultPanelWidth = 320;
        int32_t panelHeadingHeight = 26;
        int32_t panelGap = 10;
        int32_t minimumViewportsWidth = 200;
        int32_t minimumSidePanelWidth = 160;
        int32_t minimumViewportRegionSize = 96;
        int32_t dividerHitRadius = 6;
        int32_t headingTextPadding = 10;
    };

    struct EditorGalleryStyle {
        int32_t columnCount = 2;
        int32_t contentPadding = 10;
        int32_t itemGap = 8;
        int32_t labelHeight = 24;
        int32_t scrollBarWidth = 12;
        int32_t selectionBorderThickness = 2;
        int32_t minimumItemHeight = 52;
        int32_t brushLabelPadding = 4;
    };

    struct EditorScrollBarStyle {
        int32_t minimumThumbSize = 24;
    };

    struct EditorToolbarStyle {
        int32_t buttonSize = 40;
        int32_t buttonGap = 8;
        int32_t viewportPadding = 8;
        int32_t labelHeight = 24;
    };

    struct EditorViewportStyle {
        int32_t labelPadding = 8;
    };

    struct EditorStyle {
        EditorColors colors;
        EditorFontStyle font;
        EditorMenuStyle menu;
        EditorModalStyle modal;
        EditorDialogStyle dialog;
        EditorFileDialogStyle fileDialog;
        EditorHierarchyStyle hierarchy;
        EditorInputStyle input;
        EditorLayoutStyle layout;
        EditorGalleryStyle gallery;
        EditorScrollBarStyle scrollBar;
        EditorToolbarStyle toolbar;
        EditorViewportStyle viewport;
    };

    inline EditorStyle& GetStyle() {
        static EditorStyle style;
        return style;
    }
}
