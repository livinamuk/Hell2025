#pragma once

#include <nlohmann/json_fwd.hpp>

#include <cstdint>
#include <glm/mat4x4.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <vector>

namespace Hell::AssetLoader::LegacyRag {

    class ComponentView {
    public:
        bool Has(const std::string& key) const;

        double ReadDouble(const std::string& key, double defaultValue = 0.0) const;
        float ReadFloat(const std::string& key, float defaultValue = 0.0f) const;
        bool ReadBool(const std::string& key, bool defaultValue = false) const;
        std::string ReadString(const std::string& key, const std::string& defaultValue = {}) const;
        std::vector<std::string> ReadStrings(const std::string& key, const std::vector<std::string>& defaultValue = {}) const;
        int ReadInt(const std::string& key, int defaultValue = 0) const;
        uint32_t ReadUint(const std::string& key, uint32_t defaultValue = 0) const;
        glm::dmat4 ReadMatrix(const std::string& key) const;
        glm::dquat ReadQuaternion(const std::string& key) const;
        glm::dvec3 ReadVector(const std::string& key) const;
        glm::vec3 ReadVectorF(const std::string& key) const;
        std::vector<glm::dvec3> ReadPoints(const std::string& key) const;
        std::vector<uint32_t> ReadUints(const std::string& key) const;
        glm::vec4 ReadColor(const std::string& key) const;
        std::string ReadEntity(const std::string& key) const;
        std::string ReadPath(const std::string& key) const;
        std::string ReadData() const;

    private:
        friend class Reader;
        ComponentView(const nlohmann::ordered_json& value, std::string context);

        const nlohmann::ordered_json* m_value = nullptr;
        std::string m_context;
    };

    class Reader {
    public:
        explicit Reader(const nlohmann::ordered_json& document);

        std::vector<std::string> GetEntityNames() const;
        bool HasEntity(const std::string& entity) const;
        bool HasComponent(const std::string& entity, const std::string& component) const;
        ComponentView GetComponent(const std::string& entity, const std::string& component) const;

    private:
        const nlohmann::ordered_json* m_document = nullptr;
    };
}
