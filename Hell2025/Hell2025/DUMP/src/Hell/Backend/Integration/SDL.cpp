#include "SDL.h"
#include "Hell/Logging.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

//#include "Hell/Input/InputMulti2.h"

#include <iostream>
#include <string>
#include <unordered_map>

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#endif

namespace Hell::BackEnd::SDL {
    SDL_Window* g_window = nullptr;
    SDL_GLContext g_glContext = nullptr;

    WindowedMode g_windowedMode = WindowedMode::WINDOWED;
    bool g_forceCloseWindow = false;
    bool g_windowHasFocus = true;

    int g_windowedWidth = 0;
    int g_windowedHeight = 0;
    int g_fullscreenWidth = 0;
    int g_fullscreenHeight = 0;
    int g_currentWindowWidth = 0;
    int g_currentWindowHeight = 0;

    int g_currentCursor = 0;
    std::unordered_map<int, SDL_Cursor*> g_cursorsPtrs;

    SDL_DisplayID g_displayID = 0;
    const SDL_DisplayMode* g_displayMode = nullptr;

    bool Init(API api, WindowedMode windowedMode) {
        // Hints required for detecting ids on multiple keyboards
        SDL_SetHint(SDL_HINT_WINDOWS_RAW_KEYBOARD, "1");
        //SDL_SetHint(SDL_HINT_WINDOWS_GAMEINPUT, "1");
        
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
            std::cout << "SDLIntegration() failed to init SDL: " << SDL_GetError() << "\n";
            return false;
        }

        g_displayID = SDL_GetPrimaryDisplay();
        if (g_displayID == 0) {
            std::cout << "SDLIntegration() failed to get primary display: " << SDL_GetError() << "\n";
            return false;
        }

        g_displayMode = SDL_GetCurrentDisplayMode(g_displayID);
        if (!g_displayMode) {
            std::cout << "SDLIntegration() failed to get current display mode: " << SDL_GetError() << "\n";
            return false;
        }

        g_fullscreenWidth = g_displayMode->w;
        g_fullscreenHeight = g_displayMode->h;
        g_windowedWidth = static_cast<int>(g_fullscreenWidth * 0.75f);
        g_windowedHeight = static_cast<int>(g_fullscreenHeight * 0.75f);

        Uint32 flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE;
        if (api == API::OPENGL) {
            flags |= SDL_WINDOW_OPENGL;
        }
        else if (api == API::VULKAN) {
            flags |= SDL_WINDOW_VULKAN;
        }

        g_windowedMode = windowedMode;

        int createWidth = 0;
        int createHeight = 0;
        if (g_windowedMode == WindowedMode::WINDOWED) {
            createWidth = g_windowedWidth;
            createHeight = g_windowedHeight;
        }
        else {
            createWidth = g_fullscreenWidth;
            createHeight = g_fullscreenHeight;
        }

        g_window = SDL_CreateWindow("Unloved", createWidth, createHeight, flags); 
        SDL_RaiseWindow(g_window);

        if (!g_window) {
            Logging::Fatal() << "SDLIntegration() failed to create window: " << SDL_GetError();
            SDL_Quit();
            return false;
        }

        if (g_windowedMode == WindowedMode::WINDOWED) {
            SDL_SetWindowBordered(g_window, false);
        }

        SDL_SetWindowPosition(g_window, 0, 0);

        g_currentWindowWidth = createWidth;
        g_currentWindowHeight = createHeight;

        if (api == API::OPENGL) {
            g_glContext = SDL_GL_CreateContext(g_window);
            if (!g_glContext) {
                Logging::Fatal() << "SDLIntegration() failed to create GL context: " << SDL_GetError() << "\n";
                SDL_DestroyWindow(g_window);
                SDL_Quit();
                return false;
            }
            SDL_GL_MakeCurrent(g_window, g_glContext);
        }

