#include "BackEnd.h"
#include "HellLogging.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "API/OpenGL/GL_backEnd.h"
#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/Vulkan/VK_backEnd.h"
#include "AssetManagement/AssetManager.h"
#include "Config/Config.h"
#include "Audio/Audio.h"
#include "Audio/MidiFileManager.h"
#include "Audio/Synth.h"
#include "Bible/Bible.h"
#include "Core/Debug.h"
#include "Core/Game.h"
#include "Editor/Editor.h"
#include "Editor/Gizmo.h"
#include "Pathfinding/AStarMap.h"
#include "ImGui/ImGuiBackend.h"
#include "Input/Input.h"
#include "Input/InputMulti.h"
#include "Managers/OpenableManager.h"
#include "Managers/HouseManager.h"
#include "Managers/MirrorManager.h"
#include "Modelling/Unused/Modelling.h"
#include "Physics/Physics.h"
#include "Ragdoll/RagdollManager.h"
#include "Renderer/GlobalIllumination.h"
#include "Renderer/Renderer.h"
#include "Renderer/RenderDataManager.h"
#include "UI/UIBackEnd.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"

#include "Integration/GLFW.h"
#include "Integration/SDL.h"

#include "Pathfinding/NavMesh.h"

#define NOMINMAX
#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#endif

// Prevent accidentally selecting integrated GPU
extern "C" {
    __declspec(dllexport) unsigned __int32 AmdPowerXpressRequestHighPerformance = 0x1;
    __declspec(dllexport) unsigned __int32 NvOptimusEnablement = 0x1;
}

namespace BackEnd {
    API g_api = API::UNDEFINED;
    int g_presentTargetWidth = 0;
    int g_presentTargetHeight = 0;
    bool g_renderDocFound = false;

    void CheckForRenderDoc();
    void UpdateLazyKeypresses();

    bool Init(API api, WindowedMode windowMode) {
        g_api = api;
        CheckForRenderDoc();        // <--- COMMENT OUT THIS
        //g_renderDocFound = true;  // <--- ADD THIS

        Config::Init();
        if (!GLFW::Init(api, windowMode)) {
            return false;
        }
        if (GetAPI() == API::OPENGL) {
            OpenGLBackEnd::Init();
            OpenGLRenderer::Init();
        }
        else if (GetAPI() == API::VULKAN) {
            if (!VulkanBackEnd::Init()) {
                return false;
            }
        }
        AssetManager::Init();
        Logging::Init() << "AssetManager::Init()";

        UIBackEnd::Init();
        Logging::Init() << "UIBackEnd::Init()";

        Audio::Init();
        Logging::Init() << "Audio::Init()";

        Bible::Init();
        Logging::Init() << "Bible::Init()";


        Input::Init(BackEnd::GetWindowPointer());
        Logging::Init() << "Input::Init()";

        InputMulti::Init();
        Logging::Init() << "InputMulti::Init()";

        Gizmo::Init();
        Logging::Init() << "Gizmo::Init()";

        ViewportManager::Init();
        Logging::Init() << "ViewportManager::Init()";

        Editor::Init();
        Logging::Init() << "Editor::Init()";

        //EditorImGui::Init();
        Synth::Init();
        Logging::Init() << "Synth::Init()";

        MidiFileManager::Init();
        Logging::Init() << "MidiFileManager::Init()";

        //WeaponManager::Init();
        //Logging::Init() << "WeaponManager::Init()";

        Physics::Init();
        RagdollManager::Init();

        ImGuiBackEnd::Init();
        Logging::Init() << "ImGuiBackEnd::Init()";

        NavMeshManager::Init();

        //Modelling::Init();

        glfwShowWindow(static_cast<GLFWwindow*>(BackEnd::GetWindowPointer()));
        return true;
    }

    void BeginFrame() {
        GLFW::BeginFrame(g_api);
        RenderDataManager::BeginFrame();
        if (GetAPI() == API::OPENGL) {
            OpenGLBackEnd::BeginFrame();
            OpenGLBackEnd::UpdateTextureBaking();
        }
        else if (GetAPI() == API::VULKAN) {
            //VulkanBackEnd::BeginFrame();
        }
        //Physics::ClearCollisionReports();

        if (!GLFW::WindowHasFocus()) {
            InputMulti::ResetState();
        }
        Game::BeginFrame();
        World::BeginFrame();
        Physics::BeginFrame();
    }

