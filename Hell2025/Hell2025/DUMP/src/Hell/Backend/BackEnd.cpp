#include "BackEnd.h"

#include "Hell/Logging.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/VK_back_end.h"
#include "Unloved/Config/Config.h"

#include "Integration/GLFW.h"
#include "Integration/SDL.h"

#include "Hell/Audio.h"
#include "Hell/Audio/Synth.h"
#include "Hell/AssetCompiler/AssetCompiler.h"
#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/TextureUploader.h"
#include "Hell/Time.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;
namespace InputMulti = Hell::InputMulti;
namespace Synth = Hell::Synth;
namespace Time = Hell::Time;

// Prevent accidentally selecting integrated GPU
extern "C" {
    __declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
    __declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

namespace Hell::BackEnd {
    API g_api = API::UNDEFINED;
    std::string g_title;
    int g_presentTargetWidth = 0;
    int g_presentTargetHeight = 0;

    bool Init(API api, WindowedMode windowMode, const std::string& title, uint32_t maxCompressedTextureResolution) {
        Logging::EnableLevel(Logging::Level::INIT);
        Logging::EnableLevel(Logging::Level::DEBUG);
        Logging::EnableLevel(Logging::Level::ERROR);
        Logging::EnableLevel(Logging::Level::FATAL);
        Logging::EnableLevel(Logging::Level::TODO);
        Logging::EnableLevel(Logging::Level::WARNING);
        Logging::EnableLevel(Logging::Level::FUNCTION);
        Logging::EnableLevel(Logging::Level::SUPPORT);
        Logging::EnableLevel(Logging::Level::EDITOR);

        g_api = api;
        g_title = title;

        Config::Init();
        if (!GLFW::Init(api, windowMode)) {
            return false;
        }

        if (GetAPI() == API::OPENGL) {
            OpenGL::BackEnd::Init();
        }
        else if (GetAPI() == API::VULKAN) {
            if (!Vulkan::BackEnd::Init()) {
                return false;
            }
        }

        ResourceManager::Init();
        Time::Init();
        Audio::Init();
        Synth::Init();
        Input::Init(GetWindowPointer());
        InputMulti::Init();

        AssetCompiler::CompileOutOfDateAssets();
        AssetLoader::Init(maxCompressedTextureResolution);

        glfwShowWindow(static_cast<GLFWwindow*>(GetWindowPointer()));
        return true;
    }

    void BeginFrame() {
        ProfilerCPUZoneFunction();

        Time::Update();
        GLFW::BeginFrame(g_api);
        Input::Update();
        InputMulti::Update();
        Audio::Update();
        Synth::Update(Time::RawDeltaTime());

        if (!WindowHasFocus()) {
            InputMulti::ResetState();
        }

        Physics::BeginFrame();

        if (GetAPI() == API::OPENGL) {
            OpenGL::BackEnd::BeginFrame();
            TextureUploader::Update();
        }
        else if (GetAPI() == API::VULKAN) {
            Vulkan::BackEnd::BeginFrame();
            TextureUploader::Update();
        }
    }

    void EndFrame() {
        ProfilerCPUZoneFunction();

        InputMulti::ResetMouseOffsets();
        GLFW::EndFrame(g_api);
    }

    void CleanUp() {
        Synth::CleanUp();
        TextureUploader::CleanUp();
        ResourceManager::CleanUp();

        if (GetAPI() == API::OPENGL) {
            OpenGL::ResourceManager::CleanUp();
        }
        else if (GetAPI() == API::VULKAN) {
            Vulkan::BackEnd::CleanUp();
        }

        GLFW::Destroy();
    }

    void SetAPI(API api) {
        g_api = api;
    }

    void SetPresentTargetSize(int width, int height) {
        g_presentTargetWidth = width;
        g_presentTargetHeight = height;
    }

    const API GetAPI() {
        return g_api;
    }

    void SetCursor(int cursor) {
        GLFW::SetCursor(cursor);
    }

    // Window
    void* GetWindowPointer() {
        return GLFW::GetWindowPointer();;
    }

    const WindowedMode& GetWindowedMode() {
        return GLFW::GetWindowedMode();
    }

    void SetWindowedMode(const WindowedMode& windowedMode) {
        GLFW::SetWindowedMode(windowedMode);
    }

    void ToggleFullscreen() {
        GLFW::ToggleFullscreen();
    }

    void ForceCloseWindow() {
        GLFW::ForceCloseWindow();
    }

    bool WindowIsOpen() {
        return GLFW::WindowIsOpen();
    }

    bool WindowHasFocus() {
        return GLFW::WindowHasFocus();
    }

    bool WindowHasNotBeenForceClosed() {
        return GLFW::WindowHasNotBeenForceClosed();
    }

    bool WindowIsMinimized() {
        return GLFW::WindowIsMinimized();
    }

    int GetWindowedWidth() {
        return GLFW::GetWindowedWidth();
    }

    int GetWindowedHeight() {
        return GLFW::GetWindowedHeight();
    }

    int GetCurrentWindowWidth() {
        return GLFW::GetCurrentWindowWidth();
    }

    int GetCurrentWindowHeight() {
        return GLFW::GetCurrentWindowHeight();
    }

    int GetDrawableWidth() {
        return GLFW::GetDrawableWidth();
    }

    int GetDrawableHeight() {
        return GLFW::GetDrawableHeight();
    }

    int GetFullScreenWidth() {
        return GLFW::GetFullScreenWidth();
    }

    int GetFullScreenHeight() {
        return GLFW::GetFullScreenHeight();
    }

    int GetPresentTargetWidth() {
        return g_presentTargetWidth;
    }

    int GetPresentTargetHeight() {
        return g_presentTargetHeight;
    }

}