        g_cursorsPtrs[0] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
        g_cursorsPtrs[1] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
        g_cursorsPtrs[2] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_CROSSHAIR);
        g_cursorsPtrs[3] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
        g_cursorsPtrs[4] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
        g_cursorsPtrs[5] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);

        SDL_ShowWindow(g_window);

        return true;
    }

    void SetCursor(int cursor) {
        g_currentCursor = cursor;
        auto it = g_cursorsPtrs.find(cursor);
        if (it != g_cursorsPtrs.end() && it->second) {
            SDL_SetCursor(it->second);
        }
    }

    void* GetWin32Window() {
        #ifdef _WIN32
        SDL_PropertiesID props = SDL_GetWindowProperties(g_window);
        void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
        return hwnd;
        #else
        return nullptr;
        #endif
    }

    void MakeContextCurrent() {
        if (g_window && g_glContext) {
            SDL_GL_MakeCurrent(g_window, g_glContext);
        }
    }

    bool CreateSurface(void* surface) {
        // Vulkan surface creation will be rewired with the new backend.
        return false;
    }

    void BeginFrame(API /*api*/) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_EVENT_QUIT: 
                    g_forceCloseWindow = true; 
                    break;
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED: 
                    g_forceCloseWindow = true; 
                    break;
                case SDL_EVENT_WINDOW_RESIZED:
                    g_currentWindowWidth = e.window.data1;
                    g_currentWindowHeight = e.window.data2;
                    break;
                default:
                    break;
            }

            // TODO InputMulti::ProcessEvent(e);
        }

        Uint32 flags = SDL_GetWindowFlags(g_window);
        g_windowHasFocus = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    }

    void EndFrame(API api) {
        if (api == API::OPENGL) {
            SDL_GL_SwapWindow(g_window);
        }
    }

    void Destroy() {
        for (auto& it : g_cursorsPtrs) {
            if (it.second) {
                SDL_DestroyCursor(it.second);
            }
        }
        g_cursorsPtrs.clear();

        if (g_glContext) {
            SDL_GL_DestroyContext(g_glContext);
            g_glContext = nullptr;
        }

        if (g_window) {
            SDL_DestroyWindow(g_window);
            g_window = nullptr;
        }

        SDL_Quit();
    }

    void SetWindowedMode(const WindowedMode& windowedMode) {
        g_windowedMode = windowedMode;

        if (windowedMode == WindowedMode::WINDOWED) {
            g_currentWindowWidth = g_windowedWidth;
            g_currentWindowHeight = g_windowedHeight;
            SDL_SetWindowFullscreen(g_window, false);
            SDL_SetWindowSize(g_window, g_currentWindowWidth, g_currentWindowHeight);
            SDL_SetWindowPosition(g_window, 0, 0);
            SDL_SetWindowBordered(g_window, false);
        }
        else {
            g_currentWindowWidth = g_fullscreenWidth;
            g_currentWindowHeight = g_fullscreenHeight;
            SDL_SetWindowFullscreen(g_window, true);
            SDL_SyncWindow(g_window);
        }
    }

    void ToggleFullscreen() {
        if (g_windowedMode == WindowedMode::WINDOWED) {
            SetWindowedMode(WindowedMode::FULLSCREEN);
        }
        else {
            SetWindowedMode(WindowedMode::WINDOWED);
        }
    }

    void ForceCloseWindow() {
        g_forceCloseWindow = true;
    }

    bool WindowHasFocus() {
        return g_windowHasFocus;
    }

    bool WindowHasNotBeenForceClosed() {
        return !g_forceCloseWindow;
    }

    bool WindowIsOpen() {
        return !g_forceCloseWindow && g_window != nullptr;
    }

    bool WindowIsMinimized() {
        int w = 0;
        int h = 0;
        SDL_GetWindowSize(g_window, &w, &h);
        return (w == 0 || h == 0);
    }

    void* GetWindowPointer() {
        return g_window;
    }

    int GetWindowedWidth() {
        return g_windowedWidth;
    }

    int GetWindowedHeight() {
        return g_windowedHeight;
    }

    int GetFullScreenWidth() {
        return g_fullscreenWidth;
    }

    int GetFullScreenHeight() {
        return g_fullscreenHeight;
    }

    int GetCurrentWindowWidth() {
        return g_currentWindowWidth;
    }

    int GetCurrentWindowHeight() {
        return g_currentWindowHeight;
    }

    const WindowedMode& GetWindowedMode() {
        return g_windowedMode;
    }

    const std::vector<const char*> GetRequiredInstanceExtensions() {
        Uint32 count = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(&count)) {
            std::cout << "SDLIntegration::GetRequiredInstanceExtensions() failed to get count: " << SDL_GetError() << "\n";
            return {};
        }
        std::vector<const char*> extensions(count);
        if (!SDL_Vulkan_GetInstanceExtensions(&count)) {
            std::cout << "SDLIntegration::GetRequiredInstanceExtensions() failed to get names: " << SDL_GetError() << "\n";
            return {};
        }
        return extensions;
    }
}
