#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Maps/MapData.h"
#include "Unloved/Systems/HeightMap/HeightMap.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"
#include "World/LegacyWorld.h"

#include "Unloved/Systems/Pathfinding/AStarMap.h"

#include "Hell/Physics/Physics.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include <algorithm>
#include <array>
#include <limits>

namespace OpenGL::Renderer {
    using namespace Unloved;

    namespace {
        bool g_terrainDisplacementDirty = true;
        std::array<glm::vec2, TERRAIN_DISPLACEMENT_LAYER_COUNT> g_previousDisplacementTargets {
            glm::vec2(std::numeric_limits<float>::max()),
            glm::vec2(std::numeric_limits<float>::max()),
            glm::vec2(std::numeric_limits<float>::max()),
            glm::vec2(std::numeric_limits<float>::max())
        };
        float g_previousDisplacementTextureScaling = -1.0f;

        void SetTerrainMaterialUniforms() {
            int32_t fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
            if (fallbackMaterialIndex == -1) fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
            if (fallbackMaterialIndex == -1) fallbackMaterialIndex = 0;
            int32_t grassMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Grass");
            int32_t dirtRoadMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("DirtRoad");
            int32_t rockFaceMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("RockFace");
            int32_t sandMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Sand");
            OpenGL::SetUniformInt("u_terrainMaterial0", grassMaterialIndex == -1 ? fallbackMaterialIndex : grassMaterialIndex);
            OpenGL::SetUniformInt("u_terrainMaterial1", dirtRoadMaterialIndex == -1 ? fallbackMaterialIndex : dirtRoadMaterialIndex);
            OpenGL::SetUniformInt("u_terrainMaterial2", rockFaceMaterialIndex == -1 ? fallbackMaterialIndex : rockFaceMaterialIndex);
            OpenGL::SetUniformInt("u_terrainMaterial3", sandMaterialIndex == -1 ? fallbackMaterialIndex : sandMaterialIndex);
        }
    }

    void UploadWorldHeightData();
    void GenerateHeightMapVertexData();
    void GeneratePhysXTextures();
    void DrawHeightMap();

    void RecalculateAllHeightMapData(bool uploadWorldHeightData, bool updatePhysics) {
        if (uploadWorldHeightData) {
            UploadWorldHeightData();
        }
        GenerateHeightMapVertexData();
        if (updatePhysics) {
            GeneratePhysXTextures();
            AStarMap::Init();
            AStarMap::UpdateDebugMeshesFromHeightField();
        }
    }

    void HeightMapPass() {
        DrawHeightMap();
    }

    void UploadWorldHeightData() {
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        if (!worldFramebuffer) return;
        if (!roadFramebuffer) return;

        const std::vector<float>& worldHeightData = HeightMap::GetWorldHeightData();
        const std::vector<uint32_t>& worldTerrainControlData = HeightMap::GetWorldTerrainControlData();
        const uint32_t textureWidth = HeightMap::GetWorldTextureWidth();
        const uint32_t textureHeight = HeightMap::GetWorldTextureHeight();
        if (worldHeightData.empty() || textureWidth == 0 || textureHeight == 0) return;
        if (worldTerrainControlData.size() != worldHeightData.size()) return;

        static std::vector<float> gpuHeightData;
        gpuHeightData.resize(worldHeightData.size());
        for (size_t i = 0; i < worldHeightData.size(); i++) gpuHeightData[i] = worldHeightData[i] / HEIGHTMAP_SCALE_Y;

        // Resize the runtime textures to the assembled world
        if (worldFramebuffer->GetWidth() != textureWidth || worldFramebuffer->GetHeight() != textureHeight) {
            worldFramebuffer->Resize(textureWidth, textureHeight);

            const int roadScale = 4;
            roadFramebuffer->Resize(textureWidth * roadScale, textureHeight * roadScale);
        }

        GLuint heightMapHandle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");
        GLuint terrainControlHandle = worldFramebuffer->GetColorAttachmentHandleByName("TerrainControl");
        glTextureSubImage2D(heightMapHandle, 0, 0, 0, textureWidth, textureHeight, GL_RED, GL_FLOAT, gpuHeightData.data());
        glTextureSubImage2D(terrainControlHandle, 0, 0, 0, textureWidth, textureHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, worldTerrainControlData.data());
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        g_terrainDisplacementDirty = true;
    }

