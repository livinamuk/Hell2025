#include "AssetFormats.h"
#include "AssetFormatHeaders.h"

#include "Hell/Logging.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Hell::AssetFormats {

    namespace {
        template <size_t Size>
        void CopyBvhSignature(char (&destination)[Size], const char* source) {
            std::memset(destination, 0, Size);
            std::memcpy(destination, source, std::min(std::strlen(source), Size - 1));
        }

        bool HasBvhSignature(const char* actual, const char* expected) {
            return std::memcmp(actual, expected, std::strlen(expected)) == 0;
        }
    }

    bool ReadModelBvhMetadata(const std::string& path, ModelBvhMetadata& outMetadata) {
        outMetadata = {};

        std::ifstream file(path, std::ios::binary);
        ModelBvhHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file || !HasBvhSignature(header.signature, MODEL_BVH_SIGNATURE)) {
            Logging::Error() << "AssetFormats::ReadModelBvhMetadata() found an invalid header in '" << path << "'\n";
            return false;
        }

        outMetadata.version = header.version;
        outMetadata.meshCount = header.meshCount;
        outMetadata.timestamp = header.timestamp;
        return true;
    }

    bool SaveModelBvh(const std::string& path, const ModelBvhData& bvh) {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::SaveModelBvh() failed to open '" << path << "'\n";
            return false;
        }

        ModelBvhHeader modelHeader{};
        CopyBvhSignature(modelHeader.signature, MODEL_BVH_SIGNATURE);
        modelHeader.version = 1;
        modelHeader.meshCount = bvh.bvhs.size();
        modelHeader.timestamp = bvh.timestamp;
        file.write(reinterpret_cast<const char*>(&modelHeader), sizeof(modelHeader));

        for (const MeshBvh& meshBvh : bvh.bvhs) {
            MeshBvhHeader meshHeader{};
            CopyBvhSignature(meshHeader.signature, MESH_BVH_SIGNATURE);
            meshHeader.nodeCount = meshBvh.m_nodes.size();
            meshHeader.floatCount = meshBvh.m_triangles.size() * 12;

            file.write(reinterpret_cast<const char*>(&meshHeader), sizeof(meshHeader));
            file.write(reinterpret_cast<const char*>(meshBvh.m_nodes.data()), meshBvh.m_nodes.size() * sizeof(BvhNode));
            file.write(reinterpret_cast<const char*>(meshBvh.m_triangles.data()), meshBvh.m_triangles.size() * sizeof(BVHTriangle));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::SaveModelBvh() failed while writing '" << path << "'\n";
            return false;
        }

        Logging::Debug() << "Saved model BVH '" << path << "'\n";
        return true;
    }

    bool LoadModelBvh(const std::string& path, ModelBvhData& outBvh) {
        outBvh = {};

        ModelBvhMetadata metadata;
        if (!ReadModelBvhMetadata(path, metadata)) {
            return false;
        }

        std::ifstream file(path, std::ios::binary);
        file.seekg(sizeof(ModelBvhHeader), std::ios::beg);

        outBvh.timestamp = metadata.timestamp;
        outBvh.bvhs.resize(metadata.meshCount);

        for (MeshBvh& meshBvh : outBvh.bvhs) {
            MeshBvhHeader header{};
            file.read(reinterpret_cast<char*>(&header), sizeof(header));

            if (!file || !HasBvhSignature(header.signature, MESH_BVH_SIGNATURE)) {
                Logging::Error() << "AssetFormats::LoadModelBvh() found an invalid mesh header in '" << path << "'\n";
                outBvh = {};
                return false;
            }

            if (header.floatCount % 12 != 0) {
                Logging::Error() << "AssetFormats::LoadModelBvh(..) found an invalid triangle float count in '" << path << "'\n";
                outBvh = {};
                return false;
            }

            meshBvh.m_nodes.resize(header.nodeCount);
            meshBvh.m_triangles.resize(header.floatCount / 12);
            file.read(reinterpret_cast<char*>(meshBvh.m_nodes.data()), meshBvh.m_nodes.size() * sizeof(BvhNode));
            file.read(reinterpret_cast<char*>(meshBvh.m_triangles.data()), meshBvh.m_triangles.size() * sizeof(BVHTriangle));
        }

        if (!file) {
            Logging::Error() << "AssetFormats::LoadModelBvh() failed while reading '" << path << "'\n";
            outBvh = {};
            return false;
        }

        return true;
    }
}