    void UpdateGame() {
        ViewportManager::Update();

        AStarMap::Update();
        Game::Update();
        float deltaTime = Game::GetDeltaTime();

        if (Editor::IsOpen()) {
            Editor::Update(deltaTime);
        }

        MirrorManager::Update();

        Physics::UpdateAllRigidDynamics(deltaTime);
        Physics::UpdateActiveRigidDynamicAABBList();
        Physics::UpdateHeightFields();

        World::SubmitRenderItems();

        Debug::Update();
        UIBackEnd::Update();
        RenderDataManager::Update();
        ImGuiBackEnd::Update();

        //if (Input::KeyPressed(HELL_KEY_SPACE)) {
        //    for (auto& model : AssetManager::GetModels()) {
        //        std::cout << model.GetName() << "\n";
        //        for (auto meshIndex : model.GetMeshIndices()) {
        //            auto mesh = AssetManager::GetMeshByIndex(meshIndex);
        //            std::cout << " - " << mesh->GetName() << "\n";
        //        }
        //        std::cout << "\n";
        //    }
        //}
    }

    void EndFrame() {
        GLFW::EndFrame(g_api);
        UIBackEnd::EndFrame();
        Debug::EndFrame();
        World::EndFrame();
        InputMulti::ResetMouseOffsets();
    }

    void UpdateSubSystems() {
        float deltaTime = Game::GetDeltaTime();
        //glfwSwapInterval(0);

        InputMulti::Update(deltaTime);
        Synth::Update(deltaTime);
        Audio::Update(deltaTime);
        MidiFileManager::Update(deltaTime);
        Input::Update();
        GlobalIllumination::Update();

        UpdateLazyKeypresses();
    }

