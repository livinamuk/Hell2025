#include "LegacyRagdollImporter.h"
#include "LegacyRagReader.h"

#include "Hell/Common/Constants.h"
#include "Hell/File.h"
#include "Hell/Math/Matrix.h"
#include "Hell/Math/Rotation.h"
#include "Hell/Physics/Ragdoll/RagdollAsset.h"
#include "Hell/Physics/Ragdoll/RagdollMass.h"

#include <nlohmann/json.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Hell::AssetLoader {
    namespace {
        enum class LegacyRagdollBehavior : uint8_t {
            INHERIT,
            KINEMATIC,
            DYNAMIC
        };

        enum class LegacyRagdollMotion : uint8_t {
            INHERIT,
            LOCKED,
            LIMITED,
            FREE
        };

        struct LegacyRagdollGroupImportData {
            bool enabled = true;
            LegacyRagdollBehavior inputType = LegacyRagdollBehavior::DYNAMIC;
            LegacyRagdollMotion linearMotion = LegacyRagdollMotion::LOCKED;
        };

        struct LegacyRagdollMarkerImportData {
            int32_t groupIndex = -1;
            LegacyRagdollBehavior inputType = LegacyRagdollBehavior::INHERIT;
            LegacyRagdollMotion linearMotion = LegacyRagdollMotion::LOCKED;
            float density = 1.0f;
            float limitStiffness = 1.0f;
            float limitDampingRatio = 1.0f;
            glm::dmat4 restTransform = glm::dmat4(1.0);
            glm::dvec3 scale = glm::dvec3(1.0);
            glm::dvec3 rotatePivot = glm::dvec3(0.0);

            LegacyRagdollMotion resolvedLinearMotion = LegacyRagdollMotion::LOCKED;
        };

        struct LegacyRagdollImportData {
            std::unordered_map<std::string, size_t> groupIndices;
            std::unordered_map<std::string, size_t> markerIndices;
            std::vector<LegacyRagdollGroupImportData> groups;
            std::vector<LegacyRagdollMarkerImportData> markers;
            float physicsScale = 1.0f;
        };

        LegacyRagdollBehavior ParseBehavior(const std::string& value) {
            if (value == "Inherit") return LegacyRagdollBehavior::INHERIT;
            if (value == "Kinematic") return LegacyRagdollBehavior::KINEMATIC;
            return LegacyRagdollBehavior::DYNAMIC;
        }

        LegacyRagdollMotion ParseMotion(const std::string& value) {
            if (value == "Inherit") return LegacyRagdollMotion::INHERIT;
            if (value == "Limited") return LegacyRagdollMotion::LIMITED;
            if (value == "Free") return LegacyRagdollMotion::FREE;
            return LegacyRagdollMotion::LOCKED;
        }

        RagdollShapeType ParseShapeType(const std::string& value) {
            if (value == "Box") return RagdollShapeType::BOX;
            if (value == "Capsule") return RagdollShapeType::CAPSULE;
            if (value == "ConvexHull") return RagdollShapeType::CONVEX_HULL;
            return RagdollShapeType::SPHERE;
        }

        RagdollCombineMode ParseCombineMode(const std::string& value) {
            if (value == "Average" || value == "kAverage") return RagdollCombineMode::AVERAGE;
            if (value == "Min" || value == "kMin") return RagdollCombineMode::MINIMUM;
            if (value == "Max" || value == "kMax") return RagdollCombineMode::MAXIMUM;
            return RagdollCombineMode::MULTIPLY;
        }

        RagdollAxisLimit MakeAxisLimit(float value) {
            RagdollAxisLimit limit;
            if (value > 0.0f) {
                limit.motion = RagdollAxisMotion::LIMITED;
                limit.limit = value;
            }
            else if (value < 0.0f) {
                limit.motion = RagdollAxisMotion::LOCKED;
            }
            else {
                limit.motion = RagdollAxisMotion::FREE;
            }
            return limit;
        }

        glm::vec3 ToVec3(const glm::dvec3& value) {
            return glm::vec3(value);
        }

        glm::mat4 ToMat4(const glm::dmat4& value) {
            return glm::mat4(value);
        }

        glm::quat ToQuat(const glm::dquat& value) {
            return glm::quat(
                static_cast<float>(value.w),
                static_cast<float>(value.x),
                static_cast<float>(value.y),
                static_cast<float>(value.z)
            );
        }

        glm::vec3 DescalePoint(const glm::dvec3& point, const glm::dvec3& scale) {
            const auto inverseScaleOrOne = [](double value) {
                return value == 0.0 ? 1.0 : 1.0 / value;
            };
            return glm::vec3(
                point.x * inverseScaleOrOne(scale.x),
                point.y * inverseScaleOrOne(scale.y),
                point.z * inverseScaleOrOne(scale.z)
            );
        }

        void BakeLegacyShapeScale(RagdollShape& shape, const glm::vec3& scale, const glm::vec3& absoluteScale, bool capsuleLengthAlongY) {
            shape.offset *= scale;

            switch (shape.type) {
                case RagdollShapeType::BOX:
                    shape.extents *= absoluteScale;
                    break;
                case RagdollShapeType::SPHERE:
                    shape.radius *= absoluteScale.x;
                    shape.length = 0.0f;
                    shape.extents = glm::vec3(shape.radius * 2.0f);
                    break;
                case RagdollShapeType::CAPSULE: {
                    const float lengthScale = capsuleLengthAlongY ? absoluteScale.y : absoluteScale.x;
                    const float radiusScale = capsuleLengthAlongY ? absoluteScale.x : absoluteScale.y;
                    shape.length *= lengthScale;
                    shape.radius *= radiusScale;
                    shape.extents = glm::vec3(shape.length + shape.radius * 2.0f, shape.radius * 2.0f, shape.radius * 2.0f);
                    break;
                }
                case RagdollShapeType::CONVEX_HULL:
                    break;
            }
        }

        std::string CleanLegacyString(const std::string& value) {
            return value == UNDEFINED_STRING ? std::string() : value;
        }

        void NormalizeBonePath(std::string& path) {
            const auto notSpace = [](unsigned char character) { return !std::isspace(character); };
            path.erase(path.begin(), std::find_if(path.begin(), path.end(), notSpace));
            path.erase(std::find_if(path.rbegin(), path.rend(), notSpace).base(), path.end());

            for (char& character : path) {
                if (character == '/') character = '|';
            }
            path.erase(std::remove(path.begin(), path.end(), '\r'), path.end());

            while (!path.empty() && path.front() == '|') path.erase(path.begin());
            while (!path.empty() && path.back() == '|') path.pop_back();
        }

        std::string GetBoneName(const std::string& bonePath) {
            if (bonePath.empty()) return {};
            const size_t position = bonePath.rfind('|');
            return position == std::string::npos ? bonePath : bonePath.substr(position + 1);
        }

        void LoadSolver(RagdollAsset& asset, LegacyRagdollImportData& importData, const LegacyRag::Reader& reader) {
            for (const std::string& entity : reader.GetEntityNames()) {
                if (!reader.HasComponent(entity, "SolverUIComponent")) continue;

                const LegacyRag::ComponentView solverComponent = reader.GetComponent(entity, "SolverComponent");
                const LegacyRag::ComponentView solverUI = reader.GetComponent(entity, "SolverUIComponent");
                RagdollSolverAsset& solver = asset.solver;

                const float physicsScale = solverComponent.Has("sceneScale")
                    ? solverComponent.ReadFloat("sceneScale")
                    : solverComponent.ReadFloat("spaceMultiplier", 1.0f);
                importData.physicsScale = std::isfinite(physicsScale) && physicsScale > 0.0f ? physicsScale : 1.0f;

                solver.linearLimitStiffness = solverUI.ReadFloat("linearLimitStiffness", solver.linearLimitStiffness);
                solver.linearLimitDamping = solverUI.ReadFloat("linearLimitDamping", solver.linearLimitDamping);
                solver.angularLimitStiffness = solverUI.ReadFloat("angularLimitStiffness", solver.angularLimitStiffness);
                solver.angularLimitDamping = solverUI.ReadFloat("angularLimitDamping", solver.angularLimitDamping);
                return;
            }
        }

        void LoadGroups(LegacyRagdollImportData& importData, const LegacyRag::Reader& reader) {
            for (const std::string& entity : reader.GetEntityNames()) {
                if (!reader.HasComponent(entity, "GroupUIComponent")) continue;
                if (reader.HasComponent(entity, "SolverComponent")) continue;

                const LegacyRag::ComponentView groupUI = reader.GetComponent(entity, "GroupUIComponent");
                LegacyRagdollGroupImportData group;
                group.enabled = groupUI.ReadBool("enabled", true);
                group.inputType = ParseBehavior(groupUI.ReadString("inputType", "Inherit"));
                group.linearMotion = ParseMotion(groupUI.ReadString("linearMotion", "Locked"));

                importData.groupIndices.emplace(entity, importData.groups.size());
                importData.groups.push_back(group);
            }
        }

        void LoadMarkers(
            RagdollAsset& asset,
            LegacyRagdollImportData& importData,
            const LegacyRag::Reader& reader,
            std::vector<std::string>& warnings
        ) {
            for (const std::string& entity : reader.GetEntityNames()) {
                if (reader.HasComponent(entity, "SolverComponent")) continue;
                if (!reader.HasComponent(entity, "MarkerUIComponent")) continue;

                const LegacyRag::ComponentView rigid = reader.GetComponent(entity, "RigidComponent");
                if (rigid.ReadBool("kinematic", false)) {
                    // Old files may contain the solver ground plane as a marker.
                    continue;
                }

                const LegacyRag::ComponentView color = reader.GetComponent(entity, "ColorComponent");
                const LegacyRag::ComponentView convexMesh = reader.GetComponent(entity, "ConvexMeshComponents");
                const LegacyRag::ComponentView geometry = reader.GetComponent(entity, "GeometryDescriptionComponent");
                const LegacyRag::ComponentView markerUI = reader.GetComponent(entity, "MarkerUIComponent");
                const LegacyRag::ComponentView nameComponent = reader.GetComponent(entity, "NameComponent");
                const LegacyRag::ComponentView restComponent = reader.GetComponent(entity, "RestComponent");
                const LegacyRag::ComponentView scaleComponent = reader.GetComponent(entity, "ScaleComponent");

                const glm::dmat4 restTransform = restComponent.ReadMatrix("matrix");
                const glm::dvec3 scale = scaleComponent.ReadVector("value");
                const glm::dvec3 absoluteScale = scaleComponent.ReadVector("absolute");
                const glm::dvec3 rotatePivot = scale * (markerUI.Has("rotatePivot")
                    ? markerUI.ReadVector("rotatePivot")
                    : glm::dvec3(0.0));
                glm::dmat4 bodyTransform = restTransform;
                if (reader.HasComponent(entity, "OriginComponent")) {
                    bodyTransform = reader.GetComponent(entity, "OriginComponent").ReadMatrix("matrix");
                }

                RagdollMarkerAsset marker;
                marker.id = static_cast<RagdollMarkerId>(asset.markers.size() + 1);
                marker.name = CleanLegacyString(nameComponent.ReadString("value"));
                marker.inputTransform = ToMat4(glm::scale(restTransform, scale));
                marker.restTransform = ToMat4(restTransform);
                marker.bodyTransform = ToMat4(bodyTransform);
                marker.color = color.ReadColor("value");

                RagdollShape& shape = marker.shape;
                shape.type = ParseShapeType(geometry.ReadString("type"));
                shape.extents = ToVec3(geometry.ReadVector("extents"));
                shape.offset = ToVec3(geometry.ReadVector("offset"));
                shape.rotationRadians = Hell::Math::QuaternionToEulerXYZ(ToQuat(geometry.ReadQuaternion("rotation")));
                shape.radius = geometry.ReadFloat("radius");
                shape.length = geometry.ReadFloat("length");
                const bool capsuleLengthAlongY = geometry.ReadBool("capsuleLengthAlongY", true);
                BakeLegacyShapeScale(shape, ToVec3(scale), ToVec3(absoluteScale), capsuleLengthAlongY);

                const std::vector<glm::dvec3> convexVertices = convexMesh.ReadPoints("vertices");
                const std::vector<uint32_t> convexIndices = convexMesh.ReadUints("indices");
                if (shape.type == RagdollShapeType::CONVEX_HULL) {
                    shape.convexVertices.reserve(convexVertices.size());
                    for (const glm::dvec3& vertex : convexVertices) {
                        shape.convexVertices.push_back(DescalePoint(vertex, scale));
                    }
                    shape.convexIndices = convexIndices;
                }

                RagdollRigidBodyAsset& rigidBody = marker.rigidBody;
                rigidBody.enableCCD = rigid.ReadBool("enableCCD", false);
                rigidBody.mass = markerUI.ReadFloat("mass");
                rigidBody.linearDamping = rigid.ReadFloat("linearDamping", rigidBody.linearDamping);
                rigidBody.angularDamping = rigid.ReadFloat("angularDamping", rigidBody.angularDamping);
                rigidBody.friction = rigid.ReadFloat("friction", rigidBody.friction);
                rigidBody.restitution = rigid.ReadFloat("restitution", rigidBody.restitution);
                rigidBody.restitutionCombineMode = ParseCombineMode(rigid.ReadString("contactCombineMode", "Multiply"));
                rigidBody.frictionCombineMode = ParseCombineMode(rigid.ReadString("frictionCombineMode", "Multiply"));
                rigidBody.thickness = std::max(0.0f, rigid.ReadFloat("thickness", rigidBody.thickness));
                rigidBody.positionIterations = static_cast<uint32_t>(std::clamp(rigid.ReadInt("positionIterations", static_cast<int>(rigidBody.positionIterations)), 0, 255));
                rigidBody.velocityIterations = static_cast<uint32_t>(std::clamp(rigid.ReadInt("velocityIterations", static_cast<int>(rigidBody.velocityIterations)), 0, 255));
                rigidBody.maxDepenetrationVelocity = rigid.ReadFloat("maxDepenetrationVelocity", rigidBody.maxDepenetrationVelocity);

                LegacyRagdollMarkerImportData markerImport;
                markerImport.density = rigid.ReadFloat("densityCustom", markerImport.density);
                markerImport.restTransform = restTransform;
                markerImport.scale = scale;
                markerImport.rotatePivot = rotatePivot;
                markerImport.inputType = ParseBehavior(markerUI.ReadString("inputType", "Inherit"));
                markerImport.linearMotion = ParseMotion(markerUI.ReadString("linearMotion", "Locked"));
                markerImport.limitStiffness = markerUI.ReadFloat("limitStiffness", markerImport.limitStiffness);
                markerImport.limitDampingRatio = markerUI.ReadFloat("limitDampingRatio", markerImport.limitDampingRatio);
                if (reader.HasComponent(entity, "GroupComponent")) {
                    const std::string groupEntity = reader.GetComponent(entity, "GroupComponent").ReadEntity("entity");
                    const auto group = importData.groupIndices.find(groupEntity);
                    if (group != importData.groupIndices.end()) {
                        markerImport.groupIndex = static_cast<int32_t>(group->second);
                    }
                }

                for (std::string bonePath : markerUI.ReadStrings("destinationTransforms")) {
                    NormalizeBonePath(bonePath);
                    if (bonePath.empty()) continue;
                    marker.bonePath = bonePath;
                    marker.boneName = GetBoneName(bonePath);
                    break;
                }

                const size_t markerIndex = asset.markers.size();
                const bool insertedIndex = importData.markerIndices.emplace(entity, markerIndex).second;
                if (!insertedIndex) {
                    warnings.push_back("Duplicate legacy marker id " + entity);
                }
                if (marker.boneName.empty()) {
                    const std::string markerLabel = marker.name.empty() ? std::to_string(marker.id) : marker.name;
                    warnings.push_back("Marker " + markerLabel + " has no destination bone");
                }

                asset.markers.push_back(std::move(marker));
                importData.markers.push_back(markerImport);
            }
        }

        void ResolveMarkerSettings(RagdollAsset& asset, LegacyRagdollImportData& importData) {
            for (size_t markerIndex = 0; markerIndex < asset.markers.size(); markerIndex++) {
                RagdollMarkerAsset& marker = asset.markers[markerIndex];
                LegacyRagdollMarkerImportData& markerImport = importData.markers[markerIndex];

                LegacyRagdollBehavior inputType = markerImport.inputType;
                LegacyRagdollMotion linearMotion = markerImport.linearMotion;

                const bool hasEnabledGroup = markerImport.groupIndex >= 0 &&
                    markerImport.groupIndex < static_cast<int32_t>(importData.groups.size()) &&
                    importData.groups[markerImport.groupIndex].enabled;
                if (hasEnabledGroup) {
                    const LegacyRagdollGroupImportData& group = importData.groups[markerImport.groupIndex];
                    if (linearMotion == LegacyRagdollMotion::INHERIT) linearMotion = group.linearMotion;
                    if (inputType == LegacyRagdollBehavior::INHERIT) inputType = group.inputType;
                }
                else {
                    inputType = inputType == LegacyRagdollBehavior::KINEMATIC
                        ? LegacyRagdollBehavior::KINEMATIC
                        : LegacyRagdollBehavior::DYNAMIC;
                }

                if (inputType == LegacyRagdollBehavior::INHERIT) inputType = LegacyRagdollBehavior::DYNAMIC;
                if (linearMotion == LegacyRagdollMotion::INHERIT) linearMotion = LegacyRagdollMotion::LOCKED;

                markerImport.resolvedLinearMotion = linearMotion;
                marker.rigidBody.isKinematic = inputType == LegacyRagdollBehavior::KINEMATIC;
            }
        }

        void ReconstructLegacyJointFrames(
            const LegacyRagdollMarkerImportData& parentMarker,
            const LegacyRagdollMarkerImportData& childMarker,
            glm::dmat4& parentFrame,
            glm::dmat4& childFrame
        ) {
            const glm::dvec3 pivot = childMarker.rotatePivot / childMarker.scale;
            glm::dmat4 pivotTransform(1.0);
            pivotTransform[3] = glm::dvec4(pivot, 1.0);

            const glm::dmat4 parentPosition =
                glm::inverse(parentMarker.restTransform) *
                childMarker.restTransform *
                pivotTransform;

            parentFrame[3] = glm::dvec4(glm::dvec3(parentPosition[3]), 1.0);
            childFrame[3] = glm::dvec4(pivot, 1.0);
        }

        void LoadJoints(
            RagdollAsset& asset,
            const LegacyRagdollImportData& importData,
            const LegacyRag::Reader& reader,
            std::vector<std::string>& warnings
        ) {
            size_t convertedDrivenLinearJointCount = 0;

            for (const std::string& childEntity : reader.GetEntityNames()) {
                if (!reader.HasComponent(childEntity, "MarkerUIComponent")) continue;

                std::string parentEntity = "-1";
                if (reader.HasComponent(childEntity, "ParentComponent")) {
                    parentEntity = reader.GetComponent(childEntity, "ParentComponent").ReadEntity("entity");
                }
                else if (reader.HasComponent(childEntity, "RigidComponent")) {
                    parentEntity = reader.GetComponent(childEntity, "RigidComponent").ReadEntity("parentRigid");
                }
                if (!reader.HasEntity(parentEntity)) continue;
                if (!reader.HasComponent(childEntity, "SubEntitiesComponent")) continue;

                const std::string jointEntity = reader.GetComponent(childEntity, "SubEntitiesComponent").ReadEntity("relative");
                if (!reader.HasEntity(jointEntity)) continue;

                const auto childMarkerEntry = importData.markerIndices.find(childEntity);
                const auto parentMarkerEntry = importData.markerIndices.find(parentEntity);
                if (childMarkerEntry == importData.markerIndices.end() || parentMarkerEntry == importData.markerIndices.end()) {
                    warnings.push_back("Skipped unresolved joint " + childEntity + "_to_" + parentEntity);
                    continue;
                }

                const size_t childMarkerIndex = childMarkerEntry->second;
                const size_t parentMarkerIndex = parentMarkerEntry->second;
                const RagdollMarkerAsset& childMarker = asset.markers[childMarkerIndex];
                const RagdollMarkerAsset& parentMarker = asset.markers[parentMarkerIndex];
                const LegacyRagdollMarkerImportData& childImport = importData.markers[childMarkerIndex];
                const LegacyRagdollMarkerImportData& parentImport = importData.markers[parentMarkerIndex];

                const LegacyRag::ComponentView markerUI = reader.GetComponent(childEntity, "MarkerUIComponent");
                const LegacyRag::ComponentView limit = reader.GetComponent(jointEntity, "LimitComponent");
                const LegacyRag::ComponentView drive = reader.GetComponent(jointEntity, "DriveComponent");

                RagdollJointAsset joint;
                joint.name = childMarker.name + "_to_" + parentMarker.name;
                joint.parentMarkerId = parentMarker.id;
                joint.childMarkerId = childMarker.id;
                glm::dmat4 parentFrame = Hell::Math::RemoveScaleAndShear(markerUI.ReadMatrix("parentFrame"));
                glm::dmat4 childFrame = Hell::Math::RemoveScaleAndShear(markerUI.ReadMatrix("childFrame"));
                ReconstructLegacyJointFrames(parentImport, childImport, parentFrame, childFrame);
                joint.parentFrame = ToMat4(parentFrame);
                joint.childFrame = ToMat4(childFrame);

                joint.angularLimits = {
                    MakeAxisLimit(limit.ReadFloat("twist")),
                    MakeAxisLimit(limit.ReadFloat("swing1")),
                    MakeAxisLimit(limit.ReadFloat("swing2"))
                };
                const bool hasActiveLinearDrive = drive.ReadBool("enabled", true) &&
                    (drive.ReadFloat("linearStiffness", 0.0f) > 0.0f || drive.ReadFloat("linearDamping", 0.0f) > 0.0f);
                const bool convertDrivenFreeTranslation =
                    childImport.resolvedLinearMotion == LegacyRagdollMotion::FREE && hasActiveLinearDrive;
                const float linearLimit = childImport.resolvedLinearMotion == LegacyRagdollMotion::FREE && !convertDrivenFreeTranslation
                    ? 0.0f
                    : -1.0f;
                joint.linearLimits = { MakeAxisLimit(linearLimit), MakeAxisLimit(linearLimit), MakeAxisLimit(linearLimit) };
                if (convertDrivenFreeTranslation) convertedDrivenLinearJointCount++;
                joint.limitEnabled = limit.ReadBool("enabled", true);
                joint.linearLimitStiffness = childImport.limitStiffness * asset.solver.linearLimitStiffness;
                joint.linearLimitDamping = childImport.limitStiffness * childImport.limitDampingRatio * asset.solver.linearLimitDamping;
                joint.angularLimitStiffness = childImport.limitStiffness * asset.solver.angularLimitStiffness;
                joint.angularLimitDamping = childImport.limitStiffness * childImport.limitDampingRatio * asset.solver.angularLimitDamping;

                asset.joints.push_back(std::move(joint));
            }

            if (convertedDrivenLinearJointCount > 0) {
                warnings.push_back(
                    "Converted " + std::to_string(convertedDrivenLinearJointCount) +
                    " drive-dependent free-translation joints to locked translation for passive ragdoll compatibility"
                );
            }
        }

        void BakeLegacyPhysicsScale(RagdollAsset& asset, float physicsScale) {
            if (physicsScale == 1.0f) return;

            const auto scaleTranslation = [physicsScale](glm::mat4& transform) {
                transform[3] = glm::vec4(glm::vec3(transform[3]) * physicsScale, transform[3].w);
            };

            for (RagdollMarkerAsset& marker : asset.markers) {
                scaleTranslation(marker.inputTransform);
                scaleTranslation(marker.restTransform);
                scaleTranslation(marker.bodyTransform);
                marker.shape.extents *= physicsScale;
                marker.shape.offset *= physicsScale;
                marker.shape.length *= physicsScale;
                marker.shape.radius *= physicsScale;
                for (glm::vec3& vertex : marker.shape.convexVertices) vertex *= physicsScale;
            }

            for (RagdollJointAsset& joint : asset.joints) {
                scaleTranslation(joint.parentFrame);
                scaleTranslation(joint.childFrame);
                for (RagdollAxisLimit& limit : joint.linearLimits) limit.limit *= physicsScale;
            }
        }

        void FinalizeMass(
            RagdollAsset& asset,
            const LegacyRagdollImportData& importData,
            std::vector<std::string>& warnings
        ) {
            double totalMass = 0.0;
            for (size_t markerIndex = 0; markerIndex < asset.markers.size(); markerIndex++) {
                RagdollMarkerAsset& marker = asset.markers[markerIndex];
                if (marker.rigidBody.isKinematic) continue;

                const float volume = RagdollMass::ComputeMarkerVolume(marker);
                float effectiveMass = marker.rigidBody.mass;
                const float legacyDensity = markerIndex < importData.markers.size()
                    ? importData.markers[markerIndex].density
                    : 1.0f;
                if (legacyDensity > 0.0f && std::isfinite(volume) && volume > 0.0f) {
                    effectiveMass = volume * legacyDensity;
                }
                if (!std::isfinite(effectiveMass) || effectiveMass <= 0.0f) {
                    effectiveMass = 1.0f;
                    const std::string markerLabel = marker.name.empty() ? std::to_string(marker.id) : marker.name;
                    warnings.push_back("Marker " + markerLabel + " had no usable legacy mass; imported as 1 kg");
                }

                marker.rigidBody.massMode = RagdollMassMode::OVERRIDE;
                marker.rigidBody.mass = effectiveMass;
                totalMass += effectiveMass;
            }
            if (totalMass > 0.0 && std::isfinite(totalMass)) {
                asset.targetMass = static_cast<float>(totalMass);
            }
        }

        bool TryImportLegacyRagdoll(
            const FileInfo& fileInfo,
            RagdollAsset& asset,
            std::vector<std::string>& warnings,
            std::string& error
        ) {
            std::ifstream file(fileInfo.path, std::ios::binary);
            if (!file) {
                error = "Could not open legacy ragdoll file " + fileInfo.path;
                return false;
            }

            const std::string jsonString{
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            };

            nlohmann::ordered_json document;
            try {
                document = nlohmann::ordered_json::parse(jsonString);
            }
            catch (const nlohmann::ordered_json::parse_error& exception) {
                error = "Invalid JSON in legacy ragdoll file " + fileInfo.path + ": " + exception.what();
                return false;
            }

            if (!document.is_object()) {
                error = "Legacy ragdoll root must be an object in " + fileInfo.path;
                return false;
            }
            const auto schema = document.find("schema");
            if (schema == document.end() || !schema->is_string()) {
                error = "Legacy ragdoll schema is missing in " + fileInfo.path;
                return false;
            }
            const std::string schemaName = schema->get<std::string>();
            if (schemaName != "ragdoll-1.0") {
                error = "Unsupported legacy ragdoll schema " + schemaName;
                return false;
            }
            const auto entities = document.find("entities");
            if (entities == document.end() || !entities->is_object()) {
                error = "Legacy ragdoll entities are missing in " + fileInfo.path;
                return false;
            }

            const LegacyRag::Reader reader(document);
            bool hasSolver = false;
            for (const std::string& entity : reader.GetEntityNames()) {
                if (reader.HasComponent(entity, "SolverComponent") && reader.HasComponent(entity, "SolverUIComponent")) {
                    hasSolver = true;
                    break;
                }
            }
            if (!hasSolver) {
                error = "Legacy ragdoll solver is missing in " + fileInfo.path;
                return false;
            }

            RagdollAsset importedAsset;
            importedAsset.name = CleanLegacyString(fileInfo.name);
            LegacyRagdollImportData importData;
            std::vector<std::string> importWarnings;

            try {
                LoadSolver(importedAsset, importData, reader);
                LoadGroups(importData, reader);
                LoadMarkers(importedAsset, importData, reader, importWarnings);
                ResolveMarkerSettings(importedAsset, importData);
                LoadJoints(importedAsset, importData, reader, importWarnings);
                BakeLegacyPhysicsScale(importedAsset, importData.physicsScale);
                FinalizeMass(importedAsset, importData, importWarnings);
            }
            catch (const std::exception& exception) {
                error = "Failed to read legacy ragdoll " + fileInfo.path + ": " + exception.what();
                return false;
            }
            catch (...) {
                error = "Failed to read legacy ragdoll " + fileInfo.path;
                return false;
            }

            if (importedAsset.markers.empty()) {
                error = "Legacy ragdoll has no markers " + fileInfo.path;
                return false;
            }

            asset = std::move(importedAsset);
            warnings = std::move(importWarnings);
            error.clear();
            return true;
        }
    }

    bool ImportLegacyRagdollAsset(
        const std::string& path,
        RagdollAsset& asset,
        std::vector<std::string>& warnings,
        std::string& error
    ) {
        FileInfo fileInfo = File::GetInfo(path);
        if (fileInfo.path.empty()) fileInfo.path = path;
        return TryImportLegacyRagdoll(fileInfo, asset, warnings, error);
    }
}
