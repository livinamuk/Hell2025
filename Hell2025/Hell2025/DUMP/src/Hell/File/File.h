#pragma once
#include "FileInfo.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hell::File {

    bool Delete(const std::string& path);
    bool Exists(std::string_view path);
    bool Exists(const std::string& directory, const std::string& extension, const std::string& name);
    bool GetSize(const std::string& path, size_t& outSize);
    bool Rename(const std::string& oldPath, const std::string& newPath);
    uint64_t GetLastModifiedTime(const std::string& path);

    std::string GetName(const std::string& path);
    std::string GetExtension(const std::string& path);
    std::string RemoveExtension(const std::string& path);
    FileInfo GetInfo(const std::string& path);
    std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions = {});
}
