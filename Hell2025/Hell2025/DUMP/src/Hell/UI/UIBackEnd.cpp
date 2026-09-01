#include "UIBackEnd.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/UI/TextBlitter.h"

#include <algorithm>
#include <array>

using namespace Hell;

namespace UIBackEnd {
    namespace {
        struct CanvasState {
            glm::uvec2 resolution = glm::uvec2(1);
            std::vector<RenderItemUI> renderItems;
        };

        constexpr size_t CANVAS_COUNT = static_cast<size_t>(UICanvas::COUNT);

        std::array<CanvasState, CANVAS_COUNT> g_canvasStates;
        std::vector<RenderItemUI> g_renderItems;
        std::vector<Vertex2D> g_vertices;
        std::vector<uint32_t> g_indices;

        size_t GetCanvasIndex(UICanvas canvas) {
            const size_t index = static_cast<size_t>(canvas);
            return index < CANVAS_COUNT ? index : 0;
        }

        CanvasState& GetCanvasState(UICanvas canvas) {
            return g_canvasStates[GetCanvasIndex(canvas)];
        }

        const CanvasState& GetCanvasStateConst(UICanvas canvas) {
            return g_canvasStates[GetCanvasIndex(canvas)];
        }

        void PackRenderItems() {
            size_t renderItemCount = 0;
            for (const CanvasState& state : g_canvasStates) renderItemCount += state.renderItems.size();

            g_renderItems.clear();
            g_renderItems.reserve(renderItemCount);
            for (const CanvasState& state : g_canvasStates) g_renderItems.insert(g_renderItems.end(), state.renderItems.begin(), state.renderItems.end());
        }
    }

    void Init() {
        Logging::Init() << "Initialized the UI Backend";
        TextBlitter::Init();
    }

    void BeginFrame() {
        g_vertices.clear();
        g_indices.clear();
        g_renderItems.clear();
        for (CanvasState& state : g_canvasStates) state.renderItems.clear();
    }

    void Update() {
        PackRenderItems();

        if (BackEnd::GetAPI() == API::OPENGL) {
            GenericMesh& genericMesh = ResourceManager::GetGenericMesh("UI");
            genericMesh.UpdateVertexData(g_vertices);
            genericMesh.UpdateIndexData(g_indices);
        }
    }

    void SetUIResolution(uint32_t width, uint32_t height) {
        SetCanvasResolution(UICanvas::INTERNAL, width, height);
    }

    void SetCanvasResolution(UICanvas canvas, uint32_t width, uint32_t height) {
        GetCanvasState(canvas).resolution = glm::uvec2(std::max(1u, width), std::max(1u, height));
    }

    void ClearCanvas(UICanvas canvas) {
        GetCanvasState(canvas).renderItems.clear();
    }

    void BlitText(const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        BlitText(UICanvas::INTERNAL, text, fontName, location.x, location.y, alignment, scale, textureFilter, clipMinX, clipMinY, clipMaxX, clipMaxY);
    }

    void BlitText(const std::string& text, const std::string& fontName, int originX, int originY, Alignment alignment, float scale, TextureFilter textureFilter, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        BlitText(UICanvas::INTERNAL, text, fontName, originX, originY, alignment, scale, textureFilter, clipMinX, clipMinY, clipMaxX, clipMaxY);
    }

    void BlitText(UICanvas canvas, const std::string& text, const std::string& fontName, glm::ivec2 location, Alignment alignment, float scale, TextureFilter textureFilter, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        BlitText(canvas, text, fontName, location.x, location.y, alignment, scale, textureFilter, clipMinX, clipMinY, clipMaxX, clipMaxY);
    }

