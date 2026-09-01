#include "AssetFormats.h"
#include "AssetFormatHeaders.h"

#include "Hell/File.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <cstring>
#include <fstream>

namespace Hell::AssetFormats {

    bool LoadHeightMap(const std::string& path, HeightMapData& outHeightMap) {
        outHeightMap = {};

        std::ifstream file(path, std::ios::binary);
        HeightMapHeader header{};
        file.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!file || std::memcmp(header.signature, HEIGHT_MAP_SIGNATURE, std::strlen(HEIGHT_MAP_SIGNATURE)) != 0) {
            Logging::Error() << "AssetFormats::LoadHeightMap() found an invalid header in '" << path << "'\n";
            return false;
        }

        outHeightMap.textureWidth = header.width;
        outHeightMap.textureHeight = header.height;
        outHeightMap.data.resize(static_cast<size_t>(header.width) * header.height);
        file.read(reinterpret_cast<char*>(outHeightMap.data.data()), outHeightMap.data.size() * sizeof(float));

        if (!file) {
            Logging::Error() << "AssetFormats::LoadHeightMap() failed while reading '" << path << "'\n";
            outHeightMap = {};
            return false;
        }

        return true;
    }

    bool SaveHeightMap(const std::string& path, const HeightMapData& heightMap) {
        const size_t expectedSize = static_cast<size_t>(heightMap.textureWidth) * heightMap.textureHeight;
        if (heightMap.data.size() != expectedSize) {
            Logging::Error() << "AssetFormats::SaveHeightMap() received invalid dimensions for '" << path << "'\n";
            return false;
        }

        std::ofstream file(path, std::ios::binary);
        if (!file) {
            Logging::Error() << "AssetFormats::SaveHeightMap() failed to open '" << path << "'\n";
            return false;
        }

        HeightMapHeader header{};
        std::memcpy(header.signature, HEIGHT_MAP_SIGNATURE, std::strlen(HEIGHT_MAP_SIGNATURE));
        const std::string name = File::GetName(path);
        std::memcpy(header.name, name.data(), std::min(name.size(), sizeof(header.name) - 1));
        header.width = heightMap.textureWidth;
        header.height = heightMap.textureHeight;

        file.write(reinterpret_cast<const char*>(&header), sizeof(header));
        file.write(reinterpret_cast<const char*>(heightMap.data.data()), heightMap.data.size() * sizeof(float));
        return static_cast<bool>(file);
    }
}
