#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/BoneMask.h"
#include "Hell/Serialization/Json.h"

#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <utility>

namespace Hell::AssetLoader {
    namespace {
        constexpr const char* FORMAT_NAME = "hell.bone_mask";
        constexpr uint32_t FORMAT_VERSION = 1;

        using Json = nlohmann::json;

        bool ValidateBoneMask(const BoneMask& boneMask, std::string& error) {
            if (boneMask.name.empty()) {
                error = "The bone mask has no name";
                return false;
            }
            if (boneMask.skinnedModelName.empty()) {
                error = "The bone mask has no skinned model";
                return false;
            }
            for (const std::pair<const std::string, float>& entry : boneMask.weights) {
                if (entry.first.empty()) {
                    error = "The bone mask contains an unnamed bone";
                    return false;
                }
                if (entry.second < 0.0f || entry.second > 1.0f) {
                    error = "Bone '" + entry.first + "' has a weight outside 0 to 1";
                    return false;
                }
            }
            return true;
        }
    }

    void LoadBoneMasks() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/bone_masks", { "bonemask" })) {
            BoneMask boneMask;
            std::string error;
            if (!LoadBoneMask(fileInfo.path, boneMask, error)) {
                Logging::Error() << "AssetLoader::LoadBoneMasks() " << error << "\n";
                continue;
            }
            ResourceManager::CreateBoneMask(std::move(boneMask));
            AddLoadLogItem("Loaded " + fileInfo.path);
        }
    }

    bool LoadBoneMask(const std::string& path, BoneMask& boneMask, std::string& error) {
        Json json;
        if (!Hell::Json::LoadFromFile(json, path)) {
            error = "Failed to read '" + path + "'";
            return false;
        }

        try {
            if (json.at("format").get<std::string>() != FORMAT_NAME) throw std::runtime_error("Not a Hell bone mask");
            const uint32_t version = json.at("version").get<uint32_t>();
            if (version != FORMAT_VERSION) throw std::runtime_error("Unsupported .bonemask version " + std::to_string(version));

            BoneMask loadedBoneMask;
            loadedBoneMask.version = version;
            loadedBoneMask.name = json.at("name").get<std::string>();
            loadedBoneMask.skinnedModelName = json.at("skinnedModelName").get<std::string>();

            const Json& weights = json.at("weights");
            if (!weights.is_object()) throw std::runtime_error("Bone mask weights must be an object");
            for (Json::const_iterator entry = weights.begin(); entry != weights.end(); entry++) loadedBoneMask.weights[entry.key()] = entry.value().get<float>();

            if (!ValidateBoneMask(loadedBoneMask, error)) return false;
            boneMask = std::move(loadedBoneMask);
            error.clear();
            return true;
        }
        catch (const std::exception& exception) {
            error = "Failed to load '" + path + "': " + exception.what();
            return false;
        }
    }

    bool SaveBoneMask(const std::string& path, const BoneMask& boneMask, std::string& error) {
        if (path.empty()) {
            error = "The bone mask has no save path";
            return false;
        }
        if (!ValidateBoneMask(boneMask, error)) return false;

        Json weights = Json::object();
        for (const std::pair<const std::string, float>& entry : boneMask.weights) weights[entry.first] = entry.second;
        const Json json = {
            { "format", FORMAT_NAME },
            { "version", FORMAT_VERSION },
            { "name", boneMask.name },
            { "skinnedModelName", boneMask.skinnedModelName },
            { "weights", weights }
        };

        std::error_code fileSystemError;
        const std::filesystem::path parentPath = std::filesystem::path(path).parent_path();
        if (!parentPath.empty()) std::filesystem::create_directories(parentPath, fileSystemError);
        if (fileSystemError) {
            error = "Failed to create the bone mask directory: " + fileSystemError.message();
            return false;
        }
        if (!Hell::Json::SaveToFile(json, path)) {
            error = "Failed to save '" + path + "'";
            return false;
        }

        error.clear();
        return true;
    }
}
