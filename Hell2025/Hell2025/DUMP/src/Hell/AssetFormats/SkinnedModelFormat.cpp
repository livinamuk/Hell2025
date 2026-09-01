#include "AssetFormats.h"
#include "AssetFormatHeaders.h"

#include "Hell/Common/String.h"
#include "Hell/Logging.h"

#include <cstring>
#include <filesystem>
#include <fstream>

namespace Hell::AssetFormats {

    namespace {
        inline constexpr char SKINNED_MODEL_SIGNATURE[] = "HELL_SKINNED_MODEL";
        inline constexpr char MORPH_SECTION_SIGNATURE[] = "HELL_MORPHS";
        // Version 3 invalidates assets compiled with the old lossy 5% skin-weight
        // cutoff. The binary layout is unchanged, but the vertex-weight data is not.
        inline constexpr uint32_t MIN_SUPPORTED_SKINNED_MODEL_VERSION = 3;

        bool ReadSkinnedHeader(std::ifstream& file, SkinnedModelHeader& header) {
            file.read(header.signature, sizeof(header.signature));
            file.read(reinterpret_cast<char*>(&header.version), sizeof(header.version));
            file.read(reinterpret_cast<char*>(&header.nameLength), sizeof(header.nameLength));
            file.read(reinterpret_cast<char*>(&header.vertexCount), sizeof(header.vertexCount));
            file.read(reinterpret_cast<char*>(&header.indexCount), sizeof(header.indexCount));
            file.read(reinterpret_cast<char*>(&header.meshCount), sizeof(header.meshCount));
            file.read(reinterpret_cast<char*>(&header.nodeCount), sizeof(header.nodeCount));
            file.read(reinterpret_cast<char*>(&header.boneCount), sizeof(header.boneCount));
            file.read(reinterpret_cast<char*>(&header.timestamp), sizeof(header.timestamp));

            return file &&
                   header.version >= MIN_SUPPORTED_SKINNED_MODEL_VERSION &&
                   header.version <= SKINNED_MODEL_VERSION &&
                   std::memcmp(header.signature, SKINNED_MODEL_SIGNATURE, sizeof(header.signature)) == 0;
        }

        void WriteSkinnedHeader(std::ofstream& file, const SkinnedModelHeader& header) {
            file.write(header.signature, sizeof(header.signature));
            file.write(reinterpret_cast<const char*>(&header.version), sizeof(header.version));
            file.write(reinterpret_cast<const char*>(&header.nameLength), sizeof(header.nameLength));
            file.write(reinterpret_cast<const char*>(&header.vertexCount), sizeof(header.vertexCount));
            file.write(reinterpret_cast<const char*>(&header.indexCount), sizeof(header.indexCount));
            file.write(reinterpret_cast<const char*>(&header.meshCount), sizeof(header.meshCount));
            file.write(reinterpret_cast<const char*>(&header.nodeCount), sizeof(header.nodeCount));
            file.write(reinterpret_cast<const char*>(&header.boneCount), sizeof(header.boneCount));
            file.write(reinterpret_cast<const char*>(&header.timestamp), sizeof(header.timestamp));
        }

        void WriteMorphDeltas(std::ofstream& file, const std::vector<MorphTargetVertexDelta>& vertexDeltas) {
            for (const MorphTargetVertexDelta& vertexDelta : vertexDeltas) {
                file.write(reinterpret_cast<const char*>(&vertexDelta.vertexIndex), sizeof(vertexDelta.vertexIndex));
                file.write(reinterpret_cast<const char*>(&vertexDelta.delta), sizeof(vertexDelta.delta));
            }
        }

