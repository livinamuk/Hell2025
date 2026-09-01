#include "AssetCompiler.h"
#include "AssimpImporter.h"

#include "Hell/AssetFormats/AssetFormats.h"
#include "Hell/BVH/BVH.h"
#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <unordered_set>
#include <utility>

namespace Hell::AssetCompiler {

    namespace {
        struct MorphTargetSelection {
            bool manifestExists = false;
            std::unordered_set<std::string> names;
        };

        std::string TrimWhitespace(std::string value) {
            const auto isNotWhitespace = [](unsigned char character) { return !std::isspace(character); };
            const auto begin = std::find_if(value.begin(), value.end(), isNotWhitespace);
            if (begin == value.end()) return {};

            const auto end = std::find_if(value.rbegin(), value.rend(), isNotWhitespace).base();
            return std::string(begin, end);
        }

        MorphTargetSelection LoadMorphTargetSelection(const std::string& sourceModelPath) {
            MorphTargetSelection selection;
            const std::string manifestPath = File::RemoveExtension(sourceModelPath) + ".morphs";
            selection.manifestExists = File::Exists(manifestPath);
            if (!selection.manifestExists) return selection;

            std::ifstream file(manifestPath);
            if (!file) {
                Logging::Error() << "Failed to open skinned model morph allowlist '" << manifestPath << "'\n";
                return selection;
            }

            std::string line;
            while (std::getline(file, line)) {
                line = TrimWhitespace(std::move(line));
                if (line.empty() || line.starts_with('#')) continue;
                selection.names.insert(std::move(line));
            }

            return selection;
        }

        uint64_t BuildSkinnedModelCompileStamp(uint64_t sourceTimestamp, const MorphTargetSelection& selection) {
            if (!selection.manifestExists) return sourceTimestamp;

            constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ull;
            constexpr uint64_t FNV_PRIME = 1099511628211ull;
            constexpr uint64_t MORPH_COMPILER_REVISION = 2;
            uint64_t hash = FNV_OFFSET_BASIS;

            // Bump this revision when morph import semantics change so an
            // existing .skinnedmodel cannot hide a compiler-side fix.
            for (uint32_t byteIndex = 0; byteIndex < sizeof(MORPH_COMPILER_REVISION); byteIndex++) {
                hash ^= (MORPH_COMPILER_REVISION >> (byteIndex * 8)) & 0xffu;
                hash *= FNV_PRIME;
            }

            for (uint32_t byteIndex = 0; byteIndex < sizeof(sourceTimestamp); byteIndex++) {
                hash ^= (sourceTimestamp >> (byteIndex * 8)) & 0xffu;
                hash *= FNV_PRIME;
            }

            std::vector<std::string> sortedNames(selection.names.begin(), selection.names.end());
            std::sort(sortedNames.begin(), sortedNames.end());
            for (const std::string& name : sortedNames) {
                for (const unsigned char character : name) {
                    hash ^= character;
                    hash *= FNV_PRIME;
                }
                hash ^= 0xffu;
                hash *= FNV_PRIME;
            }

            return hash;
        }

        ModelBvhData BuildModelBvh(const ModelData& model) {
            ModelBvhData result;
            result.timestamp = model.timestamp;
            result.bvhs.reserve(model.meshes.size());

            for (const MeshData& mesh : model.meshes) {
                result.bvhs.push_back(Hell::Bvh::BuildMeshBvh(mesh.vertices, mesh.indices));
            }

            return result;
        }

