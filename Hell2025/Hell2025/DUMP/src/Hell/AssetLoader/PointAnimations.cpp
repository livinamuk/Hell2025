#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/PointAnimation.h"
#include "Hell/Serialization/Json.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace Hell::AssetLoader {

    namespace {
        std::vector<std::vector<glm::vec3>> ParseFrames(const nlohmann::json& json, const std::string& pointAnimationName, int& outPointCount) {
            std::vector<std::vector<glm::vec3>> frames;
            outPointCount = 0;

            auto framesJson = json.find("frames");
            if (framesJson == json.end() || !framesJson->is_array()) {
                Logging::Error() << "AssetLoader::LoadPointAnimations(..) failed to load PointAnimation '" << pointAnimationName << "' because it has no frames array\n";
                return frames;
            }

            frames.reserve(framesJson->size());

            for (const nlohmann::json& frameJson : *framesJson) {
                std::vector<glm::vec3>& frame = frames.emplace_back();
                if (!frameJson.is_array()) {
                    continue;
                }

                frame.reserve(frameJson.size());
                for (const nlohmann::json& pointJson : frameJson) {
                    frame.push_back(pointJson.get<glm::vec3>());
                }

                outPointCount = std::max(outPointCount, static_cast<int>(frame.size()));
            }

            return frames;
        }
    }

    void LoadPointAnimations() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/PointAnimations/", { "json" })) {
            nlohmann::json json;
            if (Hell::Json::LoadFromFile(json, fileInfo.path)) {
                int inferredPointCount = 0;
                std::vector<std::vector<glm::vec3>> frames = ParseFrames(json, fileInfo.name, inferredPointCount);
                if (frames.empty()) {
                    continue;
                }

                PointAnimationMetadata metadata;
                metadata.fps = json.value("fps", json.value("houdiniFPS", json.value("houdiniFps", json.value("_houdiniFPS", 0.0f))));
                metadata.frameCount = json.value("frameCount", static_cast<int>(frames.size()));
                metadata.pointCount = json.value("pointCount", inferredPointCount);

                if (metadata.frameCount < static_cast<int>(frames.size())) {
                    metadata.frameCount = static_cast<int>(frames.size());
                }

                if (metadata.frameCount > static_cast<int>(frames.size())) {
                    frames.resize(metadata.frameCount);
                }

                PointAnimation pointAnimation(fileInfo.name);
                pointAnimation.SetFileInfo(fileInfo);
                pointAnimation.SetMetadata(metadata);
                pointAnimation.SetFrames(std::move(frames));

                Logging::Init() << "Loaded PointAnimation metadata '" << fileInfo.name << "' " << metadata.frameCount << " frames, " << metadata.pointCount << " points at " << metadata.fps << " fps\n";

                ResourceManager::CreatePointAnimation(std::move(pointAnimation));
            }
        }
    }
}
