#include "AssetLoader.h"

#include "Hell/Common/Enum.h"
#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Ragdoll/RagdollAsset.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Serialization/Json.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace Hell::AssetLoader {
    namespace {
        constexpr const char* FORMAT_NAME = "hell.ragdoll";
        constexpr uint32_t FORMAT_VERSION = 4;

        using Json = nlohmann::json;

        Json ToJson(const glm::vec3& value) {
            return Json::array({ value.x, value.y, value.z });
        }

        Json ToJson(const glm::vec4& value) {
            return Json::array({ value.x, value.y, value.z, value.w });
        }

        Json ToJson(const glm::mat4& value) {
            Json result = Json::array();
            const float* elements = glm::value_ptr(value);
            for (size_t index = 0; index < 16; index++) result.push_back(elements[index]);
            return result;
        }

        glm::vec3 ReadVec3(const Json& value) {
            const std::array<float, 3> elements = value.get<std::array<float, 3>>();
            return glm::vec3(elements[0], elements[1], elements[2]);
        }

        glm::vec4 ReadVec4(const Json& value) {
            const std::array<float, 4> elements = value.get<std::array<float, 4>>();
            return glm::vec4(elements[0], elements[1], elements[2], elements[3]);
        }

        glm::mat4 ReadMat4(const Json& value) {
            const std::array<float, 16> elements = value.get<std::array<float, 16>>();
            return glm::make_mat4(elements.data());
        }

        template <typename EnumType>
        Json EnumToJson(EnumType value) {
            return Hell::Enum::ToString(value);
        }

        template <typename EnumType>
        EnumType ReadEnum(const Json& object, const char* fieldName) {
            const std::string name = object.at(fieldName).get<std::string>();
            const std::vector<std::string> names = Hell::Enum::GetNames<EnumType>();
            if (std::find(names.begin(), names.end(), name) == names.end()) {
                throw std::runtime_error("Invalid value '" + name + "' for field '" + fieldName + "'");
            }
            return Hell::Enum::FromString(name, static_cast<EnumType>(0));
        }

        Json SerializeSolver(const RagdollSolverAsset& solver) {
            return {
                { "linearLimitStiffness", solver.linearLimitStiffness },
                { "linearLimitDamping", solver.linearLimitDamping },
                { "angularLimitStiffness", solver.angularLimitStiffness },
                { "angularLimitDamping", solver.angularLimitDamping }
            };
        }

        RagdollSolverAsset DeserializeSolver(const Json& json) {
            RagdollSolverAsset solver;
            solver.linearLimitStiffness = json.at("linearLimitStiffness").get<float>();
            solver.linearLimitDamping = json.at("linearLimitDamping").get<float>();
            solver.angularLimitStiffness = json.at("angularLimitStiffness").get<float>();
            solver.angularLimitDamping = json.at("angularLimitDamping").get<float>();
            return solver;
        }

        Json SerializeShape(const RagdollShape& shape) {
            Json vertices = Json::array();
            for (const glm::vec3& vertex : shape.convexVertices) vertices.push_back(ToJson(vertex));

            return {
                { "type", EnumToJson(shape.type) },
                { "extents", ToJson(shape.extents) },
                { "offset", ToJson(shape.offset) },
                { "rotationRadians", ToJson(shape.rotationRadians) },
                { "length", shape.length },
                { "radius", shape.radius },
                { "convexVertices", std::move(vertices) },
                { "convexIndices", shape.convexIndices }
            };
        }

        RagdollShape DeserializeShape(const Json& json) {
            RagdollShape shape;
            shape.type = ReadEnum<RagdollShapeType>(json, "type");
            shape.extents = ReadVec3(json.at("extents"));
            shape.offset = ReadVec3(json.at("offset"));
            shape.rotationRadians = ReadVec3(json.at("rotationRadians"));
            shape.length = json.at("length").get<float>();
            shape.radius = json.at("radius").get<float>();
            for (const Json& vertex : json.at("convexVertices")) shape.convexVertices.push_back(ReadVec3(vertex));
            shape.convexIndices = json.at("convexIndices").get<std::vector<uint32_t>>();
            return shape;
        }

        Json SerializeRigidBody(const RagdollRigidBodyAsset& rigidBody) {
            return {
                { "isKinematic", rigidBody.isKinematic },
                { "enableCCD", rigidBody.enableCCD },
                { "massMode", EnumToJson(rigidBody.massMode) },
                { "mass", rigidBody.mass },
                { "linearDamping", rigidBody.linearDamping },
                { "angularDamping", rigidBody.angularDamping },
                { "friction", rigidBody.friction },
                { "restitution", rigidBody.restitution },
                { "restitutionCombineMode", EnumToJson(rigidBody.restitutionCombineMode) },
                { "frictionCombineMode", EnumToJson(rigidBody.frictionCombineMode) },
                { "thickness", rigidBody.thickness },
                { "positionIterations", rigidBody.positionIterations },
                { "velocityIterations", rigidBody.velocityIterations },
                { "maxDepenetrationVelocity", rigidBody.maxDepenetrationVelocity }
            };
        }

        RagdollRigidBodyAsset DeserializeRigidBody(const Json& json) {
            RagdollRigidBodyAsset rigidBody;
            rigidBody.isKinematic = json.at("isKinematic").get<bool>();
            rigidBody.enableCCD = json.at("enableCCD").get<bool>();
            rigidBody.massMode = ReadEnum<RagdollMassMode>(json, "massMode");
            rigidBody.mass = json.at("mass").get<float>();
            rigidBody.linearDamping = json.at("linearDamping").get<float>();
            rigidBody.angularDamping = json.at("angularDamping").get<float>();
            rigidBody.friction = json.value("friction", 0.5f);
            rigidBody.restitution = json.at("restitution").get<float>();
            rigidBody.restitutionCombineMode = ReadEnum<RagdollCombineMode>(json, "restitutionCombineMode");
            rigidBody.frictionCombineMode = ReadEnum<RagdollCombineMode>(json, "frictionCombineMode");
            rigidBody.thickness = json.at("thickness").get<float>();
            rigidBody.positionIterations = json.at("positionIterations").get<uint32_t>();
            rigidBody.velocityIterations = json.at("velocityIterations").get<uint32_t>();
            rigidBody.maxDepenetrationVelocity = json.at("maxDepenetrationVelocity").get<float>();
            return rigidBody;
        }

        Json SerializeMarker(const RagdollMarkerAsset& marker) {
            return {
                { "id", marker.id },
                { "name", marker.name },
                { "bonePath", marker.bonePath },
                { "boneName", marker.boneName },
                { "inputTransform", ToJson(marker.inputTransform) },
                { "restTransform", ToJson(marker.restTransform) },
                { "bodyTransform", ToJson(marker.bodyTransform) },
                { "color", ToJson(marker.color) },
                { "shape", SerializeShape(marker.shape) },
                { "rigidBody", SerializeRigidBody(marker.rigidBody) }
            };
        }

        RagdollMarkerAsset DeserializeMarker(const Json& json) {
            RagdollMarkerAsset marker;
            marker.id = json.at("id").get<RagdollMarkerId>();
            marker.name = json.at("name").get<std::string>();
            marker.bonePath = json.at("bonePath").get<std::string>();
            marker.boneName = json.at("boneName").get<std::string>();
            marker.inputTransform = ReadMat4(json.at("inputTransform"));
            marker.restTransform = ReadMat4(json.at("restTransform"));
            marker.bodyTransform = ReadMat4(json.at("bodyTransform"));
            marker.color = ReadVec4(json.at("color"));
            marker.shape = DeserializeShape(json.at("shape"));
            marker.rigidBody = DeserializeRigidBody(json.at("rigidBody"));
            return marker;
        }

        Json SerializeAxisLimit(const RagdollAxisLimit& limit) {
            return { { "motion", EnumToJson(limit.motion) }, { "limit", limit.limit } };
        }

        RagdollAxisLimit DeserializeAxisLimit(const Json& json) {
            RagdollAxisLimit limit;
            limit.motion = ReadEnum<RagdollAxisMotion>(json, "motion");
            limit.limit = json.at("limit").get<float>();
            return limit;
        }

        Json SerializeJoint(const RagdollJointAsset& joint) {
            Json linearLimits = Json::array();
            Json angularLimits = Json::array();
            for (const RagdollAxisLimit& limit : joint.linearLimits) linearLimits.push_back(SerializeAxisLimit(limit));
            for (const RagdollAxisLimit& limit : joint.angularLimits) angularLimits.push_back(SerializeAxisLimit(limit));

            return {
                { "name", joint.name },
                { "parentMarkerId", joint.parentMarkerId },
                { "childMarkerId", joint.childMarkerId },
                { "parentFrame", ToJson(joint.parentFrame) },
                { "childFrame", ToJson(joint.childFrame) },
                { "linearLimits", std::move(linearLimits) },
                { "angularLimits", std::move(angularLimits) },
                { "limitEnabled", joint.limitEnabled },
                { "linearLimitStiffness", joint.linearLimitStiffness },
                { "linearLimitDamping", joint.linearLimitDamping },
                { "angularLimitStiffness", joint.angularLimitStiffness },
                { "angularLimitDamping", joint.angularLimitDamping }
            };
        }

        RagdollJointAsset DeserializeJoint(const Json& json) {
            RagdollJointAsset joint;
            joint.name = json.at("name").get<std::string>();
            joint.parentMarkerId = json.at("parentMarkerId").get<RagdollMarkerId>();
            joint.childMarkerId = json.at("childMarkerId").get<RagdollMarkerId>();
            joint.parentFrame = ReadMat4(json.at("parentFrame"));
            joint.childFrame = ReadMat4(json.at("childFrame"));

            const Json& linearLimits = json.at("linearLimits");
            const Json& angularLimits = json.at("angularLimits");
            if (!linearLimits.is_array() || linearLimits.size() != joint.linearLimits.size() || !angularLimits.is_array() || angularLimits.size() != joint.angularLimits.size()) {
                throw std::runtime_error("Joint limits must contain exactly three axes");
            }
            for (size_t axisIndex = 0; axisIndex < joint.linearLimits.size(); axisIndex++) joint.linearLimits[axisIndex] = DeserializeAxisLimit(linearLimits[axisIndex]);
            for (size_t axisIndex = 0; axisIndex < joint.angularLimits.size(); axisIndex++) joint.angularLimits[axisIndex] = DeserializeAxisLimit(angularLimits[axisIndex]);

            joint.limitEnabled = json.at("limitEnabled").get<bool>();
            joint.linearLimitStiffness = json.at("linearLimitStiffness").get<float>();
            joint.linearLimitDamping = json.at("linearLimitDamping").get<float>();
            joint.angularLimitStiffness = json.at("angularLimitStiffness").get<float>();
            joint.angularLimitDamping = json.at("angularLimitDamping").get<float>();
            return joint;
        }

        bool ValidateAsset(const RagdollAsset& asset, std::string& error) {
            if (!std::isfinite(asset.skinnedModelScale) || asset.skinnedModelScale <= 0.0f) {
                error = "Ragdoll skinned model scale must be greater than zero";
                return false;
            }

            std::unordered_set<RagdollMarkerId> markerIds;
            for (const RagdollMarkerAsset& marker : asset.markers) {
                if (marker.id == INVALID_RAGDOLL_MARKER_ID || !markerIds.insert(marker.id).second) {
                    error = "Ragdoll contains a missing or duplicate marker id";
                    return false;
                }
                if (marker.shape.type == RagdollShapeType::CONVEX_HULL && marker.shape.convexIndices.size() % 3 != 0) {
                    error = "Marker '" + marker.name + "' contains invalid convex hull indices";
                    return false;
                }
                for (uint32_t index : marker.shape.convexIndices) {
                    if (index >= marker.shape.convexVertices.size()) {
                        error = "Marker '" + marker.name + "' contains an out-of-range convex hull index";
                        return false;
                    }
                }
            }

            std::unordered_set<RagdollMarkerId> jointChildren;
            for (const RagdollJointAsset& joint : asset.joints) {
                if (markerIds.find(joint.parentMarkerId) == markerIds.end() || markerIds.find(joint.childMarkerId) == markerIds.end()) {
                    error = "Joint '" + joint.name + "' references a missing marker";
                    return false;
                }
                if (joint.parentMarkerId == joint.childMarkerId) {
                    error = "Joint '" + joint.name + "' connects a marker to itself";
                    return false;
                }
                if (!jointChildren.insert(joint.childMarkerId).second) {
                    error = "Marker " + std::to_string(joint.childMarkerId) + " has more than one parent joint";
                    return false;
                }
            }

            for (const RagdollMarkerAsset& marker : asset.markers) {
                std::unordered_set<RagdollMarkerId> ancestors;
                RagdollMarkerId ancestorId = marker.id;
                while (ancestorId != INVALID_RAGDOLL_MARKER_ID) {
                    if (!ancestors.insert(ancestorId).second) {
                        error = "Ragdoll marker hierarchy contains a cycle";
                        return false;
                    }

                    RagdollMarkerId parentId = INVALID_RAGDOLL_MARKER_ID;
                    for (const RagdollJointAsset& joint : asset.joints) {
                        if (joint.childMarkerId == ancestorId) {
                            parentId = joint.parentMarkerId;
                            break;
                        }
                    }
                    ancestorId = parentId;
                }
            }
            return true;
        }

        Json SerializeAsset(const RagdollAsset& asset) {
            Json markers = Json::array();
            Json joints = Json::array();
            for (const RagdollMarkerAsset& marker : asset.markers) markers.push_back(SerializeMarker(marker));
            for (const RagdollJointAsset& joint : asset.joints) joints.push_back(SerializeJoint(joint));

            return {
                { "format", FORMAT_NAME },
                { "version", FORMAT_VERSION },
                { "name", asset.name },
                { "skinnedModelPresetName", asset.skinnedModelPresetName },
                { "skinnedModelScale", asset.skinnedModelScale },
                { "testAnimationName", asset.testAnimationName },
                { "skeletonSignature", asset.skeletonSignature },
                { "targetMass", asset.targetMass },
                { "solver", SerializeSolver(asset.solver) },
                { "markers", std::move(markers) },
                { "joints", std::move(joints) }
            };
        }

        RagdollAsset DeserializeAsset(const Json& json) {
            if (json.at("format").get<std::string>() != FORMAT_NAME) throw std::runtime_error("Not a Hell ragdoll asset");
            const uint32_t version = json.at("version").get<uint32_t>();
            if (version != FORMAT_VERSION) throw std::runtime_error("Unsupported .ragdoll version " + std::to_string(version));

            RagdollAsset asset;
            asset.version = version;
            asset.name = json.at("name").get<std::string>();
            asset.skinnedModelPresetName = json.value("skinnedModelPresetName", std::string{});
            asset.skinnedModelScale = json.value("skinnedModelScale", 1.0f);
            asset.testAnimationName = json.value("testAnimationName", std::string{});
            asset.skeletonSignature = json.at("skeletonSignature").get<uint64_t>();
            asset.targetMass = json.at("targetMass").get<float>();
            asset.solver = DeserializeSolver(json.at("solver"));
            for (const Json& marker : json.at("markers")) asset.markers.push_back(DeserializeMarker(marker));
            for (const Json& joint : json.at("joints")) asset.joints.push_back(DeserializeJoint(joint));
            return asset;
        }
    }

    void LoadRagdollAssets() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/ragdolls", { "ragdoll" })) {
            RagdollAsset asset;
            std::string error;
            if (!LoadRagdollAsset(fileInfo.path, asset, error)) {
                Logging::Error() << "AssetLoader::LoadRagdollAssets() " << error << "\n";
                continue;
            }

            ResourceManager::CreateRagdollAsset(std::move(asset));
            AddLoadLogItem("Loaded " + fileInfo.path);
        }
    }

    bool LoadRagdollAsset(const std::string& path, RagdollAsset& asset, std::string& error) {
        Json json;
        if (!Hell::Json::LoadFromFile(json, path)) {
            error = "Failed to read '" + path + "'";
            return false;
        }

        try {
            RagdollAsset loadedAsset = DeserializeAsset(json);
            if (!ValidateAsset(loadedAsset, error)) return false;
            asset = std::move(loadedAsset);
            error.clear();
            return true;
        }
        catch (const std::exception& exception) {
            error = "Failed to load '" + path + "': " + exception.what();
            return false;
        }
    }

    bool SaveRagdollAsset(const std::string& path, const RagdollAsset& asset, std::string& error) {
        if (path.empty()) {
            error = "The ragdoll has no native save path";
            return false;
        }
        if (!ValidateAsset(asset, error)) return false;

        std::error_code fileSystemError;
        const std::filesystem::path parentPath = std::filesystem::path(path).parent_path();
        if (!parentPath.empty()) std::filesystem::create_directories(parentPath, fileSystemError);
        if (fileSystemError) {
            error = "Failed to create the ragdoll directory: " + fileSystemError.message();
            return false;
        }
        if (!Hell::Json::SaveToFile(SerializeAsset(asset), path)) {
            error = "Failed to save '" + path + "'";
            return false;
        }

        error.clear();
        return true;
    }
}
