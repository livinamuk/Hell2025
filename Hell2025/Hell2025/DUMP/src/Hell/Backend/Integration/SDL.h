#pragma once
#include "Hell/Common.h"
#include <vector>

namespace Hell::BackEnd::SDL {
    bool Init(API api, WindowedMode windowedMode);
    void Destroy();
    void BeginFrame(API api);
    void EndFrame(API api);
    void SetWindowedMode(const WindowedMode& windowedMode);
    void ToggleFullscreen();
    void ForceCloseWindow();
    bool WindowIsOpen();
    bool WindowHasFocus();
    bool WindowHasNotBeenForceClosed();
    bool WindowIsMinimized();
    int GetWindowedWidth();
    int GetWindowedHeight();
    int GetCurrentWindowWidth();
    int GetCurrentWindowHeight();
    int GetFullScreenWidth();
    int GetFullScreenHeight();
    void* GetWindowPointer();
    const WindowedMode& GetWindowedMode();
    void SetCursor(int);

    // Windows only
    void* GetWin32Window();

    // OpenGL only
    void MakeContextCurrent();

    // Vulkan only
    bool CreateSurface(void* surface);
    const std::vector<const char*> GetRequiredInstanceExtensions();

    void Test();
}
