// vk_timer.h
#pragma once

#include "Hell/Render/API/Vulkan/vk_common.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#if !defined(VULKAN_PROFILING)
#define VULKAN_PROFILING 1
#endif

#if VULKAN_PROFILING

struct VulkanFrameTimer {
    static constexpr uint32_t kSkipZoneFrames = 1;
    static constexpr uint32_t kMaxQueriesPerFrame = 512;
    static constexpr size_t   kAverageFrameCount = 60;
    static constexpr size_t   kOutputPrecision = 2;

    using Clock = std::chrono::steady_clock;

    struct RollingAverage {
        std::deque<double> mSamples;
        size_t mCapacity = kAverageFrameCount;
        double mSum = 0.0;

        void Clear();
        void Push(double value);
        double GetValue() const;
    };

    enum struct ZoneColor {
        COL_WHITE,
        COL_ORANGE,
        COL_YELLOW,
        COL_RED,
        COL_LIGHT_BLUE,
        COL_LIGHT_GREEN
    };

    struct Zone {
        std::string mName;
        RollingAverage mCpuRollingAverage;
        RollingAverage mGpuRollingAverage;
        Clock::time_point mCpuStartTime{};
        uint64_t mTouchedFrameId = 0;
        uint32_t mActiveRecordIndex = UINT32_MAX;
        int mNestingDepth = 0;
        bool mCpuRunning = false;
        bool mInitialized = false;
        ZoneColor mColor = ZoneColor::COL_WHITE;

        void Init(const std::string& name);
        void ResetRuntimeState();
    };

    struct ZoneScope {
        VulkanFrameTimer* mTimer = nullptr;
        VkCommandBuffer mCommandBuffer = VK_NULL_HANDLE;
        std::string mName;
        bool mActive = false;

        ZoneScope(VkCommandBuffer commandBuffer, VulkanFrameTimer& timer, const char* name, VulkanFrameTimer::ZoneColor color);
        ZoneScope(VkCommandBuffer commandBuffer, VulkanFrameTimer& timer, const std::string& name, VulkanFrameTimer::ZoneColor color);
        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;
        ~ZoneScope();

        void Release();
    };

    static inline ZoneColor s_DefaultColor = ZoneColor::COL_ORANGE;

