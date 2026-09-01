// vk_timer.cpp
#include "vk_timer.h"

#if VULKAN_PROFILING

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

static VulkanFrameTimer gFrameTimer;

void VulkanFrameTimer::RollingAverage::Clear() {
    mSamples.clear();
    mSum = 0.0;
}

void VulkanFrameTimer::RollingAverage::Push(double value) {
    mSamples.push_back(value);
    mSum += value;

    if (mSamples.size() > mCapacity) {
        mSum -= mSamples.front();
        mSamples.pop_front();
    }
}

double VulkanFrameTimer::RollingAverage::GetValue() const {
    if (mSamples.empty()) return 0.0;
    return mSum / double(mSamples.size());
}

void VulkanFrameTimer::Zone::Init(const std::string& name) {
    if (mInitialized) return;
    mName = name;
    mInitialized = true;
}

void VulkanFrameTimer::Zone::ResetRuntimeState() {
    mCpuRunning = false;
    mNestingDepth = 0;
    mActiveRecordIndex = UINT32_MAX;
}

std::string VulkanFrameTimer::FormatMs(double ms, size_t precision) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << std::setprecision((int)precision) << ms << " ms";
    return oss.str();
}

double VulkanFrameTimer::TimestampDeltaToMs(uint64_t start, uint64_t end) {
    if (end <= start) return 0.0;
    double timestampPeriod = double(VulkanDeviceManager::GetProperties().limits.timestampPeriod);
    return double(end - start) * timestampPeriod / 1000000.0;
}

bool VulkanFrameTimer::EnsureFrameQueryPool(uint32_t frameIndex) {
    if (frameIndex >= FRAME_OVERLAP) return false;
    if (mFrameQueries[frameIndex].queryPool != VK_NULL_HANDLE) return true;

    VkDevice device = VulkanDeviceManager::GetDevice();
    if (device == VK_NULL_HANDLE) return false;

    VkQueryPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO };
    createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
    createInfo.queryCount = kMaxQueriesPerFrame;

    return vkCreateQueryPool(device, &createInfo, nullptr, &mFrameQueries[frameIndex].queryPool) == VK_SUCCESS;
}

void VulkanFrameTimer::ClearStrings() {
    m_zoneList.clear();
    m_cpuTimingList.clear();
    m_gpuTimingList.clear();
    m_totalCpuFrameTime.clear();
    m_totalGpuFrameTime.clear();
}

void VulkanFrameTimer::BuildStringsFromResolvedFrame(const FrameQueryData& frame, const std::vector<uint64_t>& timestamps) {
    ClearStrings();

    std::vector<std::pair<uint64_t, const FrameZoneRecord*>> ordered;
    ordered.reserve(frame.records.size());

    for (const FrameZoneRecord& record : frame.records) {
        if (!record.complete) continue;
        if (record.startQuery >= timestamps.size() || record.endQuery >= timestamps.size()) continue;
        ordered.emplace_back(timestamps[record.startQuery], &record);
    }

    std::stable_sort(ordered.begin(), ordered.end(), [](const std::pair<uint64_t, const FrameZoneRecord*>& a, const std::pair<uint64_t, const FrameZoneRecord*>& b) { return a.first < b.first; });

    const std::string white = "[COL=1.0,1.0,1.0,1.0]";
    const std::string orange = "[COL=1.0,0.65,0.0,1.0]";
    const std::string yellow = "[COL=1.0,1.0,0.0,1.0]";
    const std::string red = "[COL=1.0,0.0,0.0,1.0]";
    const std::string lightBlue = "[COL=0.68,0.85,0.9,1.0]";
    const std::string lightGreen = "[COL=0.56,0.93,0.56,1.0]";

    for (const auto& orderedRecord : ordered) {
        const FrameZoneRecord* record = orderedRecord.second;
        auto zoneIt = mZones.find(record->name);
        if (zoneIt == mZones.end()) continue;

        const Zone& zone = zoneIt->second;
        if (record->color != ZoneColor::COL_WHITE) {
            std::string activeColor = white;

            switch (record->color) {
            case ZoneColor::COL_ORANGE: activeColor = orange; break;
            case ZoneColor::COL_YELLOW: activeColor = yellow; break;
            case ZoneColor::COL_RED: activeColor = red; break;
            case ZoneColor::COL_LIGHT_BLUE: activeColor = lightBlue; break;
            case ZoneColor::COL_LIGHT_GREEN: activeColor = lightGreen; break;
            default: break;
            }

            m_zoneList += activeColor;
            m_cpuTimingList += activeColor;
            m_gpuTimingList += activeColor;
        }

        m_zoneList += zone.mName;
        m_zoneList += "\n";

        m_cpuTimingList += FormatMs(zone.mCpuRollingAverage.GetValue(), kOutputPrecision);
        m_cpuTimingList += "\n";

        m_gpuTimingList += FormatMs(zone.mGpuRollingAverage.GetValue(), kOutputPrecision);
        m_gpuTimingList += "\n";

        if (record->color != ZoneColor::COL_WHITE) {
            m_zoneList += white;
            m_cpuTimingList += white;
            m_gpuTimingList += white;
        }
    }

    m_totalCpuFrameTime = "Total: " + FormatMs(mCpuFrameRollingAverage.GetValue(), kOutputPrecision);
    m_totalGpuFrameTime = "Total: " + FormatMs(mGpuFrameRollingAverage.GetValue(), kOutputPrecision);
}