    void CleanUp() {
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

    void BackEnd::SetWindowedMode(const WindowedMode& windowedMode) {
        GLFW::SetWindowedMode(windowedMode);
    }

    void BackEnd::ToggleFullscreen() {
        GLFW::ToggleFullscreen();
    }

    void BackEnd::ForceCloseWindow() {
        GLFW::ForceCloseWindow();
    }

    bool BackEnd::WindowIsOpen() {
        return GLFW::WindowIsOpen();
    }

    bool BackEnd::WindowHasFocus() {
        return GLFW::WindowHasFocus();
    }

    bool BackEnd::WindowHasNotBeenForceClosed() {
        return GLFW::WindowHasNotBeenForceClosed();
    }

    bool BackEnd::WindowIsMinimized() {
        return GLFW::WindowIsMinimized();
    }

    int BackEnd::GetWindowedWidth() {
        return GLFW::GetWindowedWidth();
    }

    int BackEnd::GetWindowedHeight() {
        return GLFW::GetWindowedHeight();
    }

    int BackEnd::GetCurrentWindowWidth() {
        return GLFW::GetCurrentWindowWidth();
    }

    int BackEnd::GetCurrentWindowHeight() {
        return GLFW::GetCurrentWindowHeight();
    }

    int BackEnd::GetFullScreenWidth() {
        return GLFW::GetFullScreenWidth();
    }

    int BackEnd::GetFullScreenHeight() {
        return GLFW::GetFullScreenHeight();
    }

    int GetPresentTargetWidth() {
        return g_presentTargetWidth;
    }

    int GetPresentTargetHeight() {
        return g_presentTargetHeight;
    }

    void CheckForRenderDoc() {
        #ifdef _WIN32
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
        if (snapshot == INVALID_HANDLE_VALUE) {
            g_renderDocFound = false;
        }

        MODULEENTRY32 moduleEntry;
        moduleEntry.dwSize = sizeof(MODULEENTRY32);
        bool found = false;
        if (Module32First(snapshot, &moduleEntry)) {
            do {
                std::wstring wmodule(moduleEntry.szModule);
                std::string moduleName(wmodule.begin(), wmodule.end());

                if (moduleName.find("renderdoc.dll") != std::string::npos) {
                    found = true;
                    break;
                }
            } while (Module32Next(snapshot, &moduleEntry));
        }
        CloseHandle(snapshot);

        g_renderDocFound = found;
        #else
        g_renderDocActive = false;
        #endif
    }

    void ToggleBindlessTextures() {
        g_renderDocFound = !g_renderDocFound;
    }

    bool RenderDocFound() {
        return g_renderDocFound;
    }

    void UpdateLazyKeypresses() {

        if (ImGuiBackEnd::HasKeyboardFocus()) {
            return;
        }

        // Bail early if player 1 is playing piano
        //Player* player = Game::GetLocalPlayerByIndex(0);
        //if (player && player->IsPlayingPiano()) {
        //      return;
        //}

        if (Input::KeyPressed(HELL_KEY_F1)) {
            Callbacks::NewRun();
        }
        if (Input::KeyPressed(HELL_KEY_F4)) {
            Callbacks::OpenHouseEditor();
        }
        if (Input::KeyPressed(HELL_KEY_F6)) {
            Callbacks::OpenMapHeightEditor();
        }
        if (Input::KeyPressed(HELL_KEY_F5)) {
            Callbacks::OpenMapObjectEditor();
        }
        if (Input::KeyPressed(HELL_KEY_PERIOD)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::GetCurrentRendererSettings().screenspaceReflections = !Renderer::GetCurrentRendererSettings().screenspaceReflections;
        }


        if (Input::KeyPressed(HELL_KEY_K)) {
            Game::RespawnPlayers();
        }

        if (Input::KeyPressed(HELL_KEY_H)) {
            Renderer::HotloadShaders();
        }
        //if (Input::KeyPressed(HELL_KEY_F9)) {
        //    player->SetFootPosition(glm::vec3(14.11f, 0.0f, 18.24f));
        //    player->GetCamera().SetEulerRotation(glm::vec3(-0.19f, -1.36f, 0.0f));
        //    BackEnd::ToggleBindlessTextures();
        //    Renderer::HotloadShaders();
        //}
        //if (Input::KeyPressed(HELL_KEY_ESCAPE) && Input::KeyDown(HELL_KEY_1)) {
        if (Input::KeyPressed(HELL_KEY_ESCAPE)) {
            BackEnd::ForceCloseWindow();
        }
        if (Input::KeyPressed(HELL_KEY_X)) {
            BackEnd::ToggleFullscreen();
        }
        if (Input::KeyPressed(HELL_KEY_BACKSLASH)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Renderer::NextRendererOverrideState();
        }
        if (Input::KeyPressed(HELL_KEY_GRAVE_ACCENT)) {
            Audio::PlayAudio(AUDIO_SELECT, 1.00f);
            Debug::NextDebugTextMode();
        }
        if (!Editor::IsOpen()) {
            if (Input::KeyPressed(HELL_KEY_C)) {
                Game::NextSplitScreenMode();
            }
            if (Input::KeyPressed(HELL_KEY_1) && Game::GetLocalPlayerCount() >= 1) {
                Game::SetPlayerKeyboardAndMouseIndex(0, 0, 0);
                Game::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_2) && Game::GetLocalPlayerCount() >= 2) {
                Game::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(1, 0, 0);
                Game::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_3) && Game::GetLocalPlayerCount() >= 3) {
                Game::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(2, 0, 0);
                Game::SetPlayerKeyboardAndMouseIndex(3, 1, 1);
            }
            if (Input::KeyPressed(HELL_KEY_4) && Game::GetLocalPlayerCount() >= 4) {
                Game::SetPlayerKeyboardAndMouseIndex(0, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(1, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(2, 1, 1);
                Game::SetPlayerKeyboardAndMouseIndex(3, 0, 0);
            }
            if (Input::KeyPressed(HELL_KEY_B)) {
                Audio::PlayAudio(AUDIO_SELECT, 1.00f);
                Debug::NextDebugRenderMode();
            }
            if (Input::KeyPressed(HELL_KEY_APOSTROPHE)) {
                Audio::PlayAudio(AUDIO_SELECT, 1.00f);
                Debug::SetDebugRenderMode(DebugRenderMode::BVH_CPU_PLAYER_RAYS);
            }
        }

        //if (Input::KeyPressed(HELL_KEY_F11)) {
        //    HouseManager::LoadAllHouseFilesFromDisk();
        //}
    }
}
