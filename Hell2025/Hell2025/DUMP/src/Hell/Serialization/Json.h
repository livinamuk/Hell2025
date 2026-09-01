#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include <string>

namespace nlohmann {
    void to_json(nlohmann::json& j, const glm::vec2& v);
    void to_json(nlohmann::json& j, const glm::vec3& v);

    void from_json(const nlohmann::json& j, glm::mat4& m);
    void from_json(const nlohmann::json& j, glm::quat& q);
    void from_json(const nlohmann::json& j, glm::vec2& v);
    void from_json(const nlohmann::json& j, glm::vec3& v);
}

namespace Hell::Json {
    bool LoadFromFile(nlohmann::json& json, const std::string& filepath);
    bool SaveToFile(const nlohmann::json& json, const std::string& filepath);
}
