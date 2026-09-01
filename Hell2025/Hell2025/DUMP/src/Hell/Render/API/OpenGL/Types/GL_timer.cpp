// GL_timer.cpp
#include "GL_timer.h"

#if OPENGL_PROFILING

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <vector>

static OpenGLFrameTimer gFrameTimer;

void OpenGLFrameTimer::RollingAverage::Clear() {
	mSamples.clear();
	mSum = 0.0;
}

void OpenGLFrameTimer::RollingAverage::Push(double value) {
	mSamples.push_back(value);
	mSum += value;

	if (mSamples.size() > mCapacity) {
		mSum -= mSamples.front();
		mSamples.pop_front();
	}
}

double OpenGLFrameTimer::RollingAverage::GetValue() const {
	if (mSamples.empty()) return 0.0;
	return mSum / double(mSamples.size());
}

void OpenGLFrameTimer::Zone::Init(const std::string& name) {
	if (mInitialized) return;

	mName = name;
	glGenQueries(1, &mStartTimestampQueryId);
	glGenQueries(1, &mEndTimestampQueryId);
	mInitialized = true;
}

void OpenGLFrameTimer::Zone::Destroy() {
	if (!mInitialized) return;

	glDeleteQueries(1, &mStartTimestampQueryId);
	glDeleteQueries(1, &mEndTimestampQueryId);

	mStartTimestampQueryId = 0;
	mEndTimestampQueryId = 0;
	mNestingDepth = 0;
	mStartFrameId = 0;
	mEndFrameId = 0;
	mTouchedFrameId = 0;

	mCpuRollingAverage.Clear();
	mGpuRollingAverage.Clear();

	mCpuRunning = false;
	mInitialized = false;
}

std::string OpenGLFrameTimer::FormatMs(double ms, size_t precision) {
	std::ostringstream oss;
	oss.setf(std::ios::fixed);
	oss << std::setprecision((int)precision) << ms << " ms";
	return oss.str();
}

void OpenGLFrameTimer::ClearStrings() {
	m_zoneList.clear();
	m_cpuTimingList.clear();
	m_gpuTimingList.clear();
	m_totalCpuFrameTime.clear();
	m_totalGpuFrameTime.clear();
}

void OpenGLFrameTimer::BuildStringsFromCurrentFrame() {
	ClearStrings();

	std::vector<std::pair<GLuint64, Zone*>> ordered;
	ordered.reserve(mZones.size());

	for (auto& kv : mZones) {
		Zone& zone = kv.second;
		if (!zone.mInitialized) continue;
		if (zone.mTouchedFrameId != mFrameId) continue;

		GLuint64 zoneStartTime = 0;
		glGetQueryObjectui64v(zone.mStartTimestampQueryId, GL_QUERY_RESULT, &zoneStartTime);
		ordered.emplace_back(zoneStartTime, &zone);
	}

	std::stable_sort(
		ordered.begin(),
		ordered.end(),
		[](const std::pair<GLuint64, Zone*>& a, const std::pair<GLuint64, Zone*>& b) {
			return a.first < b.first;
		}
	);

	// Color tags
	const std::string white =      "[COL=1.0,1.0,1.0,1.0]";
	const std::string orange =     "[COL=1.0,0.65,0.0,1.0]";
	const std::string yellow =     "[COL=1.0,1.0,0.0,1.0]";
	const std::string red =        "[COL=1.0,0.0,0.0,1.0]";
	const std::string lightBlue =  "[COL=0.68,0.85,0.9,1.0]";
	const std::string lightGreen = "[COL=0.56,0.93,0.56,1.0]";

	for (size_t i = 0; i < ordered.size(); ++i) {
		Zone* zone = ordered[i].second;

		// apply color tag if not white
		if (zone->mColor != ZoneColor::COL_WHITE) {
			std::string activeColor = white;

			switch (zone->mColor) {
			case ZoneColor::COL_ORANGE:      activeColor = orange;     break;
			case ZoneColor::COL_YELLOW:      activeColor = yellow;     break;
			case ZoneColor::COL_RED:         activeColor = red;        break;
			case ZoneColor::COL_LIGHT_BLUE:  activeColor = lightBlue;  break;
			case ZoneColor::COL_LIGHT_GREEN: activeColor = lightGreen; break;
			default: break;
			}

			m_zoneList += activeColor;
			m_cpuTimingList += activeColor;
			m_gpuTimingList += activeColor;
		}

		m_zoneList += zone->mName;
		m_zoneList += "\n";

		m_cpuTimingList += FormatMs(zone->mCpuRollingAverage.GetValue(), kOutputPrecision);
		m_cpuTimingList += "\n";

		m_gpuTimingList += FormatMs(zone->mGpuRollingAverage.GetValue(), kOutputPrecision);
		m_gpuTimingList += "\n";

		// reset to white for the next line
		if (zone->mColor != ZoneColor::COL_WHITE) {
			m_zoneList += white;
			m_cpuTimingList += white;
			m_gpuTimingList += white;
		}
	}

	m_totalCpuFrameTime = "Total: " + FormatMs(mCpuFrameRollingAverage.GetValue(), kOutputPrecision);
	m_totalGpuFrameTime = "Total: " + FormatMs(mGpuFrameRollingAverage.GetValue(), kOutputPrecision);
    m_totalGpuFrameTimeFloat = mGpuFrameRollingAverage.GetValue();
}

