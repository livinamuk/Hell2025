#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Unloved::EditorSession::InputElements {

    struct AxisLimitValue {
        bool enabled = false;
        bool locked = false;
        float minimumDegrees = 0.0f;
        float maximumDegrees = 0.0f;
    };

    struct PropertyList {
        PropertyList();

        void Button(const char* text, std::function<void()> onPress = {});
        void Button(const char* text, bool& pressed);
        void ReadOnly(const char* label, const std::string& value);
        void String(uint64_t objectId, const char* label, std::string& value, std::function<void()> onChange = {}, bool commitOnFocusLoss = false);
        void CheckBox(const char* label, bool& value, std::function<void()> onChange = {});
        void AxisLimit(uint64_t objectId, const char* label, AxisLimitValue& value, std::function<void()> onChange = {});
        void DropDown(uint64_t objectId, const char* label, const std::vector<std::string>& options, std::string& value, std::function<void()> onChange = {});
        void Float(uint64_t objectId, const char* label, float& value, std::function<void()> onChange = {});
        void FloatSlider(uint64_t objectId, const char* label, float& value, float minimum, float maximum, std::function<void()> onChange = {});
        void IntSlider(uint64_t objectId, const char* label, int32_t& value, int32_t minimum, int32_t maximum, std::function<void()> onChange = {});
        void UInt(uint64_t objectId, const char* label, uint32_t& value, std::function<void()> onChange = {});
        void Vec2(uint64_t objectId, const char* label, glm::vec2& value, std::function<void()> onChange = {});
        void Vec3(uint64_t objectId, const char* label, glm::vec3& value, std::function<void()> onChange = {});
        void Render(const EditorRect& rect);

    private:
        struct Element {
            std::string label;
            std::function<void(const std::string&)> render;
            bool* pressed = nullptr;
            bool includeInLabelWidth = true;
        };

        std::vector<Element> m_elements;
    };

    void BeginFrame(bool allowInput = true);
    void EndFrame();
    void Reset();
    void FocusString(uint64_t objectId, const std::string& label, const std::string& value);

    int32_t GetLastRenderedHeight();
    bool DidConsumeMousePress();
    bool WantsKeyboardCapture();
}
