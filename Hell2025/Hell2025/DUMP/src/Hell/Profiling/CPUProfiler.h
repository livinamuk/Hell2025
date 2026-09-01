#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#if !defined(CPU_PROFILING)
#define CPU_PROFILING 1
#endif

namespace Hell::CPUProfiler {

    struct Report {
        std::string zoneNames;
        std::vector<std::string> timingColumns;
    };

    const Report& GetReport();
    float GetZoneTime(const std::string& zoneName);

    class FrameScope {
    public:
        FrameScope();
        FrameScope(const FrameScope&) = delete;
        FrameScope& operator=(const FrameScope&) = delete;
        FrameScope(FrameScope&&) = delete;
        FrameScope& operator=(FrameScope&&) = delete;
        ~FrameScope();

    private:
        bool m_active = false;
    };

    class ZoneScope {
    public:
        ZoneScope(std::string_view name, const char* file, uint32_t line, bool functionSignature = false);
        ZoneScope(const ZoneScope&) = delete;
        ZoneScope& operator=(const ZoneScope&) = delete;
        ZoneScope(ZoneScope&&) = delete;
        ZoneScope& operator=(ZoneScope&&) = delete;
        ~ZoneScope();

    private:
        uint32_t m_nodeIndex = UINT32_MAX;
        bool m_active = false;
    };
}

#define CPUProfilerConcatImpl(a, b) a##b
#define CPUProfilerConcat(a, b) CPUProfilerConcatImpl(a, b)

#if defined(_MSC_VER)
#define CPUProfilerFunctionSignature __FUNCSIG__
#else
#define CPUProfilerFunctionSignature __PRETTY_FUNCTION__
#endif

#if CPU_PROFILING

#define ProfilerCPUFrame() \
    Hell::CPUProfiler::FrameScope CPUProfilerConcat(_cpu_frame_, __COUNTER__)

#define ProfilerCPUZone(label) \
    Hell::CPUProfiler::ZoneScope CPUProfilerConcat(_cpu_zone_, __COUNTER__){ (label), __FILE__, __LINE__ }

#define ProfilerCPUZoneFunction() \
    Hell::CPUProfiler::ZoneScope CPUProfilerConcat(_cpu_zone_, __COUNTER__){ CPUProfilerFunctionSignature, __FILE__, __LINE__, true }

#else

#define ProfilerCPUFrame()
#define ProfilerCPUZone(label)
#define ProfilerCPUZoneFunction()

#endif