        bool ReadMorphDeltas(
            std::ifstream& file,
            std::vector<MorphTargetVertexDelta>& outVertexDeltas,
            uint32_t vertexDeltaCount,
            uint32_t meshVertexCount,
            const std::string& meshName,
            const std::string& path) {
            if (vertexDeltaCount > meshVertexCount) {
                Logging::Error() << "AssetFormats::LoadSkinnedModel() found too many morph deltas in mesh '"
                                 << meshName << "' from '" << path << "'\n";
                return false;
            }

            outVertexDeltas.resize(vertexDeltaCount);
            uint32_t previousVertexIndex = 0;
            bool firstDelta = true;
            for (MorphTargetVertexDelta& vertexDelta : outVertexDeltas) {
                file.read(reinterpret_cast<char*>(&vertexDelta.vertexIndex), sizeof(vertexDelta.vertexIndex));
                file.read(reinterpret_cast<char*>(&vertexDelta.delta), sizeof(vertexDelta.delta));
                if (!file ||
                    vertexDelta.vertexIndex >= meshVertexCount ||
                    (!firstDelta && vertexDelta.vertexIndex <= previousVertexIndex)) {
                    Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid morph vertex in mesh '"
                                     << meshName << "' from '" << path << "'\n";
                    return false;
                }
                previousVertexIndex = vertexDelta.vertexIndex;
                firstDelta = false;
            }

            return true;
        }
    }

    bool ReadSkinnedModelMetadata(const std::string& path, SkinnedModelMetadata& outMetadata) {
        outMetadata = {};

        std::ifstream file(path, std::ios::binary);
        SkinnedModelHeader header{};
        if (!ReadSkinnedHeader(file, header)) {
            Logging::Error() << "AssetFormats::ReadSkinnedModelMetadata() found an invalid header in '" << path << "'\n";
            return false;
        }

        outMetadata.version = header.version;
        outMetadata.vertexCount = header.vertexCount;
        outMetadata.indexCount = header.indexCount;
        outMetadata.meshCount = header.meshCount;
        outMetadata.nodeCount = header.nodeCount;
        outMetadata.boneCount = header.boneCount;
        outMetadata.timestamp = header.timestamp;
        return true;
    }

    bool SaveSkinnedModel(const std::string& path, const SkinnedModelData& model) {
        std::error_code fileSizeError;
        const uintmax_t previousFileSize = std::filesystem::file_size(path, fileSizeError);
        const bool previousFileExisted = !fileSizeError;

        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::SaveSkinnedModel() failed to open '" << path << "'\n";
            return false;
        }

        SkinnedModelHeader header{};
        std::memcpy(header.signature, SKINNED_MODEL_SIGNATURE, sizeof(header.signature));
        header.version = SKINNED_MODEL_VERSION;
        header.nameLength = static_cast<uint32_t>(model.name.size());
        header.vertexCount = model.vertexCount;
        header.indexCount = model.indexCount;
        header.meshCount = model.GetMeshCount();
        header.nodeCount = model.GetNodeCount();
        header.boneCount = model.GetBoneCount();
        header.timestamp = model.timestamp;
        WriteSkinnedHeader(file, header);

        file.write(model.name.data(), model.name.size());

        for (const Node& node : model.nodes) {
            const uint32_t nameLength = static_cast<uint32_t>(node.name.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(node.name.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&node.parentIndex), sizeof(node.parentIndex));
            file.write(reinterpret_cast<const char*>(&node.localBindTransform), sizeof(node.localBindTransform));
        }

        for (const glm::mat4& boneOffset : model.boneOffsets) {
            file.write(reinterpret_cast<const char*>(&boneOffset), sizeof(boneOffset));
        }

