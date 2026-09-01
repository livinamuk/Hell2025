#pragma once
#include "DebugTypes.h"
#include <Unloved/Render/RendererEnums.h>

#include <string>

namespace Debug {
    void Update();
    void AddText(const std::string& text);
    void BlitQuickDebugMessage(const std::string& message);
    void EndFrame();
    void NextDebugRenderMode();
    void SetDebugTextMode(DebugTextMode mode);
    void SetDebugRenderMode(DebugRenderMode mode);

    void PrintModelMeshNames(const std::string& name);

    // Menu
    void ToggleMenuVisiblity();
    void HideMenu();
    void ShowMenu();
    void UpdateMenu();
    bool IsMenuVisible();
    bool MenuHadFocusThisFrame();
    void EndMenuFrame();

    const std::string& GetText();
    const DebugRenderMode& GetDebugRenderMode();
    const DebugTextMode& GetDebugTextMode();
}
