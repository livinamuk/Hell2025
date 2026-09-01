#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

struct TimerResult {
    double allTimes = 0.0;
    std::uint64_t sampleCount = 0;
};

inline std::unordered_map<std::string, TimerResult> g_timerResults;
inline std::mutex g_timerResultsMutex;

struct Timer {
    std::chrono::time_point<std::chrono::steady_clock> m_startTime;
    std::string m_name;

    Timer(const std::string& name)
        : m_startTime(std::chrono::steady_clock::now())
        , m_name(name)
    {
    }

    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    Timer(Timer&&) = delete;
    Timer& operator=(Timer&&) = delete;

    ~Timer() {
        auto now = std::chrono::steady_clock::now();
        std::chrono::duration<double, std::milli> duration = now - m_startTime;
        double timeMs = duration.count();

        std::lock_guard<std::mutex> lock(g_timerResultsMutex);

        auto& entry = g_timerResults[m_name];
        entry.allTimes += timeMs;
        entry.sampleCount++;

        double avg = entry.allTimes / static_cast<double>(entry.sampleCount);

        int extraSpaces = 50 - static_cast<int>(m_name.length());
        if (extraSpaces < 0) extraSpaces = 0;
        std::string spacing(extraSpaces, ' ');

        std::cout << m_name << ":" << spacing
            << std::format("{:.4f}", timeMs) << "ms      average: "
            << std::format("{:.4f}", avg) << "ms\n";
    }
};