void OpenGLFrameTimer::BeginFrame() {
	if (!mInitialized) {
		glGenQueries(1, &mFrameStartQueryId);
		glGenQueries(1, &mFrameEndQueryId);
		mInitialized = true;
	}

	mFrameId += 1;

	if (mFrameId <= kSkipZoneFrames) {
		ClearStrings();
		mCpuFrameRunning = false;
		return;
	}

	mCpuFrameStartTime = Clock::now();
	mCpuFrameRunning = true;

	glQueryCounter(mFrameStartQueryId, GL_TIMESTAMP);
}

void OpenGLFrameTimer::EndFrame() {
	if (mFrameId <= kSkipZoneFrames) return;

	if (mCpuFrameRunning) {
		Clock::time_point endCpu = Clock::now();
		double cpuFrameMs = std::chrono::duration<double, std::milli>(endCpu - mCpuFrameStartTime).count();
		mCpuFrameRollingAverage.Push(cpuFrameMs);
		mCpuFrameRunning = false;
	}

	GLuint64 frameStartTime = 0;
	GLuint64 frameEndTime = 0;

	glQueryCounter(mFrameEndQueryId, GL_TIMESTAMP);
	glGetQueryObjectui64v(mFrameEndQueryId, GL_QUERY_RESULT, &frameEndTime);
	glGetQueryObjectui64v(mFrameStartQueryId, GL_QUERY_RESULT, &frameStartTime);

	double gpuFrameMs = double(frameEndTime - frameStartTime) / 1e6;
	mGpuFrameRollingAverage.Push(gpuFrameMs);

	for (auto& kv : mZones) {
		Zone& zone = kv.second;
		if (!zone.mInitialized) continue;

		if (zone.mStartFrameId == mFrameId && zone.mEndFrameId == mFrameId) {
			GLuint64 zoneStartTime = 0;
			GLuint64 zoneEndTime = 0;

			glGetQueryObjectui64v(zone.mEndTimestampQueryId, GL_QUERY_RESULT, &zoneEndTime);
			glGetQueryObjectui64v(zone.mStartTimestampQueryId, GL_QUERY_RESULT, &zoneStartTime);

			double zoneGpuMs = double(zoneEndTime - zoneStartTime) / 1e6;
			zone.mGpuRollingAverage.Push(zoneGpuMs);
		}
	}

	BuildStringsFromCurrentFrame();

	for (auto it = mZones.begin(); it != mZones.end(); ) {
		Zone& zone = it->second;
		if (zone.mTouchedFrameId != mFrameId) {
			zone.Destroy();
			it = mZones.erase(it);
		}
		else {
			++it;
		}
	}
}

void OpenGLFrameTimer::BeginZone(const std::string& name, OpenGLFrameTimer::ZoneColor color) {
	if (!mInitialized) return;
	if (mFrameId <= kSkipZoneFrames) return;

	Zone& zone = mZones[name];
	if (!zone.mInitialized) zone.Init(name);

	zone.mColor = color;
	zone.mTouchedFrameId = mFrameId;

	if (zone.mNestingDepth == 0) {
		zone.mCpuStartTime = Clock::now();
		zone.mCpuRunning = true;

		glQueryCounter(zone.mStartTimestampQueryId, GL_TIMESTAMP);
		zone.mStartFrameId = mFrameId;
	}

	zone.mNestingDepth += 1;
}