    void BeginFrame(VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void EndFrame(VkCommandBuffer commandBuffer);
    void BeginZone(VkCommandBuffer commandBuffer, const std::string& name, VulkanFrameTimer::ZoneColor color);
    void EndZone(VkCommandBuffer commandBuffer, const std::string& name);
    void Reset();

    const std::string& GetZoneList() const;
    const std::string& GetCPUTimingList() const;
    const std::string& GetGPUTimingList() const;
    bool TryGetZoneTiming(std::string_view zoneName, double& cpuMilliseconds, double& gpuMilliseconds) const;
    const std::string& GetTotalCPUFrameTime() const;
    const std::string& GetTotalGPUFrameTime() const;
    float GetTotalGPUFrameTimeFloat() const;

private:
    struct FrameZoneRecord {
        std::string name;
        ZoneColor color = ZoneColor::COL_WHITE;
        uint32_t startQuery = UINT32_MAX;
        uint32_t endQuery = UINT32_MAX;
        bool complete = false;
    };

    struct FrameQueryData {
        VkQueryPool queryPool = VK_NULL_HANDLE;
        uint32_t queryCount = 0;
        uint32_t frameStartQuery = UINT32_MAX;
        uint32_t frameEndQuery = UINT32_MAX;
        uint64_t frameId = 0;
        bool hasResults = false;
        std::vector<FrameZoneRecord> records;
    };

    bool EnsureFrameQueryPool(uint32_t frameIndex);
    void ResolveFrame(uint32_t frameIndex);
    void ClearStrings();
    void BuildStringsFromResolvedFrame(const FrameQueryData& frame, const std::vector<uint64_t>& timestamps);
    static std::string FormatMs(double ms, size_t precision);
    static double TimestampDeltaToMs(uint64_t start, uint64_t end);

    std::unordered_map<std::string, Zone> mZones;
    FrameQueryData mFrameQueries[FRAME_OVERLAP];
    RollingAverage mCpuFrameRollingAverage;
    RollingAverage mGpuFrameRollingAverage;

    Clock::time_point mCpuFrameStartTime{};
    uint64_t mFrameId = 0;
    uint32_t mActiveFrameIndex = 0;
    bool mCpuFrameRunning = false;
    bool mFrameActive = false;

    std::string m_zoneList;
    std::string m_cpuTimingList;
    std::string m_gpuTimingList;
    std::string m_totalCpuFrameTime;
    std::string m_totalGpuFrameTime;
};

VulkanFrameTimer& GetVulkanTimer();

#define VulkanProfilerConcatImpl(a,b) a##b
#define VulkanProfilerConcat(a,b) VulkanProfilerConcatImpl(a,b)

#define ProfilerVulkanBeginFrame(commandBuffer, frameIndex) GetVulkanTimer().BeginFrame((commandBuffer), (frameIndex))
#define ProfilerVulkanEndFrame(commandBuffer) GetVulkanTimer().EndFrame((commandBuffer))
#define ProfilerVulkanReset() GetVulkanTimer().Reset()

#define ProfilerVulkanZone(label) VulkanFrameTimer::ZoneScope VulkanProfilerConcat(_vk_gpu_zone_, __COUNTER__){ commandBuffer, GetVulkanTimer(), (label), VulkanFrameTimer::s_DefaultColor }
#define ProfilerVulkanZoneCommandBuffer(commandBuffer, label) VulkanFrameTimer::ZoneScope VulkanProfilerConcat(_vk_gpu_zone_, __COUNTER__){ (commandBuffer), GetVulkanTimer(), (label), VulkanFrameTimer::s_DefaultColor }
#define ProfilerVulkanZoneColor(label, color) VulkanFrameTimer::ZoneScope VulkanProfilerConcat(_vk_gpu_zone_, __COUNTER__){ commandBuffer, GetVulkanTimer(), (label), VulkanFrameTimer::ZoneColor::color }
#define ProfilerVulkanZoneRed(label) ProfilerVulkanZoneColor(label, COL_RED)
#define ProfilerVulkanZoneOrange(label) ProfilerVulkanZoneColor(label, COL_ORANGE)
#define ProfilerVulkanZoneYellow(label) ProfilerVulkanZoneColor(label, COL_YELLOW)
#define ProfilerVulkanZoneLightBlue(label) ProfilerVulkanZoneColor(label, COL_LIGHT_BLUE)
#define ProfilerVulkanZoneLightGreen(label) ProfilerVulkanZoneColor(label, COL_LIGHT_GREEN)

#define ProfilerVulkanSetDefaultZoneColor(color) VulkanFrameTimer::s_DefaultColor = VulkanFrameTimer::ZoneColor::color
#define ProfilerVulkanZoneNames() (GetVulkanTimer().GetZoneList())
#define ProfilerVulkanCpuTimings() (GetVulkanTimer().GetCPUTimingList())
#define ProfilerVulkanGpuTimings() (GetVulkanTimer().GetGPUTimingList())
#define ProfilerVulkanTotalCPU() (GetVulkanTimer().GetTotalCPUFrameTime())
#define ProfilerVulkanTotalGPU() (GetVulkanTimer().GetTotalGPUFrameTime())
#define ProfilerVulkanTotalGPUFloat() (GetVulkanTimer().GetTotalGPUFrameTimeFloat())

static inline std::string ProfilerVulkanStripToFinalFunctionName(const char* functionString) {
    std::string_view v = functionString ? functionString : "";

    size_t paren = v.find('(');
    if (paren != std::string_view::npos) v = v.substr(0, paren);

    size_t scope = v.rfind("::");
    if (scope != std::string_view::npos) v = v.substr(scope + 2);

    size_t space = v.rfind(' ');
    if (space != std::string_view::npos) v = v.substr(space + 1);

    return std::string(v);
}

#if defined(_MSC_VER)
#define ProfilerVulkanZoneFunctionColor(color) ProfilerVulkanZoneColor(ProfilerVulkanStripToFinalFunctionName(__FUNCSIG__), color)
#define ProfilerVulkanZoneFunction() VulkanFrameTimer::ZoneScope VulkanProfilerConcat(_vk_gpu_zone_, __COUNTER__){ commandBuffer, GetVulkanTimer(), ProfilerVulkanStripToFinalFunctionName(__FUNCSIG__), VulkanFrameTimer::s_DefaultColor }
#else
#define ProfilerVulkanZoneFunctionColor(color) ProfilerVulkanZoneColor(ProfilerVulkanStripToFinalFunctionName(__PRETTY_FUNCTION__), color)
#define ProfilerVulkanZoneFunction() VulkanFrameTimer::ZoneScope VulkanProfilerConcat(_vk_gpu_zone_, __COUNTER__){ commandBuffer, GetVulkanTimer(), ProfilerVulkanStripToFinalFunctionName(__PRETTY_FUNCTION__), VulkanFrameTimer::s_DefaultColor }
#endif

#define ProfilerVulkanZoneFunctionRed() ProfilerVulkanZoneFunctionColor(COL_RED)
#define ProfilerVulkanZoneFunctionOrange() ProfilerVulkanZoneFunctionColor(COL_ORANGE)
#define ProfilerVulkanZoneFunctionYellow() ProfilerVulkanZoneFunctionColor(COL_YELLOW)
#define ProfilerVulkanZoneFunctionLightBlue() ProfilerVulkanZoneFunctionColor(COL_LIGHT_BLUE)
#define ProfilerVulkanZoneFunctionLightGreen() ProfilerVulkanZoneFunctionColor(COL_LIGHT_GREEN)

#else

struct VulkanFrameTimer {
    enum struct ZoneColor { COL_WHITE, COL_ORANGE, COL_YELLOW, COL_RED, COL_LIGHT_BLUE, COL_LIGHT_GREEN };
    static inline ZoneColor s_DefaultColor = ZoneColor::COL_ORANGE;
    void BeginFrame(VkCommandBuffer, uint32_t) {}
    void EndFrame(VkCommandBuffer) {}
    void BeginZone(VkCommandBuffer, const std::string&, ZoneColor) {}
    void EndZone(VkCommandBuffer, const std::string&) {}
    void Reset() {}
    const std::string& GetZoneList() const { static std::string s; return s; }
    const std::string& GetCPUTimingList() const { static std::string s; return s; }
    const std::string& GetGPUTimingList() const { static std::string s; return s; }
    bool TryGetZoneTiming(std::string_view, double& cpuMilliseconds, double& gpuMilliseconds) const { cpuMilliseconds = 0.0; gpuMilliseconds = 0.0; return false; }
    const std::string& GetTotalCPUFrameTime() const { static std::string s; return s; }
    const std::string& GetTotalGPUFrameTime() const { static std::string s; return s; }

