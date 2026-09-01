// GL_timer.h
#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <deque>
#include <chrono>

#if !defined(OPENGL_PROFILING)
#define OPENGL_PROFILING 1
#endif

#if OPENGL_PROFILING

struct OpenGLFrameTimer {
	static constexpr uint32_t kSkipZoneFrames = 1;
	static constexpr size_t   kAverageFrameCount = 60;
	static constexpr size_t   kOutputPrecision = 2;
	static constexpr bool     kStrictZones = false;

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
		bool mCpuRunning = false;

		GLuint    mStartTimestampQueryId = 0;
		GLuint    mEndTimestampQueryId = 0;
		uint64_t  mStartFrameId = 0;
		uint64_t  mEndFrameId = 0;
		uint64_t  mTouchedFrameId = 0;
		int        mNestingDepth = 0;
		bool      mInitialized = false;
		ZoneColor mColor = ZoneColor::COL_WHITE;


		Zone() = default;
		void Init(const std::string& name);
		void Destroy();
	};

	struct ZoneScope {
		OpenGLFrameTimer* mTimer = nullptr;
		std::string mName;
		bool mActive = false;

		ZoneScope(OpenGLFrameTimer& timer, const char* name, OpenGLFrameTimer::ZoneColor color);
		ZoneScope(OpenGLFrameTimer& timer, const std::string& name, OpenGLFrameTimer::ZoneColor color);
		ZoneScope(const ZoneScope&) = delete;
		ZoneScope& operator=(const ZoneScope&) = delete;
		ZoneScope(ZoneScope&&) = delete;
		ZoneScope& operator=(ZoneScope&&) = delete;
		~ZoneScope();

		void Release();
	};

	struct FrameScope {
		OpenGLFrameTimer* mTimerPtr = nullptr;
		bool mActive = false;

		FrameScope(OpenGLFrameTimer& timer);
		FrameScope(const FrameScope&) = delete;
		FrameScope& operator=(const FrameScope&) = delete;
		FrameScope(FrameScope&&) = delete;
		FrameScope& operator=(FrameScope&&) = delete;
		~FrameScope();

		void Release();
	};

	OpenGLFrameTimer() = default;
	static inline ZoneColor s_DefaultColor = ZoneColor::COL_ORANGE;

	void BeginFrame();
	void EndFrame();
	void BeginZone(const std::string& name, OpenGLFrameTimer::ZoneColor color);
	void EndZone(const std::string& name);
	void Reset();

	const std::string& GetZoneList() const;
	const std::string& GetCPUTimingList() const;
	const std::string& GetGPUTimingList() const;
	bool TryGetZoneTiming(std::string_view zoneName, double& cpuMilliseconds, double& gpuMilliseconds) const;

    const std::string& GetTotalCPUFrameTime() const;
    const std::string& GetTotalGPUFrameTime() const;
    float GetTotalGPUFrameTimeFloat() const;

	std::unordered_map<std::string, Zone> mZones;

	RollingAverage mCpuFrameRollingAverage;
	RollingAverage mGpuFrameRollingAverage;

	GLuint   mFrameStartQueryId = 0;
	GLuint   mFrameEndQueryId = 0;
	uint64_t mFrameId = 0;
	bool      mInitialized = false;

private:
	void ClearStrings();
	void BuildStringsFromCurrentFrame();
	static std::string FormatMs(double ms, size_t precision);

	Clock::time_point mCpuFrameStartTime{};
	bool mCpuFrameRunning = false;

	std::string m_zoneList;
	std::string m_cpuTimingList;
	std::string m_gpuTimingList;

	std::string m_totalCpuFrameTime;
	std::string m_totalGpuFrameTime;

    float m_totalGpuFrameTimeFloat = 0.0f;
};

OpenGLFrameTimer& GetTimer();

#define ConcatImpl(a,b) a##b
#define Concat(a,b) ConcatImpl(a,b)

#define ProfilerOpenGLFrame() \
    OpenGLFrameTimer::FrameScope Concat(_gpu_frame_, __COUNTER__){ GetTimer() }

// For dynamic default color
#define ProfilerOpenGLZone(label) \
    OpenGLFrameTimer::ZoneScope Concat(_gpu_zone_, __COUNTER__){ GetTimer(), (label), OpenGLFrameTimer::s_DefaultColor }

// For specific colors
#define ProfilerOpenGLZoneColor(label, color) \
    OpenGLFrameTimer::ZoneScope Concat(_gpu_zone_, __COUNTER__){ GetTimer(), (label), OpenGLFrameTimer::ZoneColor::color }

#define ProfilerOpenGLZoneRed(label)         ProfilerOpenGLZoneColor(label, COL_RED)
#define ProfilerOpenGLZoneOrange(label)      ProfilerOpenGLZoneColor(label, COL_ORANGE)
#define ProfilerOpenGLZoneYellow(label)      ProfilerOpenGLZoneColor(label, COL_YELLOW)
#define ProfilerOpenGLZoneLightBlue(label)   ProfilerOpenGLZoneColor(label, COL_LIGHT_BLUE)
#define ProfilerOpenGLZoneLightGreen(label)  ProfilerOpenGLZoneColor(label, COL_LIGHT_GREEN)

#define ProfilerOpenGLSetDefaultZoneColor(color) \
    OpenGLFrameTimer::s_DefaultColor = OpenGLFrameTimer::ZoneColor::color