    void UploadTerrainControlData(const MapData* mapData, int32_t minimumX, int32_t minimumZ, int32_t maximumX, int32_t maximumZ) {
        if (!mapData) return;
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        if (!worldFramebuffer) return;

        const int32_t sourceWidth = mapData->GetTextureWidth();
        const int32_t sourceHeight = mapData->GetTextureHeight();
        if (sourceWidth <= 0 || sourceHeight <= 0) return;
        minimumX = std::clamp(minimumX, 0, sourceWidth - 1);
        minimumZ = std::clamp(minimumZ, 0, sourceHeight - 1);
        maximumX = std::clamp(maximumX, 0, sourceWidth - 1);
        maximumZ = std::clamp(maximumZ, 0, sourceHeight - 1);
        if (minimumX > maximumX || minimumZ > maximumZ) return;

        const std::vector<uint32_t>& sourceData = mapData->GetTerrainControlData();
        if (sourceData.size() != static_cast<size_t>(sourceWidth) * sourceHeight) return;

        const GLuint textureHandle = worldFramebuffer->GetColorAttachmentHandleByName("TerrainControl");
        if (!textureHandle) return;

        GLint previousRowLength = 0;
        GLint previousAlignment = 0;
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &previousRowLength);
        glGetIntegerv(GL_UNPACK_ALIGNMENT, &previousAlignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, sourceWidth);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        const int32_t uploadWidth = maximumX - minimumX + 1;
        const int32_t uploadHeight = maximumZ - minimumZ + 1;
        const uint32_t* uploadData = sourceData.data() + static_cast<size_t>(minimumZ) * sourceWidth + minimumX;
        for (const Map& map : World::GetMaps()) {
            if (MapManager::GetMapDataByIndex(map.m_mapIndex) != mapData) continue;
            const int32_t destinationX = map.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_PIXEL_SIZE + minimumX;
            const int32_t destinationZ = map.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE + minimumZ;
            if (destinationX < 0 || destinationZ < 0 || destinationX + uploadWidth > worldFramebuffer->GetWidth() || destinationZ + uploadHeight > worldFramebuffer->GetHeight()) continue;
            glTextureSubImage2D(textureHandle, 0, destinationX, destinationZ, uploadWidth, uploadHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, uploadData);

            const int32_t mapEndX = map.spawnOffsetChunkX * HEIGHT_MAP_CHUNK_PIXEL_SIZE + sourceWidth;
            const int32_t mapEndZ = map.spawnOffsetChunkZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE + sourceHeight;
            if (maximumX == sourceWidth - 1 && mapEndX == worldFramebuffer->GetWidth() - 1) {
                const uint32_t* edgeData = sourceData.data() + static_cast<size_t>(minimumZ) * sourceWidth + maximumX;
                glTextureSubImage2D(textureHandle, 0, mapEndX, destinationZ, 1, uploadHeight, GL_RED_INTEGER, GL_UNSIGNED_INT, edgeData);
            }
            if (maximumZ == sourceHeight - 1 && mapEndZ == worldFramebuffer->GetHeight() - 1) {
                const uint32_t* edgeData = sourceData.data() + static_cast<size_t>(maximumZ) * sourceWidth + minimumX;
                glTextureSubImage2D(textureHandle, 0, destinationX, mapEndZ, uploadWidth, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, edgeData);
            }
            if (maximumX == sourceWidth - 1 && maximumZ == sourceHeight - 1 && mapEndX == worldFramebuffer->GetWidth() - 1 && mapEndZ == worldFramebuffer->GetHeight() - 1) {
                const uint32_t* cornerData = sourceData.data() + static_cast<size_t>(maximumZ) * sourceWidth + maximumX;
                glTextureSubImage2D(textureHandle, 0, mapEndX, mapEndZ, 1, 1, GL_RED_INTEGER, GL_UNSIGNED_INT, cornerData);
            }
        }

        glPixelStorei(GL_UNPACK_ROW_LENGTH, previousRowLength);
        glPixelStorei(GL_UNPACK_ALIGNMENT, previousAlignment);
        glMemoryBarrier(GL_TEXTURE_UPDATE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        g_terrainDisplacementDirty = true;
    }

