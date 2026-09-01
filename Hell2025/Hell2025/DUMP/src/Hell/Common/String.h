#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <format>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace Hell::String {

    inline bool Equals(const char* queryA, const char* queryB) {
        if (queryA == queryB) {
            return true;
        }
        if (!queryA || !queryB) {
            return false;
        }
        return std::strcmp(queryA, queryB) == 0;
    }

    inline bool Equals(std::string_view queryA, std::string_view queryB) {
        return queryA == queryB;
    }

    inline std::string FormatFloat(float value, int precision = 3) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
        return std::string(buffer);
    }

    inline std::string FormatDouble(double value, int precision = 3) {
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
        return std::string(buffer);
    }

    inline std::string FormatBool(bool value) {
        return value ? "TRUE" : "FALSE";
    }

    inline std::string ToLower(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (unsigned char c : str) {
            result += static_cast<char>(std::tolower(c));
        }
        return result;
    }

    inline std::string ToUpper(std::string_view str) {
        std::string result;
        result.reserve(str.size());
        for (unsigned char c : str) {
            result += static_cast<char>(std::toupper(c));
        }
        return result;
    }

    inline std::string FormatVec2(const glm::vec2& value) {
        return std::format("({:.2f}, {:.2f})", value.x, value.y);
    }

    inline std::string FormatVec3(const glm::vec3& value) {
        return std::format("({:.2f}, {:.2f}, {:.2f})", value.x, value.y, value.z);
    }

    inline std::string FormatMat4(const glm::mat4& value, int precision = 2) {
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(precision);
        stream << value[0][0] << " " << value[1][0] << " " << value[2][0] << " " << value[3][0] << "\n";
        stream << value[0][1] << " " << value[1][1] << " " << value[2][1] << " " << value[3][1] << "\n";
        stream << value[0][2] << " " << value[1][2] << " " << value[2][2] << " " << value[3][2] << "\n";
        stream << value[0][3] << " " << value[1][3] << " " << value[2][3] << " " << value[3][3];
        return stream.str();
    }

    inline std::string FormatBytesMB(std::size_t bytes) {
        return std::format("{:.2f} MB", bytes / (1024.0 * 1024.0));
    }
}
