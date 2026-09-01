#include "AssetLoader.h"

#include "Hell/File.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/Types/VAT.h"
#include "Hell/Serialization/Json.h"

#include <utility>

namespace Hell::AssetLoader {

    namespace {
        bool IsVATMetadataJson(const nlohmann::json& json) {
            return json.contains("positionTexture") ||
                json.contains("rotationTexture") ||
                json.contains("lookupTexture") ||
                json.contains("model") ||
                json.contains("modelName") ||
                json.contains("mesh");
        }

        VATMetadata ParseVATMetadata(const nlohmann::json& json) {
            VATMetadata metadata;
            metadata.frameCount = json.value("frameCount", 0);
            metadata.fps = json.value("fps", json.value("houdiniFPS", json.value("houdiniFps", json.value("_houdiniFPS", 0.0f))));
            metadata.boundsMin = json.value("boundsMin", glm::vec3(0.0f));
            metadata.boundsMax = json.value("boundsMax", glm::vec3(0.0f));
            metadata.positionTexture = json.value("positionTexture", "");
            metadata.rotationTexture = json.value("rotationTexture", "");
            metadata.lookupTexture = json.value("lookupTexture", "");
            metadata.model = json.value("model", json.value("modelName", json.value("mesh", "")));
            return metadata;
        }

        std::string GetResourceName(const std::string& filename) {
            return File::GetName(filename);
        }

        bool ResolveVATModel(const std::string& vatName, const std::string& modelFilename, uint32_t& outModelId) {
            const std::string modelName = GetResourceName(modelFilename);

            if (modelName.empty()) {
                Logging::Error() << "AssetLoader::LoadVATFiles(..) failed to load VAT '" << vatName << "' because the model name is empty\n";
                return false;
            }

            Model* model = ResourceManager::GetModelByName(modelName);
            if (!model) {
                Logging::Error() << "AssetLoader::LoadVATFiles(..) failed to load VAT '" << vatName << "' because model '" << modelFilename << "' was not found\n";
                return false;
            }

            outModelId = model->GetModelId();
            return true;
        }
    }

    void LoadVATFiles() {
        for (FileInfo& fileInfo : File::IterateDirectory("res/VAT/", { "json" })) {
            nlohmann::json json;
            if (Hell::Json::LoadFromFile(json, fileInfo.path)) {
                if (!IsVATMetadataJson(json)) {
                    continue;
                }

                VATMetadata metadata = ParseVATMetadata(json);
                if (metadata.model.empty()) {
                    metadata.model = fileInfo.name + "_mesh";
                }

                uint32_t modelId = 0;

                const bool loaded = ResolveVATModel(fileInfo.name, metadata.model, modelId);

                if (!loaded) {
                    continue;
                }

                Vat vat(fileInfo.name);
                vat.SetFileInfo(fileInfo);
                vat.SetMetadata(metadata);
                vat.SetModelId(modelId);

                Logging::Init() << "Loaded VAT metadata '" << fileInfo.name << "' " << metadata.frameCount << " frames at " << metadata.fps << " fps\n";

                ResourceManager::CreateVAT(std::move(vat));
            }
        }
    }
}