void OpenGLFrameTimer::EndZone(const std::string& name) {
	if (!mInitialized) return;
	if (mFrameId <= kSkipZoneFrames) return;

	auto it = mZones.find(name);
	if (it == mZones.end()) return;

	Zone& zone = it->second;
	if (zone.mNestingDepth == 0) return;

	zone.mNestingDepth -= 1;

	if (zone.mNestingDepth == 0) {
		if (zone.mCpuRunning) {
			Clock::time_point endCpu = Clock::now();
			double zoneCpuMs = std::chrono::duration<double, std::milli>(endCpu - zone.mCpuStartTime).count();
			zone.mCpuRollingAverage.Push(zoneCpuMs);
			zone.mCpuRunning = false;
		}

		if (OpenGLFrameTimer::kStrictZones) {
			GLsync s = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			glWaitSync(s, 0, GL_TIMEOUT_IGNORED);
			glDeleteSync(s);
		}

		glQueryCounter(zone.mEndTimestampQueryId, GL_TIMESTAMP);
		zone.mEndFrameId = mFrameId;
		zone.mTouchedFrameId = mFrameId;
	}
}

void OpenGLFrameTimer::Reset() {
	if (mInitialized) {
		glDeleteQueries(1, &mFrameStartQueryId);
		glDeleteQueries(1, &mFrameEndQueryId);
		mFrameStartQueryId = 0;
		mFrameEndQueryId = 0;
		mInitialized = false;
	}

	for (auto& kv : mZones) {
		kv.second.Destroy();
	}

	mZones.clear();

	mCpuFrameRollingAverage.Clear();
	mGpuFrameRollingAverage.Clear();

	mFrameId = 0;
	mCpuFrameRunning = false;

	ClearStrings();
}

const std::string& OpenGLFrameTimer::GetZoneList() const {
	return m_zoneList;
}

const std::string& OpenGLFrameTimer::GetCPUTimingList() const {
	return m_cpuTimingList;
}

const std::string& OpenGLFrameTimer::GetGPUTimingList() const {
	return m_gpuTimingList;
}

bool OpenGLFrameTimer::TryGetZoneTiming(std::string_view zoneName, double& cpuMilliseconds, double& gpuMilliseconds) const {
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

const std::string& OpenGLFrameTimer::GetTotalCPUFrameTime() const {
	return m_totalCpuFrameTime;
}

const std::string& OpenGLFrameTimer::GetTotalGPUFrameTime() const {
	return m_totalGpuFrameTime;
}

float OpenGLFrameTimer::GetTotalGPUFrameTimeFloat() const  {
    return m_totalGpuFrameTimeFloat;
}

OpenGLFrameTimer::ZoneScope::ZoneScope(OpenGLFrameTimer& timer, const char* name, OpenGLFrameTimer::ZoneColor color) {
	mTimer = &timer;
	mName = name ? name : "";
	mActive = true;
	mTimer->BeginZone(mName, color);
}

OpenGLFrameTimer::ZoneScope::ZoneScope(OpenGLFrameTimer& timer, const std::string& name, OpenGLFrameTimer::ZoneColor color) {
	mTimer = &timer;
	mName = name;
	mActive = true;
	mTimer->BeginZone(mName, color);
}

OpenGLFrameTimer::ZoneScope::~ZoneScope() {
	if (mActive && mTimer) {
		mTimer->EndZone(mName);
	}
}

void OpenGLFrameTimer::ZoneScope::Release() {
	mActive = false;
}

OpenGLFrameTimer::FrameScope::FrameScope(OpenGLFrameTimer& timer) {
	mTimerPtr = &timer;
	mActive = true;
	mTimerPtr->BeginFrame();
}

OpenGLFrameTimer::FrameScope::~FrameScope() {
	if (mActive && mTimerPtr) mTimerPtr->EndFrame();
}

void OpenGLFrameTimer::FrameScope::Release() {
	mActive = false;
}

OpenGLFrameTimer& GetTimer() {
	return gFrameTimer;
}

#endif