    void UpdateTerrainDisplacementBuffer() {
#if TERRAIN_DISPLACEMENT_ENABLED
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("TerrainDisplacementBuffer");
        Hell::TextureArray* displacementBuffer = Hell::ResourceManager::GetTextureArrayPtr("TerrainDisplacement");
        if (!worldFramebuffer || !shader || !displacementBuffer) return;
        if (worldFramebuffer->GetWidth() <= 1 || worldFramebuffer->GetHeight() <= 1) return;

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        if (viewportData.size() < TERRAIN_DISPLACEMENT_LAYER_COUNT) return;

        const float maximumDensity = float(1 << (TERRAIN_DISPLACEMENT_TESSELLATION_LEVEL - 1));
        std::array<glm::vec2, TERRAIN_DISPLACEMENT_LAYER_COUNT> snappedTargets;
        for (int i = 0; i < TERRAIN_DISPLACEMENT_LAYER_COUNT; i++) {
            glm::vec2 controlPosition = glm::vec2(viewportData[i].viewPos.x, viewportData[i].viewPos.z) / HEIGHTMAP_SCALE_XZ;
            snappedTargets[i] = glm::round(controlPosition * maximumDensity) / maximumDensity;
            g_terrainDisplacementDirty |= snappedTargets[i] != g_previousDisplacementTargets[i];
        }

        float textureScaling = 1.0f;
        if (EditorSession::IsHeightMapEditorActive()) textureScaling = 0.1f;
        g_terrainDisplacementDirty |= textureScaling != g_previousDisplacementTextureScaling;
        if (!g_terrainDisplacementDirty) return;

        OpenGL::BindShader("TerrainDisplacementBuffer");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        SetTerrainMaterialUniforms();
        OpenGL::SetUniformFloat("u_textureScaling", textureScaling);

        glBindImageTexture(0, displacementBuffer->GetHandle(), 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
        OpenGL::BindTextureUnit(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"));
        OpenGL::BindTextureUnit(1, worldFramebuffer->GetColorAttachmentHandleByName("TerrainControl"));

        const uint32_t groupCountX = (displacementBuffer->GetWidth() + 7) / 8;
        const uint32_t groupCountY = (displacementBuffer->GetHeight() + 7) / 8;
        OpenGL::DispatchCompute(groupCountX, groupCountY, TERRAIN_DISPLACEMENT_LAYER_COUNT);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

        g_previousDisplacementTargets = snappedTargets;
        g_previousDisplacementTextureScaling = textureScaling;
        g_terrainDisplacementDirty = false;
#endif
    }

    void GenerateHeightMapVertexData() {
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        if (chunks.empty()) return;

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapVertexGeneration");
        if (!worldFramebuffer) return;
        if (!shader) return;

        Hell::MeshBuffer& heightMapMeshBuffer = Hell::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");

        OpenGL::BindShader("HeightMapVertexGeneration");
        glBindImageTexture(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"), 0, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_HEIGHTMAP_VERTEX_OUTPUT, glHeightMapMeshBuffer.GetVBO());

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);

        for (HeightMapChunk& chunk : chunks) {
            Mesh* mesh = heightMapMeshBuffer.GetMeshById(chunk.meshId);
            if (!mesh) continue;

            OpenGL::SetUniformInt("u_baseVertex", mesh->baseVertex);
            OpenGL::SetUniformInt("u_chunkX", chunk.coord.x);
            OpenGL::SetUniformInt("u_chunkZ", chunk.coord.z);
            int chunkSize = HEIGHT_MAP_SIZE / 8;
            int chunkWidth = chunkSize + 1;
            int chunkDepth = chunkSize + 1;
            int groupSizeX = (chunkWidth + 16 - 1) / 16;
            int groupSizeY = (chunkDepth + 16 - 1) / 16;
            OpenGL::DispatchCompute(groupSizeX, groupSizeY, 1);
        }

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    }

    void GeneratePhysXTextures() {
        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");

        GLuint handle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");
        GLint level = 0;
        GLint zOffset = 0;
        GLsizei width = 33;
        GLsizei height = 33;
        GLsizei depth = 1;
        GLenum format = GL_RED;
        GLenum type = GL_FLOAT;
        GLsizei numPixels = width * height * depth;
        GLsizei dataSize = numPixels * sizeof(float);
        std::vector<float> pixels(numPixels);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

        struct ChunkReadBackData {
            float vertices[VERTICES_PER_CHUNK];
        };

        int chunkCount = LegacyWorld::GetChunkCount();
        std::vector<ChunkReadBackData> chunkReadBackDataSet(chunkCount);

        // Readback height chunk data from gpu
        std::vector<HeightMapChunk>& chunks = LegacyWorld::GetHeightMapChunks();
        for (int i = 0; i < chunkCount; i++) {
            HeightMapChunk& chunk = chunks[i];
            GLint xOffset = chunk.coord.x * 32;
            GLint yOffset = chunk.coord.z * 32;

            if (xOffset + width > worldFramebuffer->GetWidth() ||
                yOffset + height > worldFramebuffer->GetHeight()) {
                std::cout << "YOU HAVE PROBLEMS: \n";
                std::cout << " - worldFramebuffer->GetWidth(): " << worldFramebuffer->GetWidth() << "\n";
                std::cout << " - worldFramebuffer->GetHeight(): " << worldFramebuffer->GetHeight() << "\n";
                std::cout << " - xOffset: " << xOffset << "\n";
                std::cout << " - yOffset: " << yOffset << "\n";
                std::cout << " - width: " << width << "\n";
                std::cout << " - height: " << height << "\n";
                std::cout << " - chunkCount: " << chunkCount << "\n";
            }

            glGetTextureSubImage(handle, level, xOffset, yOffset, zOffset, width, height, depth, GL_RED, GL_FLOAT, dataSize, chunkReadBackDataSet[i].vertices);
        }

        Hell::Physics::MarkAllHeightFieldsForRemoval();

        // For each chunk determine the AABB
        for (int i = 0; i < chunkCount; i++) {
            HeightMapChunk& chunk = chunks[i];
            glm::vec3 aabbMin(std::numeric_limits<float>::max());
            glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

            for (size_t j = 0; j < VERTICES_PER_CHUNK; j++) {
                float x = ((j % 33) + (chunk.coord.x * 32)) * HEIGHTMAP_SCALE_XZ;
                float y = chunkReadBackDataSet[i].vertices[j] * HEIGHTMAP_SCALE_Y;
                float z = ((j / 33) + (chunk.coord.z * 32)) * HEIGHTMAP_SCALE_XZ;

                glm::vec3 position(x, y, z);
                aabbMin = glm::min(aabbMin, position);
                aabbMax = glm::max(aabbMax, position);
            }
            const glm::vec3 displacementMargin(TERRAIN_DISPLACEMENT_MAX_OFFSET);
            chunk.aabbMin = aabbMin - displacementMargin;
            chunk.aabbMax = aabbMax + displacementMargin;

            Hell::vecXZ worldSpaceOffest = Hell::vecXZ(chunk.coord.x * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE, chunk.coord.z * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
            Hell::Physics::CreateHeightField(worldSpaceOffest, chunkReadBackDataSet[i].vertices, HEIGHTMAP_SCALE_Y, HEIGHTMAP_SCALE_XZ, HEIGHTMAP_SCALE_XZ);
       }
    }

    void DrawHeightMap() {
        ProfilerOpenGLZoneFunction();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("HeightMapColor");

        if (!gBuffer) return;
        if (!roadFramebuffer) return;
        if (!shader) return;

        OpenGLMeshBuffer& glHeightMapMeshBuffer = OpenGL::ResourceManager::GetMeshBuffer("HeightMapGeometry");
        const DrawCommandsSet& drawInfoSet = Unloved::RenderDataManager::GetDrawInfoSet();

        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("HeightMapColor");
        OpenGL::BindSSBO(SSBO_IDX_SAMPLERS, "Samplers");
        OpenGL::BindSSBO(SSBO_IDX_MATERIALS, "Materials");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindSSBO(SSBO_IDX_SCENE_RENDER_ITEMS, "SceneRenderItems");
        OpenGL::BindSSBO(SSBO_IDX_DRAW_RENDER_ITEM_INDICES, "DrawRenderItemIndices");
        OpenGL::SetUniformFloat("u_textureScaling", 1);

        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        int32_t fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Ground_MudVeg");
        if (fallbackMaterialIndex == -1) fallbackMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("CheckerBoard");
        if (fallbackMaterialIndex == -1) fallbackMaterialIndex = 0;
        int32_t grassMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Grass");
        int32_t dirtRoadMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("DirtRoad");
        int32_t rockFaceMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("RockFace");
        int32_t sandMaterialIndex = Hell::ResourceManager::GetMaterialIndexByName("Sand");
        OpenGL::SetUniformInt("u_terrainMaterial0", grassMaterialIndex == -1 ? fallbackMaterialIndex : grassMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial1", dirtRoadMaterialIndex == -1 ? fallbackMaterialIndex : dirtRoadMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial2", rockFaceMaterialIndex == -1 ? fallbackMaterialIndex : rockFaceMaterialIndex);
        OpenGL::SetUniformInt("u_terrainMaterial3", sandMaterialIndex == -1 ? fallbackMaterialIndex : sandMaterialIndex);

        if (EditorSession::IsHeightMapEditorActive()) {
            OpenGL::SetUniformFloat("u_textureScaling", 0.1);
        }

        glBindTextureUnit(6, roadFramebuffer->GetColorAttachmentHandleByName("RoadMask"));
        glBindTextureUnit(7, OpenGL::ResourceManager::GetFrameBuffer("World").GetColorAttachmentHandleByName("TerrainControl"));

        glBindVertexArray(glHeightMapMeshBuffer.GetVAO());

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);
            OpenGL::SetUniformInt("u_viewportIndex", i);
            MultiDrawIndirect(drawInfoSet.heightMap[i]);
        }
        glBindVertexArray(0);
    }

}
