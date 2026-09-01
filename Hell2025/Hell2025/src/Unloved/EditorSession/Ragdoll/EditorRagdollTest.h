#pragma once

struct RagdollAsset;

namespace Unloved::EditorSession::RagdollTest {
    bool IsActive();
    void Start(const RagdollAsset& asset);
    void Stop();
    void Simulate();
    void SetToBindPose();
    void SetToTestAnimation();
    void Elevate();
}