void VulkanFrameTimer::ResolveFrame(uint32_t frameIndex) {
    if (frameIndex >= FRAME_OVERLAP) return;

    FrameQueryData& frame = mFrameQueries[frameIndex];
    if (frame.queryPool == VK_NULL_HANDLE || !frame.hasResults || frame.queryCount == 0) return;

    VkDevice device = VulkanDeviceManager::GetDevice();
    if (device == VK_NULL_HANDLE) return;

    std::vector<uint64_t> timestamps(frame.queryCount, 0);
    VkDeviceSize dataSize = VkDeviceSize(sizeof(uint64_t) * timestamps.size());
    VkResult result = vkGetQueryPoolResults(device, frame.queryPool, 0, frame.queryCount, dataSize, timestamps.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT);
    if (result != VK_SUCCESS) return;

    if (frame.frameStartQuery < timestamps.size() && frame.frameEndQuery < timestamps.size()) {
        mGpuFrameRollingAverage.Push(TimestampDeltaToMs(timestamps[frame.frameStartQuery], timestamps[frame.frameEndQuery]));
    }

    for (const FrameZoneRecord& record : frame.records) {
        if (!record.complete) continue;
        if (record.startQuery >= timestamps.size() || record.endQuery >= timestamps.size()) continue;

        Zone& zone = mZones[record.name];
        if (!zone.mInitialized) zone.Init(record.name);
        zone.mColor = record.color;
        zone.mGpuRollingAverage.Push(TimestampDeltaToMs(timestamps[record.startQuery], timestamps[record.endQuery]));
    }

    BuildStringsFromResolvedFrame(frame, timestamps);
    frame.hasResults = false;
}

void VulkanFrameTimer::BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex) {
    if (commandBuffer == VK_NULL_HANDLE) return;

    uint32_t slotIndex = frameIndex % FRAME_OVERLAP;
    if (!EnsureFrameQueryPool(slotIndex)) return;

    ResolveFrame(slotIndex);

    mFrameId += 1;
    mFrameActive = false;
    mCpuFrameRunning = false;

    for (auto& kv : mZones) {
        kv.second.ResetRuntimeState();
    }

    FrameQueryData& frame = mFrameQueries[slotIndex];
    frame.queryCount = 0;
    frame.frameStartQuery = UINT32_MAX;
    frame.frameEndQuery = UINT32_MAX;
    frame.frameId = mFrameId;
    frame.hasResults = false;
    frame.records.clear();

    if (mFrameId <= kSkipZoneFrames) {
        ClearStrings();
        return;
    }

    vkCmdResetQueryPool(commandBuffer, frame.queryPool, 0, kMaxQueriesPerFrame);

    frame.frameStartQuery = frame.queryCount++;
    frame.frameEndQuery = frame.queryCount++;

    mActiveFrameIndex = slotIndex;
    mFrameActive = true;
    mCpuFrameStartTime = Clock::now();
    mCpuFrameRunning = true;

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.queryPool, frame.frameStartQuery);
}

void VulkanFrameTimer::EndFrame(VkCommandBuffer commandBuffer) {
    if (!mFrameActive) return;
    if (commandBuffer == VK_NULL_HANDLE) return;

    FrameQueryData& frame = mFrameQueries[mActiveFrameIndex];
    if (frame.queryPool == VK_NULL_HANDLE) return;

    if (mCpuFrameRunning) {
        Clock::time_point endCpu = Clock::now();
        double cpuFrameMs = std::chrono::duration<double, std::milli>(endCpu - mCpuFrameStartTime).count();
        mCpuFrameRollingAverage.Push(cpuFrameMs);
        mCpuFrameRunning = false;
    }

    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.queryPool, frame.frameEndQuery);
    frame.hasResults = true;
    mFrameActive = false;
}

void VulkanFrameTimer::BeginZone(VkCommandBuffer commandBuffer, const std::string& name, VulkanFrameTimer::ZoneColor color) {
    if (!mFrameActive) return;
    if (commandBuffer == VK_NULL_HANDLE || name.empty()) return;

    FrameQueryData& frame = mFrameQueries[mActiveFrameIndex];
    if (frame.queryPool == VK_NULL_HANDLE) return;

    Zone& zone = mZones[name];
    if (!zone.mInitialized) zone.Init(name);

    zone.mColor = color;
    zone.mTouchedFrameId = mFrameId;

    if (zone.mNestingDepth == 0) {
        if (frame.queryCount + 2 > kMaxQueriesPerFrame) return;

        FrameZoneRecord& record = frame.records.emplace_back();
        record.name = name;
        record.color = color;
        record.startQuery = frame.queryCount++;
        record.endQuery = frame.queryCount++;

        zone.mActiveRecordIndex = static_cast<uint32_t>(frame.records.size() - 1);
        zone.mCpuStartTime = Clock::now();
        zone.mCpuRunning = true;

        vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame.queryPool, record.startQuery);
    }

    zone.mNestingDepth += 1;
}

