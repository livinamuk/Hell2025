#include "Json.h"

#include "Hell/Logging.h"

#include <glm/gtc/type_ptr.hpp>

#include <array>
#include <fstream>
#include <sstream>

namespace nlohmann {
    void to_json(nlohmann::json& j, const glm::vec2& v) {
        j = json::array({ v.x, v.y });
    }

    void to_json(nlohmann::json& j, const glm::vec3& v) {
        j = json::array({ v.x, v.y, v.z });
    }

    void from_json(const nlohmann::json& j, glm::mat4& m) {
        std::array<float, 16> a = j.get<std::array<float, 16>>();
        m = glm::make_mat4(a.data());
    }

    void from_json(const nlohmann::json& j, glm::quat& q) {
        if (j.is_array() && j.size() == 4) {
            auto a = j.get<std::array<float, 4>>();
            q = glm::quat{ a[3], a[0], a[1], a[2] };
        }
        else {
            q = glm::quat{ 1.0f, 0.0f, 0.0f, 0.0f };
        }
    }

    void from_json(const nlohmann::json& j, glm::vec2& v) {
        try {
            std::array<float, 2> arr = j.get<std::array<float, 2>>();
            v = glm::vec2(arr[0], arr[1]);
        }
        catch (const nlohmann::json::exception&) {
            v = glm::vec2(0.0f, 0.0f);
        }
    }

    void from_json(const nlohmann::json& j, glm::vec3& v) {
        try {
            std::array<float, 3> arr = j.get<std::array<float, 3>>();
            v = glm::vec3(arr[0], arr[1], arr[2]);
        }
        catch (const nlohmann::json::exception&) {
            v = glm::vec3(0.0f, 0.0f, 0.0f);
        }
    }
}

namespace Hell::Json {
    bool LoadFromFile(nlohmann::json& json, const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file) {
            Logging::Error() << "Hell::Json::LoadFromFile(..) failed to open file: " << filepath << "\n";
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        try {
            json = nlohmann::json::parse(buffer.str());
            return true;
        }
        catch (const nlohmann::json::parse_error& e) {
            Logging::Error() << "Hell::Json::LoadFromFile(..) failed to parse file: " << filepath << ": " << e.what() << "\n";
            return false;
        }
    }

    bool SaveToFile(const nlohmann::json& json, const std::string& filepath) {
        std::ofstream file(filepath);
        if (!file) {
            Logging::Error() << "Hell::Json::SaveToFile(..) failed to open file: " << filepath << "\n";
            return false;
        }

        file << json.dump(4);
        file.close();

        Logging::Debug() << "Saved " << filepath << "\n";
        return true;
    }
}
