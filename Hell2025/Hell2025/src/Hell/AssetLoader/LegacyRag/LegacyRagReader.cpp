#include "LegacyRagReader.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Hell::AssetLoader::LegacyRag {
    namespace {
        using Json = nlohmann::ordered_json;

        const Json* FindMemberValue(const Json& component, const std::string& key) {
            if (!component.is_object()) return nullptr;

            const auto members = component.find("members");
            if (members == component.end() || !members->is_object()) return nullptr;

            const auto member = members->find(key);
            return member == members->end() ? nullptr : &*member;
        }

        const Json& RequireMemberValue(const Json& component, const std::string& key, const std::string& context) {
            const Json* value = FindMemberValue(component, key);
            if (!value) throw std::runtime_error(context + " member '" + key + "' is missing");
            return *value;
        }

        const Json& RequireWrappedValue(const Json& component, const std::string& key, const std::string& context) {
            const Json& member = RequireMemberValue(component, key, context);
            if (!member.is_object()) throw std::runtime_error(context + " member '" + key + "' is not an object");

            const auto value = member.find("value");
            if (value == member.end()) throw std::runtime_error(context + " member '" + key + "' has no wrapped value");
            return *value;
        }

        const Json& RequireWrappedValues(const Json& component, const std::string& key, const std::string& context) {
            const Json& member = RequireMemberValue(component, key, context);
            if (!member.is_object()) throw std::runtime_error(context + " member '" + key + "' is not an object");

            const auto values = member.find("values");
            if (values == member.end() || !values->is_array()) {
                throw std::runtime_error(context + " member '" + key + "' has no wrapped values array");
            }
            return *values;
        }

        void RequireNumberCount(const Json& values, size_t count, const std::string& key, const std::string& context) {
            if (values.size() != count) {
                throw std::runtime_error(context + " member '" + key + "' expected " + std::to_string(count) + " numbers");
            }
            for (const Json& value : values) {
                if (!value.is_number()) throw std::runtime_error(context + " member '" + key + "' contains a non-number");
            }
        }

        int ReadIntValue(const Json& value, const std::string& key, const std::string& context) {
            if (!value.is_number_integer()) throw std::runtime_error(context + " member '" + key + "' is not an integer");

            if (value.is_number_unsigned()) {
                const uint64_t integer = value.get<uint64_t>();
                if (integer > static_cast<uint64_t>(std::numeric_limits<int>::max())) {
                    throw std::runtime_error(context + " member '" + key + "' is outside the integer range");
                }
                return static_cast<int>(integer);
            }

            const int64_t integer = value.get<int64_t>();
            if (integer < std::numeric_limits<int>::min() || integer > std::numeric_limits<int>::max()) {
                throw std::runtime_error(context + " member '" + key + "' is outside the integer range");
            }
            return static_cast<int>(integer);
        }

        uint32_t ReadUintValue(const Json& value, const std::string& key, const std::string& context) {
            if (!value.is_number_integer()) throw std::runtime_error(context + " member '" + key + "' is not an unsigned integer");

            uint64_t integer = 0;
            if (value.is_number_unsigned()) {
                integer = value.get<uint64_t>();
            }
            else {
                const int64_t signedInteger = value.get<int64_t>();
                if (signedInteger < 0) throw std::runtime_error(context + " member '" + key + "' is not an unsigned integer");
                integer = static_cast<uint64_t>(signedInteger);
            }

            if (integer > std::numeric_limits<uint32_t>::max()) {
                throw std::runtime_error(context + " member '" + key + "' is outside the unsigned integer range");
            }
            return static_cast<uint32_t>(integer);
        }
    }

    ComponentView::ComponentView(const nlohmann::ordered_json& value, std::string context)
        : m_value(&value), m_context(std::move(context)) {
    }

    bool ComponentView::Has(const std::string& key) const {
        return m_value && FindMemberValue(*m_value, key);
    }

    double ComponentView::ReadDouble(const std::string& key, double defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        if (!value) return defaultValue;
        if (!value->is_number()) throw std::runtime_error(m_context + " member '" + key + "' is not a number");
        return value->get<double>();
    }

    float ComponentView::ReadFloat(const std::string& key, float defaultValue) const {
        return static_cast<float>(ReadDouble(key, defaultValue));
    }

    bool ComponentView::ReadBool(const std::string& key, bool defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        if (!value) return defaultValue;
        if (!value->is_boolean()) throw std::runtime_error(m_context + " member '" + key + "' is not a boolean");
        return value->get<bool>();
    }

    std::string ComponentView::ReadString(const std::string& key, const std::string& defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        if (!value) return defaultValue;
        if (!value->is_string()) throw std::runtime_error(m_context + " member '" + key + "' is not a string");
        return value->get<std::string>();
    }

    std::vector<std::string> ComponentView::ReadStrings(const std::string& key, const std::vector<std::string>& defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        if (!value) return defaultValue;
        if (value->is_string()) return { value->get<std::string>() };
        if (!value->is_array()) throw std::runtime_error(m_context + " member '" + key + "' is not a string list");

        std::vector<std::string> strings;
        strings.reserve(value->size());
        for (const Json& item : *value) {
            if (!item.is_string()) throw std::runtime_error(m_context + " member '" + key + "' contains a non-string");
            strings.emplace_back(item.get<std::string>());
        }
        return strings.empty() ? defaultValue : strings;
    }

    int ComponentView::ReadInt(const std::string& key, int defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        return value ? ReadIntValue(*value, key, m_context) : defaultValue;
    }

    uint32_t ComponentView::ReadUint(const std::string& key, uint32_t defaultValue) const {
        const Json* value = FindMemberValue(*m_value, key);
        return value ? ReadUintValue(*value, key, m_context) : defaultValue;
    }

    glm::dmat4 ComponentView::ReadMatrix(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        RequireNumberCount(values, 16, key, m_context);

        glm::dmat4 matrix(1.0);
        for (size_t column = 0; column < 4; column++) {
            for (size_t row = 0; row < 4; row++) {
                matrix[column][row] = values[column * 4 + row].get<double>();
            }
        }
        return matrix;
    }

    glm::dquat ComponentView::ReadQuaternion(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        RequireNumberCount(values, 4, key, m_context);

        glm::dquat quaternion;
        quaternion.x = values[0].get<double>();
        quaternion.y = values[1].get<double>();
        quaternion.z = values[2].get<double>();
        quaternion.w = values[3].get<double>();
        return quaternion;
    }

    glm::dvec3 ComponentView::ReadVector(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        RequireNumberCount(values, 3, key, m_context);
        return { values[0].get<double>(), values[1].get<double>(), values[2].get<double>() };
    }

    glm::vec3 ComponentView::ReadVectorF(const std::string& key) const {
        return glm::vec3(ReadVector(key));
    }

    std::vector<glm::dvec3> ComponentView::ReadPoints(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        if (values.size() % 3 != 0) throw std::runtime_error(m_context + " member '" + key + "' has an invalid point count");
        for (const Json& value : values) {
            if (!value.is_number()) throw std::runtime_error(m_context + " member '" + key + "' contains a non-number");
        }

        std::vector<glm::dvec3> points;
        points.reserve(values.size() / 3);
        for (size_t index = 0; index < values.size(); index += 3) {
            points.emplace_back(values[index].get<double>(), values[index + 1].get<double>(), values[index + 2].get<double>());
        }
        return points;
    }

    std::vector<uint32_t> ComponentView::ReadUints(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        std::vector<uint32_t> integers;
        integers.reserve(values.size());
        for (const Json& value : values) {
            integers.push_back(ReadUintValue(value, key, m_context));
        }
        return integers;
    }

    glm::vec4 ComponentView::ReadColor(const std::string& key) const {
        const Json& values = RequireWrappedValues(*m_value, key, m_context);
        RequireNumberCount(values, 4, key, m_context);
        return {
            static_cast<float>(values[0].get<double>()),
            static_cast<float>(values[1].get<double>()),
            static_cast<float>(values[2].get<double>()),
            static_cast<float>(values[3].get<double>())
        };
    }

    std::string ComponentView::ReadEntity(const std::string& key) const {
        const Json& value = RequireWrappedValue(*m_value, key, m_context);
        return std::to_string(ReadUintValue(value, key, m_context));
    }

    std::string ComponentView::ReadPath(const std::string& key) const {
        const Json& value = RequireWrappedValue(*m_value, key, m_context);
        if (!value.is_string()) throw std::runtime_error(m_context + " member '" + key + "' is not a path");
        return value.get<std::string>();
    }

    std::string ComponentView::ReadData() const {
        if (!m_value->is_object()) throw std::runtime_error(m_context + " is not an object");
        const auto data = m_value->find("data");
        if (data == m_value->end() || !data->is_string()) throw std::runtime_error(m_context + " has no string data");
        return data->get<std::string>();
    }

    Reader::Reader(const nlohmann::ordered_json& document)
        : m_document(&document) {
    }

    std::vector<std::string> Reader::GetEntityNames() const {
        if (!m_document || !m_document->is_object()) throw std::runtime_error("Legacy ragdoll document is not an object");

        const auto entities = m_document->find("entities");
        if (entities == m_document->end() || !entities->is_object()) throw std::runtime_error("Legacy ragdoll document has no entities object");

        std::vector<std::string> names;
        names.reserve(entities->size());
        for (auto entity = entities->cbegin(); entity != entities->cend(); ++entity) {
            names.push_back(entity.key());
        }
        return names;
    }

    bool Reader::HasEntity(const std::string& entity) const {
        if (!m_document || !m_document->is_object()) return false;

        const auto entities = m_document->find("entities");
        return entities != m_document->end() && entities->is_object() && entities->find(entity) != entities->end();
    }

    bool Reader::HasComponent(const std::string& entity, const std::string& component) const {
        if (!m_document || !m_document->is_object()) return false;

        const auto entities = m_document->find("entities");
        if (entities == m_document->end() || !entities->is_object()) return false;

        const auto entityValue = entities->find(entity);
        if (entityValue == entities->end() || !entityValue->is_object()) return false;

        const auto components = entityValue->find("components");
        return components != entityValue->end() &&
               components->is_object() &&
               components->find(component) != components->end();
    }

    ComponentView Reader::GetComponent(const std::string& entity, const std::string& component) const {
        if (!m_document || !m_document->is_object()) throw std::runtime_error("Legacy ragdoll document is not an object");

        const auto entities = m_document->find("entities");
        if (entities == m_document->end() || !entities->is_object()) throw std::runtime_error("Legacy ragdoll document has no entities object");

        const auto entityValue = entities->find(entity);
        if (entityValue == entities->end() || !entityValue->is_object()) throw std::runtime_error("Legacy ragdoll entity '" + entity + "' is missing");

        const auto components = entityValue->find("components");
        if (components == entityValue->end() || !components->is_object()) throw std::runtime_error("Legacy ragdoll entity '" + entity + "' has no components object");

        const auto componentValue = components->find(component);
        if (componentValue == components->end()) throw std::runtime_error("Legacy ragdoll entity '" + entity + "' has no component '" + component + "'");

        return ComponentView(*componentValue, "Legacy ragdoll entity '" + entity + "' component '" + component + "'");
    }
}
