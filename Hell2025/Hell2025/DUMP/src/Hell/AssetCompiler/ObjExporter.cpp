#include "ObjExporter.h"

#include "Hell/Logging.h"

#include <cstddef>
#include <fstream>

namespace Hell::AssetCompiler {

    namespace {
        template <typename MeshType>
        bool ExportMeshObj(const std::string& path, const MeshType& mesh) {
            std::ofstream file(path);
            if (!file) {
                Logging::Error() << "AssetCompiler::ExportObj() failed to open '" << path << "'\n";
                return false;
            }

            for (const Vertex& vertex : mesh.vertices) {
                file << "v " << vertex.position.x << " " << vertex.position.y << " " << vertex.position.z << "\n";
            }
            for (const Vertex& vertex : mesh.vertices) {
                file << "vn " << vertex.normal.x << " " << vertex.normal.y << " " << vertex.normal.z << "\n";
            }
            for (const Vertex& vertex : mesh.vertices) {
                file << "vt " << vertex.uv.x << " " << vertex.uv.y << "\n";
            }
            for (size_t i = 0; i + 2 < mesh.indices.size(); i += 3) {
                file << "f "
                     << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << "/" << mesh.indices[i] + 1 << " "
                     << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << "/" << mesh.indices[i + 1] + 1 << " "
                     << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "/" << mesh.indices[i + 2] + 1 << "\n";
            }

            return static_cast<bool>(file);
        }
    }

    bool ExportObj(const std::string& path, const MeshData& mesh) {
        return ExportMeshObj(path, mesh);
    }

    bool ExportObj(const std::string& path, const SkinnedMeshData& mesh) {
        return ExportMeshObj(path, mesh);
    }
}