        void CompileTextures() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/textures/compress_me", { "png", "jpg", "tga" })) {
                const std::string outputPath = "res/textures/compressed/" + fileInfo.name + ".dds";
                if (!File::Exists(outputPath)) {
                    ImageTools::CreateAndExportDDS(fileInfo.path, outputPath, true);
                    Logging::Debug() << "Exported " << outputPath << "\n";
                }
            }
        }

        void CompileModels() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/models_raw", { "obj", "fbx" })) {
                const std::string outputPath = "res/models/" + fileInfo.name + ".model";
                const uint64_t sourceTimestamp = File::GetLastModifiedTime(fileInfo.path);

                AssetFormats::ModelMetadata metadata;
                const bool outputIsCurrent =
                    File::Exists(outputPath) &&
                    AssetFormats::ReadModelMetadata(outputPath, metadata) &&
                    metadata.timestamp == sourceTimestamp;

                if (!outputIsCurrent) {
                    ModelData model = ImportModel(fileInfo.path);
                    if (!model.meshes.empty()) {
                        AssetFormats::SaveModel(outputPath, model);
                    }
                }
            }
        }

        void CompileVATModels() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/VAT", { "fbx" })) {
                const std::string outputPath = "res/models/" + fileInfo.name + ".model";
                const uint64_t sourceTimestamp = File::GetLastModifiedTime(fileInfo.path);

                AssetFormats::ModelMetadata metadata;
                const bool outputIsCurrent =
                    File::Exists(outputPath) &&
                    AssetFormats::ReadModelMetadata(outputPath, metadata) &&
                    metadata.timestamp == sourceTimestamp;

                if (!outputIsCurrent) {
                    ModelData model = ImportVatCarrierModel(fileInfo.path);
                    if (!model.meshes.empty()) {
                        AssetFormats::SaveModel(outputPath, model);
                    }
                }
            }
        }

        void CompileModelBvhs() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/models", { "model" })) {
                const std::string modelPath = fileInfo.path;
                const std::string bvhPath = "res/models/bvh/" + fileInfo.name + ".bvh";

                AssetFormats::ModelMetadata modelMetadata;
                if (!AssetFormats::ReadModelMetadata(modelPath, modelMetadata)) {
                    continue;
                }

                AssetFormats::ModelBvhMetadata bvhMetadata;
                const bool outputIsCurrent =
                    File::Exists(bvhPath) &&
                    AssetFormats::ReadModelBvhMetadata(bvhPath, bvhMetadata) &&
                    bvhMetadata.timestamp == modelMetadata.timestamp;

                if (outputIsCurrent) {
                    continue;
                }

                ModelData model;
                if (!AssetFormats::LoadModel(modelPath, model)) {
                    continue;
                }

                ModelBvhData bvh = BuildModelBvh(model);
                if (bvh.bvhs.size() == model.meshes.size()) {
                    AssetFormats::SaveModelBvh(bvhPath, bvh);
                }
            }
        }

        void CompileSkinnedModels() {
            for (FileInfo& fileInfo : File::IterateDirectory("res/skinned_models_raw", { "obj", "fbx" })) {
                const std::string outputPath = "res/skinned_models/" + fileInfo.name + ".skinnedmodel";
                const uint64_t sourceTimestamp = File::GetLastModifiedTime(fileInfo.path);
                const MorphTargetSelection morphTargetSelection = LoadMorphTargetSelection(fileInfo.path);
                const uint64_t compileStamp = BuildSkinnedModelCompileStamp(sourceTimestamp, morphTargetSelection);

                AssetFormats::SkinnedModelMetadata metadata;
                const bool outputIsCurrent =
                    File::Exists(outputPath) &&
                    AssetFormats::ReadSkinnedModelMetadata(outputPath, metadata) &&
                    metadata.timestamp == compileStamp &&
                    (!morphTargetSelection.manifestExists || metadata.version == AssetFormats::SKINNED_MODEL_VERSION);

                if (!outputIsCurrent) {
                    SkinnedModelData model = ImportSkinnedModel(fileInfo.path, morphTargetSelection.names);
                    if (!model.meshes.empty()) {
                        model.timestamp = compileStamp;
                        AssetFormats::SaveSkinnedModel(outputPath, model);
                    }
                }
            }
        }
    }

    void CompileOutOfDateAssets() {
        CompileTextures();
        CompileModels();
        CompileVATModels();
        CompileModelBvhs();
        CompileSkinnedModels();
    }
}
