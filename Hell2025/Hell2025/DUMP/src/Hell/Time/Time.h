#pragma once

#include <cstdint>
#include <string>

namespace Hell::Time {
    void Init();
    void Update();

    double NowSeconds();
    float RawDeltaTime();
    float DeltaTime();
    float MaxDeltaTime();
    float FixedDeltaTime();
    float FixedAlpha();
    float TotalTime();

    int FixedStepsConsumedThisFrame();
    int MaxFixedStepsPerFrame();
    bool ConsumeFixedStep();

    std::string FormatTimestamp(uint64_t timestamp);
}