#define ProfilerOpenGLZoneNames()     (GetTimer().GetZoneList())
#define ProfilerOpenGLCpuTimings()    (GetTimer().GetCPUTimingList())
#define ProfilerOpenGLGpuTimings()    (GetTimer().GetGPUTimingList())
#define ProfilerOpenGLTotalCPU()      (GetTimer().GetTotalCPUFrameTime())
#define ProfilerOpenGLTotalGPU()      (GetTimer().GetTotalGPUFrameTime())
#define ProfilerOpenGLTotalGPUFloat() (GetTimer().GetTotalGPUFrameTimeFloat())

static inline std::string ProfilerOpenGLStripToFinalFunctionName(const char* functionString) {
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
#define ProfilerOpenGLZoneFunctionColor(color) \
        ProfilerOpenGLZoneColor(ProfilerOpenGLStripToFinalFunctionName(__FUNCSIG__), color)
#define ProfilerOpenGLZoneFunction() \
        OpenGLFrameTimer::ZoneScope Concat(_gpu_zone_, __COUNTER__){ GetTimer(), ProfilerOpenGLStripToFinalFunctionName(__FUNCSIG__), OpenGLFrameTimer::s_DefaultColor }
#else
#define ProfilerOpenGLZoneFunctionColor(color) \
        ProfilerOpenGLZoneColor(ProfilerOpenGLStripToFinalFunctionName(__PRETTY_FUNCTION__), color)
#define ProfilerOpenGLZoneFunction() \
        OpenGLFrameTimer::ZoneScope Concat(_gpu_zone_, __COUNTER__){ GetTimer(), ProfilerOpenGLStripToFinalFunctionName(__PRETTY_FUNCTION__), OpenGLFrameTimer::s_DefaultColor }
#endif

#define ProfilerOpenGLZoneFunctionRed()        ProfilerOpenGLZoneFunctionColor(COL_RED)
#define ProfilerOpenGLZoneFunctionOrange()     ProfilerOpenGLZoneFunctionColor(COL_ORANGE)
#define ProfilerOpenGLZoneFunctionYellow()     ProfilerOpenGLZoneFunctionColor(COL_YELLOW)
#define ProfilerOpenGLZoneFunctionLightBlue()  ProfilerOpenGLZoneFunctionColor(COL_LIGHT_BLUE)
#define ProfilerOpenGLZoneFunctionLightGreen() ProfilerOpenGLZoneFunctionColor(COL_LIGHT_GREEN)

#else

struct OpenGLFrameTimer {
	enum struct ZoneColor { COL_WHITE, COL_ORANGE, COL_YELLOW, COL_RED, COL_LIGHT_BLUE, COL_LIGHT_GREEN };
	static inline ZoneColor s_DefaultColor = ZoneColor::COL_ORANGE;
	OpenGLFrameTimer() = default;
	void BeginFrame() {}
	void EndFrame() {}
	void BeginZone(const std::string&, ZoneColor color) {}
	void EndZone(const std::string&) {}
	void Reset() {}

	const std::string& GetZoneList() const { static std::string s; return s; }
	const std::string& GetCPUTimingList() const { static std::string s; return s; }
	const std::string& GetGPUTimingList() const { static std::string s; return s; }
	bool TryGetZoneTiming(std::string_view, double& cpuMilliseconds, double& gpuMilliseconds) const { cpuMilliseconds = 0.0; gpuMilliseconds = 0.0; return false; }
	const std::string& GetTotalCPUFrameTime() const { static std::string s; return s; }
	const std::string& GetTotalGPUFrameTime() const { static std::string s; return s; }

	struct ZoneScope {
		ZoneScope(OpenGLFrameTimer&, const char*, ZoneColor color) {}
		ZoneScope(OpenGLFrameTimer&, const std::string&, ZoneColor color) {}
		void Release() {}
	};
	struct FrameScope {
		FrameScope(OpenGLFrameTimer&) {}
		void Release() {}
	};
};

inline OpenGLFrameTimer& GetTimer() {
	static OpenGLFrameTimer t;
	return t;
}

#define ProfilerOpenGLFrame()
#define ProfilerOpenGLZone(label)
#define ProfilerOpenGLSetDefaultZoneColor(color)
#define ProfilerOpenGLZoneRed(label)
#define ProfilerOpenGLZoneOrange(label)
#define ProfilerOpenGLZoneYellow(label)
#define ProfilerOpenGLZoneLightBlue(label)
#define ProfilerOpenGLZoneLightGreen(label)

#define ProfilerOpenGLZoneNames()   (GetTimer().GetZoneList())
#define ProfilerOpenGLCpuTimings()  (GetTimer().GetCPUTimingList())
#define ProfilerOpenGLGpuTimings()  (GetTimer().GetGPUTimingList())
#define ProfilerOpenGLTotalCPU()    (GetTimer().GetTotalCPUFrameTime())
#define ProfilerOpenGLTotalGPU()    (GetTimer().GetTotalGPUFrameTime())

#define ProfilerOpenGLZoneFunction()
#define ProfilerOpenGLZoneFunctionRed()
#define ProfilerOpenGLZoneFunctionOrange()
#define ProfilerOpenGLZoneFunctionYellow()
#define ProfilerOpenGLZoneFunctionLightBlue()
#define ProfilerOpenGLZoneFunctionLightGreen()

#endif
