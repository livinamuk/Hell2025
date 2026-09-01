#include "Debug.h"

#include "Hell/Input.h"
#include "Hell/Audio.h"
#include "Hell/Time.h"
#include "Hell/Common/Enum.h"
#include "Hell/Common/String.h"
#include "Hell/Logging.h"
#include "Hell/Math/Range.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Profiling/CPUProfiler.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/Backend/BackEnd.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "Unloved/Objects/Effects/Decal.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"
#include "Unloved/Systems/PianoPlayback/PianoPlaybackManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

#include "World/LegacyWorld.h"

#include <cstdint>
#include <vector>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;


namespace Debug {
    using namespace Unloved;

    std::string g_text = "";
    bool g_showDebugText = false;
    DebugRenderMode g_debugRenderMode = DebugRenderMode::NONE;
    DebugTextMode g_debugTextMode = DebugTextMode::NONE;

    std::string g_quickMessage = UNDEFINED_STRING;
    float g_quickMessageTimer = 0;

    Hell::Physics::DebugMode ToPhysicsDebugMode(DebugRenderMode debugRenderMode) {
        switch (debugRenderMode) {
            case DebugRenderMode::PHYSX_ALL:       return Hell::Physics::DebugMode::ALL;
            case DebugRenderMode::PHYSX_RAYCAST:   return Hell::Physics::DebugMode::RAYCAST_SHAPES;
            case DebugRenderMode::PHYSX_COLLISION: return Hell::Physics::DebugMode::COLLISION_SHAPES;
            case DebugRenderMode::RAGDOLLS:        return Hell::Physics::DebugMode::RAGDOLLS;
            default:                               return Hell::Physics::DebugMode::NONE;
        }
    }

    void DisplayQuickMessage();

    void UpdateDebugPointsAndLines();
    void UpdateDebugText();

    void Update() {
        UpdateDebugPointsAndLines();
        UpdateDebugText();

        DisplayQuickMessage();

        if (IsMenuVisible()) {
            UpdateMenu();
        }
    }

    std::string GetFPSString() {
        Hell::CPUProfiler::Report report = Hell::CPUProfiler::GetReport();
        if (report.timingColumns.empty()) return "UNKNOWN FPS";

        float ms = std::stof(report.timingColumns[0]);
        float fps = 1000.0f / ms;
        int fpsRounded = std::round(fps);
        return std::to_string(fpsRounded) + " FPS";
    }

    void BlitDebugStats() {
        // Blit time stats top right
        float renderTime = Unloved::Renderer::GetTotalGPUTimeFloat();
        float updateTime = Hell::CPUProfiler::GetZoneTime("Unloved::Update");

        std::string renderTimeStr = Hell::String::FormatFloat(renderTime, 2) + " ms";
        std::string updateTImeStr = Hell::String::FormatFloat(updateTime, 2) + " ms";

        static std::string spacing = "   ";
        static std::string color = "[COL=1.0,0.698039,0.2]";

        float scale = 2.0f;
        float size0 = TextBlitter::GetTextSize("60 FPS", "StandardFont", scale).x;
        float size1 = TextBlitter::GetTextSize("0.00 ms" + spacing + "60 FPS", "StandardFont", scale).x;
        float size2 = TextBlitter::GetTextSize("Render: 0.00 ms" + spacing + "60 FPS", "StandardFont", scale).x;
        float size3 = TextBlitter::GetTextSize("0.00 ms" + spacing + "Render: 0.00 ms" + spacing + "60 FPS", "StandardFont", scale).x;
        float spaceSize = TextBlitter::GetTextSize(" ", "StandardFont", scale).x;

        if (renderTime >= 10.0f) {
            size1 += spaceSize;
            size2 += spaceSize;
            size3 += spaceSize;
        }

        if (updateTime >= 10.0f) {
            size3 += spaceSize;
        }

        UIBackEnd::BlitText(color + GetFPSString(), "StandardFont", 1920, 0, Alignment::TOP_RIGHT, scale);
        UIBackEnd::BlitText(color + renderTimeStr + spacing, "StandardFont", 1920 - size0, 0, Alignment::TOP_RIGHT, scale);
        UIBackEnd::BlitText(color + "Render: ", "StandardFont", 1920 - size1, 0, Alignment::TOP_RIGHT, scale);
        UIBackEnd::BlitText(color + updateTImeStr + spacing, "StandardFont", 1920 - size2, 0, Alignment::TOP_RIGHT, scale);
        UIBackEnd::BlitText(color + "Update: ", "StandardFont", 1920 - size3, 0, Alignment::TOP_RIGHT, scale);
    }

