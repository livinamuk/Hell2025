#include "AssetFormats.h"
#include "AssetFormatHeaders.h"

#include "Hell/File.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Hell::AssetFormats {

    namespace {
        template <size_t Size>
        void CopyString(char (&destination)[Size], const std::string& source) {
            std::memset(destination, 0, Size);
            std::memcpy(destination, source.data(), std::min(source.size(), Size - 1));
        }

        bool HasSignature(const char* actual, const char* expected) {
            return std::memcmp(actual, expected, std::strlen(expected)) == 0;
        }

        uint32_t ReadModelVersion(std::ifstream& file) {
            file.seekg(SIGNATURE_BUFFER_SIZE, std::ios::beg);

            uint32_t version = 0;
            file.read(reinterpret_cast<char*>(&version), sizeof(version));
            return file ? version : 0;
        }
    }

    bool ReadModelMetadata(const std::string& path, ModelMetadata& outMetadata) {
        outMetadata = {};

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::ReadModelMetadata() failed to open '" << path << "'\n";
            return false;
        }

        const uint32_t version = ReadModelVersion(file);
        file.clear();
        file.seekg(0, std::ios::beg);

        if (version == 2) {
            ModelHeaderV2 header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            if (!file || !HasSignature(header.signature, MODEL_SIGNATURE)) {
                Logging::Error() << "AssetFormats::ReadModelMetadata() found an invalid v2 header in '" << path << "'\n";
                return false;
            }

            outMetadata.version = header.version;
            outMetadata.meshCount = header.meshCount;
            outMetadata.timestamp = header.timestamp;
            outMetadata.aabbMin = header.aabbMin;
            outMetadata.aabbMax = header.aabbMax;
            return true;
        }

        if (version == 3) {
            ModelHeaderV3 header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            if (!file || !HasSignature(header.signature, MODEL_SIGNATURE)) {
                Logging::Error() << "AssetFormats::ReadModelMetadata() found an invalid v3 header in '" << path << "'\n";
                return false;
            }

            outMetadata.version = header.version;
            outMetadata.meshCount = header.meshCount;
            outMetadata.armatureCount = header.armatureCount;
            outMetadata.timestamp = header.timestamp;
            outMetadata.aabbMin = header.aabbMin;
            outMetadata.aabbMax = header.aabbMax;
            return true;
        }

        Logging::Error() << "AssetFormats::ReadModelMetadata() found unsupported version " << version << " in '" << path << "'\n";
        return false;
    }

    bool SaveModel(const std::string& path, const ModelData& model) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::SaveModel() failed to open '" << path << "'\n";
            return false;
        }

        ModelHeaderV2 modelHeader{};
        CopyString(modelHeader.signature, MODEL_SIGNATURE);
        modelHeader.version = 2;
        modelHeader.meshCount = static_cast<uint32_t>(model.meshes.size());
        modelHeader.timestamp = model.timestamp;
        modelHeader.aabbMin = model.aabbMin;
        modelHeader.aabbMax = model.aabbMax;
        file.write(reinterpret_cast<const char*>(&modelHeader), sizeof(modelHeader));

        for (const MeshData& mesh : model.meshes) {
            MeshHeaderV2 meshHeader{};
            CopyString(meshHeader.signature, MESH_SIGNATURE);
            CopyString(meshHeader.name, mesh.name);
            meshHeader.vertexCount = static_cast<uint32_t>(mesh.vertices.size());
            meshHeader.indexCount = static_cast<uint32_t>(mesh.indices.size());
            meshHeader.parentIndex = mesh.parentIndex;
            meshHeader.aabbMin = mesh.aabbMin;
            meshHeader.aabbMax = mesh.aabbMax;
            meshHeader.localTransform = mesh.localTransform;
            meshHeader.inverseBindTransform = mesh.inverseBindTransform;

            file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(meshHeader));
            file.write(reinterpret_cast<const char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.write(reinterpret_cast<const char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::SaveModel() failed while writing '" << path << "'\n";
            return false;
        }

        Logging::Debug() << "Saved model '" << path << "'\n";
        return true;
    }

    bool LoadModel(const std::string& path, ModelData& outModel) {
        outModel = {};

        ModelMetadata metadata;
        if (!ReadModelMetadata(path, metadata)) {
            return false;
        }

        std::ifstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }

        outModel.name = File::GetName(path);
        outModel.meshCount = metadata.meshCount;
        outModel.armatureCount = metadata.armatureCount;
        outModel.timestamp = metadata.timestamp;
        outModel.aabbMin = metadata.aabbMin;
        outModel.aabbMax = metadata.aabbMax;
        outModel.meshes.resize(metadata.meshCount);

        const std::streamoff modelHeaderSize = metadata.version == 2
            ? static_cast<std::streamoff>(sizeof(ModelHeaderV2))
            : static_cast<std::streamoff>(sizeof(ModelHeaderV3));
        file.seekg(modelHeaderSize, std::ios::beg);

        for (MeshData& mesh : outModel.meshes) {
            MeshHeaderV2 header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            if (!file || !HasSignature(header.signature, MESH_SIGNATURE)) {
                Logging::Error() << "AssetFormats::LoadModel() found an invalid mesh header in '" << path << "'\n";
                outModel = {};
                return false;
            }

            mesh.name.assign(header.name, strnlen(header.name, sizeof(header.name)));
            mesh.vertexCount = header.vertexCount;
            mesh.indexCount = header.indexCount;
            mesh.parentIndex = header.parentIndex;
            mesh.aabbMin = header.aabbMin;
            mesh.aabbMax = header.aabbMax;
            mesh.localTransform = header.localTransform;
            mesh.inverseBindTransform = header.inverseBindTransform;
            mesh.vertices.resize(header.vertexCount);
            mesh.indices.resize(header.indexCount);

            file.read(reinterpret_cast<char*>(mesh.vertices.data()), mesh.vertices.size() * sizeof(Vertex));
            file.read(reinterpret_cast<char*>(mesh.indices.data()), mesh.indices.size() * sizeof(uint32_t));
        }

        outModel.armatures.resize(metadata.armatureCount);
        for (ArmatureData& armature : outModel.armatures) {
            ArmatureHeader header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));
            if (!file) {
                Logging::Error() << "AssetFormats::LoadModel() failed while reading armatures from '" << path << "'\n";
                outModel = {};
                return false;
            }

            armature.name.assign(header.name, strnlen(header.name, sizeof(header.name)));
            armature.boneCount = header.boneCount;
            armature.bones.resize(header.boneCount);
            file.read(reinterpret_cast<char*>(armature.bones.data()), armature.bones.size() * sizeof(Bone));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::LoadModel() failed while reading '" << path << "'\n";
            outModel = {};
            return false;
        }

        return true;
    }
}
