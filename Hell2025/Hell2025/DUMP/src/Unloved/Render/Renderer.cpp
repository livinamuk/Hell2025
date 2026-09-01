#include "Renderer.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/API/Vulkan/VK_renderer.h"
#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Render/RendererConstants.h"

#include "Timer.hpp"

namespace Audio = Hell::Audio;

namespace Unloved::Renderer {

    std::vector<bool> g_freeWoundMaskIndices;

    bool g_gameIsRendering = false;

    void Init() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::Init();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::Init();
        }
    }

    void CleanUp() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::CleanUp();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::CleanUp();
        }
    }

    void WaitIdle() {
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::WaitIdle();
        }
    }

    void InitMain() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::InitMain();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::InitMain();
        }

        InitWoundMaskArray();
    }

    void RenderLoadingScreen() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::RenderLoadingScreen();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderLoadingScreen();
        }
    }

    void RenderBlackFrame() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::RenderBlackFrame();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderBlackFrame();
        }
    }

    void PreGameLogicComputePasses() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::PreGameLogicComputePasses();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            //Logging::ToDo() << "Vulkan: PreGameLogicComputePasses()";
        }
    }

    void RenderGame() {
        g_gameIsRendering = true;

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::RenderGame();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::RenderGame();
        }
    }

	void HotloadShaders() {
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::UnbindShader();
            OpenGL::ResourceManager::HotloadShaders();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanRenderer::HotloadShaders();
        }
    }

    void RecalculateAllHeightMapData(bool uploadWorldHeightData, bool updatePhysics) {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::RecalculateAllHeightMapData(uploadWorldHeightData, updatePhysics);
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: RecalculateAllHeightMapData()";
        }
    }

    int32_t GetNextFreeWoundMaskIndexAndMarkItTaken() {
        for (int i = 0; i < g_freeWoundMaskIndices.size(); i++) {
            if (g_freeWoundMaskIndices[i] == true) {
                g_freeWoundMaskIndices[i] = false;
                return i;
            }
        }

        // Should never happen, unless you ran out of array levels, in which case you need to increase the size of the array
        for (int i = 0; i < g_freeWoundMaskIndices.size(); i++) {
            Logging::Error() << "GetNextFreeWoundMaskIndexAndMarkItTaken() failed because you ran out of free wound mask textures";
            std::cout << i << ": " << g_freeWoundMaskIndices[i] << "\n";
        }
        return -1;
    }

    void InitWoundMaskArray() {
        // Create and init all wound mask indices to true, aka available
        g_freeWoundMaskIndices.assign(WOUND_MASK_TEXTURE_ARRAY_SIZE, true);
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGL::Renderer::ClearAllWoundMasks();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            Logging::ToDo() << "Vulkan: InitWoundMaskArray()";
        }
    }

    void MarkWoundMaskIndexAsAvailable(int32_t index) {
        if (index < 0 || index >= g_freeWoundMaskIndices.size()) {
            Logging::Error() << "Renderer::MarkWoundMaskIndexAsAvailable() failed. Index '" << index << "' is out of range of size '" << g_freeWoundMaskIndices.size() << "'";
            return;
        }
        g_freeWoundMaskIndices[index] = true;
    }

    const std::string& GetZoneNames() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetZoneNames();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetZoneNames();
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneGPUTimings() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetZoneGPUTimings();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetZoneGPUTimings();
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetZoneCPUTimings() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetZoneCPUTimings();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetZoneCPUTimings();
        }

        static std::string empty = "";
        return empty;
    }

    const std::string& GetTotalGPUTime() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetTotalGPUTime();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetTotalGPUTime();
        }

        static std::string empty = "";
        return empty;
    }

    float GetTotalGPUTimeFloat() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetTotalGPUTimeFloat();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetTotalGPUTimeFloat();
        }

        return 0.0f;
    }

    const std::string& GetTotalCPUTime() {
        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            return OpenGL::Renderer::GetTotalCPUTime();
        }
        else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            return VulkanRenderer::GetTotalCPUTime();
        }

        static std::string empty = "";
        return empty;
    }

    uint32_t GetTileCount() {
        return GetTileCountX() * GetTileCountY();
    }

    uint32_t GetTileCountX() {
        const Resolutions& resolutions = Config::GetResolutions();
        return (resolutions.gBuffer.x + TILE_SIZE - 1) / TILE_SIZE;
    }

    uint32_t GetTileCountY() {
		const Resolutions& resolutions = Config::GetResolutions();
		return (resolutions.gBuffer.y + TILE_SIZE - 1) / TILE_SIZE;
    }

    bool GameIsRendering() {
        return g_gameIsRendering;
    }
}