    void BlitText(UICanvas canvas, const std::string& text, const std::string& fontName, int originX, int originY, Alignment alignment, float scale, TextureFilter textureFilter, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        FontSpriteSheet* spriteSheet = TextBlitter::GetFontSpriteSheet(fontName);
        if (!spriteSheet) {
            Logging::Error() << "UIBackEnd::BlitText(..) failed to find " << fontName << "\n";
            return;
        }

        int textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(spriteSheet->m_textureName);
        if (textureIndex == -1) {
            Logging::Error() << "UIBackEnd::BlitText(..) failed to find texture " << spriteSheet->m_textureName << "\n";
            return;
        }

        size_t baseIndex = g_indices.size();

        CanvasState& canvasState = GetCanvasState(canvas);
        const glm::ivec2 resolution(static_cast<int32_t>(canvasState.resolution.x), static_cast<int32_t>(canvasState.resolution.y));
        TextBlitter::BlitText(text, fontName, originX, originY, resolution, alignment, scale, textureIndex, g_vertices, g_indices);

        size_t indexCount = g_indices.size() - baseIndex;
        if (indexCount == 0) return;

        RenderItemUI& renderItem = canvasState.renderItems.emplace_back();
        renderItem.baseVertex = 0u;
        renderItem.baseIndex = static_cast<uint32_t>(baseIndex);
        renderItem.indexCount = static_cast<uint32_t>(indexCount);
        renderItem.textureIndex = static_cast<uint32_t>(textureIndex);
        renderItem.filterMode = (textureFilter == TextureFilter::NEAREST) ? 1u : 0u;
        renderItem.clipMinX = clipMinX >= 0 ? clipMinX : 0;
        renderItem.clipMinY = clipMinY >= 0 ? clipMinY : 0;
        renderItem.clipMaxX = clipMaxX >= 0 ? clipMaxX : static_cast<int32_t>(canvasState.resolution.x);
        renderItem.clipMaxY = clipMaxY >= 0 ? clipMaxY : static_cast<int32_t>(canvasState.resolution.y);
    }

    void BlitTexture(BlitTextureInfo info) {
        BlitTexture(UICanvas::INTERNAL, info);
    }

    void BlitTexture(UICanvas canvas, BlitTextureInfo info) {
        BlitTexture(canvas, info.textureName, info.location, info.alignment, info.colorTint, info.size, info.textureFilter, info.rotation, info.clipMinX, info.clipMinY, info.clipMaxX, info.clipMaxY);
    }

    void BlitTexture(const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint, glm::ivec2 size, TextureFilter textureFilter, float rotation, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        BlitTexture(UICanvas::INTERNAL, textureName, location, alignment, colorTint, size, textureFilter, rotation, clipMinX, clipMinY, clipMaxX, clipMaxY);
    }

