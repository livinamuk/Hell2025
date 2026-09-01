#pragma once

#include <MagicEnum.hpp>

#include <string>
#include <type_traits>
#include <vector>

namespace Hell::Enum {

    template <typename T>
    std::vector<std::string> GetNames() {
        auto names = magic_enum::enum_names<T>();
        return { names.begin(), names.end() };
    }

    template <typename T>
    size_t GetCount() {
        return magic_enum::enum_count<T>();
    }

    template <typename T>
    std::string ToString(T value) {
        return std::string(magic_enum::enum_name(value));
    }

    template <typename T>
    T FromString(const std::string& str, T defaultValue) {
        return magic_enum::enum_cast<T>(str).value_or(defaultValue);
    }

    template <typename T>
    T FromInt(int value) {
        static_assert(std::is_enum<T>::value, "Hell::Enum::FromInt requires an enum type");
        return static_cast<T>(value);
    }

    template <typename T>
    std::underlying_type_t<T> ToInt(T value) {
        static_assert(std::is_enum<T>::value, "Hell::Enum::ToInt requires an enum type");
        return static_cast<std::underlying_type_t<T>>(value);
    }

    template <typename T>
    T IntToEnum(int value) {
        return FromInt<T>(value);
    }

    template <typename T>
    std::underlying_type_t<T> EnumToInt(T value) {
        return ToInt(value);
    }
}
