#include "Time.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <format>

namespace Hell::Time {
    namespace {
        using Clock = std::chrono::steady_clock;

        constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;
        constexpr float MAX_DELTA_TIME = 0.1f;
        constexpr int MAX_FIXED_STEPS_PER_FRAME = 5;

        Clock::time_point g_lastFrameTime;
        float g_rawDeltaTime = FIXED_DELTA_TIME;
        float g_deltaTime = FIXED_DELTA_TIME;
        float g_totalTime = 0.0f;
        double g_fixedAccumulator = 0.0;
        int g_fixedStepsConsumedThisFrame = 0;
        bool g_initialized = false;
        bool g_hasUpdated = false;
    }

    double NowSeconds() {
        return std::chrono::duration_cast<std::chrono::duration<double>>(
            Clock::now().time_since_epoch()
        ).count();
    }

    void Init() {
        g_lastFrameTime = Clock::now();
        g_rawDeltaTime = FIXED_DELTA_TIME;
        g_deltaTime = FIXED_DELTA_TIME;
        g_totalTime = 0.0f;
        g_fixedAccumulator = 0.0;
        g_fixedStepsConsumedThisFrame = 0;
        g_initialized = true;
        g_hasUpdated = false;
    }

    void Update() {
        if (!g_initialized) {
            Init();
            return;
        }

        const Clock::time_point currentFrameTime = Clock::now();
        if (g_hasUpdated) {
            g_rawDeltaTime = std::chrono::duration<float>(currentFrameTime - g_lastFrameTime).count();
        }
        else {
            g_rawDeltaTime = FIXED_DELTA_TIME;
            g_hasUpdated = true;
        }

        g_lastFrameTime = currentFrameTime;
        g_fixedStepsConsumedThisFrame = 0;

        g_deltaTime = std::clamp(g_rawDeltaTime, 0.0f, MAX_DELTA_TIME);
        g_totalTime += g_deltaTime;

        const double maxFixedAccumulator = static_cast<double>(FIXED_DELTA_TIME) * MAX_FIXED_STEPS_PER_FRAME;
        g_fixedAccumulator = std::min(g_fixedAccumulator + g_deltaTime, maxFixedAccumulator);
    }

    float RawDeltaTime() {
        return g_rawDeltaTime;
    }

    float DeltaTime() {
        return g_deltaTime;
    }

    float MaxDeltaTime() {
        return MAX_DELTA_TIME;
    }

    float FixedDeltaTime() {
        return FIXED_DELTA_TIME;
    }

    float FixedAlpha() {
        return std::clamp(static_cast<float>(g_fixedAccumulator / FIXED_DELTA_TIME), 0.0f, 1.0f);
    }

    float TotalTime() {
        return g_totalTime;
    }

    int FixedStepsConsumedThisFrame() {
        return g_fixedStepsConsumedThisFrame;
    }

    int MaxFixedStepsPerFrame() {
        return MAX_FIXED_STEPS_PER_FRAME;
    }

    bool ConsumeFixedStep() {
        if (g_fixedStepsConsumedThisFrame >= MAX_FIXED_STEPS_PER_FRAME) {
            g_fixedAccumulator = 0.0;
            return false;
        }

        if (g_fixedAccumulator + 0.0000001 < FIXED_DELTA_TIME) {
            return false;
        }

        g_fixedAccumulator -= FIXED_DELTA_TIME;
        g_fixedStepsConsumedThisFrame++;

        if (g_fixedAccumulator < 0.0000001) {
            g_fixedAccumulator = 0.0;
        }

        return true;
    }

    std::string FormatTimestamp(uint64_t timestamp) {
        try {
            std::chrono::sys_time<std::chrono::seconds> timePoint{ std::chrono::seconds{timestamp} };
            std::chrono::zoned_time zonedTime{ std::chrono::current_zone(), timePoint };
            return std::format("{:%Y-%m-%d %H:%M:%S %Z}", zonedTime);
        }
        catch (const std::exception&) {
            return "Invalid Timestamp";
        }
    }
}