    float GetTotalGPUFrameTimeFloat222() const { return mGpuFrameRollingAverage.GetValue(); }

    struct ZoneScope {
        ZoneScope(VkCommandBuffer, VulkanFrameTimer&, const char*, ZoneColor) {}
        ZoneScope(VkCommandBuffer, VulkanFrameTimer&, const std::string&, ZoneColor) {}
        void Release() {}
    };
};

inline VulkanFrameTimer& GetVulkanTimer() {
    static VulkanFrameTimer t;
    return t;
}

#define ProfilerVulkanBeginFrame(commandBuffer, frameIndex)
#define ProfilerVulkanEndFrame(commandBuffer)
#define ProfilerVulkanReset()
#define ProfilerVulkanZone(label)
#define ProfilerVulkanZoneCommandBuffer(commandBuffer, label)
#define ProfilerVulkanZoneColor(label, color)
#define ProfilerVulkanZoneRed(label)
#define ProfilerVulkanZoneOrange(label)
#define ProfilerVulkanZoneYellow(label)
#define ProfilerVulkanZoneLightBlue(label)
#define ProfilerVulkanZoneLightGreen(label)
#define ProfilerVulkanSetDefaultZoneColor(color)
#define ProfilerVulkanZoneNames() (GetVulkanTimer().GetZoneList())
#define ProfilerVulkanCpuTimings() (GetVulkanTimer().GetCPUTimingList())
#define ProfilerVulkanGpuTimings() (GetVulkanTimer().GetGPUTimingList())
#define ProfilerVulkanTotalCPU() (GetVulkanTimer().GetTotalCPUFrameTime())
#define ProfilerVulkanTotalGPU() (GetVulkanTimer().GetTotalGPUFrameTime())
#define ProfilerVulkanTotalGPUFloat() (GetVulkanTimer().GetTotalGPUFrameTimeFloat())
#define ProfilerVulkanZoneFunctionColor(color)
#define ProfilerVulkanZoneFunction()
#define ProfilerVulkanZoneFunctionRed()
#define ProfilerVulkanZoneFunctionOrange()
#define ProfilerVulkanZoneFunctionYellow()
#define ProfilerVulkanZoneFunctionLightBlue()
#define ProfilerVulkanZoneFunctionLightGreen()

#endif