        for (const auto& [boneName, boneIndex] : model.boneMapping) {
            const uint32_t nameLength = static_cast<uint32_t>(boneName.size());
            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(boneName.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&boneIndex), sizeof(boneIndex));
        }

        for (const SkinnedMeshData& mesh : model.meshes) {
            const uint32_t nameLength = static_cast<uint32_t>(mesh.name.size());
            const uint32_t vertexCount = static_cast<uint32_t>(mesh.vertices.size());
            const uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size());

            file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
            file.write(reinterpret_cast<const char*>(&vertexCount), sizeof(vertexCount));
            file.write(reinterpret_cast<const char*>(&indexCount), sizeof(indexCount));
            file.write(mesh.name.data(), nameLength);
            file.write(reinterpret_cast<const char*>(&mesh.aabbMin), sizeof(mesh.aabbMin));
            file.write(reinterpret_cast<const char*>(&mesh.aabbMax), sizeof(mesh.aabbMax));
            file.write(reinterpret_cast<const char*>(&mesh.requiresSkinning), sizeof(mesh.requiresSkinning));
            file.write(reinterpret_cast<const char*>(&mesh.nonDeformingBoneIndex), sizeof(mesh.nonDeformingBoneIndex));
            file.write(reinterpret_cast<const char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.write(reinterpret_cast<const char*>(mesh.vertexWeights.data()), mesh.vertexWeights.size() * sizeof(VertexWeight));
            file.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        const std::streamoff morphSectionBegin = file.tellp();
        SkinnedModelMorphSectionHeader morphHeader{};
        std::memcpy(morphHeader.signature, MORPH_SECTION_SIGNATURE, sizeof(MORPH_SECTION_SIGNATURE));
        morphHeader.meshCount = model.GetMeshCount();

        for (const SkinnedMeshData& mesh : model.meshes) {
            morphHeader.targetCount += static_cast<uint32_t>(mesh.morphTargets.size());
            for (const MorphTargetData& morphTarget : mesh.morphTargets) {
                morphHeader.positionDeltaCount += morphTarget.positionDeltas.size();
                morphHeader.normalDeltaCount += morphTarget.normalDeltas.size();
                morphHeader.tangentDeltaCount += morphTarget.tangentDeltas.size();
            }
        }

        file.write(reinterpret_cast<const char*>(&morphHeader), sizeof(morphHeader));

        // Version 4 appends morph targets after the complete version 3 payload.
        // Each mesh gets a target count, then each target stores its name and
        // sparse object-space position, normal, and tangent delta streams.
        for (const SkinnedMeshData& mesh : model.meshes) {
            const uint32_t morphTargetCount = static_cast<uint32_t>(mesh.morphTargets.size());
            file.write(reinterpret_cast<const char*>(&morphTargetCount), sizeof(morphTargetCount));

            for (const MorphTargetData& morphTarget : mesh.morphTargets) {
                const uint32_t nameLength = static_cast<uint32_t>(morphTarget.name.size());
                const uint32_t positionDeltaCount = static_cast<uint32_t>(morphTarget.positionDeltas.size());
                const uint32_t normalDeltaCount = static_cast<uint32_t>(morphTarget.normalDeltas.size());
                const uint32_t tangentDeltaCount = static_cast<uint32_t>(morphTarget.tangentDeltas.size());
                file.write(reinterpret_cast<const char*>(&nameLength), sizeof(nameLength));
                file.write(reinterpret_cast<const char*>(&positionDeltaCount), sizeof(positionDeltaCount));
                file.write(reinterpret_cast<const char*>(&normalDeltaCount), sizeof(normalDeltaCount));
                file.write(reinterpret_cast<const char*>(&tangentDeltaCount), sizeof(tangentDeltaCount));
                file.write(morphTarget.name.data(), nameLength);
                WriteMorphDeltas(file, morphTarget.positionDeltas);
                WriteMorphDeltas(file, morphTarget.normalDeltas);
                WriteMorphDeltas(file, morphTarget.tangentDeltas);
            }
        }

        if (!file) {
            Logging::Error() << "AssetFormats::SaveSkinnedModel() failed while writing '" << path << "'\n";
            return false;
        }

        const std::streamoff fileEnd = file.tellp();
        const size_t savedFileSize = static_cast<size_t>(fileEnd);
        const size_t morphSectionSize = static_cast<size_t>(fileEnd - morphSectionBegin);
        const int64_t sizeChange = previousFileExisted
            ? static_cast<int64_t>(savedFileSize) - static_cast<int64_t>(previousFileSize)
            : static_cast<int64_t>(savedFileSize);
        const std::string sizeChangeSign = sizeChange >= 0 ? "+" : "-";
        const size_t absoluteSizeChange = static_cast<size_t>(sizeChange >= 0 ? sizeChange : -sizeChange);

        Logging::Debug()
            << "Saved skinned model '" << path << "'"
            << " (before " << (previousFileExisted ? Hell::String::FormatBytesMB(previousFileSize) : "not present")
            << ", after " << Hell::String::FormatBytesMB(savedFileSize)
            << ", change " << sizeChangeSign << Hell::String::FormatBytesMB(absoluteSizeChange)
            << ", morph section " << Hell::String::FormatBytesMB(morphSectionSize)
            << ", morph targets " << morphHeader.targetCount
            << ", sparse deltas [position " << morphHeader.positionDeltaCount
            << ", normal " << morphHeader.normalDeltaCount
            << ", tangent " << morphHeader.tangentDeltaCount << "])\n";

        const size_t basePayloadSize = static_cast<size_t>(morphSectionBegin);
        const bool morphSectionIsLarge = basePayloadSize > 0 && morphSectionSize > basePayloadSize / 2;
        const bool fileGrowthIsLarge = previousFileExisted && savedFileSize > previousFileSize + previousFileSize / 2;
        if (morphSectionIsLarge || fileGrowthIsLarge) {
            Logging::Warning()
                << "Skinned model morph data caused unusually large output growth for '" << path
                << "'; verify its .morphs target allowlist\n";
        }
        return true;
    }

    bool LoadSkinnedModel(const std::string& path, SkinnedModelData& outModel) {
        outModel = {};

        std::ifstream file(path, std::ios::binary);
        SkinnedModelHeader header{};
        if (!ReadSkinnedHeader(file, header)) {
            Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid header in '" << path << "'\n";
            return false;
        }

        outModel.name.resize(header.nameLength);
        file.read(outModel.name.data(), outModel.name.size());
        outModel.vertexCount = header.vertexCount;
        outModel.indexCount = header.indexCount;
        outModel.timestamp = header.timestamp;
        outModel.nodes.resize(header.nodeCount);
        outModel.boneOffsets.resize(header.boneCount);
        outModel.meshes.resize(header.meshCount);

        for (Node& node : outModel.nodes) {
            uint32_t nameLength = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            node.name.resize(nameLength);
            file.read(node.name.data(), node.name.size());
            file.read(reinterpret_cast<char*>(&node.parentIndex), sizeof(node.parentIndex));
            file.read(reinterpret_cast<char*>(&node.localBindTransform), sizeof(node.localBindTransform));
        }

        for (glm::mat4& boneOffset : outModel.boneOffsets) {
            file.read(reinterpret_cast<char*>(&boneOffset), sizeof(boneOffset));
        }

        for (uint32_t i = 0; i < header.boneCount; ++i) {
            uint32_t nameLength = 0;
            unsigned int boneIndex = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));

            std::string boneName(nameLength, '\0');
            file.read(boneName.data(), boneName.size());
            file.read(reinterpret_cast<char*>(&boneIndex), sizeof(boneIndex));
            outModel.boneMapping[boneName] = boneIndex;
        }

        for (SkinnedMeshData& mesh : outModel.meshes) {
            uint32_t nameLength = 0;
            file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
            file.read(reinterpret_cast<char*>(&mesh.vertexCount), sizeof(mesh.vertexCount));
            file.read(reinterpret_cast<char*>(&mesh.indexCount), sizeof(mesh.indexCount));

            mesh.name.resize(nameLength);
            file.read(mesh.name.data(), mesh.name.size());
            file.read(reinterpret_cast<char*>(&mesh.aabbMin), sizeof(mesh.aabbMin));
            file.read(reinterpret_cast<char*>(&mesh.aabbMax), sizeof(mesh.aabbMax));
            file.read(reinterpret_cast<char*>(&mesh.requiresSkinning), sizeof(mesh.requiresSkinning));
            file.read(reinterpret_cast<char*>(&mesh.nonDeformingBoneIndex), sizeof(mesh.nonDeformingBoneIndex));

            mesh.vertices.resize(mesh.vertexCount);
            mesh.vertexWeights.resize(mesh.vertexCount);
            mesh.indices.resize(mesh.indexCount);
            file.read(reinterpret_cast<char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.read(reinterpret_cast<char*>(mesh.vertexWeights.data()), mesh.vertexWeights.size() * sizeof(VertexWeight));
            file.read(reinterpret_cast<char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        if (header.version >= 4) {
            SkinnedModelMorphSectionHeader morphHeader{};
            file.read(reinterpret_cast<char*>(&morphHeader), sizeof(morphHeader));
            if (!file ||
                std::memcmp(morphHeader.signature, MORPH_SECTION_SIGNATURE, sizeof(MORPH_SECTION_SIGNATURE)) != 0 ||
                morphHeader.meshCount != outModel.GetMeshCount()) {
                Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid morph section in '" << path << "'\n";
                outModel = {};
                return false;
            }

            uint64_t loadedMorphTargetCount = 0;
            uint64_t loadedPositionDeltaCount = 0;
            uint64_t loadedNormalDeltaCount = 0;
            uint64_t loadedTangentDeltaCount = 0;

            for (SkinnedMeshData& mesh : outModel.meshes) {
                uint32_t morphTargetCount = 0;
                file.read(reinterpret_cast<char*>(&morphTargetCount), sizeof(morphTargetCount));
                if (!file ||
                    loadedMorphTargetCount > morphHeader.targetCount ||
                    morphTargetCount > morphHeader.targetCount - loadedMorphTargetCount) {
                    Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid morph target count in '" << path << "'\n";
                    outModel = {};
                    return false;
                }
                mesh.morphTargets.resize(morphTargetCount);

                for (MorphTargetData& morphTarget : mesh.morphTargets) {
                    uint32_t nameLength = 0;
                    uint32_t positionDeltaCount = 0;
                    uint32_t normalDeltaCount = 0;
                    uint32_t tangentDeltaCount = 0;
                    file.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
                    file.read(reinterpret_cast<char*>(&positionDeltaCount), sizeof(positionDeltaCount));
                    file.read(reinterpret_cast<char*>(&normalDeltaCount), sizeof(normalDeltaCount));
                    file.read(reinterpret_cast<char*>(&tangentDeltaCount), sizeof(tangentDeltaCount));

                    if (!file || nameLength > 4096) {
                        Logging::Error() << "AssetFormats::LoadSkinnedModel() found an invalid morph target name in '"
                                         << path << "'\n";
                        outModel = {};
                        return false;
                    }

                    morphTarget.name.resize(nameLength);
                    file.read(morphTarget.name.data(), morphTarget.name.size());
                    if (!file ||
                        !ReadMorphDeltas(file, morphTarget.positionDeltas, positionDeltaCount, mesh.vertexCount, mesh.name, path) ||
                        !ReadMorphDeltas(file, morphTarget.normalDeltas, normalDeltaCount, mesh.vertexCount, mesh.name, path) ||
                        !ReadMorphDeltas(file, morphTarget.tangentDeltas, tangentDeltaCount, mesh.vertexCount, mesh.name, path)) {
                        outModel = {};
                        return false;
                    }

                    loadedPositionDeltaCount += positionDeltaCount;
                    loadedNormalDeltaCount += normalDeltaCount;
                    loadedTangentDeltaCount += tangentDeltaCount;
                }

                loadedMorphTargetCount += morphTargetCount;
            }

            if (loadedMorphTargetCount != morphHeader.targetCount ||
                loadedPositionDeltaCount != morphHeader.positionDeltaCount ||
                loadedNormalDeltaCount != morphHeader.normalDeltaCount ||
                loadedTangentDeltaCount != morphHeader.tangentDeltaCount) {
                Logging::Error() << "AssetFormats::LoadSkinnedModel() found inconsistent morph counts in '" << path << "'\n";
                outModel = {};
                return false;
            }
        }

        if (!file) {
            Logging::Error() << "AssetFormats::LoadSkinnedModel() failed while reading '" << path << "'\n";
            outModel = {};
            return false;
        }

        return true;
    }
}
