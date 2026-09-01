#include "Unloved.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Hell/AssetLoader/AssetLoader.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/UIBackEnd.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/Gizmo/Gizmo.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/GameAudio/GameAudio.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/House/HouseGeometryBuilder.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/Systems/Systems.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

namespace Unloved {

    ProgramState g_programState = ProgramState::UNDEFINED;
    ProgramState g_requestedProgramState = ProgramState::LOADING_SCREEN;

    void UpdateLazyKeypresses();

    void BeginFrameLoadingScreen();
    void UpdateLoadingScreen();
    void UpdateGame();

    void BeginFrameGame();
    void RenderLoadingScreen();
    void RenderGame();

    bool Init() {
        const Resolutions& resolutions = Config::GetResolutions();

        Renderer::Init();
        UIBackEnd::SetUIResolution(resolutions.ui.x, resolutions.ui.y);
        UIBackEnd::Init();
        Bible::Init();
        Gizmo::Init();
        Unloved::ViewportManager::Init();
        EditorSession::Init();
        Hell::Physics::Init();
        CoarseWorldBVH::Init();
        Systems::Init();

        return true;
    }

    void BeginFrame() {
        g_programState = g_requestedProgramState;

        switch (GetProgramState()) {
            case ProgramState::GAME:           BeginFrameGame();          break;
            case ProgramState::LOADING_SCREEN: BeginFrameLoadingScreen(); break;
        }

    }

    void Update() {
        ProfilerCPUZoneFunction();

        switch (GetProgramState()) {
            case ProgramState::GAME:           UpdateGame();          break;
            case ProgramState::LOADING_SCREEN: UpdateLoadingScreen(); break;
        }

        UpdateLazyKeypresses();
    }

    void Render() {
        ProfilerCPUZoneFunction();

        switch (GetProgramState()) {
            case ProgramState::LOADING_SCREEN: RenderLoadingScreen(); break;
            case ProgramState::GAME:           RenderGame();          break;
        }
    }

    void EndFrame() {
        Debug::EndFrame();
        World::EndFrame();
    }

    void CleanUp() {
        Renderer::WaitIdle();
        Systems::CleanUp();
        World::CleanUp();
        CoarseWorldBVH::CleanUp();
        Renderer::CleanUp();
    }

    void SetProgramState(ProgramState programState) {
        g_requestedProgramState = programState;
    }

    ProgramState GetProgramState() {
        return g_programState;
    }

    // Loading Screen

    void BeginFrameLoadingScreen() {
        RenderDataManager::BeginFrame();
        UIBackEnd::BeginFrame();
    }

    void UpdateLoadingScreen() {
        ProfilerCPUZoneFunction();

        Hell::AssetLoader::Update();

        UIBackEnd::Update();
        RenderDataManager::UpdateDrawCommandsUI();

        if (Hell::AssetLoader::LoadingComplete()) {

            Renderer::InitMain();
            HouseBuilder::Init();
            HouseGeometryBuilder::Init();
            HouseManager::Init();
            MapManager::Init();
            Session::Create();
            Session::StartNewGame(GameMode::CAMPAIGN, "Shit");

            Hell::ResourceManager::FreeTextureCPUMemory();

            SetProgramState(ProgramState::GAME);
        }
    }

    void RenderLoadingScreen() {
        Renderer::RenderLoadingScreen();
    }

    // Game

    void BeginFrameGame() {
        Session::ProcessPendingNewGame();
        DebugDraw::BeginFrame();
        RenderDataManager::BeginFrame();
        Session::BeginFrame();
        Systems::BeginFrame();
        UIBackEnd::BeginFrame();
        World::BeginFrame();
    }

    void UpdateGame() {
        ProfilerCPUZoneFunction();

        // Pre World Update
        Systems::PreWorldUpdate();

        Renderer::PreGameLogicComputePasses();

        EditorSession::Update();
        Unloved::ViewportManager::Update();
        EditorSession::UpdateViewportInput();

        HouseBuilder::RebuildIfDirty();

        AStarMap::Update();
        Session::Update();
        World::UpdatePlayers();
        BulletSystem::Update();
        if (EditorSession::IsInactive()) {
            World::UpdateEnemyMovement();
        }
        World::UpdateObjects();
        World::UpdateBvhs();

        // World Update
        World::Update();

        // Physics
        if (EditorSession::IsInactive()) {
            Hell::Physics::StepSimulation();
        }
        Hell::Physics::SyncRuntimeState();
        if (EditorSession::IsInactive()) {
            Hell::Physics::UpdateHeightFields();
        }
        else {
            Hell::Physics::ActivateAllHeightFields();
        }

        // Post World Update
        Session::PostWorldUpdate();
        Systems::PostWorldUpdate();

        // Render Submit
        World::SubmitRenderItems();

        Debug::Update();
        EditorSession::Render();
        UIBackEnd::Update();
        RenderDataManager::Update();
    }

    void RenderGame() {
        if (Session::HasPendingNewGame()) {
            Renderer::RenderBlackFrame();
        }
        else {
            Renderer::RenderGame();
        }
    }
}
