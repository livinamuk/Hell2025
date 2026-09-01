#pragma once
#include "ViewportManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/EditorSession/UI/EditorLayout.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/EditorSession/Core/EditorViewports.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"

#include <iostream> // TODO clean up logging

namespace Unloved::ViewportManager {
    constexpr float ORTHOGRAPHIC_CLIP_DISTANCE = 500.0f;
    std::vector<Viewport> g_viewports;

    void Init() {
        g_viewports.clear();
        
        for (int i = 0; i < 4; i++) {
            g_viewports.emplace_back(Viewport(i));
            g_viewports[i].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
        }

        Update();
    }

    void Update() {
        // Set state / recreate matrices based on editor/splitscreen mode
        // Clean this up when you have a moment.
        if (EditorSession::IsActive()) {
            for (int i = 0; i < 4; i++) {
                const EditorSession::EditorViewportRegion* region = EditorSession::Layout::GetViewportRegionByIndex(i);
                if (region && region->visible) {
                    g_viewports[i].SetPosition(region->normalizedPosition);
                    g_viewports[i].SetSize(region->normalizedSize);
                    g_viewports[i].Show();
                }
                else {
                    g_viewports[i].Hide();
                }

                const EditorSession::EditorViewportMode mode = EditorSession::Viewports::GetMode(i);
                g_viewports[i].SetViewportMode(ShadingMode::SHADED);
                if (EditorSession::UsesOrthographicProjection(mode)) {
                    const EditorSession::EditorCamera* camera = EditorSession::Viewports::GetCameraByIndex(i);
                    const float clipCenter = camera ? camera->GetDistance() : 0.0f;

                    // Center the ortho depth range on the pivot instead of the old camera position
                    g_viewports[i].SetOrthographic(EditorSession::Viewports::GetOrthographicSize(i), clipCenter - ORTHOGRAPHIC_CLIP_DISTANCE, clipCenter + ORTHOGRAPHIC_CLIP_DISTANCE);
                }
                else {
                    g_viewports[i].SetPerspective(EditorSession::Viewports::GetPerspectiveFov(i), Config::GetNearPlane(), Config::GetFarPlane());
                }
            }
        }
        // When not in the editor
        else {
            for (int i = 0; i < 4; i++) {
                g_viewports[i].SetViewportMode(ShadingMode::SHADED);
            }
            if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::FULLSCREEN) {
                g_viewports[0].SetPosition(glm::vec2(0.0f, 0.0f));  // Fullscreen
                g_viewports[0].SetSize(glm::vec2(1.0f, 1.0f));
                g_viewports[0].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
                g_viewports[0].Show();
                g_viewports[1].Hide();
                g_viewports[2].Hide();
                g_viewports[3].Hide();
            }
            else if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::TWO_PLAYER) {
                g_viewports[0].SetPosition(glm::vec2(0.0f, 0.0f));  // Top
                g_viewports[1].SetPosition(glm::vec2(0.0f, 0.5f));  // Bottom
                g_viewports[0].SetSize(glm::vec2(1.0f, 0.5f));
                g_viewports[1].SetSize(glm::vec2(1.0f, 0.5f));
                g_viewports[0].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
                g_viewports[1].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
                g_viewports[0].Show();
                g_viewports[1].Show();
                g_viewports[2].Hide();
                g_viewports[3].Hide();
            }
            else if (Unloved::Session::GetSplitscreenMode() == SplitscreenMode::FOUR_PLAYER) {
                g_viewports[0].SetPosition(glm::vec2(0.0f, 0.0f));  // Top-left
                g_viewports[1].SetPosition(glm::vec2(0.5f, 0.0f));  // Top-right
                g_viewports[2].SetPosition(glm::vec2(0.0f, 0.5f));  // Bottom-left
                g_viewports[3].SetPosition(glm::vec2(0.5f, 0.5f));  // Bottom-right
                g_viewports[0].SetSize(glm::vec2(0.5f, 0.5f));
                g_viewports[1].SetSize(glm::vec2(0.5f, 0.5f));
                g_viewports[2].SetSize(glm::vec2(0.5f, 0.5f));
                g_viewports[3].SetSize(glm::vec2(0.5f, 0.5f));
				g_viewports[0].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
				g_viewports[1].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
				g_viewports[2].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
				g_viewports[3].SetPerspective(1.0f, Config::GetNearPlane(), Config::GetFarPlane());
                const int32_t activeViewportCount = Unloved::Session::GetActiveViewportCount();
                for (int32_t viewportIndex = 0; viewportIndex < 4; viewportIndex++) {
                    if (viewportIndex < activeViewportCount) {
                        g_viewports[viewportIndex].Show();
                    }
                    else {
                        g_viewports[viewportIndex].Hide();
                    }
                }
            }
        }

        // Update screen space coords
        for (Viewport& viewport : g_viewports) {
            viewport.Update();
        }

        // Zero out all viewport mirror ids
        for (int i = 0; i < 4; i++) {
            Viewport* viewport = GetViewportByIndex(i);
            if (!viewport) continue;

            viewport->SetMirrorId(0);
        }

        // If editor is closed, then set each viewport mirror id to the corresponding players closest valid mirror
        if (EditorSession::IsInactive()) {
            for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
                if (!player) continue;

                Viewport* viewport = GetViewportByIndex(player->GetViewportIndex());
                if (!viewport || !viewport->IsVisible()) continue;

                viewport->SetMirrorId(player->GetClosestMirrorId());
            }
        }
    }

    Viewport* GetViewportByIndex(int32_t viewportIndex) {
        if (viewportIndex >= 0 && viewportIndex < g_viewports.size()) {
            return &g_viewports[viewportIndex];
        }
        else {
            std::cout << "ViewportManager::GetViewportByIndex(int index) failed. " << viewportIndex << " out of range of size " << g_viewports.size() << "\n";
            return nullptr;
        }
    }

    uint32_t GetActiveViewportMask() {
        uint32_t activeViewportMask = 0;
        for (uint32_t viewportIndex = 0; viewportIndex < g_viewports.size(); viewportIndex++) {
            if (g_viewports[viewportIndex].IsVisible()) {
                activeViewportMask |= 1u << viewportIndex;
            }
        }
        return activeViewportMask;
    }

    std::vector<Viewport>& GetViewports() {
        return g_viewports;
    }
}
