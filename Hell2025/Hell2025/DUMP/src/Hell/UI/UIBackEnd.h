#pragma once
#include "Hell/UI/UITypes.h"
#include "Hell/Render/VertexAttributes.h"

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace UIBackEnd {
    void Init();
    void Update();
    void SetUIResolution(uint32_t width, uint32_t height);
    void SetCanvasResolution(UICanvas canvas, uint32_t width, uint32_t height);
    void ClearCanvas(UICanvas canvas);

    void BeginFrame();

    void BlitText(const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter = TextureFilter::NEAREST, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);
    void BlitText(const std::string& text, const std::string& fontName, int locationX, int locationY, Alignment alignment, float scale, TextureFilter textureFilter = TextureFilter::NEAREST, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);
    void BlitText(UICanvas canvas, const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter = TextureFilter::NEAREST, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);
    void BlitText(UICanvas canvas, const std::string& text, const std::string& fontName, int locationX, int locationY, Alignment alignment, float scale, TextureFilter textureFilter = TextureFilter::NEAREST, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);
    void BlitTexture(BlitTextureInfo info);
    void BlitTexture(UICanvas canvas, BlitTextureInfo info);
    void BlitTexture(const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint = glm::vec4(1, 1, 1, 1), glm::ivec2 size = glm::ivec2(-1, -1), TextureFilter textureFilter = TextureFilter::NEAREST, float rotation = 0.0f, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);
    void BlitTexture(UICanvas canvas, const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint = glm::vec4(1, 1, 1, 1), glm::ivec2 size = glm::ivec2(-1, -1), TextureFilter textureFilter = TextureFilter::NEAREST, float rotation = 0.0f, int clipMinX = -1, int clipMinY = -1, int clipMaxX = -1, int clipMaxY = -1);

    const std::vector<Vertex2D>& GetVertices();
    const std::vector<uint32_t>& GetIndices();
    const std::vector<RenderItemUI>& GetRenderItems();
    const std::vector<RenderItemUI>& GetRenderItems(UICanvas canvas);
    glm::uvec2 GetCanvasResolution(UICanvas canvas);
    uint32_t GetRenderItemBaseInstance(UICanvas canvas);

}
