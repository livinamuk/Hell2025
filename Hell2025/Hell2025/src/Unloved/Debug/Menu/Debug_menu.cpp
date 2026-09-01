#include "Debug_menu.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Render/API/OpenGL/Types/GL_timer.h"
#include "Hell/Render/API/Vulkan/Types/vk_timer.h"
#include "Hell/Time.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

#include <algorithm>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace Debug::Menu {

    enum struct PageType {
        MENU,
        DISPLAY,
    };

    struct Setting {
        uint32_t id = 0;
        std::string name;
        SettingType type = SettingType::FLOAT;
        Value value;
        int32_t intMinimum = 0;
        int32_t intMaximum = 0;
        int32_t intIncrement = 1;
        uint32_t uintMinimum = 0;
        uint32_t uintMaximum = 0;
        uint32_t uintIncrement = 1;
        float floatMinimum = 0.0f;
        float floatMaximum = 0.0f;
        float floatIncrement = 0.0f;
        int32_t floatPrecision = 3;
        bool floatScientific = false;
        std::vector<std::string> enumNames;
        PageId targetPage = ROOT_PAGE_ID;
    };

    struct PageDefinition {
        PageType type = PageType::MENU;
        std::string heading;
        PageId parent = ROOT_PAGE_ID;
        BuildFunction buildFunction = nullptr;
        DisplayFunction displayFunction = nullptr;
        ApplyEditFunction applyFunction = nullptr;
        int32_t selectionIndex = 0;
    };

    struct RootPageEntry {
        std::string name;
        PageId page = ROOT_PAGE_ID;
        IsVisibleFunction isVisibleFunction = nullptr;
    };

    enum struct FunctionTimingAPI {
        OPENGL,
        VULKAN,
    };

    struct FunctionTimingEntry {
        FunctionTimingAPI api = FunctionTimingAPI::OPENGL;
        std::string functionName;
    };

    bool g_menuVisible = false;
    bool g_menuHadFocusThisFrame = false;
    bool g_menuInitialized = false;
    PageId g_menuPage = ROOT_PAGE_ID;
    std::string g_menuHeading;
    std::vector<Setting> g_menuSettings;
    std::vector<PageDefinition> g_pageDefinitions;
    std::vector<RootPageEntry> g_rootPageEntries;
    std::vector<std::string> g_menuText;
    std::vector<FunctionTimingEntry> g_functionTimings;

    constexpr float KEY_REPEAT_INITIAL_DELAY = 0.36f;
    constexpr float KEY_REPEAT_INTERVAL = 0.08f;

    struct KeyRepeatState {
        float heldTime = 0.0f;
        float nextRepeatAt = KEY_REPEAT_INITIAL_DELAY;
    };

    KeyRepeatState g_leftKeyRepeat;
    KeyRepeatState g_rightKeyRepeat;

    void BuildRootPage();

    void ResetKeyRepeatState(KeyRepeatState& state) {
        state.heldTime = 0.0f;
        state.nextRepeatAt = KEY_REPEAT_INITIAL_DELAY;
    }

    void ResetHorizontalKeyRepeat() {
        ResetKeyRepeatState(g_leftKeyRepeat);
        ResetKeyRepeatState(g_rightKeyRepeat);
    }

    bool KeyRepeatPulse(uint32_t keyCode, KeyRepeatState& state) {
        if (!Hell::Input::KeyDown(keyCode) || Hell::Input::KeyPressed(keyCode)) {
            ResetKeyRepeatState(state);
            return false;
        }

        state.heldTime += Hell::Time::DeltaTime();
        if (state.heldTime < state.nextRepeatAt) return false;

        do {
            state.nextRepeatAt += KEY_REPEAT_INTERVAL;
        } while (state.heldTime >= state.nextRepeatAt);
        return true;
    }

    std::vector<RegisterFunction>& GetRegisterFunctions() {
        static std::vector<RegisterFunction> registerFunctions;
        return registerFunctions;
    }

    Registrar::Registrar(RegisterFunction registerFunction) {
        GetRegisterFunctions().push_back(registerFunction);
    }

    PageId RegisterPage(const std::string& heading, PageId parent, BuildFunction buildFunction, ApplyEditFunction applyFunction) {
        PageDefinition& pageDefinition = g_pageDefinitions.emplace_back();
        pageDefinition.type = PageType::MENU;
        pageDefinition.heading = heading;
        pageDefinition.parent = parent;
        pageDefinition.buildFunction = buildFunction;
        pageDefinition.applyFunction = applyFunction;
        return static_cast<PageId>(g_pageDefinitions.size() - 1);
    }

    PageId RegisterRootPage(const std::string& name, const std::string& heading, BuildFunction buildFunction, ApplyEditFunction applyFunction, IsVisibleFunction isVisibleFunction) {
        PageId page = RegisterPage(heading, ROOT_PAGE_ID, buildFunction, applyFunction);
        RootPageEntry& rootEntry = g_rootPageEntries.emplace_back();
        rootEntry.name = name;
        rootEntry.page = page;
        rootEntry.isVisibleFunction = isVisibleFunction;
        return page;
    }

    PageId RegisterDisplayPage(PageId parent, DisplayFunction displayFunction) {
        PageDefinition& pageDefinition = g_pageDefinitions.emplace_back();
        pageDefinition.type = PageType::DISPLAY;
        pageDefinition.parent = parent;
        pageDefinition.displayFunction = displayFunction;
        return static_cast<PageId>(g_pageDefinitions.size() - 1);
    }

    void Initialize() {
        if (g_menuInitialized) return;
        g_menuInitialized = true;

        RegisterPage("DEBUG", ROOT_PAGE_ID, BuildRootPage, nullptr);
        for (RegisterFunction registerFunction : GetRegisterFunctions()) {
            registerFunction();
        }
    }

    std::string WriteSelectedText(const std::string& text) {
        // return "[COL=0.1,1.0,0.1]" + text + "[COL=1.0,1.0,1.0]" + "\n"; // Green
        //return "[COL=0.217647,0.535294,0.950980]" + text + "[COL=1.0,1.0,1.0]\n"; // Soft blue
        //return "[COL=1.0,0.698039,0.2]" + text + "[COL=1.0,1.0,1.0]\n"; // Orange brighter kinda yellow but brighter
        //return text + "\n"; // White
        return "[COL=0.839216,0.156863,0.156863]" + text + "[COL=1.0,1.0,1.0]\n"; // RED kinda
    }

    std::string WriteText(const std::string& text) {
        //return "[COL=0.968627,0.498039,0.0]" + text + "[COL=1.0,1.0,1.0]\n"; // Orange
        // return "[COL=1.0,0.698039,0.2]" + text + "[COL=1.0,1.0,1.0]\n"; // Orange brighter kinda yellow

        return "[COL=1.0,0.698039,0.2]" + text + "[COL=1.0,1.0,1.0]\n"; // Orange brighter kinda yellow
        // return text + "\n"; // White
    }

    PageDefinition& GetCurrentPageDefinition() {
        return g_pageDefinitions[g_menuPage];
    }

    void AddBool(uint32_t id, const std::string& name, bool value) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::BOOL;
        setting.value.boolValue = value;
    }

    void AddInt(uint32_t id, const std::string& name, int32_t value, int32_t minimum, int32_t maximum, int32_t increment) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::INT;
        setting.value.intValue = value;
        setting.intMinimum = minimum;
        setting.intMaximum = maximum;
        setting.intIncrement = increment;
    }

    void AddUInt(uint32_t id, const std::string& name, uint32_t value, uint32_t minimum, uint32_t maximum, uint32_t increment) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::UINT;
        setting.value.uintValue = value;
        setting.uintMinimum = minimum;
        setting.uintMaximum = maximum;
        setting.uintIncrement = increment;
    }

    void AddFloat(uint32_t id, const std::string& name, float value, float minimum, float maximum, float increment, int32_t precision, bool scientific) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::FLOAT;
        setting.value.floatValue = value;
        setting.floatMinimum = minimum;
        setting.floatMaximum = maximum;
        setting.floatIncrement = increment;
        setting.floatPrecision = precision;
        setting.floatScientific = scientific;
    }

    void AddEnum(uint32_t id, const std::string& name, int32_t value, const std::vector<std::string>& enumNames, size_t maxEnumCount) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::ENUM;
        setting.value.intValue = value;
        setting.enumNames = enumNames;
        setting.enumNames.resize(std::min(maxEnumCount, setting.enumNames.size()));
    }

    void AddSubMenu(uint32_t id, const std::string& name, PageId targetPage) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::SUB_MENU;
        setting.targetPage = targetPage;
    }

    void AddAction(uint32_t id, const std::string& name) {
        Setting& setting = g_menuSettings.emplace_back();
        setting.id = id;
        setting.name = name;
        setting.type = SettingType::ACTION;
    }

    void AddLineBreak() {
        Setting& setting = g_menuSettings.emplace_back();
        setting.type = SettingType::LINE_BREAK;
    }

    void AddText(const std::string& text) {
        g_menuText.push_back(text);
    }

    void AddOpenGLFunctionTiming(const std::string& functionName) {
        FunctionTimingEntry& entry = g_functionTimings.emplace_back();
        entry.api = FunctionTimingAPI::OPENGL;
        entry.functionName = functionName;
    }

    void AddVulkanFunctionTiming(const std::string& functionName) {
        FunctionTimingEntry& entry = g_functionTimings.emplace_back();
        entry.api = FunctionTimingAPI::VULKAN;
        entry.functionName = functionName;
    }

    void BuildRootPage() {
        for (const RootPageEntry& rootEntry : g_rootPageEntries) {
            if (rootEntry.isVisibleFunction && !rootEntry.isVisibleFunction()) continue;
            AddSubMenu(rootEntry.page, rootEntry.name, rootEntry.page);
        }
    }

    void BuildCurrentPage() {
        PageDefinition& pageDefinition = GetCurrentPageDefinition();
        g_menuHeading = pageDefinition.heading;
        g_menuSettings.clear();
        g_menuText.clear();
        g_functionTimings.clear();
        if (pageDefinition.type == PageType::MENU && pageDefinition.buildFunction) pageDefinition.buildFunction();

        if (g_menuSettings.empty()) {
            pageDefinition.selectionIndex = 0;
            return;
        }

        if (pageDefinition.selectionIndex < 0) pageDefinition.selectionIndex = static_cast<int32_t>(g_menuSettings.size()) - 1;
        if (pageDefinition.selectionIndex >= static_cast<int32_t>(g_menuSettings.size())) pageDefinition.selectionIndex %= static_cast<int32_t>(g_menuSettings.size());
    }

    void MoveSelection(int32_t direction) {
        if (g_menuSettings.empty()) return;

        ResetHorizontalKeyRepeat();

        PageDefinition& pageDefinition = GetCurrentPageDefinition();
        pageDefinition.selectionIndex += direction;
        if (pageDefinition.selectionIndex < 0) pageDefinition.selectionIndex = static_cast<int32_t>(g_menuSettings.size()) - 1;
        if (pageDefinition.selectionIndex >= static_cast<int32_t>(g_menuSettings.size())) pageDefinition.selectionIndex = 0;
    }

    void ApplySetting(const Setting& setting) {
        PageDefinition& pageDefinition = GetCurrentPageDefinition();
        if (pageDefinition.applyFunction) pageDefinition.applyFunction(setting.id, setting.value);
    }

    bool AdjustSetting(Setting& setting, int32_t direction) {
        switch (setting.type) {
            case SettingType::BOOL: {
                setting.value.boolValue = !setting.value.boolValue;
                break;
            }
            case SettingType::INT: {
                int64_t nextValue = static_cast<int64_t>(setting.value.intValue) + static_cast<int64_t>(setting.intIncrement) * direction;
                setting.value.intValue = static_cast<int32_t>(std::clamp(nextValue, static_cast<int64_t>(setting.intMinimum), static_cast<int64_t>(setting.intMaximum)));
                break;
            }
            case SettingType::UINT: {
                if (direction < 0) {
                    uint64_t nextValue = setting.value.uintValue >= setting.uintIncrement ? static_cast<uint64_t>(setting.value.uintValue - setting.uintIncrement) : 0;
                    setting.value.uintValue = static_cast<uint32_t>(std::max(nextValue, static_cast<uint64_t>(setting.uintMinimum)));
                }
                if (direction > 0) {
                    uint64_t nextValue = static_cast<uint64_t>(setting.value.uintValue) + setting.uintIncrement;
                    setting.value.uintValue = static_cast<uint32_t>(std::min(nextValue, static_cast<uint64_t>(setting.uintMaximum)));
                }
                setting.value.uintValue = std::clamp(setting.value.uintValue, setting.uintMinimum, setting.uintMaximum);
                break;
            }
            case SettingType::FLOAT: {
                setting.value.floatValue = std::clamp(setting.value.floatValue + setting.floatIncrement * static_cast<float>(direction), setting.floatMinimum, setting.floatMaximum);
                break;
            }
            case SettingType::ENUM: {
                if (setting.enumNames.empty()) return false;
                setting.value.intValue += direction;
                if (setting.value.intValue < 0) setting.value.intValue = static_cast<int32_t>(setting.enumNames.size()) - 1;
                if (setting.value.intValue >= static_cast<int32_t>(setting.enumNames.size())) setting.value.intValue = 0;
                break;
            }
            default: {
                return false;
            }
        }

        ApplySetting(setting);
        return true;
    }

    bool ActivateSetting(Setting& setting) {
        if (setting.type == SettingType::SUB_MENU) {
            g_menuPage = setting.targetPage;
            return true;
        }
        if (setting.type == SettingType::ACTION) {
            ApplySetting(setting);
            return true;
        }
        if (setting.type == SettingType::BOOL || setting.type == SettingType::ENUM) {
            return AdjustSetting(setting, 1);
        }
        return false;
    }

    bool NavigateBack() {
        if (g_menuPage == ROOT_PAGE_ID) {
            Debug::HideMenu();
            return false;
        }

        g_menuPage = GetCurrentPageDefinition().parent;
        return true;
    }

    bool HandleInput() {
        if (Hell::Input::KeyPressed(HELL_KEY_ESCAPE) || Hell::Input::KeyPressed(HELL_KEY_BACKSPACE)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            return NavigateBack();
        }

        if (Hell::Input::KeyPressed(HELL_KEY_UP)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            MoveSelection(-1);

            if (g_menuSettings[GetCurrentPageDefinition().selectionIndex].type == SettingType::LINE_BREAK) {
                MoveSelection(-1);
            }
        }
        if (Hell::Input::KeyPressed(HELL_KEY_DOWN)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            MoveSelection(1);

            if (g_menuSettings[GetCurrentPageDefinition().selectionIndex].type == SettingType::LINE_BREAK) {
                MoveSelection(1);
            }
        }

        if (g_menuSettings.empty()) return false;

        Setting& selectedSetting = g_menuSettings[GetCurrentPageDefinition().selectionIndex];

        const bool allowHorizontalRepeat =
            selectedSetting.type == SettingType::INT ||
            selectedSetting.type == SettingType::UINT ||
            selectedSetting.type == SettingType::FLOAT;

        bool leftPressed = Hell::Input::KeyPressed(HELL_KEY_LEFT);
        bool rightPressed = Hell::Input::KeyPressed(HELL_KEY_RIGHT);
        if (allowHorizontalRepeat) {
            leftPressed |= KeyRepeatPulse(HELL_KEY_LEFT, g_leftKeyRepeat);
            rightPressed |= KeyRepeatPulse(HELL_KEY_RIGHT, g_rightKeyRepeat);
        }
        else {
            ResetHorizontalKeyRepeat();
        }

        if (leftPressed && AdjustSetting(selectedSetting, -1)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            return true;
        }
        if (rightPressed) {
            bool changed = selectedSetting.type == SettingType::SUB_MENU ? ActivateSetting(selectedSetting) : AdjustSetting(selectedSetting, 1);
            if (changed) {
                Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
                return true;
            }
        }
        if (Hell::Input::KeyPressed(HELL_KEY_ENTER) && ActivateSetting(selectedSetting)) {
            Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            return true;
        }

        return false;
    }

    std::string FormatSettingValue(const Setting& setting) {
        switch (setting.type) {
            case SettingType::BOOL:     return setting.value.boolValue ? "ON" : "OFF";
            case SettingType::INT:      return std::to_string(setting.value.intValue);
            case SettingType::UINT:     return std::to_string(setting.value.uintValue);
            case SettingType::SUB_MENU: return ">";
            case SettingType::ACTION:   return "";
            case SettingType::ENUM: {
                if (setting.value.intValue < 0 || setting.value.intValue >= static_cast<int32_t>(setting.enumNames.size())) return "INVALID";
                return setting.enumNames[setting.value.intValue];
            }
            case SettingType::FLOAT: {
                std::ostringstream stream;
                if (setting.floatScientific) stream << std::scientific;
                else                         stream << std::fixed;
                stream << std::setprecision(setting.floatPrecision) << setting.value.floatValue;
                return stream.str();
            }
        }

        return "";
    }

    std::string FormatFunctionTiming(double milliseconds) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(2) << milliseconds << " ms";
        return stream.str();
    }

    bool TryGetFunctionTiming(const FunctionTimingEntry& entry, double& cpuMilliseconds, double& gpuMilliseconds) {
        if (entry.api == FunctionTimingAPI::OPENGL) {
            return GetTimer().TryGetZoneTiming(entry.functionName, cpuMilliseconds, gpuMilliseconds);
        }

        if (entry.api == FunctionTimingAPI::VULKAN) {
            return GetVulkanTimer().TryGetZoneTiming(entry.functionName, cpuMilliseconds, gpuMilliseconds);
        }

        return false;
    }

    int32_t RenderText(int32_t y, float scale) {
        if (g_menuText.empty()) return y;

        std::string text;
        for (const std::string& line : g_menuText) {
            text += WriteText(line);
        }

        UIBackEnd::BlitText(text, "StandardFont", 0, y, Alignment::TOP_LEFT, scale, TextureFilter::NEAREST);
        return y + TextBlitter::GetTextSize(text, "StandardFont", scale).y;
    }

    void RenderFunctionTimings(int32_t y, float scale) {
        if (g_functionTimings.empty()) return;

        constexpr int32_t gpuX = 0;
        constexpr int32_t cpuX = 200;
        constexpr int32_t functionNameX = 400;

        const std::string headingColor = "[COL=0.56,0.93,0.56,1.0]";
        const std::string rowColor = "[COL=1.0,0.698039,0.2,1.0]";
        std::string gpuColumn = headingColor + "GPU\n";
        std::string cpuColumn = headingColor + "CPU\n";
        std::string functionNameColumn = "\n";

        for (const FunctionTimingEntry& entry : g_functionTimings) {
            double cpuMilliseconds = 0.0;
            double gpuMilliseconds = 0.0;
            const bool timingFound = TryGetFunctionTiming(entry, cpuMilliseconds, gpuMilliseconds);
            const std::string gpuTiming = timingFound ? FormatFunctionTiming(gpuMilliseconds) : "Waiting";
            const std::string cpuTiming = timingFound ? FormatFunctionTiming(cpuMilliseconds) : "Waiting";

            gpuColumn += rowColor + gpuTiming + "\n";
            cpuColumn += rowColor + cpuTiming + "\n";
            functionNameColumn += rowColor + entry.functionName + "\n";
        }

        UIBackEnd::BlitText(gpuColumn, "StandardFont", gpuX, y, Alignment::TOP_LEFT, scale, TextureFilter::NEAREST);
        UIBackEnd::BlitText(cpuColumn, "StandardFont", cpuX, y, Alignment::TOP_LEFT, scale, TextureFilter::NEAREST);
        UIBackEnd::BlitText(functionNameColumn, "StandardFont", functionNameX, y, Alignment::TOP_LEFT, scale, TextureFilter::NEAREST);
    }

    void Render() {
        PageDefinition& pageDefinition = GetCurrentPageDefinition();
        if (pageDefinition.type == PageType::DISPLAY) {
            if (pageDefinition.displayFunction) pageDefinition.displayFunction();
            return;
        }

        Debug::SetDebugTextMode(DebugTextMode::MENU);

        std::string names = WriteText(g_menuHeading + "\n");
        std::string values = WriteText("\n");
        const int32_t selectionIndex = pageDefinition.selectionIndex;

        for (int32_t i = 0; i < static_cast<int32_t>(g_menuSettings.size()); i++) {
            const Setting& setting = g_menuSettings[i];
            const std::string value = FormatSettingValue(setting);
            names += i == selectionIndex ? WriteSelectedText(setting.name) : WriteText(setting.name);
            values += i == selectionIndex ? WriteSelectedText(value) : WriteText(value);
        }

        constexpr float scale = 2.0f;
        constexpr int32_t valuePadding = 64;
        constexpr int32_t timingPaddingTop = 16;

        const glm::ivec2 textSize = TextBlitter::GetTextSize(names, "StandardFont", scale);
        UIBackEnd::BlitText(names, "StandardFont", 0, 0, Alignment::TOP_LEFT, scale);
        UIBackEnd::BlitText(values, "StandardFont", textSize.x + valuePadding, 0, Alignment::TOP_LEFT, scale);
        const int32_t timingY = RenderText(textSize.y + timingPaddingTop, scale);
        RenderFunctionTimings(timingY, scale);
    }

    void Update() {
        Initialize();
        g_menuHadFocusThisFrame = true;
        BuildCurrentPage();
        const bool rebuildPage = HandleInput();
        if (!g_menuVisible) return;
        if (rebuildPage) BuildCurrentPage();
        Render();
    }
}

namespace Debug {

    void UpdateMenu() {
        Menu::Update();
    }

    void ToggleMenuVisiblity() {
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);
        if (Menu::g_menuVisible) HideMenu();
        else                     ShowMenu();
    }

    void HideMenu() {
        Menu::g_menuVisible = false;
        Menu::ResetHorizontalKeyRepeat();
        SetDebugTextMode(DebugTextMode::NONE);
    }

    void ShowMenu() {
        Menu::Initialize();
        Menu::g_menuVisible = true;
        Menu::g_menuPage = Menu::ROOT_PAGE_ID;
        Menu::ResetHorizontalKeyRepeat();
        SetDebugTextMode(DebugTextMode::MENU);
    }

    bool IsMenuVisible() {
        return Menu::g_menuVisible;
    }

    bool MenuHadFocusThisFrame() {
        return Menu::g_menuHadFocusThisFrame;
    }

    void EndMenuFrame() {
        Menu::g_menuHadFocusThisFrame = false;
    }
}
