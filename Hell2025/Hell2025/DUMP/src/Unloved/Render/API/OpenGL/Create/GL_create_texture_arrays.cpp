#include "Hell/Common/Constants.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Render/RendererTypes.h"
#include "Unloved/Systems/Ocean/Ocean.h"

namespace OpenGL::Renderer {

    void CreateTextureArrays() {
        // TODO: probably move this out of OpenGL init and into the API agnostic init
        // TODO: probably move this out of OpenGL init and into the API agnostic init
        // TODO: probably move this out of OpenGL init and into the API agnostic init

        Hell::TextureArray& woundMasks = Hell::ResourceManager::CreateTextureArray("WoundMasks");
        woundMasks.CleanUp();
        woundMasks.AllocateMemory(WOUND_MASK_TEXTURE_SIZE, WOUND_MASK_TEXTURE_SIZE, GL_R8, 1, WOUND_MASK_TEXTURE_ARRAY_SIZE); // consider adding mipmaps

        Hell::TextureArray& terrainDisplacement = Hell::ResourceManager::CreateTextureArray("TerrainDisplacement");
        terrainDisplacement.CleanUp();
        terrainDisplacement.AllocateMemory(
            TERRAIN_DISPLACEMENT_STRIP_SIZE * TERRAIN_DISPLACEMENT_TESSELLATION_LEVEL,
            TERRAIN_DISPLACEMENT_STRIP_SIZE,
            GL_RGBA16F,
            1,
            TERRAIN_DISPLACEMENT_LAYER_COUNT);
        terrainDisplacement.SetWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
        terrainDisplacement.SetMinFilter(TextureFilter::LINEAR);
        terrainDisplacement.SetMagFilter(TextureFilter::LINEAR);
        terrainDisplacement.Clear(0.5f, 0.5f, 0.5f, 1.0f);

    }
}
