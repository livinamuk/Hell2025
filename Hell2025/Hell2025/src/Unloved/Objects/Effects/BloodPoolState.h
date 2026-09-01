#pragma once

#include <string>

struct BloodPoolState {
    void Configure(uint64_t ragdollId, const std::string& boneName);
    void Update();
    void Reset();

private:
    uint64_t m_ragdollId = 0;
    std::string m_boneName;
    bool m_awaitingSpawn = true;
};