    void BlitPianoInfo() {
        UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextTime(), "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
        UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextEvents(), "StandardFont", 250, 0, Alignment::TOP_LEFT, 2.0f);
        UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextVelocity(), "StandardFont", 500, 0, Alignment::TOP_LEFT, 2.0f);
        UIBackEnd::BlitText(Unloved::PianoPlaybackManager::GetDebugTextTimeDurations(), "StandardFont", 750, 0, Alignment::TOP_LEFT, 2.0f);
    }

    void UpdateDebugText() {
        if (EditorSession::IsActive()) return;

        BlitDebugStats();

        // Midi notes override
        if (Unloved::PianoPlaybackManager::IsPlaying()) {
            BlitPianoInfo();
            return;
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER) return;
        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER_LADDER_INFO) return;
        if (Debug::GetDebugTextMode() == DebugTextMode::NONE)       return;

        // Regular global debug
        std::string text = "";

        // Mirrors
        if (false) {
            text += "Mirror count: " + std::to_string(Unloved::MirrorManager::GetMirrors().size()) + "\n";
            for (Mirror& mirror : Unloved::MirrorManager::GetMirrors()) {
                text += "- ";
                text += std::to_string(mirror.GetObjectId()) + " ";
                text += Hell::String::FormatVec3(mirror.GetWorldCenter()) + "\n";
            }

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(0);

            text += " ";
            text += "Unloved::Viewport Mirror ID: " + std::to_string(viewport->GetMirrorId()) + "\n";

        }


        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);

        return;


        const DebugRenderMode& debugRenderMode = Debug::GetDebugRenderMode();
        if (debugRenderMode != DebugRenderMode::NONE) {
            AddText("Line Mode: " + Hell::Enum::ToString(debugRenderMode));
        }

        std::string text2 = "SHIT\n";
        UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);

        return;

        const Resolutions& resolutions = Config::GetResolutions();
        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        int i = 0;
        glm::mat4 projectionMatrix = viewportData[i].projection;
        glm::mat4 viewMatrix = viewportData[i].view;
        glm::mat4 inverseViewMatrix = viewportData[i].inverseView;
        glm::vec3 viewPos = inverseViewMatrix[3];
        glm::vec3 rayOrigin = viewPos;

        int hoveredViewportIndex = EditorSession::Viewports::GetHoveredViewportIndex();
        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(hoveredViewportIndex);

        int mouseX = Input::GetMouseX();
        int mouseY = Input::GetMouseY();
        int windowWidth = Hell::BackEnd::GetCurrentWindowWidth();
        int windowHeight = Hell::BackEnd::GetCurrentWindowHeight();
        int gBufferWidth = resolutions.gBuffer.x;
        int gBufferHeight = resolutions.gBuffer.y;
        int viewportWidth = gBufferWidth * viewport->GetSize().x;
        int viewportHeight = gBufferHeight * viewport->GetSize().y;
        float normalizedMouseX = Hell::Math::MapRange(mouseX, 0, windowWidth, 0, gBufferWidth);
        float normalizedMouseY = Hell::Math::MapRange(mouseY, 0, windowHeight, 0, gBufferHeight);

        float offsetX = viewport->GetPosition().x * gBufferWidth;
        float offsetY = (1 - viewport->GetPosition().y) * gBufferHeight;

        float localX = normalizedMouseX - offsetX;
        float localY = normalizedMouseY - offsetY + viewportHeight;


        float width = viewport->GetSize().x * Hell::BackEnd::GetCurrentWindowWidth();
        float height = viewport->GetSize().y * Hell::BackEnd::GetCurrentWindowHeight();
        float left = viewport->GetPosition().x * Hell::BackEnd::GetCurrentWindowWidth();
        float right = left + width;
        float top = Hell::BackEnd::GetCurrentWindowHeight() - (viewport->GetPosition().y * Hell::BackEnd::GetCurrentWindowHeight());
        float bottom = top - height;

        float viewportSpaceMouseX = Hell::Math::MapRange(mouseX, left, right, 0, viewportWidth);
        float viewportSpaceMouseY = Hell::Math::MapRange(mouseY, bottom, top, 0, viewportHeight);

        AddText("");
        AddText("viewportSpaceMouseX: " + std::to_string(viewportSpaceMouseX));
        AddText("viewportSpaceMouseY: " + std::to_string(viewportSpaceMouseY));
        AddText("");
        AddText("WindowWidth: " + std::to_string(windowWidth));
        AddText("WindowHeight: " + std::to_string(windowHeight));
        AddText("gBufferWidth: " + std::to_string(gBufferWidth));
        AddText("gBufferHeight: " + std::to_string(gBufferHeight));
        AddText("viewportWidth: " + std::to_string(viewportWidth));
        AddText("viewportHeight: " + std::to_string(viewportHeight));
        AddText("mouseX: " + std::to_string(mouseX));
        AddText("mouseY: " + std::to_string(mouseY));
        AddText("localX: " + std::to_string(localX));
        AddText("localY: " + std::to_string(localY));
        AddText("normalizedMouseX: " + std::to_string(normalizedMouseX));
        AddText("normalizedMouseY: " + std::to_string(normalizedMouseY));
        AddText("Hovered Unloved::Viewport Index: " + std::to_string(hoveredViewportIndex));
        AddText("Mouse ray origin: " + Hell::String::FormatVec3(EditorSession::Viewports::GetMouseRayOrigin(hoveredViewportIndex)));
        AddText("Mouse ray direction: " + Hell::String::FormatVec3(EditorSession::Viewports::GetMouseRayDirection(hoveredViewportIndex)));
    }

    void BlitQuickDebugMessage(const std::string& message) {
        g_quickMessageTimer = 2.0f;
        g_quickMessage = message;
    }

    void DisplayQuickMessage() {
        if (g_quickMessageTimer > 0) {
            g_quickMessageTimer -= Hell::Time::DeltaTime();
            if (EditorSession::IsActive()) {
                const glm::uvec2 internalResolution = UIBackEnd::GetCanvasResolution(UICanvas::INTERNAL);
                const glm::uvec2 nativeResolution = UIBackEnd::GetCanvasResolution(UICanvas::NATIVE);
                const float nativeToInternalScale = static_cast<float>(internalResolution.x) / static_cast<float>(nativeResolution.x);
                const int32_t viewportLeft = static_cast<int32_t>(std::round(EditorSession::Layout::GetHierarchyPanel().rect.Right() * nativeToInternalScale));
                UIBackEnd::BlitText(g_quickMessage, "StandardFont", viewportLeft, internalResolution.y, Alignment::BOTTOM_LEFT, 2.0f);
            }
            else {
                UIBackEnd::BlitText(g_quickMessage, "StandardFont", 0, Config::GetResolutions().gBuffer.y, Alignment::BOTTOM_LEFT, 2.0f);
            }
        }
    }

    void UpdateDebugPointsAndLines() {

        if (g_debugRenderMode == DebugRenderMode::LIGHTS) {
            static uint32_t lightIndex = 2;

            if (Input::KeyPressed(HELL_KEY_LEFT)) lightIndex--;
            if (Input::KeyPressed(HELL_KEY_RIGHT)) lightIndex++;

            if (lightIndex < 0) lightIndex = Unloved::World::GetLightCount() - 1;
            if (lightIndex == Unloved::World::GetLightCount()) lightIndex = 0;

            if (Light* light = Unloved::World::GetLightByIndex(lightIndex)) {
                AABB worldBounds = AABB(light->GetWorldBoundsMin(), light->GetWorldBoundsMax());
                DebugDraw::DrawAABB(worldBounds, glm::vec4(light->GetColor(), 1.0f));
            }
        }

        if (g_debugRenderMode == DebugRenderMode::BONES) {
            for (SkinnedGameObject& skinnedGameObject : Unloved::World::GetSkinnedGameObjects()) {
                skinnedGameObject.DrawBones();
            }
            for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                //player->GetCharacterModelSkinnedGameObject()->DrawBones(RED, i);
                player->GetViewWeaponSkinnedGameObject()->DrawBones(i);
                //player->GetCharacterModelSkinnedGameObject()->DrawBones();
            }
        }
        if (g_debugRenderMode == DebugRenderMode::BONE_TANGENTS) {
            for (SkinnedGameObject& skinnedGameObject : Unloved::World::GetSkinnedGameObjects()) {
                //skinnedGameObject.DrawBoneTangentVectors();
            }
            for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                //player->GetCharacterModelSkinnedGameObject()->DrawBoneTangentVectors(0.001f, i);
                player->GetViewWeaponSkinnedGameObject()->DrawBoneTangentVectors(0.0025f, i);
                //player->GetCharacterModelSkinnedGameObject()->DrawBoneTangentVectors(0.001f, i);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::CLIPPING_VOLUMES) {
            for (const Door& door : Unloved::World::GetDoors()) {
                door.GetClippingVolume().DrawDebugCorners(OUTLINE_COLOR);
                door.GetClippingVolume().DrawDebugEdges(WHITE);
            }
            for (const Window& window : Unloved::World::GetWindows()) {
                window.GetClippingVolume().DrawDebugCorners(OUTLINE_COLOR);
                window.GetClippingVolume().DrawDebugEdges(WHITE);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::BLOCKING_VOLUMES) {
            for (const Fireplace& fireplace : Unloved::World::GetFireplaces()) {
                fireplace.GetBlockingVolume().DrawDebugCorners(OUTLINE_COLOR);
                fireplace.GetBlockingVolume().DrawDebugEdges(WHITE);
            }
        }
        if (g_debugRenderMode == DebugRenderMode::DECALS) {
            for (const Decal& decal : Unloved::World::GetDecals()) {
                DebugDraw::DrawPoint(decal.GetPosition(), OUTLINE_COLOR);
                DebugDraw::DrawLine(decal.GetPosition(), decal.GetPosition() + decal.GetWorldNormal() * 0.05f, OUTLINE_COLOR);
            }
        }
        Hell::Physics::DebugMode physicsDebugMode = ToPhysicsDebugMode(g_debugRenderMode);
        if (physicsDebugMode != Hell::Physics::DebugMode::NONE) {
            Hell::Physics::ForceZeroStepUpdate();
            for (const Hell::Physics::PhysicsDebugLine& line : Hell::Physics::GetPhysicsDebugLines(physicsDebugMode)) {
                DebugDraw::DrawLine(line.p1, line.p2, line.color);
            }
        }
    }

    void AddText(const std::string& text) {
        g_text += text + "\n";
    }

    const std::string& GetText() {
        return g_text;
    }

    void EndFrame() {
        g_text = "";
        EndMenuFrame();
    }

    void SetDebugTextMode(DebugTextMode mode) {
        g_debugTextMode = mode;
    }

    void SetDebugRenderMode(DebugRenderMode mode) {
        g_debugRenderMode = mode;
    }

    void NextDebugRenderMode() {
        std::vector<DebugRenderMode> allowedDebugRenderModes = {
            NONE,
            PHYSX_ALL,
            RAGDOLLS,
            CLIPPING_VOLUMES,
            BLOCKING_VOLUMES,
            //HOUSE_GEOMETRY,
            DECALS,
            BONES,
            BONE_TANGENTS,
            LIGHTS,
            BVH_CPU_PLAYER_RAYS
            //PATHFINDING,
            //PHYSX_COLLISION,
            //PATHFINDING_RECAST,
            //RTX_LAND_TOP_LEVEL_ACCELERATION_STRUCTURE,
            //RTX_LAND_BOTTOM_LEVEL_ACCELERATION_STRUCTURES,
            //BOUNDING_BOXES,
        };

        g_debugRenderMode = (DebugRenderMode)(int(g_debugRenderMode) + 1);
        if (g_debugRenderMode == DEBUG_LINE_MODE_COUNT) {
            g_debugRenderMode = (DebugRenderMode)0;
        }
        // If mode isn't in available modes list, then go to next
        bool allowed = false;
        for (auto& avaliableMode : allowedDebugRenderModes) {
            if (g_debugRenderMode == avaliableMode) {
                allowed = true;
                break;
            }
        }
        if (!allowed && g_debugRenderMode != DebugRenderMode::NONE) {
            NextDebugRenderMode();
        }

        Debug::BlitQuickDebugMessage("Debug Render Mode: " + Hell::Enum::ToString(g_debugRenderMode));
        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    void PrintModelMeshNames(const std::string& name) {
        Model* model = Hell::ResourceManager::GetModelByName(name);
        if (!model) {
            Logging::Error() << "Debug::PrintModelMeshNames(..) failed coz model param was nullptr\n";
            return;
        }

        std::cout << model->GetName() << "\n";
        for (const uint32_t& meshId : model->GetMeshIndices()) {
            Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
            if (mesh) {
                std::cout << " - " << mesh->name << "\n";
            }
            else {
                std::cout << " - INVALID MESH SOMEHOW\n";
            }
        }
    }

    const DebugRenderMode& GetDebugRenderMode() {
        return g_debugRenderMode;
    }

    const DebugTextMode& GetDebugTextMode() {
        return g_debugTextMode;
    }
}
