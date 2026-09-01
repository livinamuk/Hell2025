#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/Types/Animation.h"
#include "Hell/Serialization/Json.h"

#include <assimp/config.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace Hell::AssetLoader {

    namespace {
        constexpr const char* SHAPE_ANIMATION_FORMAT = "HellShapeAnimation";
        constexpr uint32_t SHAPE_ANIMATION_VERSION = 2;

        bool LoadShapeAnimationSidecar(const FileInfo& animationFileInfo, ShapeAnimationData& outShapeAnimation) {
            outShapeAnimation = {};
            const std::string sidecarPath = File::RemoveExtension(animationFileInfo.path) + ".shapeanim.json";
            if (!File::Exists(sidecarPath)) return true;

            nlohmann::json json;
            if (!Json::LoadFromFile(json, sidecarPath)) return false;

            try {
                if (!json.is_object()) throw std::runtime_error("root must be an object");
                if (json.at("format").get<std::string>() != SHAPE_ANIMATION_FORMAT) {
                    throw std::runtime_error("unsupported format");
                }
                if (json.at("version").get<uint32_t>() != SHAPE_ANIMATION_VERSION) {
                    throw std::runtime_error("unsupported version");
                }
                if (json.at("clip").get<std::string>() != animationFileInfo.name) {
                    throw std::runtime_error("clip name does not match the FBX filename");
                }

                ShapeAnimationData loadedShapeAnimation;
                loadedShapeAnimation.framesPerSecond = json.at("frames_per_second").get<float>();
                loadedShapeAnimation.duration = json.at("duration_seconds").get<float>();
                loadedShapeAnimation.frameStart = json.at("frame_start").get<int32_t>();
                loadedShapeAnimation.frameEnd = json.at("frame_end").get<int32_t>();
                loadedShapeAnimation.sampleStepFrames = json.at("sample_step_frames").get<uint32_t>();

                if (!std::isfinite(loadedShapeAnimation.framesPerSecond) || loadedShapeAnimation.framesPerSecond <= 0.0f) {
                    throw std::runtime_error("frames_per_second must be finite and greater than zero");
                }
                if (!std::isfinite(loadedShapeAnimation.duration) || loadedShapeAnimation.duration < 0.0f) {
                    throw std::runtime_error("duration_seconds must be finite and nonnegative");
                }
                if (loadedShapeAnimation.frameEnd < loadedShapeAnimation.frameStart) {
                    throw std::runtime_error("frame_end precedes frame_start");
                }
                if (loadedShapeAnimation.sampleStepFrames == 0) {
                    throw std::runtime_error("sample_step_frames must be greater than zero");
                }

                const uint64_t frameSpan = static_cast<uint64_t>(
                    static_cast<int64_t>(loadedShapeAnimation.frameEnd) - loadedShapeAnimation.frameStart
                );
                if (frameSpan % loadedShapeAnimation.sampleStepFrames != 0) {
                    throw std::runtime_error("frame range is not divisible by sample_step_frames");
                }
                const uint64_t expectedSampleCount = frameSpan / loadedShapeAnimation.sampleStepFrames + 1;
                if (expectedSampleCount > std::numeric_limits<uint32_t>::max()) {
                    throw std::runtime_error("sample count is too large");
                }

                const float expectedDuration = static_cast<float>(frameSpan) / loadedShapeAnimation.framesPerSecond;
                if (std::abs(loadedShapeAnimation.duration - expectedDuration) > 1.0e-4f) {
                    throw std::runtime_error("duration_seconds does not match the frame range and frame rate");
                }

                const nlohmann::json& channels = json.at("channels");
                if (!channels.is_array() || channels.empty()) {
                    throw std::runtime_error("channels must be a nonempty array");
                }

                loadedShapeAnimation.channels.reserve(channels.size());
                std::unordered_set<std::string> targetMeshPairs;
                std::unordered_set<std::string> targetNames;

                for (const nlohmann::json& sourceChannel : channels) {
                    ShapeAnimationChannel channel;
                    channel.targetName = sourceChannel.at("target").get<std::string>();
                    channel.meshObjectNames = sourceChannel.at("mesh_objects").get<std::vector<std::string>>();
                    channel.samples = sourceChannel.at("samples").get<std::vector<float>>();

                    if (channel.targetName.empty()) throw std::runtime_error("channel target name is empty");
                    if (!targetNames.insert(channel.targetName).second) throw std::runtime_error("channel target name appears more than once");
                    if (channel.meshObjectNames.empty()) throw std::runtime_error("channel mesh_objects is empty");
                    if (channel.samples.size() != expectedSampleCount) {
                        throw std::runtime_error("channel sample count does not match the frame range");
                    }

                    for (const float sample : channel.samples) {
                        if (!std::isfinite(sample)) throw std::runtime_error("channel contains a non-finite sample");
                    }

                    std::unordered_set<std::string> channelMeshNames;
                    for (const std::string& meshObjectName : channel.meshObjectNames) {
                        if (meshObjectName.empty()) throw std::runtime_error("channel contains an empty mesh object name");
                        if (!channelMeshNames.insert(meshObjectName).second) {
                            throw std::runtime_error("channel contains a duplicate mesh object name");
                        }

                        const std::string pairKey = channel.targetName + '\0' + meshObjectName;
                        if (!targetMeshPairs.insert(pairKey).second) {
                            throw std::runtime_error("target and mesh object pair appears in more than one channel");
                        }
                    }

                    loadedShapeAnimation.channels.push_back(std::move(channel));
                }

                outShapeAnimation = std::move(loadedShapeAnimation);
                Logging::Debug() << "Loaded " << outShapeAnimation.channels.size()
                                 << " shape animation channel(s) from '" << sidecarPath << "'\n";
                return true;
            }
            catch (const nlohmann::json::exception& error) {
                Logging::Error() << "AssetLoader::LoadAnimation(..) found invalid shape animation JSON in '"
                                 << sidecarPath << "': " << error.what() << "\n";
            }
            catch (const std::exception& error) {
                Logging::Error() << "AssetLoader::LoadAnimation(..) found invalid shape animation data in '"
                                 << sidecarPath << "': " << error.what() << "\n";
            }

            outShapeAnimation = {};
            return false;
        }
    }

    bool LoadAnimation(const FileInfo& fileInfo, Animation& outAnimation) {
        const unsigned int animationImportFlags = aiProcess_RemoveComponent;
        const unsigned int animationRemoveComponents =
            aiComponent_MESHES |
            aiComponent_MATERIALS |
            aiComponent_TEXTURES |
            aiComponent_LIGHTS |
            aiComponent_CAMERAS |
            aiComponent_BONEWEIGHTS;

        Assimp::Importer importer;
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ALL_GEOMETRY_LAYERS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_MATERIALS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_TEXTURES, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_CAMERAS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_LIGHTS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, true);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_READ_WEIGHTS, false);
        importer.SetPropertyBool(AI_CONFIG_IMPORT_NO_SKELETON_MESHES, true);
        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, animationRemoveComponents);

        const aiScene* scene = importer.ReadFile(fileInfo.path, animationImportFlags);

        if (!scene) {
            Logging::Error() << "AssetLoader::LoadAnimation(..) failed to load '" << fileInfo.path << "': " << importer.GetErrorString() << "\n";
            return false;
        }

        if (scene->mNumAnimations == 0 || !scene->mAnimations[0]) {
            Logging::Warning() << "AssetLoader::LoadAnimation(..) found no animations in '" << fileInfo.path << "'\n";
            return false;
        }

        const aiAnimation& sourceAnimation = *scene->mAnimations[0];
        const double ticksPerSecond = sourceAnimation.mTicksPerSecond > 0.0 ? sourceAnimation.mTicksPerSecond : 25.0;
        const double tickToSeconds = 1.0 / ticksPerSecond;

        Animation loadedAnimation;
        loadedAnimation.m_duration = static_cast<float>(sourceAnimation.mDuration * tickToSeconds);
        loadedAnimation.m_ticksPerSecond = static_cast<float>(ticksPerSecond);
        loadedAnimation.m_channels.reserve(sourceAnimation.mNumChannels);

        for (uint32_t channelIndex = 0; channelIndex < sourceAnimation.mNumChannels; channelIndex++) {
            const aiNodeAnim* sourceChannel = sourceAnimation.mChannels[channelIndex];
            if (!sourceChannel) {
                continue;
            }

            AnimationChannel channel;
            channel.nodeName = sourceChannel->mNodeName.C_Str();
            channel.translationKeys.resize(sourceChannel->mNumPositionKeys);
            channel.rotationKeys.resize(sourceChannel->mNumRotationKeys);
            channel.scaleKeys.resize(sourceChannel->mNumScalingKeys);

            // Position keys
            for (uint32_t keyIndex = 0; keyIndex < sourceChannel->mNumPositionKeys; keyIndex++) {
                const aiVectorKey& sourceKey = sourceChannel->mPositionKeys[keyIndex];

                AnimationVectorKey& key = channel.translationKeys[keyIndex];
                key.value = glm::vec3(sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z);
                key.time = static_cast<float>(sourceKey.mTime * tickToSeconds);
            }

            // Rotation keys
            for (uint32_t keyIndex = 0; keyIndex < sourceChannel->mNumRotationKeys; keyIndex++) {
                const aiQuatKey& sourceKey = sourceChannel->mRotationKeys[keyIndex];

                AnimationQuaternionKey& key = channel.rotationKeys[keyIndex];
                key.value = glm::quat(sourceKey.mValue.w, sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z);
                key.time = static_cast<float>(sourceKey.mTime * tickToSeconds);
            }

            // Scale keys
            for (uint32_t keyIndex = 0; keyIndex < sourceChannel->mNumScalingKeys; keyIndex++) {
                const aiVectorKey& sourceKey = sourceChannel->mScalingKeys[keyIndex];

                AnimationVectorKey& key = channel.scaleKeys[keyIndex];
                key.value = glm::vec3(sourceKey.mValue.x, sourceKey.mValue.y, sourceKey.mValue.z);
                key.time = static_cast<float>(sourceKey.mTime * tickToSeconds);
            }

            if (!channel.translationKeys.empty() || !channel.rotationKeys.empty() || !channel.scaleKeys.empty()) {
                loadedAnimation.m_channels.push_back(std::move(channel));
            }
        }

        if (loadedAnimation.m_channels.empty()) {
            Logging::Warning() << "AssetLoader::LoadAnimation(..) found no animation channels in '" << fileInfo.path << "'\n";
            return false;
        }

        if (!LoadShapeAnimationSidecar(fileInfo, loadedAnimation.m_shapeAnimation)) {
            return false;
        }

        if (!loadedAnimation.m_shapeAnimation.channels.empty()) {
            const float durationDifference = std::abs(
                loadedAnimation.m_shapeAnimation.duration - loadedAnimation.m_duration
            );
            const float durationTolerance = 1.0f / loadedAnimation.m_shapeAnimation.framesPerSecond;
            if (durationDifference > durationTolerance) {
                Logging::Warning() << "AssetLoader::LoadAnimation(..) shape animation duration differs from skeletal duration in '"
                                   << fileInfo.path << "'\n";
            }
        }

        outAnimation = std::move(loadedAnimation);
        return true;
    }
}
