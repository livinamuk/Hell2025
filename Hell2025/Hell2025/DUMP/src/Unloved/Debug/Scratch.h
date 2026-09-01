#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Debug::Scratch {

    namespace Detail {

        template<typename T>
        inline std::unordered_map<std::string, T>& GetValues() {
            static std::unordered_map<std::string, T> values;
            return values;
        }

        template<typename T>
        inline void Init(const std::string& name, T defaultValue) {
            GetValues<T>().try_emplace(name, std::move(defaultValue));
        }

        template<typename T>
        inline T Get(const std::string& name, T fallback) {
            const auto& values = GetValues<T>();
            const auto it = values.find(name);
            return it == values.end() ? std::move(fallback) : it->second;
        }

        template<typename T>
        inline void Set(const std::string& name, T value) {
            GetValues<T>().insert_or_assign(name, std::move(value));
        }
    }

    inline void InitBool(const std::string& name, bool defaultValue)                  { Detail::Init(name, defaultValue); }
    inline void InitInt(const std::string& name, int32_t defaultValue)                { Detail::Init(name, defaultValue); }
    inline void InitUInt(const std::string& name, uint32_t defaultValue)              { Detail::Init(name, defaultValue); }
    inline void InitFloat(const std::string& name, float defaultValue)                { Detail::Init(name, defaultValue); }
    inline void InitString(const std::string& name, const std::string& defaultValue)  { Detail::Init(name, defaultValue); }

    inline void InitStringList(const std::string& name, std::vector<std::string> values, int32_t defaultIndex = 0) {
        auto& stringLists = Detail::GetValues<std::vector<std::string>>();
        const auto it = stringLists.try_emplace(name, std::move(values)).first;
        const std::vector<std::string>& storedValues = it->second;

        if (storedValues.empty()) {
            Detail::Set<int32_t>(name, 0);
            return;
        }

        const int32_t currentIndex = Detail::Get<int32_t>(name, defaultIndex);
        const int32_t maximumIndex = static_cast<int32_t>(storedValues.size()) - 1;
        Detail::Set<int32_t>(name, std::clamp(currentIndex, 0, maximumIndex));
    }

    inline bool GetBool(const std::string& name, bool fallback = false)                         { return Detail::Get(name, fallback); }
    inline int32_t GetInt(const std::string& name, int32_t fallback = 0)                        { return Detail::Get(name, fallback); }
    inline uint32_t GetUInt(const std::string& name, uint32_t fallback = 0)                     { return Detail::Get(name, fallback); }
    inline float GetFloat(const std::string& name, float fallback = 0.0f)                       { return Detail::Get(name, fallback); }
    inline const std::vector<std::string>& GetStringList(const std::string& name) {
        static const std::vector<std::string> emptyList;
        const auto& stringLists = Detail::GetValues<std::vector<std::string>>();
        const auto it = stringLists.find(name);
        return it == stringLists.end() ? emptyList : it->second;
    }

    inline int32_t GetStringListIndex(const std::string& name, int32_t fallback = 0) {
        return Detail::Get<int32_t>(name, fallback);
    }

    inline std::string GetSelectedString(const std::string& name, const std::string& fallback = {}) {
        const std::vector<std::string>& values = GetStringList(name);
        if (values.empty()) return fallback;

        const int32_t maximumIndex = static_cast<int32_t>(values.size()) - 1;
        const int32_t index = std::clamp(GetStringListIndex(name), 0, maximumIndex);
        return values[index];
    }

    inline std::string GetString(const std::string& name, const std::string& fallback = {}) {
        if (!GetStringList(name).empty()) return GetSelectedString(name, fallback);
        return Detail::Get(name, fallback);
    }

    inline void SetBool(const std::string& name, bool value)                       { Detail::Set(name, value); }
    inline void SetInt(const std::string& name, int32_t value)                     { Detail::Set(name, value); }
    inline void SetUInt(const std::string& name, uint32_t value)                   { Detail::Set(name, value); }
    inline void SetFloat(const std::string& name, float value)                     { Detail::Set(name, value); }
    inline void SetStringListIndex(const std::string& name, int32_t index) {
        const std::vector<std::string>& values = GetStringList(name);
        if (values.empty()) return;

        const int32_t maximumIndex = static_cast<int32_t>(values.size()) - 1;
        Detail::Set<int32_t>(name, std::clamp(index, 0, maximumIndex));
    }

    inline void SetString(const std::string& name, const std::string& value) {
        const std::vector<std::string>& values = GetStringList(name);
        if (!values.empty()) {
            const auto it = std::find(values.begin(), values.end(), value);
            if (it != values.end()) {
                SetStringListIndex(name, static_cast<int32_t>(it - values.begin()));
            }
            return;
        }

        Detail::Set(name, value);
    }
}
