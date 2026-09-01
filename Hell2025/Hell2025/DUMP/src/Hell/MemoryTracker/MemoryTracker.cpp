#include "MemoryTracker.h"

#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

#include <iomanip>
#include <sstream>

namespace Hell::MemoryTracker {

size_t MemoryReportCategory::GetTotalCPUBytes() const {
    size_t totalBytes = 0;

    for (const MemoryReportEntry& entry : entries) {
        totalBytes += entry.cpuBytes;
    }

    return totalBytes;
}

size_t MemoryReportCategory::GetTotalGPUBytes() const {
    size_t totalBytes = 0;

    for (const MemoryReportEntry& entry : entries) {
        totalBytes += entry.gpuBytes;
    }

    return totalBytes;
}

size_t MemoryReport::GetTotalCPUBytes() const {
    size_t totalBytes = 0;

    for (const MemoryReportCategory& category : categories) {
        totalBytes += category.GetTotalCPUBytes();
    }

    return totalBytes;
}

size_t MemoryReport::GetTotalGPUBytes() const {
    size_t totalBytes = 0;

    for (const MemoryReportCategory& category : categories) {
        totalBytes += category.GetTotalGPUBytes();
    }

    return totalBytes;
}

std::string FormatMemorySize(size_t bytes) {
    constexpr size_t bytesPerKilobyte = 1024;
    constexpr size_t bytesPerMegabyte = bytesPerKilobyte * 1024;
    constexpr size_t bytesPerGigabyte = bytesPerMegabyte * 1024;

    if (bytes == 0) return "-";

    if (bytes < bytesPerKilobyte) {
        return std::to_string(bytes) + " B";
    }

    size_t divisor = bytesPerKilobyte;
    const char* suffix = "KB";

    if (bytes >= bytesPerGigabyte) {
        divisor = bytesPerGigabyte;
        suffix = "GB";
    }
    else if (bytes >= bytesPerMegabyte) {
        divisor = bytesPerMegabyte;
        suffix = "MB";
    }

    const double value = static_cast<double>(bytes) / static_cast<double>(divisor);

    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value << ' ' << suffix;
    return stream.str();
}

MemoryReport GetMemoryReport() {
    MemoryReport report;
    ResourceManager::AppendMemoryReport(report);
    OpenGL::ResourceManager::AppendMemoryReport(report);
    VulkanResourceManager::AppendMemoryReport(report);
    return report;
}

} // namespace Hell::MemoryTracker