    void BlitTexture(UICanvas canvas, const std::string& textureName, glm::ivec2 location, Alignment alignment, glm::vec4 colorTint, glm::ivec2 size, TextureFilter textureFilter, float rotation, int clipMinX, int clipMinY, int clipMaxX, int clipMaxY) {
        // Bail if texture not found
        int textureIndex = Hell::ResourceManager::GetTextureBindlessIndexByName(textureName);
        if (textureIndex == -1) {
            Logging::Error() << "UIBackEnd::BlitTexture(..) failed to find texture " << textureName << "\n";
            return;
        }
        // Get texture dimensions
        Texture* texture = Hell::ResourceManager::GetTextureByBindlessIndex(textureIndex);
        const float w = (size.x != -1) ? static_cast<float>(size.x) : static_cast<float>(texture->GetWidth());
        const float h = (size.y != -1) ? static_cast<float>(size.y) : static_cast<float>(texture->GetHeight());

        glm::vec2 positions[4] = {};
        glm::vec2 uvs[4] = {
            { 0.0f, 0.0f },
            { 1.0f, 0.0f },
            { 1.0f, 1.0f },
            { 0.0f, 1.0f }
        };

        // Alignment
        switch (alignment) {
            case Alignment::TOP_LEFT:
                positions[0] = { 0, 0 }; positions[1] = { w, 0 };
                positions[2] = { w, h }; positions[3] = { 0, h };
                break;
            case Alignment::TOP_RIGHT:
                positions[0] = { -w, 0 }; positions[1] = { 0, 0 };
                positions[2] = { 0, h }; positions[3] = { -w, h };
                break;
            case Alignment::BOTTOM_LEFT:
                positions[0] = { 0, -h }; positions[1] = { w, -h };
                positions[2] = { w, 0 }; positions[3] = { 0, 0 };
                break;
            case Alignment::BOTTOM_RIGHT:
                positions[0] = { -w, -h }; positions[1] = { 0, -h };
                positions[2] = { 0, 0 }; positions[3] = { -w, 0 };
                break;
            case Alignment::CENTERED:
                positions[0] = { -w * 0.5f, -h * 0.5f }; positions[1] = { w * 0.5f, -h * 0.5f };
                positions[2] = { w * 0.5f,  h * 0.5f }; positions[3] = { -w * 0.5f,  h * 0.5f };
                break;
            case Alignment::CENTERED_HORIZONTAL:
                positions[0] = { -w * 0.5f, 0 }; positions[1] = { w * 0.5f, 0 };
                positions[2] = { w * 0.5f,  h }; positions[3] = { -w * 0.5f,  h };
                break;
            case Alignment::CENTERED_VERTICAL:
                positions[0] = { 0, -h * 0.5f }; positions[1] = { w, -h * 0.5f };
                positions[2] = { w, h * 0.5f }; positions[3] = { 0, h * 0.5f };
                break;
            default:
                return;
        };

        // Rotation
        float s = sin(rotation);
        float c = cos(rotation);

        for (int i = 0; i < 4; ++i) {
            float newX = positions[i].x * c - positions[i].y * s;
            float newY = positions[i].x * s + positions[i].y * c;
            positions[i] = { newX, newY };
        }

        // Snap to integer pixels
        glm::vec2 anchor = glm::round(glm::vec2(location));

        // Convert the final screen position to NDC
        glm::vec2 finalVertices[4] = {};
        CanvasState& canvasState = GetCanvasState(canvas);

        for (int i = 0; i < 4; ++i) {
            glm::vec2 screenPos = glm::vec2(anchor) + positions[i];
            finalVertices[i].x = (screenPos.x / static_cast<float>(canvasState.resolution.x)) * 2.0f - 1.0f;
            finalVertices[i].y = 1.0f - (screenPos.y / static_cast<float>(canvasState.resolution.y)) * 2.0f;
        }

        const uint32_t baseVertex = static_cast<uint32_t>(g_vertices.size());
        g_vertices.reserve(baseVertex + 4u);
        g_vertices.push_back({ { finalVertices[0].x, finalVertices[0].y }, uvs[0], colorTint });
        g_vertices.push_back({ { finalVertices[1].x, finalVertices[1].y }, uvs[1], colorTint });
        g_vertices.push_back({ { finalVertices[2].x, finalVertices[2].y }, uvs[2], colorTint });
        g_vertices.push_back({ { finalVertices[3].x, finalVertices[3].y }, uvs[3], colorTint });

        const uint32_t baseIndex = static_cast<uint32_t>(g_indices.size());
        g_indices.reserve(baseIndex + 6u);
        g_indices.push_back(baseVertex + 0u);
        g_indices.push_back(baseVertex + 1u);
        g_indices.push_back(baseVertex + 2u);
        g_indices.push_back(baseVertex + 0u);
        g_indices.push_back(baseVertex + 2u);
        g_indices.push_back(baseVertex + 3u);

        RenderItemUI& renderItem = canvasState.renderItems.emplace_back();
        renderItem.baseVertex = 0u;
        renderItem.baseIndex = baseIndex;
        renderItem.indexCount = 6u;
        renderItem.textureIndex = static_cast<uint32_t>(textureIndex);
        renderItem.filterMode = (textureFilter == TextureFilter::NEAREST) ? 1u : 0u;
        renderItem.clipMinX = clipMinX;
        renderItem.clipMinY = clipMinY;
        renderItem.clipMaxX = clipMaxX;
        renderItem.clipMaxY = clipMaxY;

        // Maybe tidy this up later
        renderItem.clipMinX = (clipMinX >= 0) ? clipMinX : 0;
        renderItem.clipMinY = (clipMinY >= 0) ? clipMinY : 0;
        renderItem.clipMaxX = (clipMaxX >= 0) ? clipMaxX : static_cast<int32_t>(canvasState.resolution.x);
        renderItem.clipMaxY = (clipMaxY >= 0) ? clipMaxY : static_cast<int32_t>(canvasState.resolution.y);
    }

    const std::vector<Vertex2D>& GetVertices() { return g_vertices; }
    const std::vector<uint32_t>& GetIndices() { return g_indices; }
    const std::vector<RenderItemUI>& GetRenderItems() { return g_renderItems; }
    const std::vector<RenderItemUI>& GetRenderItems(UICanvas canvas) { return GetCanvasStateConst(canvas).renderItems; }
    glm::uvec2 GetCanvasResolution(UICanvas canvas) { return GetCanvasStateConst(canvas).resolution; }

    uint32_t GetRenderItemBaseInstance(UICanvas canvas) {
        const size_t canvasIndex = GetCanvasIndex(canvas);
        size_t baseInstance = 0;
        for (size_t i = 0; i < canvasIndex; i++) baseInstance += g_canvasStates[i].renderItems.size();
        return static_cast<uint32_t>(baseInstance);
    }
}
