#include "File.h"

#include "Hell/Common/String.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace Hell::File {

    bool Delete(const std::string& path) {
        try {
            if (std::filesystem::remove(path)) {
                return true;
            }
            Logging::Error() << "File::Delete() failed to delete '" << path << "', file does not exist or could not be deleted\n";
            return false;
        }
        catch (const std::filesystem::filesystem_error& e) {
            Logging::Error() << "File::Delete() failed: " << e.what() << "\n";
            return false;
        }
    }

    uint64_t GetLastModifiedTime(const std::string& path) {
        try {
            auto ftime = std::filesystem::last_write_time(path);
            auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(ftime);
            return std::chrono::duration_cast<std::chrono::seconds>(systemTime.time_since_epoch()).count();
        }
        catch (const std::filesystem::filesystem_error& e) {
            Logging::Error() << "File::GetLastModifiedTime() failed for '" << path << "': " << e.what() << "\n";
            return 0;
        }
    }

    bool GetSize(const std::string& path, size_t& outSize) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) {
            Logging::Error() << "File::GetSize() failed for '" << path << "'\n";
            return false;
        }
        outSize = static_cast<size_t>(file.tellg());
        return true;
    }

    bool Exists(std::string_view path) {
        return std::filesystem::exists(std::filesystem::path(std::string(path)));
    }

    bool Exists(const std::string& directory, const std::string& extension, const std::string& name) {
        const std::string lowerName = Hell::String::ToLower(name);
        for (const FileInfo& fileInfo : Hell::File::IterateDirectory(directory, { extension })) {
            if (Hell::String::ToLower(fileInfo.name) == lowerName) return true;
        }
        return false;
    }


    bool Rename(const std::string& oldPath, const std::string& newPath) {
        if (!Exists(oldPath)) {
            Logging::Error() << "File::Rename() failed because old path '" << oldPath << "' does not exist\n";
            return false;
        }

        try {
            std::filesystem::rename(oldPath, newPath);
            return true;
        }
        catch (const std::filesystem::filesystem_error& e) {
            Logging::Error() << "File::Rename() failed: " << e.what() << "\n";
            return false;
        }
    }

    std::string GetName(const std::string& path) {
        return std::filesystem::path(path).stem().string();
    }

    std::string GetExtension(const std::string& path) {
        std::string extension = std::filesystem::path(path).extension().string();
        return extension.starts_with('.') ? extension.substr(1) : extension;
    }

    std::string RemoveExtension(const std::string& path) {
        std::filesystem::path filesystemPath(path);
        filesystemPath.replace_extension();
        return filesystemPath.string();
    }

    FileInfo GetInfo(const std::string& path) {
        if (!Exists(path)) {
            Logging::Error() << "File::GetInfo() failed because '" << path << "' does not exist\n";
            return {};
        }

        const std::filesystem::path filesystemPath(path);
        return { filesystemPath.string(), filesystemPath.stem().string(), GetExtension(path), filesystemPath.parent_path().string() };
    }

    std::vector<FileInfo> IterateDirectory(const std::string& directory, std::vector<std::string> extensions) {
        std::vector<FileInfo> fileInfoList;
        if (!std::filesystem::exists(directory)) return fileInfoList;

        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!std::filesystem::is_regular_file(entry)) continue;

            FileInfo fileInfo = { entry.path().string(), entry.path().stem().string(), GetExtension(entry.path().string()), directory };

            if (extensions.empty() || std::find(extensions.begin(), extensions.end(), fileInfo.ext) != extensions.end()) {
                fileInfoList.push_back(fileInfo);
            }
        }
        return fileInfoList;
    }
}
