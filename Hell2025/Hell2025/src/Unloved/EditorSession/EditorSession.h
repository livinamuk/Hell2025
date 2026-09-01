#pragma once

#include "Editor_enums.h"
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
    void CreateNewFile(NewFileDialogType type, const std::string& fileName);
    void SaveRagdollAs(const std::string& fileName);

    bool IsActive();
    bool IsInactive();
    bool IsHeightMapEditorActive();
    bool HasMode();
    EditorSessionMode GetMode();
    EditorRenderMode GetRenderMode();
    bool WantsMouseCapture();
    bool WantsKeyboardCapture();
}