void VulkanFrameTimer::EndZone(VkCommandBuffer commandBuffer, const std::string& name) {
    if (!mFrameActive) return;
    if (commandBuffer == VK_NULL_HANDLE || name.empty()) return;

    auto it = mZones.find(name);
    if (it == mZones.end()) return;

    Zone& zone = it->second;
    if (zone.mNestingDepth == 0) return;

    zone.mNestingDepth -= 1;
    if (zone.mNestingDepth != 0) return;

    FrameQueryData& frame = mFrameQueries[mActiveFrameIndex];
    if (frame.queryPool == VK_NULL_HANDLE) return;
    if (zone.mActiveRecordIndex >= frame.records.size()) return;

    if (zone.mCpuRunning) {
        Clock::time_point endCpu = Clock::now();
        double zoneCpuMs = std::chrono::duration<double, std::milli>(endCpu - zone.mCpuStartTime).count();
        zone.mCpuRollingAverage.Push(zoneCpuMs);
        zone.mCpuRunning = false;
    }

    FrameZoneRecord& record = frame.records[zone.mActiveRecordIndex];
    vkCmdWriteTimestamp(commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame.queryPool, record.endQuery);
    record.complete = true;
    zone.mTouchedFrameId = mFrameId;
    zone.mActiveRecordIndex = UINT32_MAX;
}

void VulkanFrameTimer::Reset() {
    VkDevice device = VulkanDeviceManager::GetDevice();
    if (device != VK_NULL_HANDLE) {
        for (FrameQueryData& frame : mFrameQueries) {
            if (frame.queryPool != VK_NULL_HANDLE) {
                vkDestroyQueryPool(device, frame.queryPool, nullptr);
                frame.queryPool = VK_NULL_HANDLE;
            }
        }
    }

    for (FrameQueryData& frame : mFrameQueries) {
        frame.queryCount = 0;
        frame.frameStartQuery = UINT32_MAX;
        frame.frameEndQuery = UINT32_MAX;
        frame.frameId = 0;
        frame.hasResults = false;
        frame.records.clear();
    }

    mZones.clear();
    mCpuFrameRollingAverage.Clear();
    mGpuFrameRollingAverage.Clear();
    mFrameId = 0;
    mActiveFrameIndex = 0;
    mCpuFrameRunning = false;
    mFrameActive = false;
    ClearStrings();
}

const std::string& VulkanFrameTimer::GetZoneList() const {
    return m_zoneList;
}

const std::string& VulkanFrameTimer::GetCPUTimingList() const {
    return m_cpuTimingList;
}

const std::string& VulkanFrameTimer::GetGPUTimingList() const {
    return m_gpuTimingList;
}

bool VulkanFrameTimer::TryGetZoneTiming(std::string_view zoneName, double& cpuMilliseconds, double& gpuMilliseconds) const {
    const auto zoneIt = mZones.find(std::string(zoneName));
    if (zoneIt == mZones.end()) {
        cpuMilliseconds = 0.0;
        gpuMilliseconds = 0.0;
        return false;
    }

    cpuMilliseconds = zoneIt->second.mCpuRollingAverage.GetValue();
    gpuMilliseconds = zoneIt->second.mGpuRollingAverage.GetValue();
    return true;
}

const std::string& VulkanFrameTimer::GetTotalCPUFrameTime() const {
    return m_totalCpuFrameTime;
}

const std::string& VulkanFrameTimer::GetTotalGPUFrameTime() const {
    return m_totalGpuFrameTime;
}

float VulkanFrameTimer::GetTotalGPUFrameTimeFloat() const {
    return mGpuFrameRollingAverage.GetValue();
}

VulkanFrameTimer::ZoneScope::ZoneScope(VkCommandBuffer commandBuffer, VulkanFrameTimer& timer, const char* name, VulkanFrameTimer::ZoneColor color) {
    mTimer = &timer;
    mCommandBuffer = commandBuffer;
    mName = name ? name : "";
    mActive = true;
    mTimer->BeginZone(mCommandBuffer, mName, color);
}

VulkanFrameTimer::ZoneScope::ZoneScope(VkCommandBuffer commandBuffer, VulkanFrameTimer& timer, const std::string& name, VulkanFrameTimer::ZoneColor color) {
    mTimer = &timer;
    mCommandBuffer = commandBuffer;
    mName = name;
    mActive = true;
    mTimer->BeginZone(mCommandBuffer, mName, color);
}

VulkanFrameTimer::ZoneScope::~ZoneScope() {
    if (mActive && mTimer) {
        mTimer->EndZone(mCommandBuffer, mName);
    }
}

void VulkanFrameTimer::ZoneScope::Release() {
    mActive = false;
}

VulkanFrameTimer& GetVulkanTimer() {
    return gFrameTimer;
}

#endif
