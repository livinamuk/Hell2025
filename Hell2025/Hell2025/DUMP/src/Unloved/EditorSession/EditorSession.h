#pragma once

#include "EditorSessionTypes.h"

#include <string>

namespace Unloved::EditorSession {

    void Init();
    void Open();
    void Open(EditorSessionMode mode);
    void Close();
    void SetActive(bool active);
    void SetRenderMode(EditorRenderMode renderMode);
    void Update();
    void UpdateViewportInput();
    void Render();
    bool RequestRagdollTest(std::string& error);
    void SimulateRagdollTest();
    void SetRagdollTestToBindPose();
    void SetRagdollTestToTestAnimation();
    void ElevateRagdollTest();

    bool IsActive();
    bool IsInactive();
    bool IsHeightMapEditorActive();
    bool HasMode();
    EditorSessionMode GetMode();
    EditorRenderMode GetRenderMode();
    bool WantsMouseCapture();
    bool WantsKeyboardCapture();
}
