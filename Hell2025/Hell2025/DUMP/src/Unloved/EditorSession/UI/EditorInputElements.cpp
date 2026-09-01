#include "EditorInputElements.h"

#include "EditorStyle.h"
#include "EditorUI.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Input.h"
#include "Hell/Time/Time.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"
#include "Unloved/Common/Constants.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace Unloved::EditorSession::InputElements {
    void Begin(const EditorRect& rect, int32_t labelColumnWidth);
    bool Button(const std::string& text);
    void ReadOnly(const std::string& label, const std::string& value);
    bool String(uint64_t objectId, const std::string& label, std::string& value, bool commitOnFocusLoss = false);
    bool CheckBox(const std::string& label, bool& value);
    bool AxisLimit(uint64_t objectId, const std::string& label, AxisLimitValue& value);
    bool DropDown(uint64_t objectId, const std::string& label, const std::vector<std::string>& options, std::string& value);
    bool Float(uint64_t objectId, const std::string& label, float& value);
    bool FloatSlider(uint64_t objectId, const std::string& label, float& value, float minimum, float maximum);
    bool IntSlider(uint64_t objectId, const std::string& label, int32_t& value, int32_t minimum, int32_t maximum);
    bool UInt(uint64_t objectId, const std::string& label, uint32_t& value);
    bool Vec2(uint64_t objectId, const std::string& label, glm::vec2& value);
    bool Vec3(uint64_t objectId, const std::string& label, glm::vec3& value);

    namespace {
        constexpr int32_t DROP_DOWN_MAX_VISIBLE_OPTIONS = 8;
        constexpr int32_t DROP_DOWN_ROWS_PER_SCROLL = 3;
        constexpr float CARET_FLASH_TIME = 0.5f;

        struct KeyCharacter {
            uint32_t keyCode = 0;
            char character = 0;
            char shiftedCharacter = 0;
        };

        enum struct TextEditAction {
            NONE,
            COMMIT,
            FOCUS_LOST,
            CANCEL
        };

        EditorRect g_rect;
        uint64_t g_activeObjectId = 0;
        std::string g_activeLabel;
        std::string g_editValue;
        int32_t g_activeComponentIndex = -1;
        int32_t g_labelColumnWidth = 0;
        int32_t g_rowIndex = 0;
        int32_t g_rowExtraHeight = 0;
        int32_t g_lastRenderedHeight = 0;
        size_t g_caretIndex = 0;
        size_t g_selectionAnchor = 0;
        float g_caretFlashTimer = 0.0f;
        bool g_activeInputWasDrawn = false;
        bool g_dragSelecting = false;
        bool g_ignoreActiveMousePress = false;
        uint64_t g_openDropDownObjectId = 0;
        std::string g_openDropDownLabel;
        std::string g_openDropDownValue;
        std::vector<std::string> g_openDropDownOptions;
        EditorRect g_openDropDownPopupRect;
        EditorScrollBar g_dropDownScrollBar;
        int32_t g_dropDownVisibleOptionCount = 0;
        int32_t g_hoveredDropDownOption = -1;
        bool g_openDropDownWasDrawn = false;
        bool g_dropDownConsumedMousePress = false;
        uint64_t g_activeSliderObjectId = 0;
        std::string g_activeSliderLabel;
        uint64_t g_pendingRangeSliderObjectId = 0;
        std::string g_pendingRangeSliderLabel;
        int32_t g_pendingRangeSliderMouseX = 0;
        bool g_activeSliderWasDrawn = false;

        bool ShiftIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_SHIFT_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_SHIFT);
        }

        bool ControlIsDown() {
            return Hell::Input::KeyDown(HELL_KEY_LEFT_CONTROL_GLFW) || Hell::Input::KeyDown(HELL_KEY_RIGHT_CONTROL);
        }

        char GetPressedCharacter() {
            const bool shiftDown = ShiftIsDown();

            for (uint32_t keyCode = HELL_KEY_A; keyCode <= HELL_KEY_Z; keyCode++) {
                if (Hell::Input::KeyPressed(keyCode)) {
                    return static_cast<char>((shiftDown ? 'A' : 'a') + keyCode - HELL_KEY_A);
                }
            }

            constexpr char SHIFTED_NUMBER_CHARACTERS[] = { ')', '!', '@', '#', '$', '%', '^', '&', '*', '(' };
            for (uint32_t keyCode = HELL_KEY_0; keyCode <= HELL_KEY_9; keyCode++) {
                if (Hell::Input::KeyPressed(keyCode)) {
                    return shiftDown ? SHIFTED_NUMBER_CHARACTERS[keyCode - HELL_KEY_0] : static_cast<char>(keyCode);
                }
            }

            constexpr KeyCharacter CHARACTERS[] = {
                { HELL_KEY_SPACE,         ' ', ' ' },
                { HELL_KEY_APOSTROPHE,    '\'', '"' },
                { HELL_KEY_COMMA,         ',', '<' },
                { HELL_KEY_MINUS,         '-', '_' },
                { HELL_KEY_PERIOD,        '.', '>' },
                { HELL_KEY_SLASH,         '/', '?' },
                { HELL_KEY_SEMICOLON,     ';', ':' },
                { HELL_KEY_EQUAL,         '=', '+' },
                { HELL_KEY_LEFT_BRACKET,  '[', '{' },
                { HELL_KEY_BACKSLASH,     '\\', '|' },
                { HELL_KEY_RIGHT_BRACKET, ']', '}' },
                { HELL_KEY_GRAVE_ACCENT,  '`', '~' },
            };

            for (const KeyCharacter& keyCharacter : CHARACTERS) {
                if (Hell::Input::KeyPressed(keyCharacter.keyCode)) {
                    return shiftDown ? keyCharacter.shiftedCharacter : keyCharacter.character;
                }
            }

            return 0;
        }

        bool IsActive(uint64_t objectId, const std::string& label, int32_t componentIndex) {
            return g_activeObjectId == objectId && g_activeLabel == label && g_activeComponentIndex == componentIndex;
        }

        size_t GetSelectionBegin() {
            return std::min(g_caretIndex, g_selectionAnchor);
        }

        size_t GetSelectionEnd() {
            return std::max(g_caretIndex, g_selectionAnchor);
        }

        bool HasSelection() {
            return g_caretIndex != g_selectionAnchor;
        }

        int32_t GetCharacterWidth() {
            const EditorFontStyle& font = GetStyle().font;
            return TextBlitter::GetTextSize(" ", font.name, font.scale).x;
        }

        int32_t GetCharacterHeight() {
            const EditorFontStyle& fontStyle = GetStyle().font;
            const FontSpriteSheet* fontSheet = TextBlitter::GetFontSpriteSheet(fontStyle.name);
            return fontSheet ? static_cast<int32_t>(std::round(static_cast<float>(fontSheet->m_charHeight) * fontStyle.scale)) : 0;
        }

        size_t GetCharacterIndexAtMouse(const EditorRect& fieldRect, const std::string& value) {
            const int32_t characterWidth = GetCharacterWidth();
            const int32_t localMouseX = Coordinates::GetMousePositionUI().x - fieldRect.x - GetStyle().input.fieldPadding;
            if (localMouseX <= 0 || characterWidth <= 0) return 0;

            const size_t index = static_cast<size_t>((localMouseX + characterWidth / 2) / characterWidth);
            return std::min(index, value.size());
        }

        bool DeleteSelection(std::string& value) {
            if (!HasSelection()) return false;

            const size_t selectionBegin = GetSelectionBegin();
            value.erase(selectionBegin, GetSelectionEnd() - selectionBegin);
            g_caretIndex = selectionBegin;
            g_selectionAnchor = selectionBegin;
            g_caretFlashTimer = 0.0f;
            return true;
        }

        void StopEditing() {
            g_activeObjectId = 0;
            g_activeLabel.clear();
            g_activeComponentIndex = -1;
            g_caretIndex = 0;
            g_selectionAnchor = 0;
            g_caretFlashTimer = 0.0f;
            g_dragSelecting = false;
            g_ignoreActiveMousePress = false;
        }

        void CloseDropDown() {
            g_openDropDownObjectId = 0;
            g_openDropDownLabel.clear();
            g_openDropDownValue.clear();
            g_openDropDownOptions.clear();
            g_dropDownScrollBar = {};
            g_dropDownVisibleOptionCount = 0;
            g_hoveredDropDownOption = -1;
        }

        bool IsDropDownOpen(uint64_t objectId, const std::string& label) {
            return g_openDropDownObjectId == objectId && g_openDropDownLabel == label;
        }

        bool IsSliderActive(uint64_t objectId, const std::string& label) {
            return g_activeSliderObjectId == objectId && g_activeSliderLabel == label;
        }

        bool IsRangeSliderPending(uint64_t objectId, const std::string& label) {
            return g_pendingRangeSliderObjectId == objectId && g_pendingRangeSliderLabel == label;
        }

        void StopPendingRangeSlider() {
            g_pendingRangeSliderObjectId = 0;
            g_pendingRangeSliderLabel.clear();
            g_pendingRangeSliderMouseX = 0;
        }

        void StopSliderDrag() {
            g_activeSliderObjectId = 0;
            g_activeSliderLabel.clear();
            StopPendingRangeSlider();
        }

        TextEditAction UpdateString(std::string& value, bool numeric, bool unsignedInteger) {
            // Cancel or commit the edit
            if (Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
                StopEditing();
                return TextEditAction::CANCEL;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_ENTER)) return TextEditAction::COMMIT;

            // Select all text
            if (ControlIsDown() && Hell::Input::KeyPressed(HELL_KEY_A)) {
                g_selectionAnchor = 0;
                g_caretIndex = value.size();
                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            if (ControlIsDown()) return TextEditAction::NONE;

            // Move the caret
            if (Hell::Input::KeyPressed(HELL_KEY_LEFT)) {
                if (!ShiftIsDown() && HasSelection()) {
                    g_caretIndex = GetSelectionBegin();
                }
                else if (g_caretIndex > 0) {
                    g_caretIndex--;
                }

                if (!ShiftIsDown()) {
                    g_selectionAnchor = g_caretIndex;
                }

                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_RIGHT)) {
                if (!ShiftIsDown() && HasSelection()) {
                    g_caretIndex = GetSelectionEnd();
                }
                else if (g_caretIndex < value.size()) {
                    g_caretIndex++;
                }

                if (!ShiftIsDown()) {
                    g_selectionAnchor = g_caretIndex;
                }

                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_HOME)) {
                g_caretIndex = 0;
                if (!ShiftIsDown()) {
                    g_selectionAnchor = g_caretIndex;
                }

                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_END)) {
                g_caretIndex = value.size();
                if (!ShiftIsDown()) {
                    g_selectionAnchor = g_caretIndex;
                }

                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            // Delete text
            if (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE)) {
                if (DeleteSelection(value)) return TextEditAction::NONE;
                if (g_caretIndex == 0) return TextEditAction::NONE;

                value.erase(g_caretIndex - 1, 1);
                g_caretIndex--;
                g_selectionAnchor = g_caretIndex;
                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            if (Hell::Input::KeyPressed(HELL_KEY_DELETE)) {
                if (DeleteSelection(value)) return TextEditAction::NONE;
                if (g_caretIndex >= value.size()) return TextEditAction::NONE;

                value.erase(g_caretIndex, 1);
                g_caretFlashTimer = 0.0f;
                return TextEditAction::NONE;
            }

            // Insert a character
            const char character = GetPressedCharacter();
            if (character == 0) return TextEditAction::NONE;
            if (unsignedInteger && (character < '0' || character > '9')) return TextEditAction::NONE;
            if (numeric && !unsignedInteger && character != '-' && character != '+' && character != '.' && character != 'e' && character != 'E' && (character < '0' || character > '9')) return TextEditAction::NONE;

            DeleteSelection(value);
            value.insert(value.begin() + g_caretIndex, character);
            g_caretIndex++;
            g_selectionAnchor = g_caretIndex;
            g_caretFlashTimer = 0.0f;
            return TextEditAction::NONE;
        }

        std::string FloatToString(float value) {
            char buffer[64];
            std::snprintf(buffer, sizeof(buffer), "%.3f", value);
            std::string text = buffer;

            while (!text.empty() && text.back() == '0') {
                text.pop_back();
            }

            if (!text.empty() && text.back() == '.') {
                text.pop_back();
            }

            if (text == "-0") {
                text = "0";
            }

            return text;
        }

        bool StringToFloat(const std::string& text, float& value) {
            if (text.empty()) return false;

            char* end = nullptr;
            const float parsedValue = std::strtof(text.c_str(), &end);
            if (end == text.c_str() || *end != '\0') return false;

            value = parsedValue;
            return true;
        }

        int32_t GetRowY() {
            const EditorInputStyle& style = GetStyle().input;
            return g_rect.y + style.contentPadding + g_rowIndex * style.rowHeight + g_rowExtraHeight;
        }

        void DrawLabel(const std::string& label, int32_t rowY) {
            const EditorStyle& style = GetStyle();
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + label, style.font.name, glm::ivec2(g_rect.x + style.input.contentPadding, rowY + style.input.rowHeight / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST);
        }

        EditorRect GetFieldRect(int32_t rowY, int32_t componentIndex, int32_t componentCount) {
            const EditorInputStyle& style = GetStyle().input;
            const int32_t fieldX = g_rect.x + style.contentPadding + g_labelColumnWidth + style.labelFieldGap;
            const int32_t totalWidth = std::max(0, g_rect.Right() - style.contentPadding - fieldX);
            const int32_t componentWidth = std::max(0, totalWidth - style.fieldGap * (componentCount - 1)) / componentCount;
            const int32_t componentX = fieldX + componentIndex * (componentWidth + style.fieldGap);
            const int32_t componentRight = componentIndex == componentCount - 1 ? g_rect.Right() - style.contentPadding : componentX + componentWidth;
            return { componentX, rowY + 2, std::max(0, componentRight - componentX), style.rowHeight - 4 };
        }

        EditorRect GetDropDownOptionRect(int32_t visibleIndex) {
            const EditorInputStyle& style = GetStyle().input;
            const int32_t scrollBarWidth = g_dropDownScrollBar.visible ? style.dropDownScrollBarWidth : 0;
            return { g_openDropDownPopupRect.x, g_openDropDownPopupRect.y + visibleIndex * style.rowHeight, g_openDropDownPopupRect.width - scrollBarWidth, style.rowHeight };
        }

        bool UpdateSlider(uint64_t objectId, const std::string& label, const EditorRect& fieldRect, float& interpolation) {
            const bool hovered = fieldRect.Contains(Coordinates::GetMousePositionUI());
            bool active = IsSliderActive(objectId, label);

            // Start or stop the drag
            if (Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress) {
                if (hovered) {
                    StopEditing();
                    CloseDropDown();
                    g_activeSliderObjectId = objectId;
                    g_activeSliderLabel = label;
                    active = true;
                }
                else if (active) {
                    StopSliderDrag();
                    active = false;
                }
            }

            // Track the drag lifetime
            if (active) {
                g_activeSliderWasDrawn = true;
            }

            if (active && !Hell::Input::LeftMouseDown()) {
                StopSliderDrag();
                active = false;
            }

            // Update the value
            bool changed = false;
            if (active && fieldRect.width > 0) {
                const float newInterpolation = std::clamp(static_cast<float>(Coordinates::GetMousePositionUI().x - fieldRect.x) / static_cast<float>(std::max(1, fieldRect.width - 1)), 0.0f, 1.0f);
                changed = newInterpolation != interpolation;
                interpolation = newInterpolation;
            }

            // Hover cursor
            if (hovered || active) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }

            return changed;
        }

        void DrawSlider(const EditorRect& fieldRect, float interpolation, const std::string& valueText, bool hovered) {
            const EditorStyle& style = GetStyle();
            interpolation = std::clamp(interpolation, 0.0f, 1.0f);

            // Slider background
            UI::DrawSolidRect(fieldRect, style.colors.controlBackground);

            // Filled range
            const int32_t fillWidth = static_cast<int32_t>(std::round(interpolation * fieldRect.width));
            if (fillWidth > 0) {
                UI::DrawSolidRect({ fieldRect.x, fieldRect.y, fillWidth, fieldRect.height }, hovered ? style.colors.sliderHover : style.colors.selected);
            }

            // Grabber and value
            const int32_t grabberCenter = fieldRect.x + static_cast<int32_t>(std::round(interpolation * std::max(0, fieldRect.width - 1)));
            const int32_t grabberX = std::clamp(grabberCenter - style.input.sliderGrabberWidth / 2, fieldRect.x, std::max(fieldRect.x, fieldRect.Right() - style.input.sliderGrabberWidth));
            UI::DrawSolidRect({ grabberX, fieldRect.y, std::min(style.input.sliderGrabberWidth, fieldRect.width), fieldRect.height }, style.colors.text);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + valueText, style.font.name, glm::ivec2(fieldRect.x + fieldRect.width / 2, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, fieldRect.Right(), fieldRect.Bottom());
        }

        int32_t UpdateRangeSlider(uint64_t objectId, const std::string& label, const EditorRect& fieldRect, float minimumInterpolation, float maximumInterpolation, float& draggedInterpolation) {
            const std::string minimumLabel = label + " Minimum";
            const std::string maximumLabel = label + " Maximum";
            const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
            const bool hovered = fieldRect.Contains(mousePosition);
            bool minimumActive = IsSliderActive(objectId, minimumLabel);
            bool maximumActive = IsSliderActive(objectId, maximumLabel);
            bool pending = IsRangeSliderPending(objectId, label);

            // Select the nearest handle
            if (Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress) {
                if (hovered) {
                    const int32_t minimumX = fieldRect.x + static_cast<int32_t>(std::round(minimumInterpolation * std::max(0, fieldRect.width - 1)));
                    const int32_t maximumX = fieldRect.x + static_cast<int32_t>(std::round(maximumInterpolation * std::max(0, fieldRect.width - 1)));
                    const int32_t minimumDistance = std::abs(mousePosition.x - minimumX);
                    const int32_t maximumDistance = std::abs(mousePosition.x - maximumX);
                    StopEditing();
                    StopSliderDrag();
                    CloseDropDown();
                    pending = false;

                    if (minimumDistance == maximumDistance) {
                        g_pendingRangeSliderObjectId = objectId;
                        g_pendingRangeSliderLabel = label;
                        g_pendingRangeSliderMouseX = mousePosition.x;
                        pending = true;
                    }
                    else {
                        g_activeSliderObjectId = objectId;
                        g_activeSliderLabel = minimumDistance < maximumDistance ? minimumLabel : maximumLabel;
                    }

                    minimumActive = IsSliderActive(objectId, minimumLabel);
                    maximumActive = IsSliderActive(objectId, maximumLabel);
                }
                else if (minimumActive || maximumActive || pending) {
                    StopSliderDrag();
                    minimumActive = false;
                    maximumActive = false;
                    pending = false;
                }
            }

            // Pick an overlapping handle from the drag direction
            if (pending && Hell::Input::LeftMouseDown() && mousePosition.x != g_pendingRangeSliderMouseX) {
                g_activeSliderObjectId = objectId;
                g_activeSliderLabel = mousePosition.x < g_pendingRangeSliderMouseX ? minimumLabel : maximumLabel;
                StopPendingRangeSlider();
                minimumActive = IsSliderActive(objectId, minimumLabel);
                maximumActive = IsSliderActive(objectId, maximumLabel);
                pending = false;
            }

            const bool active = minimumActive || maximumActive;
            if (active || pending) {
                g_activeSliderWasDrawn = true;
            }
            if ((active || pending) && !Hell::Input::LeftMouseDown()) {
                StopSliderDrag();
                return -1;
            }

            if (hovered || active || pending) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
            }
            if (!active || fieldRect.width <= 0) return -1;

            draggedInterpolation = std::clamp(static_cast<float>(mousePosition.x - fieldRect.x) / static_cast<float>(std::max(1, fieldRect.width - 1)), 0.0f, 1.0f);
            const float currentInterpolation = minimumActive ? minimumInterpolation : maximumInterpolation;
            if (draggedInterpolation == currentInterpolation) return -1;
            return minimumActive ? 0 : 1;
        }

        void DrawRangeSlider(const EditorRect& fieldRect, float minimumInterpolation, float maximumInterpolation, const std::string& valueText, bool hovered) {
            const EditorStyle& style = GetStyle();
            minimumInterpolation = std::clamp(minimumInterpolation, 0.0f, 1.0f);
            maximumInterpolation = std::clamp(maximumInterpolation, minimumInterpolation, 1.0f);

            // Slider background
            UI::DrawSolidRect(fieldRect, style.colors.controlBackground);

            // Allowed range
            const int32_t minimumCenter = fieldRect.x + static_cast<int32_t>(std::round(minimumInterpolation * std::max(0, fieldRect.width - 1)));
            const int32_t maximumCenter = fieldRect.x + static_cast<int32_t>(std::round(maximumInterpolation * std::max(0, fieldRect.width - 1)));
            if (maximumCenter > minimumCenter) {
                UI::DrawSolidRect({ minimumCenter, fieldRect.y, maximumCenter - minimumCenter, fieldRect.height }, hovered ? style.colors.sliderHover : style.colors.selected);
            }

            // Limit handles
            const int32_t minimumX = std::clamp(minimumCenter - style.input.sliderGrabberWidth / 2, fieldRect.x, std::max(fieldRect.x, fieldRect.Right() - style.input.sliderGrabberWidth));
            const int32_t maximumX = std::clamp(maximumCenter - style.input.sliderGrabberWidth / 2, fieldRect.x, std::max(fieldRect.x, fieldRect.Right() - style.input.sliderGrabberWidth));
            const int32_t grabberWidth = std::min(style.input.sliderGrabberWidth, fieldRect.width);
            UI::DrawSolidRect({ minimumX, fieldRect.y, grabberWidth, fieldRect.height }, style.colors.text);
            UI::DrawSolidRect({ maximumX, fieldRect.y, grabberWidth, fieldRect.height }, style.colors.text);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + valueText, style.font.name, glm::ivec2(fieldRect.x + fieldRect.width / 2, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, fieldRect.Right(), fieldRect.Bottom());
        }

        void RenderOpenDropDown() {
            if (g_openDropDownObjectId == 0 || g_dropDownVisibleOptionCount <= 0) return;

            const EditorStyle& style = GetStyle();

            // Popup background
            UI::DrawSolidRect(g_openDropDownPopupRect, style.colors.controlBackground);

            // Option rows
            for (int32_t i = 0; i < g_dropDownVisibleOptionCount; i++) {
                const int32_t optionIndex = g_dropDownScrollBar.value + i;
                if (optionIndex < 0 || optionIndex >= static_cast<int32_t>(g_openDropDownOptions.size())) {
                    break;
                }

                const EditorRect optionRect = GetDropDownOptionRect(i);
                const std::string& option = g_openDropDownOptions[optionIndex];
                if (option == g_openDropDownValue) {
                    UI::DrawSolidRect(optionRect, style.colors.selected);
                }
                else if (optionIndex == g_hoveredDropDownOption) {
                    UI::DrawSolidRect(optionRect, style.colors.hover);
                }

                UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + option, style.font.name, glm::ivec2(optionRect.x + style.input.fieldPadding, optionRect.y + optionRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, optionRect.x, optionRect.y, optionRect.Right(), optionRect.Bottom());
            }

            // Scroll bar
            ScrollBar::Render(g_dropDownScrollBar);
        }

        TextEditAction DrawTextField(uint64_t objectId, const std::string& label, int32_t componentIndex, const EditorRect& fieldRect, const std::string& value, bool numeric, bool unsignedInteger = false) {
            const EditorStyle& style = GetStyle();
            const bool hovered = fieldRect.Contains(Coordinates::GetMousePositionUI());
            bool active = IsActive(objectId, label, componentIndex);
            const bool ignoreMousePress = active && g_ignoreActiveMousePress;
            if (ignoreMousePress) {
                g_ignoreActiveMousePress = false;
            }

            TextEditAction action = TextEditAction::NONE;

            // Handle focus and caret placement
            if (Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress && !ignoreMousePress) {
                if (hovered) {
                    const bool startingEdit = !active;
                    if (!active) {
                        g_activeObjectId = objectId;
                        g_activeLabel = label;
                        g_activeComponentIndex = componentIndex;
                        g_editValue = value;
                        active = true;
                    }

                    g_caretIndex = GetCharacterIndexAtMouse(fieldRect, g_editValue);
                    if (startingEdit || !ShiftIsDown()) {
                        g_selectionAnchor = g_caretIndex;
                    }

                    g_caretFlashTimer = 0.0f;
                    g_dragSelecting = true;
                }
                else if (active) {
                    StopEditing();
                    active = false;
                    action = TextEditAction::FOCUS_LOST;
                }
            }

            if (active && g_dragSelecting && Hell::Input::LeftMouseDown()) {
                g_caretIndex = GetCharacterIndexAtMouse(fieldRect, g_editValue);
                g_caretFlashTimer = 0.0f;
            }

            if (!Hell::Input::LeftMouseDown()) {
                g_dragSelecting = false;
            }

            if (active) {
                g_caretIndex = std::min(g_caretIndex, g_editValue.size());
                g_selectionAnchor = std::min(g_selectionAnchor, g_editValue.size());
                g_activeInputWasDrawn = true;
                action = UpdateString(g_editValue, numeric, unsignedInteger);
                active = IsActive(objectId, label, componentIndex);
                if (active) {
                    g_caretFlashTimer += Hell::Time::RawDeltaTime();
                }
            }

            if (hovered) {
                Hell::BackEnd::SetCursor(HELL_CURSOR_IBEAM);
            }

            // Field background
            UI::DrawSolidRect(fieldRect, hovered || active ? style.colors.hover : style.colors.controlBackground);

            // Selection highlight
            const std::string& displayedValue = active ? g_editValue : value;
            const int32_t characterWidth = GetCharacterWidth();
            const int32_t textX = fieldRect.x + style.input.fieldPadding;
            if (active && HasSelection()) {
                const int32_t selectionX = textX + static_cast<int32_t>(GetSelectionBegin()) * characterWidth;
                const int32_t selectionWidth = static_cast<int32_t>(GetSelectionEnd() - GetSelectionBegin()) * characterWidth;
                const int32_t clippedSelectionX = std::max(selectionX, fieldRect.x);
                const int32_t clippedSelectionRight = std::min(selectionX + selectionWidth, fieldRect.Right());
                if (clippedSelectionRight > clippedSelectionX) {
                    UI::DrawSolidRect({ clippedSelectionX, fieldRect.y + 1, clippedSelectionRight - clippedSelectionX, fieldRect.height - 2 }, style.colors.selected);
                }
            }

            // Text and caret
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + displayedValue, style.font.name, glm::ivec2(textX, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, fieldRect.Right(), fieldRect.Bottom());
            if (active && std::fmod(g_caretFlashTimer, CARET_FLASH_TIME * 2.0f) < CARET_FLASH_TIME) {
                const int32_t caretX = textX + static_cast<int32_t>(g_caretIndex) * characterWidth;
                const int32_t caretHeight = GetCharacterHeight();
                if (caretX >= fieldRect.x && caretX < fieldRect.Right()) {
                    UI::DrawSolidRect({ caretX, fieldRect.y + (fieldRect.height - caretHeight) / 2, 1, caretHeight }, style.colors.text);
                }
            }

            return action;
        }

        bool DrawFloatField(uint64_t objectId, const std::string& label, int32_t componentIndex, const EditorRect& fieldRect, float& value) {
            std::string text = FloatToString(value);
            if (DrawTextField(objectId, label, componentIndex, fieldRect, text, true) != TextEditAction::COMMIT) return false;

            float parsedValue = 0.0f;
            if (!StringToFloat(g_editValue, parsedValue)) return false;

            const bool changed = parsedValue != value;
            value = parsedValue;
            StopEditing();
            return changed;
        }

        bool DrawUIntField(uint64_t objectId, const std::string& label, const EditorRect& fieldRect, uint32_t& value) {
            std::string text = std::to_string(value);
            if (DrawTextField(objectId, label, 0, fieldRect, text, true, true) != TextEditAction::COMMIT || g_editValue.empty()) return false;

            char* end = nullptr;
            const unsigned long long parsedValue = std::strtoull(g_editValue.c_str(), &end, 10);
            if (end == g_editValue.c_str() || *end != '\0' || parsedValue > UINT32_MAX) return false;

            const bool changed = parsedValue != value;
            value = static_cast<uint32_t>(parsedValue);
            StopEditing();
            return changed;
        }
    }

    PropertyList::PropertyList() {
        m_elements.reserve(16);
    }

    void PropertyList::Button(const char* text, std::function<void()> onPress) {
        Element& element = m_elements.emplace_back();
        element.label = text;
        element.includeInLabelWidth = false;
        element.render = [onPress](const std::string& text) {
            if (InputElements::Button(text) && onPress) {
                onPress();
            }
        };
    }

    void PropertyList::Button(const char* text, bool& pressed) {
        Element& element = m_elements.emplace_back();
        element.label = text;
        element.pressed = &pressed;
        element.includeInLabelWidth = false;
    }

    void PropertyList::ReadOnly(const char* label, const std::string& value) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [value](const std::string& label) {
            InputElements::ReadOnly(label, value);
        };
    }

    void PropertyList::String(uint64_t objectId, const char* label, std::string& value, std::function<void()> onChange, bool commitOnFocusLoss) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange, commitOnFocusLoss](const std::string& label) {
            if (InputElements::String(objectId, label, *value, commitOnFocusLoss) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::CheckBox(const char* label, bool& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [value = &value, onChange](const std::string& label) {
            if (InputElements::CheckBox(label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::AxisLimit(uint64_t objectId, const char* label, AxisLimitValue& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::AxisLimit(objectId, label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::DropDown(uint64_t objectId, const char* label, const std::vector<std::string>& options, std::string& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, options = &options, value = &value, onChange](const std::string& label) {
            if (InputElements::DropDown(objectId, label, *options, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::Float(uint64_t objectId, const char* label, float& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Float(objectId, label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::FloatSlider(uint64_t objectId, const char* label, float& value, float minimum, float maximum, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, minimum, maximum, onChange](const std::string& label) {
            if (InputElements::FloatSlider(objectId, label, *value, minimum, maximum) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::IntSlider(uint64_t objectId, const char* label, int32_t& value, int32_t minimum, int32_t maximum, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, minimum, maximum, onChange](const std::string& label) {
            if (InputElements::IntSlider(objectId, label, *value, minimum, maximum) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::UInt(uint64_t objectId, const char* label, uint32_t& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::UInt(objectId, label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::Vec2(uint64_t objectId, const char* label, glm::vec2& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Vec2(objectId, label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::Vec3(uint64_t objectId, const char* label, glm::vec3& value, std::function<void()> onChange) {
        Element& element = m_elements.emplace_back();
        element.label = label;
        element.render = [objectId, value = &value, onChange](const std::string& label) {
            if (InputElements::Vec3(objectId, label, *value) && onChange) {
                onChange();
            }
        };
    }

    void PropertyList::Render(const EditorRect& rect) {
        const EditorStyle& style = GetStyle();
        int32_t labelColumnWidth = 0;
        for (const Element& element : m_elements) {
            if (element.includeInLabelWidth) {
                labelColumnWidth = std::max(labelColumnWidth, TextBlitter::GetTextSize(element.label, style.font.name, style.font.scale).x);
            }
        }

        Begin(rect, labelColumnWidth);
        for (Element& element : m_elements) {
            if (element.pressed) *element.pressed = InputElements::Button(element.label);
            else element.render(element.label);
        }
        g_lastRenderedHeight = style.input.contentPadding * 2 + g_rowIndex * style.input.rowHeight + g_rowExtraHeight;
        m_elements.clear();
    }

    void Begin(const EditorRect& rect, int32_t labelColumnWidth) {
        g_rect = rect;
        g_labelColumnWidth = labelColumnWidth;
        g_rowIndex = 0;
        g_rowExtraHeight = 0;
    }

    bool Button(const std::string& text) {
        const EditorStyle& style = GetStyle();
        const int32_t rowY = GetRowY();
        const EditorRect buttonRect = { g_rect.x + style.input.contentPadding, rowY + style.input.buttonElementGap, std::max(0, g_rect.width - style.input.contentPadding * 2), style.input.buttonHeight };
        const bool hovered = buttonRect.Contains(Coordinates::GetMousePositionUI());
        const bool pressed = hovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;

        if (pressed) {
            StopEditing();
            StopSliderDrag();
            CloseDropDown();
        }

        if (hovered) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        // Button background and label
        UI::DrawSolidRect(buttonRect, hovered ? style.colors.hover : style.colors.controlBackground);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + text, style.font.name, glm::ivec2(buttonRect.x + buttonRect.width / 2, buttonRect.y + buttonRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, buttonRect.x, buttonRect.y, buttonRect.Right(), buttonRect.Bottom());
        g_rowIndex++;
        g_rowExtraHeight += style.input.buttonHeight + style.input.buttonElementGap * 2 - style.input.rowHeight;
        return pressed;
    }

    bool String(uint64_t objectId, const std::string& label, std::string& value, bool commitOnFocusLoss) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const TextEditAction action = DrawTextField(objectId, label, 0, GetFieldRect(rowY, 0, 1), value, false);
        const bool commit = action == TextEditAction::COMMIT || (commitOnFocusLoss && action == TextEditAction::FOCUS_LOST);
        const bool changed = commit && g_editValue != value;
        if (commit) {
            value = g_editValue;
            StopEditing();
        }
        g_rowIndex++;
        return changed;
    }

    void ReadOnly(const std::string& label, const std::string& value) {
        const EditorStyle& style = GetStyle();
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);

        DrawLabel(label, rowY);
        UI::DrawSolidRect(fieldRect, style.colors.controlBackground);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + value, style.font.name, glm::ivec2(fieldRect.x + style.input.fieldPadding, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, fieldRect.Right(), fieldRect.Bottom());
        g_rowIndex++;
    }

    bool CheckBox(const std::string& label, bool& value) {
        const EditorStyle& style = GetStyle();
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const EditorRect checkBoxRect = { fieldRect.x, rowY + (style.input.rowHeight - style.input.checkBoxSize) / 2, style.input.checkBoxSize, style.input.checkBoxSize };
        const bool hovered = checkBoxRect.Contains(Coordinates::GetMousePositionUI());
        const bool changed = hovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;

        if (changed) {
            value = !value;
        }

        DrawLabel(label, rowY);
        UI::DrawSolidRect(checkBoxRect, hovered ? style.colors.hover : style.colors.controlBackground);
        if (value) {
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + "x", style.font.name, glm::ivec2(checkBoxRect.x + checkBoxRect.width / 2, checkBoxRect.y + checkBoxRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST);
        }

        g_rowIndex++;
        return changed;
    }

    bool AxisLimit(uint64_t objectId, const std::string& label, AxisLimitValue& value) {
        constexpr float DEFAULT_LIMIT = 45.0f;
        constexpr float MINIMUM_ANGLE = -179.0f;
        constexpr float MAXIMUM_ANGLE = 179.0f;
        constexpr float MINIMUM_LIMIT = 5.0f;
        constexpr float MAXIMUM_LIMIT = 179.0f;

        const EditorStyle& style = GetStyle();
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const int32_t enabledSize = std::min(style.input.checkBoxSize, fieldRect.width / 2);
        const int32_t lockWidth = std::min(fieldRect.height, fieldRect.width - enabledSize);
        const EditorRect enabledRect = { fieldRect.x, rowY + (style.input.rowHeight - enabledSize) / 2, enabledSize, enabledSize };
        const EditorRect lockRect = { fieldRect.Right() - lockWidth, fieldRect.y, lockWidth, fieldRect.height };
        const int32_t limitsX = enabledRect.Right() + style.input.fieldGap;
        const int32_t limitsWidth = std::max(0, lockRect.x - style.input.fieldGap - limitsX);
        const EditorRect limitsRect = { limitsX, fieldRect.y, limitsWidth, fieldRect.height };
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const bool enabledHovered = enabledRect.Contains(mousePosition);
        const bool lockHovered = lockRect.Contains(mousePosition);
        const bool enabledPressed = enabledHovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;
        const bool lockPressed = lockHovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;
        const AxisLimitValue previousValue = value;

        // Toggle the axis
        if (enabledPressed) {
            StopEditing();
            StopSliderDrag();
            CloseDropDown();

            if (value.enabled) {
                value.enabled = false;
                value.locked = false;
            }
            else {
                value.enabled = true;
                value.locked = false;
                value.minimumDegrees = -DEFAULT_LIMIT;
                value.maximumDegrees = DEFAULT_LIMIT;
            }
        }

        // Toggle the axis lock
        if (lockPressed) {
            StopEditing();
            StopSliderDrag();
            CloseDropDown();

            if (value.locked) {
                value.enabled = false;
                value.locked = false;
            }
            else {
                value.enabled = true;
                value.locked = true;
                value.minimumDegrees = 0.0f;
                value.maximumDegrees = 0.0f;
            }
        }

        if (enabledHovered || lockHovered) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        // Axis state
        DrawLabel(label, rowY);
        UI::DrawSolidRect(enabledRect, enabledHovered ? style.colors.hover : style.colors.controlBackground);
        if (value.enabled) {
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + "x", style.font.name, glm::ivec2(enabledRect.x + enabledRect.width / 2, enabledRect.y + enabledRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST);
        }

        // Minimum and maximum angles
        if (value.enabled && !value.locked) {
            float minimumInterpolation = (std::clamp(value.minimumDegrees, MINIMUM_ANGLE, MAXIMUM_ANGLE) - MINIMUM_ANGLE) / (MAXIMUM_ANGLE - MINIMUM_ANGLE);
            float maximumInterpolation = (std::clamp(value.maximumDegrees, MINIMUM_ANGLE, MAXIMUM_ANGLE) - MINIMUM_ANGLE) / (MAXIMUM_ANGLE - MINIMUM_ANGLE);
            float draggedInterpolation = 0.0f;
            const int32_t draggedHandle = UpdateRangeSlider(objectId, label, limitsRect, minimumInterpolation, maximumInterpolation, draggedInterpolation);

            if (draggedHandle == 0) {
                const float draggedAngle = std::round(MINIMUM_ANGLE + (MAXIMUM_ANGLE - MINIMUM_ANGLE) * draggedInterpolation);
                const float halfRange = std::clamp(-draggedAngle, MINIMUM_LIMIT, MAXIMUM_LIMIT);
                value.minimumDegrees = -halfRange;
                value.maximumDegrees = halfRange;
            }
            else if (draggedHandle == 1) {
                const float draggedAngle = std::round(MINIMUM_ANGLE + (MAXIMUM_ANGLE - MINIMUM_ANGLE) * draggedInterpolation);
                const float halfRange = std::clamp(draggedAngle, MINIMUM_LIMIT, MAXIMUM_LIMIT);
                value.minimumDegrees = -halfRange;
                value.maximumDegrees = halfRange;
            }

            minimumInterpolation = (std::clamp(value.minimumDegrees, MINIMUM_ANGLE, MAXIMUM_ANGLE) - MINIMUM_ANGLE) / (MAXIMUM_ANGLE - MINIMUM_ANGLE);
            maximumInterpolation = (std::clamp(value.maximumDegrees, MINIMUM_ANGLE, MAXIMUM_ANGLE) - MINIMUM_ANGLE) / (MAXIMUM_ANGLE - MINIMUM_ANGLE);
            const bool sliderActive = IsSliderActive(objectId, label + " Minimum") || IsSliderActive(objectId, label + " Maximum");
            DrawRangeSlider(limitsRect, minimumInterpolation, maximumInterpolation, FloatToString(value.minimumDegrees) + " / " + FloatToString(value.maximumDegrees) + " deg", limitsRect.Contains(mousePosition) || sliderActive);
        }
        else {
            UI::DrawSolidRect(limitsRect, style.colors.controlBackground);
            UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + (value.locked ? "Locked" : "Free"), style.font.name, glm::ivec2(limitsRect.x + limitsRect.width / 2, limitsRect.y + limitsRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST, limitsRect.x, limitsRect.y, limitsRect.Right(), limitsRect.Bottom());
        }

        // Lock state
        glm::vec4 lockColor = value.locked ? style.colors.selected : style.colors.controlBackground;
        if (lockHovered) {
            lockColor = style.colors.hover;
        }

        UI::DrawSolidRect(lockRect, lockColor);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + "L", style.font.name, glm::ivec2(lockRect.x + lockRect.width / 2, lockRect.y + lockRect.height / 2), Alignment::CENTERED, style.font.scale, TextureFilter::NEAREST);

        g_rowIndex++;
        return value.enabled != previousValue.enabled || value.locked != previousValue.locked || value.minimumDegrees != previousValue.minimumDegrees || value.maximumDegrees != previousValue.maximumDegrees;
    }

    bool DropDown(uint64_t objectId, const std::string& label, const std::vector<std::string>& options, std::string& value) {
        const EditorStyle& style = GetStyle();
        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const glm::ivec2 mousePosition = Coordinates::GetMousePositionUI();
        const bool hovered = fieldRect.Contains(mousePosition);
        const bool fieldPressed = hovered && Hell::Input::LeftMousePressed() && !g_dropDownConsumedMousePress;
        bool changed = false;

        // Open or close the options list
        if (fieldPressed) {
            if (IsDropDownOpen(objectId, label)) {
                CloseDropDown();
            }
            else if (!options.empty()) {
                StopEditing();
                CloseDropDown();
                g_openDropDownObjectId = objectId;
                g_openDropDownLabel = label;
                g_dropDownScrollBar = {};
            }
        }

        if (IsDropDownOpen(objectId, label)) {
            g_openDropDownValue = value;
            g_openDropDownOptions = options;

            const int32_t availableRows = std::max(1, (static_cast<int32_t>(UIBackEnd::GetCanvasResolution(UICanvas::NATIVE).y) - fieldRect.Bottom()) / style.input.rowHeight);
            g_dropDownVisibleOptionCount = std::min({ DROP_DOWN_MAX_VISIBLE_OPTIONS, static_cast<int32_t>(options.size()), availableRows });
            g_openDropDownPopupRect = { fieldRect.x, fieldRect.Bottom(), fieldRect.width, g_dropDownVisibleOptionCount * style.input.rowHeight };

            // Scroll the options under the mouse
            if (g_openDropDownPopupRect.Contains(mousePosition)) {
                if (Hell::Input::MouseWheelUp()) {
                    g_dropDownScrollBar.value -= DROP_DOWN_ROWS_PER_SCROLL;
                }
                else if (Hell::Input::MouseWheelDown()) {
                    g_dropDownScrollBar.value += DROP_DOWN_ROWS_PER_SCROLL;
                }
            }

            const EditorRect scrollBarRect = { g_openDropDownPopupRect.Right() - style.input.dropDownScrollBarWidth, g_openDropDownPopupRect.y, style.input.dropDownScrollBarWidth, g_openDropDownPopupRect.height };
            ScrollBar::Update(g_dropDownScrollBar, scrollBarRect, static_cast<int32_t>(options.size()), g_dropDownVisibleOptionCount, true);

            // Find the hovered option
            g_hoveredDropDownOption = -1;
            for (int32_t i = 0; i < g_dropDownVisibleOptionCount; i++) {
                if (GetDropDownOptionRect(i).Contains(mousePosition)) {
                    g_hoveredDropDownOption = g_dropDownScrollBar.value + i;
                }
            }

            // Select an option or close the list
            if (!fieldPressed && Hell::Input::LeftMousePressed() && g_openDropDownPopupRect.Contains(mousePosition)) {
                g_dropDownConsumedMousePress = true;
                if (!ScrollBar::WantsMouseCapture(g_dropDownScrollBar) && g_hoveredDropDownOption >= 0 && g_hoveredDropDownOption < static_cast<int32_t>(options.size())) {
                    value = options[g_hoveredDropDownOption];
                    changed = true;
                    CloseDropDown();
                }
            }
            else if (!fieldPressed && Hell::Input::LeftMousePressed()) {
                CloseDropDown();
            }

            if (IsDropDownOpen(objectId, label) && Hell::Input::KeyPressed(HELL_KEY_ESCAPE)) {
                CloseDropDown();
            }

            if (IsDropDownOpen(objectId, label)) {
                g_openDropDownWasDrawn = true;
            }
        }

        if (hovered) {
            Hell::BackEnd::SetCursor(HELL_CURSOR_ARROW);
        }

        // Field background, label, and arrow
        DrawLabel(label, rowY);
        UI::DrawSolidRect(fieldRect, hovered || IsDropDownOpen(objectId, label) ? style.colors.hover : style.colors.controlBackground);
        const int32_t textRight = std::max(fieldRect.x, fieldRect.Right() - style.input.fieldPadding * 2 - style.input.dropDownArrowSize);
        UIBackEnd::BlitText(UICanvas::NATIVE, std::string(style.font.textColorTag) + value, style.font.name, glm::ivec2(fieldRect.x + style.input.fieldPadding, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED_VERTICAL, style.font.scale, TextureFilter::NEAREST, fieldRect.x, fieldRect.y, textRight, fieldRect.Bottom());
        UIBackEnd::BlitTexture(UICanvas::NATIVE, "DropDownArrow", glm::ivec2(fieldRect.Right() - style.input.fieldPadding - style.input.dropDownArrowSize / 2, fieldRect.y + fieldRect.height / 2), Alignment::CENTERED, style.colors.text, glm::ivec2(style.input.dropDownArrowSize), TextureFilter::NEAREST);

        g_rowIndex++;
        return changed;
    }

    bool Float(uint64_t objectId, const std::string& label, float& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const bool changed = DrawFloatField(objectId, label, 0, GetFieldRect(rowY, 0, 1), value);
        g_rowIndex++;
        return changed;
    }

    bool FloatSlider(uint64_t objectId, const std::string& label, float& value, float minimum, float maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }

        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const float clampedValue = std::clamp(value, minimum, maximum);
        float interpolation = maximum > minimum ? (clampedValue - minimum) / (maximum - minimum) : 0.0f;
        const bool sliderChanged = UpdateSlider(objectId, label, fieldRect, interpolation);

        const float newValue = sliderChanged ? minimum + (maximum - minimum) * interpolation : clampedValue;
        const bool changed = newValue != value;
        value = newValue;

        DrawLabel(label, rowY);
        DrawSlider(fieldRect, interpolation, FloatToString(value), fieldRect.Contains(Coordinates::GetMousePositionUI()) || IsSliderActive(objectId, label));
        g_rowIndex++;
        return changed;
    }

    bool IntSlider(uint64_t objectId, const std::string& label, int32_t& value, int32_t minimum, int32_t maximum) {
        if (minimum > maximum) {
            std::swap(minimum, maximum);
        }

        const int32_t rowY = GetRowY();
        const EditorRect fieldRect = GetFieldRect(rowY, 0, 1);
        const int32_t clampedValue = std::clamp(value, minimum, maximum);
        const float range = static_cast<float>(maximum) - static_cast<float>(minimum);
        float interpolation = range > 0.0f ? (static_cast<float>(clampedValue) - static_cast<float>(minimum)) / range : 0.0f;
        const bool sliderChanged = UpdateSlider(objectId, label, fieldRect, interpolation);

        const int32_t newValue = sliderChanged ? std::clamp(static_cast<int32_t>(std::round(static_cast<float>(minimum) + range * interpolation)), minimum, maximum) : clampedValue;
        const bool changed = newValue != value;
        value = newValue;
        interpolation = range > 0.0f ? (static_cast<float>(value) - static_cast<float>(minimum)) / range : 0.0f;

        DrawLabel(label, rowY);
        DrawSlider(fieldRect, interpolation, std::to_string(value), fieldRect.Contains(Coordinates::GetMousePositionUI()) || IsSliderActive(objectId, label));
        g_rowIndex++;
        return changed;
    }

    bool UInt(uint64_t objectId, const std::string& label, uint32_t& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);
        const bool changed = DrawUIntField(objectId, label, GetFieldRect(rowY, 0, 1), value);
        g_rowIndex++;
        return changed;
    }

    bool Vec2(uint64_t objectId, const std::string& label, glm::vec2& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);

        bool changed = false;
        for (int32_t i = 0; i < 2; i++) {
            changed = DrawFloatField(objectId, label, i, GetFieldRect(rowY, i, 2), value[i]) || changed;
        }

        g_rowIndex++;
        return changed;
    }

    bool Vec3(uint64_t objectId, const std::string& label, glm::vec3& value) {
        const int32_t rowY = GetRowY();
        DrawLabel(label, rowY);

        bool changed = false;
        for (int32_t i = 0; i < 3; i++) {
            changed = DrawFloatField(objectId, label, i, GetFieldRect(rowY, i, 3), value[i]) || changed;
        }

        g_rowIndex++;
        return changed;
    }

    void BeginFrame(bool allowInput) {
        g_activeInputWasDrawn = false;
        g_openDropDownWasDrawn = false;
        g_dropDownConsumedMousePress = false;
        g_activeSliderWasDrawn = false;
        if (!allowInput) {
            Reset();
            g_dropDownConsumedMousePress = true;
        }
    }

    bool DidConsumeMousePress() {
        return g_dropDownConsumedMousePress;
    }

    void EndFrame() {
        if (g_activeObjectId != 0 && !g_activeInputWasDrawn) {
            StopEditing();
        }

        if ((g_activeSliderObjectId != 0 || g_pendingRangeSliderObjectId != 0) && !g_activeSliderWasDrawn) {
            StopSliderDrag();
        }

        if (g_openDropDownObjectId != 0 && !g_openDropDownWasDrawn) {
            CloseDropDown();
        }
        else {
            RenderOpenDropDown();
        }
    }

    void Reset() {
        StopEditing();
        StopSliderDrag();
        CloseDropDown();
    }

    void FocusString(uint64_t objectId, const std::string& label, const std::string& value) {
        Reset();
        g_activeObjectId = objectId;
        g_activeLabel = label;
        g_activeComponentIndex = 0;
        g_editValue = value;
        g_caretIndex = value.size();
        g_selectionAnchor = g_caretIndex;
        g_ignoreActiveMousePress = true;
    }

    int32_t GetLastRenderedHeight() {
        return g_lastRenderedHeight;
    }

    bool WantsKeyboardCapture() {
        return g_activeObjectId != 0 || g_openDropDownObjectId != 0;
    }
}
