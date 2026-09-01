#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace Hell::MemoryTracker {

struct MemoryReportEntry {
    std::string name;
    size_t cpuBytes = 0;
    size_t gpuBytes = 0;
};

struct MemoryReportCategory {
    std::string name;
    std::vector<MemoryReportEntry> entries;

    size_t GetTotalCPUBytes() const;
    size_t GetTotalGPUBytes() const;
};

struct MemoryReport {
    std::vector<MemoryReportCategory> categories;

    size_t GetTotalCPUBytes() const;
    size_t GetTotalGPUBytes() const;
};

std::string FormatMemorySize(size_t bytes);
MemoryReport GetMemoryReport();

} // namespace Hell::MemoryTracker
