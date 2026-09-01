#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Debug::Menu {

    using PageId = uint32_t;
    constexpr PageId ROOT_PAGE_ID = 0;

    enum struct SettingType {
        BOOL,
        INT,
        UINT,
        FLOAT,
        ENUM,
        SUB_MENU,
        ACTION,
        LINE_BREAK
    };

    union Value {
        bool boolValue;
        int32_t intValue;
        uint32_t uintValue;
        float floatValue;

        Value() : uintValue(0) {}
    };

    using BuildFunction = void(*)();
    using DisplayFunction = void(*)();
    using ApplyEditFunction = void(*)(uint32_t, const Value&);
    using RegisterFunction = void(*)();
    using IsVisibleFunction = bool(*)();

    // You gotta drop one of these in each menu cpp
    struct Registrar {
        explicit Registrar(RegisterFunction registerFunction);
    };

    PageId RegisterRootPage(const std::string& name, const std::string& heading, BuildFunction buildFunction, ApplyEditFunction applyFunction, IsVisibleFunction isVisibleFunction = nullptr);
    PageId RegisterPage(const std::string& heading, PageId parent, BuildFunction buildFunction, ApplyEditFunction applyFunction);
    PageId RegisterDisplayPage(PageId parent, DisplayFunction displayFunction);

    void AddBool(uint32_t id, const std::string& name, bool value);
    void AddInt(uint32_t id, const std::string& name, int32_t value, int32_t minimum, int32_t maximum, int32_t increment);
    void AddUInt(uint32_t id, const std::string& name, uint32_t value, uint32_t minimum, uint32_t maximum, uint32_t increment);
    void AddFloat(uint32_t id, const std::string& name, float value, float minimum, float maximum, float increment, int32_t precision = 3, bool scientific = false);
    void AddEnum(uint32_t id, const std::string& name, int32_t value, const std::vector<std::string>& enumNames, size_t maxEnumCount = -1);
    void AddSubMenu(uint32_t id, const std::string& name, PageId targetPage);
    void AddAction(uint32_t id, const std::string& name);
    void AddLineBreak();
    void AddText(const std::string& text);
    void AddOpenGLFunctionTiming(const std::string& functionName);
    void AddVulkanFunctionTiming(const std::string& functionName);
}
