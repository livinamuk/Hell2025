#include "Unloved/Render/API/OpenGL/GL_renderer.h"

#include "Hell/Logging.h"
#include "Hell/Math/Math.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Scratch.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Objects/Exterior/GrassMesh.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"
#include "World/LegacyWorld.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>

struct GrassVertex {
    glm::vec3 position;
    glm::vec3 normal;
};

struct OpenGLGrassMesh {

private:
    unsigned int VBO = 0;
    unsigned int VAO = 0;
    unsigned int EBO = 0;

public:

    int GetVAO() {
        return VAO;
    }

    int GetVBO() {
        return VBO;
    }

    int GetEBO() {
        return EBO;
    }

    void AllocateBuffers(size_t vertexCount, size_t indexCount) {
        if (vertexCount == 0 || indexCount == 0) {
            if (VAO != 0) {
                glDeleteVertexArrays(1, &VAO);
                glDeleteBuffers(1, &VBO);
                glDeleteBuffers(1, &EBO);
                VAO = VBO = EBO = 0;
            }
            return;
        }

        if (VAO != 0) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            glDeleteBuffers(1, &EBO);
        }

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(GrassVertex), nullptr, GL_DYNAMIC_DRAW); // Allocate, no data

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(uint32_t), nullptr, GL_DYNAMIC_DRAW); // Allocate, no data

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(GrassVertex), (void*)offsetof(GrassVertex, normal));

        glBindVertexArray(0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
};

namespace OpenGL::Renderer {
    using namespace Unloved;


    OpenGLGrassMesh g_grassGeometryMesh;
    GLuint g_indirectBuffer = 0;

#define MIN_GRASS_BLADE_SPACING 0.0185185185185185f
#define MAX_GRASS_GRID_SIZE 432
#define GRASS_COVERAGE_THRESHOLD 0.5f

    constexpr uint32_t NEW_GRASS_CHUNK_SLOT_COUNT = 128;
    // Each slot is sized for the minimum supported spacing. Larger spacing
    // values use fewer entries without reallocating the persistent cache.
    constexpr uint32_t NEW_GRASS_POINTS_PER_CHUNK = MAX_GRASS_GRID_SIZE * MAX_GRASS_GRID_SIZE;
    constexpr GLuint NEW_GRASS_CHUNK_METADATA_BINDING = 7;
    constexpr GLuint NEW_GRASS_CACHED_POINTS_BINDING = 8;
    constexpr GLuint NEW_GRASS_VISIBLE_CHUNKS_BINDING = 12;
    constexpr GLuint NEW_GRASS_CHUNK_VISIBILITY_BINDING = 13;

    constexpr uint32_t GRASS_BLADE_VARIANT_COUNT = 360;

    struct NewGrassCachedPointGPU {
        uint32_t localGridIndex = 0;
        float baseHeight = 0.0f;
    };

    struct NewGrassChunkMetadataGPU {
        int32_t chunkX = 0;
        int32_t chunkZ = 0;
        uint32_t pointCount = 0;
        uint32_t unused = 0;
        glm::vec4 boundsMin = glm::vec4(0.0f);
        glm::vec4 boundsMax = glm::vec4(0.0f);
    };

    struct NewGrassDrawElementsIndirectCommand {
        uint32_t count = 0;
        uint32_t instanceCount = 0;
        uint32_t firstIndex = 0;
        int32_t baseVertex = 0;
        uint32_t baseInstance = 0;
    };

    struct NewGrassChunkSlot {
        int32_t chunkX = 0;
        int32_t chunkZ = 0;
        uint64_t lastTouchedFrame = 0;
        bool occupied = false;
    };

    static_assert(sizeof(NewGrassCachedPointGPU) == 8);
    static_assert(sizeof(NewGrassChunkMetadataGPU) == 48);
    static_assert(sizeof(NewGrassDrawElementsIndirectCommand) == 20);

    GLuint g_newGrassCachedPointBuffer = 0;
    GLuint g_newGrassChunkMetadataBuffer = 0;
    GLuint g_newGrassVisibleChunkBuffer = 0;
    GLuint g_newGrassChunkVisibilityBuffer = 0;
    std::array<NewGrassChunkSlot, NEW_GRASS_CHUNK_SLOT_COUNT> g_newGrassChunkSlots;
    std::unordered_map<uint64_t, uint32_t> g_newGrassChunkLookup;
    uint64_t g_newGrassCacheFrame = 0;
    GLuint g_newGrassCachedHeightMapHandle = 0;
    GLuint g_newGrassCachedTerrainControlHandle = 0;
    GLuint g_newGrassCachedRoadMaskHandle = 0;
    const HeightMapChunk* g_newGrassCachedChunkData = nullptr;
    size_t g_newGrassCachedChunkCount = 0;
    float g_newGrassCachedSpacing = -1.0f;
    bool g_newGrassCacheExhaustedWarningShown = false;
    int g_grassGeneratedSegmentCount = 0;
    float g_grassGeneratedCurveAmount = -1.0f;
    float g_grassGeneratedBladeHeight = -1.0f;
    float g_grassGeneratedBladeWidth = -1.0f;
    uint32_t g_grassFrontVertexCountPerBlade = 0;
    uint32_t g_grassVertexCountPerBlade = 0;
    uint32_t g_grassFrontIndexCountPerBlade = 0;
    uint32_t g_grassIndexCountPerBlade = 0;

    uint64_t GetNewGrassChunkKey(int32_t chunkX, int32_t chunkZ) {
        return (uint64_t(uint32_t(chunkX)) << 32) | uint32_t(chunkZ);
    }

    int32_t GetNewGrassChunkX(uint64_t key) {
        return int32_t(uint32_t(key >> 32));
    }

    int32_t GetNewGrassChunkZ(uint64_t key) {
        return int32_t(uint32_t(key));
    }

    uint32_t GetGrassGridSize() {
        const float spacing = Config::Grass::GetSettings().spacing;
        return std::clamp(
            uint32_t(std::ceil(float(HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE) / spacing)),
            1u,
            uint32_t(MAX_GRASS_GRID_SIZE));
    }

    AABB GetNewGrassChunkBounds(const HeightMapChunk& chunk) {
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        const float horizontalMargin =
            settings.curveAmount * 3.0f + settings.spacing * 0.5f + settings.bladeWidth + 0.02f;
        const float verticalMargin = settings.bladeHeight * (4.0f / 3.0f) + 0.05f;
        return AABB(
            chunk.aabbMin - glm::vec3(horizontalMargin, 0.05f, horizontalMargin),
            chunk.aabbMax + glm::vec3(horizontalMargin, verticalMargin, horizontalMargin));
    }

    void GenerateBladePositions(float xOffset, float zOffset, int viewportIndex);
    void NewGrassPass();
    void NewGrassCacheGeneration(
        const std::unordered_set<uint64_t>& requiredChunkKeys,
        const std::vector<glm::vec3>& activeViewPositions,
        GLuint heightMapHandle,
        GLuint terrainControlHandle,
        GLuint roadMaskHandle);
    void NewGrassChunkCulling(
        int viewportIndex,
        uint32_t visibleChunkCount,
        OpenGLFrameBuffer* gBuffer,
        OpenGLFrameBuffer* occlusionHiZFbo);
    void NewGrassCulling(int viewportIndex, uint32_t visibleChunkCount);
    void NewGrassDraw(
        Unloved::Viewport* viewport,
        OpenGLFrameBuffer* gBuffer,
        const ViewportData& viewportData);
    void OldGrassPass();
    void RenderGrass(int viewportIndex);

    void InitGrass() {
        int bladesPerHeightMapAxis = HEIGHT_MAP_SIZE * HEIGHTMAP_SCALE_XZ / MIN_GRASS_BLADE_SPACING;
        int bufferSize = bladesPerHeightMapAxis * bladesPerHeightMapAxis * sizeof(glm::vec4);
        //std::cout << "Grass SSBO allocated: " << Hell::String::FormatBytesMB(bufferSize) << "\n";
        OpenGL::ResourceManager::CreateSSBO("BladePositions").Create(bufferSize, GL_DYNAMIC_STORAGE_BIT);
        CreateGrassGeometry();

        // Create indirect buffer
        if (g_indirectBuffer == 0) {
            glGenBuffers(1, &g_indirectBuffer);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectBuffer);
            glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(DrawIndexedIndirectCommand), NULL, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        Logging::Init() << "Initialized grass geometry";
    }

    void CreateGrassGeometry() {
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        const uint32_t segmentCount = uint32_t(settings.segmentCount);
        g_grassFrontVertexCountPerBlade = (segmentCount + 1) * 2;
        g_grassVertexCountPerBlade = g_grassFrontVertexCountPerBlade * 2;
        g_grassFrontIndexCountPerBlade = segmentCount * 6;
        g_grassIndexCountPerBlade = g_grassFrontIndexCountPerBlade * 2;

        g_grassGeometryMesh.AllocateBuffers(
            GRASS_BLADE_VARIANT_COUNT * g_grassVertexCountPerBlade,
            GRASS_BLADE_VARIANT_COUNT * g_grassIndexCountPerBlade);

        OpenGLShader* geometryShader = OpenGL::ResourceManager::GetShaderPtr("GrassGeometryGeneration");
        OpenGL::BindShader("GrassGeometryGeneration");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_GEOMETRY_OUTPUT_VERTICES, g_grassGeometryMesh.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_GEOMETRY_OUTPUT_INDICES, g_grassGeometryMesh.GetEBO());
        OpenGL::SetUniformInt("u_segmentCount", settings.segmentCount);
        OpenGL::SetUniformFloat("u_curveAmount", settings.curveAmount);
        OpenGL::SetUniformFloat("u_bladeHeight", settings.bladeHeight);
        OpenGL::SetUniformFloat("u_bladeWidth", settings.bladeWidth);
        OpenGL::DispatchCompute(GRASS_BLADE_VARIANT_COUNT, 1, 1);
        glMemoryBarrier(
            GL_SHADER_STORAGE_BARRIER_BIT |
            GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT |
            GL_ELEMENT_ARRAY_BARRIER_BIT);

        g_grassGeneratedSegmentCount = settings.segmentCount;
        g_grassGeneratedCurveAmount = settings.curveAmount;
        g_grassGeneratedBladeHeight = settings.bladeHeight;
        g_grassGeneratedBladeWidth = settings.bladeWidth;

        // Geometry edits change the conservative grass volume without changing
        // cached root positions. Refresh only the bounds fields so point
        // counts and the persistent cache remain intact.
        if (g_newGrassChunkMetadataBuffer != 0) {
            for (uint32_t slotIndex = 0; slotIndex < NEW_GRASS_CHUNK_SLOT_COUNT; slotIndex++) {
                const NewGrassChunkSlot& slot = g_newGrassChunkSlots[slotIndex];
                if (!slot.occupied) continue;

                const HeightMapChunk* chunk = LegacyWorld::GetChunk(slot.chunkX, slot.chunkZ);
                if (!chunk) continue;

                const AABB bounds = GetNewGrassChunkBounds(*chunk);
                const std::array<glm::vec4, 2> gpuBounds {
                    glm::vec4(bounds.GetBoundsMin(), 0.0f),
                    glm::vec4(bounds.GetBoundsMax(), 0.0f)
                };
                const GLintptr boundsOffset = GLintptr(
                    slotIndex * sizeof(NewGrassChunkMetadataGPU) +
                    offsetof(NewGrassChunkMetadataGPU, boundsMin));
                glNamedBufferSubData(
                    g_newGrassChunkMetadataBuffer,
                    boundsOffset,
                    sizeof(gpuBounds),
                    gpuBounds.data());
            }
            glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        }
    }

    void GrassPass() {
        ProfilerOpenGLZoneFunction();

        const Config::Grass::Settings& grassSettings = Config::Grass::GetSettings();
        if (grassSettings.segmentCount != g_grassGeneratedSegmentCount ||
            grassSettings.curveAmount != g_grassGeneratedCurveAmount ||
            grassSettings.bladeHeight != g_grassGeneratedBladeHeight ||
            grassSettings.bladeWidth != g_grassGeneratedBladeWidth) {
            CreateGrassGeometry();
        }

        if (Debug::Scratch::GetBool("New Grass", true)) {
            NewGrassPass();
        }
        else {
            OldGrassPass();
        }
    }

    void NewGrassPass() {
        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (!rendererSettings.drawGrass) return;

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* occlusionHiZFbo = OpenGL::ResourceManager::GetFrameBufferPtr("OcclusionHiZ");
        OpenGLSSBO* visibleBladePositions = OpenGL::ResourceManager::GetSSBOPtr("BladePositions");
        if (!worldFramebuffer || !roadFramebuffer || !gBuffer || !visibleBladePositions) return;

        const GLuint heightMapHandle = worldFramebuffer->GetColorAttachmentHandleByName("HeightMap");
        const GLuint terrainControlHandle = worldFramebuffer->GetColorAttachmentHandleByName("TerrainControl");
        const GLuint roadMaskHandle = roadFramebuffer->GetColorAttachmentHandleByName("RoadMask");
        std::vector<HeightMapChunk>& heightMapChunks = LegacyWorld::GetHeightMapChunks();

        // Allocate the persistent cache on first use. A cached point is only
        // an exact local grid index and its undisplaced base height: 8 bytes.
        if (g_newGrassCachedPointBuffer == 0) {
            const size_t pointCount = size_t(NEW_GRASS_CHUNK_SLOT_COUNT) * NEW_GRASS_POINTS_PER_CHUNK;
            const GLsizeiptr pointBufferSize = GLsizeiptr(pointCount * sizeof(NewGrassCachedPointGPU));
            const GLsizeiptr metadataBufferSize = GLsizeiptr(NEW_GRASS_CHUNK_SLOT_COUNT * sizeof(NewGrassChunkMetadataGPU));
            const GLsizeiptr visibleChunkBufferSize = GLsizeiptr(NEW_GRASS_CHUNK_SLOT_COUNT * sizeof(uint32_t));

            glCreateBuffers(1, &g_newGrassCachedPointBuffer);
            glNamedBufferStorage(g_newGrassCachedPointBuffer, pointBufferSize, nullptr, 0);

            glCreateBuffers(1, &g_newGrassChunkMetadataBuffer);
            glNamedBufferStorage(g_newGrassChunkMetadataBuffer, metadataBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

            glCreateBuffers(1, &g_newGrassVisibleChunkBuffer);
            glNamedBufferStorage(g_newGrassVisibleChunkBuffer, visibleChunkBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

            glCreateBuffers(1, &g_newGrassChunkVisibilityBuffer);
            glNamedBufferStorage(g_newGrassChunkVisibilityBuffer, visibleChunkBufferSize, nullptr, GL_DYNAMIC_STORAGE_BIT);

            Logging::Init() << "Allocated 128-slot persistent grass cache (182.25 MiB)\n";
        }

        // A recreated world texture or chunk array represents different
        // source data, so its old cache keys must not be reused.
        const bool sourceDataChanged =
            heightMapHandle != g_newGrassCachedHeightMapHandle ||
            terrainControlHandle != g_newGrassCachedTerrainControlHandle ||
            roadMaskHandle != g_newGrassCachedRoadMaskHandle ||
            heightMapChunks.data() != g_newGrassCachedChunkData ||
            heightMapChunks.size() != g_newGrassCachedChunkCount ||
            Config::Grass::GetSettings().spacing != g_newGrassCachedSpacing ||
            EditorSession::IsHeightMapEditorActive();
        if (sourceDataChanged) {
            g_newGrassChunkLookup.clear();
            for (NewGrassChunkSlot& slot : g_newGrassChunkSlots) slot = {};
            g_newGrassCachedHeightMapHandle = heightMapHandle;
            g_newGrassCachedTerrainControlHandle = terrainControlHandle;
            g_newGrassCachedRoadMaskHandle = roadMaskHandle;
            g_newGrassCachedChunkData = heightMapChunks.data();
            g_newGrassCachedChunkCount = heightMapChunks.size();
            g_newGrassCachedSpacing = Config::Grass::GetSettings().spacing;
            g_newGrassCacheExhaustedWarningShown = false;
        }

        g_newGrassCacheFrame++;

        // First determine the complete union of chunks needed by all active
        // viewports. Knowing the union up front prevents eviction of a chunk
        // that another viewport still needs during this frame.
        constexpr int VIEWPORT_COUNT = 4;
        constexpr std::array<const char*, VIEWPORT_COUNT> VIEWPORT_ZONE_NAMES {
            "NewGrassViewport0",
            "NewGrassViewport1",
            "NewGrassViewport2",
            "NewGrassViewport3"
        };
        std::array<std::vector<uint64_t>, VIEWPORT_COUNT> visibleChunkKeys;
        std::unordered_set<uint64_t> requiredChunkKeys;
        std::vector<glm::vec3> activeViewPositions;
        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
        const float maxCullDistance = Config::Grass::GetSettings().maxCullDistance;

        for (int viewportIndex = 0; viewportIndex < VIEWPORT_COUNT; viewportIndex++) {
            if (viewportIndex >= int(viewportData.size())) continue;

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;

            const glm::vec3 viewPosition = glm::vec3(viewportData[viewportIndex].inverseView[3]);
            const glm::vec2 viewXZ(viewPosition.x, viewPosition.z);
            Unloved::Frustum& frustum = viewport->GetFrustum();
            activeViewPositions.push_back(viewPosition);

            for (HeightMapChunk& chunk : heightMapChunks) {
                const AABB chunkAABB = GetNewGrassChunkBounds(chunk);
                const glm::vec3 boundsMin = chunkAABB.GetBoundsMin();
                const glm::vec3 boundsMax = chunkAABB.GetBoundsMax();
                const glm::vec2 closestChunkPoint = glm::clamp(
                    viewXZ,
                    glm::vec2(boundsMin.x, boundsMin.z),
                    glm::vec2(boundsMax.x, boundsMax.z));
                if (glm::length(closestChunkPoint - viewXZ) >= maxCullDistance) continue;
                if (!frustum.IntersectsAABBFast(chunkAABB)) continue;

                const uint64_t chunkKey = GetNewGrassChunkKey(chunk.coord.x, chunk.coord.z);
                visibleChunkKeys[viewportIndex].push_back(chunkKey);
                requiredChunkKeys.insert(chunkKey);
            }
        }

        // Existing required chunks are simply touched. Their point data is
        // not regenerated or copied.
        for (uint64_t chunkKey : requiredChunkKeys) {
            const auto resident = g_newGrassChunkLookup.find(chunkKey);
            if (resident != g_newGrassChunkLookup.end()) {
                g_newGrassChunkSlots[resident->second].lastTouchedFrame = g_newGrassCacheFrame;
            }
        }

        NewGrassCacheGeneration(
            requiredChunkKeys,
            activeViewPositions,
            heightMapHandle,
            terrainControlHandle,
            roadMaskHandle);

        // The persistent cache is source data, not a draw list. Each viewport
        // compacts only its currently visible, unoccluded blades into the
        // existing transient BladePositions buffer, then draws that list.
        glBindVertexArray(g_grassGeometryMesh.GetVAO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INDIRECT_COMMAND, g_indirectBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_CHUNK_METADATA_BINDING, g_newGrassChunkMetadataBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_CACHED_POINTS_BINDING, g_newGrassCachedPointBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_BLADE_POSITIONS, visibleBladePositions->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INPUT_VERTICES, g_grassGeometryMesh.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INPUT_INDICES, g_grassGeometryMesh.GetEBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_VISIBLE_CHUNKS_BINDING, g_newGrassVisibleChunkBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_CHUNK_VISIBILITY_BINDING, g_newGrassChunkVisibilityBuffer);
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::BindTextureUnit(2, GetTextureHandleByName("Perlin"));

        if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            OpenGL::RasterizerStateManager::ForceRasterizerState("GrassPass_RE");
        }
        else {
            OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        }

        for (int viewportIndex = 0; viewportIndex < VIEWPORT_COUNT; viewportIndex++) {
            if (viewportIndex >= int(viewportData.size())) continue;

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport || !viewport->IsVisible()) continue;
            ProfilerOpenGLZone(VIEWPORT_ZONE_NAMES[viewportIndex]);

            std::vector<uint32_t> visibleChunkSlots;
            visibleChunkSlots.reserve(visibleChunkKeys[viewportIndex].size());
            for (uint64_t chunkKey : visibleChunkKeys[viewportIndex]) {
                const auto resident = g_newGrassChunkLookup.find(chunkKey);
                if (resident == g_newGrassChunkLookup.end()) continue;
                visibleChunkSlots.push_back(resident->second);
            }
            if (visibleChunkSlots.empty()) continue;

            glNamedBufferSubData(
                g_newGrassVisibleChunkBuffer,
                0,
                GLsizeiptr(visibleChunkSlots.size() * sizeof(uint32_t)),
                visibleChunkSlots.data());

            NewGrassChunkCulling(
                viewportIndex,
                uint32_t(visibleChunkSlots.size()),
                gBuffer,
                occlusionHiZFbo);
            NewGrassCulling(viewportIndex, uint32_t(visibleChunkSlots.size()));
            NewGrassDraw(viewport, gBuffer, viewportData[viewportIndex]);
        }
    }

    void NewGrassCacheGeneration(
        const std::unordered_set<uint64_t>& requiredChunkKeys,
        const std::vector<glm::vec3>& activeViewPositions,
        GLuint heightMapHandle,
        GLuint terrainControlHandle,
        GLuint roadMaskHandle) {
        ProfilerOpenGLZoneFunction();

        // Generate cache misses. Free slots are consumed first. Once the
        // fixed allocation is full, the non-visible chunk furthest from its
        // nearest active player is replaced.
        const uint32_t gridSize = GetGrassGridSize();
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        const float spacing = settings.spacing;

        OpenGL::BindShader("NewGrassPositionGeneration");
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_CHUNK_METADATA_BINDING, g_newGrassChunkMetadataBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, NEW_GRASS_CACHED_POINTS_BINDING, g_newGrassCachedPointBuffer);
        OpenGL::BindTextureUnit(0, heightMapHandle);
        OpenGL::BindTextureUnit(1, terrainControlHandle);
        OpenGL::BindTextureUnit(3, roadMaskHandle);
        OpenGL::SetUniformInt("u_gridSize", int(gridSize));
        OpenGL::SetUniformInt("u_pointsPerChunk", NEW_GRASS_POINTS_PER_CHUNK);
        OpenGL::SetUniformFloat("u_spacing", spacing);
        OpenGL::SetUniformFloat("u_grassCoverageThreshold", GRASS_COVERAGE_THRESHOLD);
        OpenGL::SetUniformFloat("u_waterHeight", World::HasOcean() ? Ocean::GetOceanOriginY() : -1000.0f);

        for (uint64_t chunkKey : requiredChunkKeys) {
            if (g_newGrassChunkLookup.contains(chunkKey)) continue;

            uint32_t selectedSlot = NEW_GRASS_CHUNK_SLOT_COUNT;
            for (uint32_t slotIndex = 0; slotIndex < NEW_GRASS_CHUNK_SLOT_COUNT; slotIndex++) {
                if (!g_newGrassChunkSlots[slotIndex].occupied) {
                    selectedSlot = slotIndex;
                    break;
                }
            }

            if (selectedSlot == NEW_GRASS_CHUNK_SLOT_COUNT) {
                float furthestDistanceSquared = -1.0f;
                uint64_t oldestFrameAtFurthestDistance = std::numeric_limits<uint64_t>::max();

                for (uint32_t slotIndex = 0; slotIndex < NEW_GRASS_CHUNK_SLOT_COUNT; slotIndex++) {
                    const NewGrassChunkSlot& slot = g_newGrassChunkSlots[slotIndex];
                    const uint64_t residentKey = GetNewGrassChunkKey(slot.chunkX, slot.chunkZ);
                    if (requiredChunkKeys.contains(residentKey)) continue;

                    const glm::vec2 chunkCenter(
                        (float(slot.chunkX) + 0.5f) * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE,
                        (float(slot.chunkZ) + 0.5f) * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
                    float nearestPlayerDistanceSquared = std::numeric_limits<float>::max();
                    for (const glm::vec3& viewPosition : activeViewPositions) {
                        const glm::vec2 delta = chunkCenter - glm::vec2(viewPosition.x, viewPosition.z);
                        nearestPlayerDistanceSquared = std::min(nearestPlayerDistanceSquared, glm::dot(delta, delta));
                    }

                    if (nearestPlayerDistanceSquared > furthestDistanceSquared ||
                        (nearestPlayerDistanceSquared == furthestDistanceSquared && slot.lastTouchedFrame < oldestFrameAtFurthestDistance)) {
                        selectedSlot = slotIndex;
                        furthestDistanceSquared = nearestPlayerDistanceSquared;
                        oldestFrameAtFurthestDistance = slot.lastTouchedFrame;
                    }
                }
            }

            if (selectedSlot == NEW_GRASS_CHUNK_SLOT_COUNT) {
                if (!g_newGrassCacheExhaustedWarningShown) {
                    Logging::Warning() << "Persistent grass cache has more than 128 simultaneously required chunks\n";
                    g_newGrassCacheExhaustedWarningShown = true;
                }
                continue;
            }

            const int32_t chunkX = GetNewGrassChunkX(chunkKey);
            const int32_t chunkZ = GetNewGrassChunkZ(chunkKey);
            const HeightMapChunk* sourceChunk = LegacyWorld::GetChunk(chunkX, chunkZ);
            if (!sourceChunk) continue;

            NewGrassChunkSlot& slot = g_newGrassChunkSlots[selectedSlot];
            if (slot.occupied) {
                g_newGrassChunkLookup.erase(GetNewGrassChunkKey(slot.chunkX, slot.chunkZ));
            }

            slot.chunkX = chunkX;
            slot.chunkZ = chunkZ;
            slot.lastTouchedFrame = g_newGrassCacheFrame;
            slot.occupied = true;
            g_newGrassChunkLookup[chunkKey] = selectedSlot;

            const AABB grassBounds = GetNewGrassChunkBounds(*sourceChunk);
            NewGrassChunkMetadataGPU metadata;
            metadata.chunkX = chunkX;
            metadata.chunkZ = chunkZ;
            metadata.boundsMin = glm::vec4(grassBounds.GetBoundsMin(), 0.0f);
            metadata.boundsMax = glm::vec4(grassBounds.GetBoundsMax(), 0.0f);
            const GLintptr metadataOffset = GLintptr(selectedSlot * sizeof(NewGrassChunkMetadataGPU));
            glNamedBufferSubData(g_newGrassChunkMetadataBuffer, metadataOffset, sizeof(metadata), &metadata);

            OpenGL::SetUniformInt("u_slotIndex", selectedSlot);
            OpenGL::SetUniformIVec2("u_chunkCoord", glm::ivec2(chunkX, chunkZ));
            OpenGL::SetUniformVec3("u_chunkOffset", glm::vec3(
                chunkX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE,
                0.0f,
                chunkZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE));
            OpenGL::DispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
        }

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);
    }

    void NewGrassChunkCulling(
        int viewportIndex,
        uint32_t visibleChunkCount,
        OpenGLFrameBuffer* gBuffer,
        OpenGLFrameBuffer* occlusionHiZFbo) {
        ProfilerOpenGLZoneFunction();

        if (visibleChunkCount == 0) return;

        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("NewGrassChunkCulling");
        const GLuint occlusionHiZHandle = occlusionHiZFbo
            ? occlusionHiZFbo->GetColorAttachmentHandleByName("MinDepth")
            : 0;

        if (!shader || !shader->GetHandle() || !gBuffer || !occlusionHiZHandle) {
            const uint32_t visible = 1;
            glClearNamedBufferData(
                g_newGrassChunkVisibilityBuffer,
                GL_R32UI,
                GL_RED_INTEGER,
                GL_UNSIGNED_INT,
                &visible);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            return;
        }

        OpenGL::BindShader("NewGrassChunkCulling");
        OpenGL::BindTextureUnit(4, occlusionHiZHandle);
        OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);
        OpenGL::SetUniformInt("u_visibleChunkCount", int(visibleChunkCount));
        OpenGL::SetUniformInt(
            "u_enableOcclusionCulling",
            Debug::Scratch::GetBool("Grass HiZ Culling", true));
        OpenGL::SetUniformIVec2("u_depthSize", glm::ivec2(gBuffer->GetWidth(), gBuffer->GetHeight()));

        constexpr uint32_t CHUNK_CULLING_LOCAL_SIZE = 64;
        OpenGL::DispatchCompute(
            (visibleChunkCount + CHUNK_CULLING_LOCAL_SIZE - 1) / CHUNK_CULLING_LOCAL_SIZE,
            1,
            1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
    }

    void NewGrassCulling(int viewportIndex, uint32_t visibleChunkCount) {
        ProfilerOpenGLZoneFunction();

        const uint32_t gridSize = GetGrassGridSize();
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        const float spacing = settings.spacing;

        NewGrassDrawElementsIndirectCommand visibleCommand;
        visibleCommand.count = g_grassFrontIndexCountPerBlade;
        glNamedBufferSubData(g_indirectBuffer, 0, sizeof(visibleCommand), &visibleCommand);

        OpenGL::BindShader("NewGrassCulling");
        OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);
        OpenGL::SetUniformInt("u_gridSize", int(gridSize));
        OpenGL::SetUniformInt("u_pointsPerChunk", NEW_GRASS_POINTS_PER_CHUNK);
        OpenGL::SetUniformFloat("u_spacing", spacing);
        OpenGL::SetUniformFloat("u_chunkWorldSize", HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE);
        OpenGL::SetUniformFloat("u_minCullDistance", settings.minCullDistance);
        OpenGL::SetUniformFloat("u_maxCullDistance", settings.maxCullDistance);
        OpenGL::SetUniformFloat("u_cullExponent", settings.cullExponent);
        OpenGL::SetUniformFloat(
            "u_bladeBoundingRadius",
            settings.curveAmount * 3.0f +
            settings.bladeHeight * (4.0f / 3.0f) +
            settings.bladeWidth + 0.02f);

        constexpr uint32_t CULLING_LOCAL_SIZE = 256;
        const uint32_t candidateCount = gridSize * gridSize;
        const uint32_t cullingGroupsPerChunk = (candidateCount + CULLING_LOCAL_SIZE - 1) / CULLING_LOCAL_SIZE;
        OpenGL::DispatchCompute(cullingGroupsPerChunk, visibleChunkCount, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_COMMAND_BARRIER_BIT);
    }

    void NewGrassDraw(
        Unloved::Viewport* viewport,
        OpenGLFrameBuffer* gBuffer,
        const ViewportData& viewportData) {
        ProfilerOpenGLZoneFunction();

        OpenGL::Renderer::SetViewport(gBuffer, viewport);
        gBuffer->Bind();
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        OpenGL::BindShader("NewGrass");
        OpenGL::SetUniformMat4("u_projectionView", viewportData.projectionViewReverseZ);
        OpenGL::SetUniformMat4("u_prevProjectionView", viewportData.prevProjectionViewReverseZ);
        OpenGL::SetUniformMat4("u_rasterProjectionView", viewportData.jitteredProjectionViewReverseZ);
        OpenGL::SetUniformVec3("u_viewPosition", viewportData.viewPos);
        OpenGL::SetUniformInt("u_verticesPerBlade", int(g_grassVertexCountPerBlade));
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        OpenGL::SetUniformVec3("u_grassColor1", settings.color1);
        OpenGL::SetUniformVec3("u_grassColor2", settings.color2);
        OpenGL::SetUniformFloat("u_grassColor1Darkness", settings.color1DarknessFactor);
        OpenGL::SetUniformFloat("u_grassColor2Darkness", settings.color2DarknessFactor);
        OpenGL::SetUniformFloat("u_noiseSquareMultiplier", settings.noiseSquareMultiplier);
        OpenGL::SetUniformFloat("u_noiseMixMultiplier", settings.noiseMixMultiplier);
        OpenGL::SetUniformFloat("u_grassRoughness", settings.roughness);
        OpenGL::SetUniformFloat("u_grassSubSurfaceFactor", settings.subSurfaceFactor);

        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectBuffer);
        glDisable(GL_CULL_FACE);
        glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr);
        glEnable(GL_CULL_FACE);
    }

    void OldGrassPass() {

        RendererSettings& rendererSettings = Unloved::Renderer::GetCurrentRendererSettings();
        if (!rendererSettings.drawGrass) return;

        //return;
        //if (Input::KeyPressed(HELL_KEY_X)) {
        //    CreateGrassGeometry();
        //}

        OpenGLFrameBuffer* worldFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("World");
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLFrameBuffer* wipBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("WIP");
        OpenGLSSBO* bladeositionsSSBO = OpenGL::ResourceManager::GetSSBOPtr("BladePositions");

        OpenGL::BlitFrameBufferDepth(gBuffer, wipBuffer);

        // Bindings
        glBindVertexArray(g_grassGeometryMesh.GetVAO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INDIRECT_COMMAND, g_indirectBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_BLADE_POSITIONS, bladeositionsSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INPUT_VERTICES, g_grassGeometryMesh.GetVBO());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_GRASS_POSITION_INPUT_INDICES, g_grassGeometryMesh.GetEBO());
        glBindTextureUnit(0, worldFramebuffer->GetColorAttachmentHandleByName("HeightMap"));
        glBindTextureUnit(1, worldFramebuffer->GetColorAttachmentHandleByName("TerrainControl"));
        glBindTextureUnit(2, GetTextureHandleByName("Perlin"));
        glBindTextureUnit(3, roadFramebuffer->GetColorAttachmentHandleByName("RoadMask"));
        glBindTextureUnit(4, wipBuffer->GetDepthAttachmentHandle());

        // GL State
        if (Unloved::Renderer::GetRendererMode() == RendererMode::RE_STYLE) {
            OpenGL::RasterizerStateManager::ForceRasterizerState("GrassPass_RE");
        }
        else {
            OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");
        }

        // Generate and draw
        for (int i = 0; i < 4; i++) {   // CHANGE TO VIEWPORT NOT PLAYER!!!!

            int viewportIndex = i;

            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
            if (!viewport->IsVisible()) continue;

            Unloved::Frustum& frustum = viewport->GetFrustum();

            const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();
            glm::vec3 viewPos = viewportData[viewportIndex].inverseView[3];

            int maxChunkDrawDistance = 3;

            Hell::ivecXZ cameraChunk(static_cast<int>(std::floor(viewPos.x / HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE)),
                                static_cast<int>(std::floor(viewPos.z / HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE)));

            int32_t xBegin = cameraChunk.x - maxChunkDrawDistance;
            int32_t xEnd = cameraChunk.x + maxChunkDrawDistance;
            int32_t zBegin = cameraChunk.z - maxChunkDrawDistance;
            int32_t zEnd = cameraChunk.z + maxChunkDrawDistance;

            if (EditorSession::IsActive()) {
                xBegin = 0;
                zBegin = 0;
                xEnd = LegacyWorld::GetChunkCountX();
                zEnd = LegacyWorld::GetChunkCountZ();
            }

            xBegin = std::max(xBegin, 0);
            zBegin = std::max(zBegin, 0);
            xEnd = std::min(xEnd, (int32_t)LegacyWorld::GetChunkCountX());
            zEnd = std::min(zEnd, (int32_t)LegacyWorld::GetChunkCountZ());

            std::vector<Hell::vecXZ> chunkOffsets;
            AABB chunkAABB;
            glm::vec3 chunkBoundsMin;
            glm::vec3 chunkBoundsMax;

            /*
            for (int x = xBegin; x < xEnd; x++) {
                for (int z = zBegin; z < zEnd; z++) {

                    // Skip chunks that don't exist
                    const HeightMapChunk* chunk = LegacyWorld::GetChunk(x, z);
                    if (!chunk) continue;

                    chunkAABB = AABB(chunk->aabbMin, chunk->aabbMax);

                    // Check if within threshold to camera
                    //float threshold = 40.0f;
                    //glm::vec3 viewPosNormalized = viewPos * glm::vec3(1.0f, 0.0f, 1.0f);
                    //glm::vec3 aabbCenterNormalized = chunkAABB.GetCenter() * glm::vec3(1.0f, 0.0f, 1.0f);

                    //float distance = Hell::Math::ManhattanDistance(viewPosNormalized, aabbCenterNormalized);
                    //if (distance >= threshold) {
                    //    continue;
                    //}

                    if (frustum.IntersectsAABBFast(chunkAABB)) {
                        float xOffset = x * CHUNK_SIZE_WORLDSPACE;
                        float zOffset = z * CHUNK_SIZE_WORLDSPACE;
                        chunkOffsets.emplace_back(vecXZ(xOffset, zOffset));
                        //DrawAABB(chunkAABB, GREEN);
                    }
                }
            }
            */

            for (HeightMapChunk& chunk : LegacyWorld::GetHeightMapChunks()) {
                chunkAABB = AABB(chunk.aabbMin, chunk.aabbMax);

                // Check if within threshold to camera
                float threshold = 30.0f;
                glm::vec3 viewPosNormalized = viewPos * glm::vec3(1.0f, 0.0f, 1.0f);
                glm::vec3 aabbCenterNormalized = chunkAABB.GetCenter() * glm::vec3(1.0f, 0.0f, 1.0f);

                float distance = Hell::Math::ManhattanDistance(viewPosNormalized, aabbCenterNormalized);
                if (distance >= threshold) {
                    //std::cout << "skipping " << chunk.coord.x << ", " << chunk.coord.z << "\n";
                    continue;
                }

                if (frustum.IntersectsAABBFast(chunkAABB)) {
                    float xOffset = chunk.coord.x * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
                    float zOffset = chunk.coord.z * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
                    chunkOffsets.emplace_back(Hell::vecXZ(xOffset, zOffset));
                    //DrawAABB(chunkAABB, GREEN);
                }
            }

            // Zero out indirect buffer
            DrawIndexedIndirectCommand initialCmd;
            initialCmd.instanceCount = 1;
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectBuffer);
            glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, sizeof(DrawIndexedIndirectCommand), &initialCmd);

            // Generate grass positions
            for (Hell::vecXZ& chunkOffset: chunkOffsets) {
                GenerateBladePositions(chunkOffset.x, chunkOffset.z, viewportIndex);
            }
            // Then render for the current viewport
            RenderGrass(viewportIndex);
        }
    }

    void GenerateBladePositions(float xOffset, float zOffset, int viewportIndex) {
        const uint32_t gridSize = GetGrassGridSize();
        const float spacing = Config::Grass::GetSettings().spacing;

        // Uniforms
        OpenGLShader* generationShader = OpenGL::ResourceManager::GetShaderPtr("GrassPositionGeneration");
        OpenGL::BindShader("GrassPositionGeneration");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::SetUniformInt("gridSize", int(gridSize));
        OpenGL::SetUniformInt("u_viewportIndex", viewportIndex);
        OpenGL::SetUniformFloat("u_spacing", spacing);
        OpenGL::SetUniformIVec2("u_chunkCoord", glm::ivec2(
            int(std::round(xOffset / HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE)),
            int(std::round(zOffset / HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE))));
        OpenGL::SetUniformFloat("u_grassCoverageThreshold", GRASS_COVERAGE_THRESHOLD);
        OpenGL::SetUniformInt("u_indicesPerBlade", int(g_grassIndexCountPerBlade));
        OpenGL::SetUniformFloat("u_bladeHeight", Config::Grass::GetSettings().bladeHeight);
        OpenGL::SetUniformVec3("offset", glm::vec3(xOffset, 0.0f, zOffset));
        OpenGL::SetUniformFloat("u_heightMapWorldSpaceSize", HEIGHT_MAP_SIZE * HEIGHTMAP_SCALE_XZ);
        OpenGL::SetUniformFloat("u_waterHeight", World::HasOcean() ? Ocean::GetOceanOriginY() : -1000.0f);

        // Dispatch compute shader
        int workGroupsX = int((gridSize + 15) / 16);
        int workGroupsY = int((gridSize + 15) / 16);
        OpenGL::DispatchCompute(workGroupsX, workGroupsY, 1);
    }

    void RenderGrass(int viewportIndex) {

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLSSBO* bladeositionsSSBO = OpenGL::ResourceManager::GetSSBOPtr("BladePositions");
        OpenGLShader* geometryShader = OpenGL::ResourceManager::GetShaderPtr("Grass");
        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");

        Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(viewportIndex);
        if (!viewport->IsVisible()) return;

        OpenGL::Renderer::SetViewport(gBuffer, viewport);

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });

        const ViewportData& currentViewportData = viewportData[viewportIndex];

        OpenGL::BindShader("Grass");
        OpenGL::SetUniformMat4("u_projectionView", currentViewportData.projectionViewReverseZ);
        OpenGL::SetUniformMat4("u_prevProjectionView", currentViewportData.prevProjectionViewReverseZ);
        OpenGL::SetUniformMat4("u_rasterProjectionView", currentViewportData.jitteredProjectionViewReverseZ);
        OpenGL::SetUniformVec3("u_viewPosition", currentViewportData.viewPos);
        OpenGL::SetUniformInt("u_segmentCount", g_grassGeneratedSegmentCount);
        OpenGL::SetUniformInt("u_verticesPerBlade", int(g_grassVertexCountPerBlade));
        OpenGL::SetUniformInt("u_indicesPerBlade", int(g_grassIndexCountPerBlade));
        const Config::Grass::Settings& settings = Config::Grass::GetSettings();
        OpenGL::SetUniformVec3("u_grassColor1", settings.color1);
        OpenGL::SetUniformVec3("u_grassColor2", settings.color2);
        OpenGL::SetUniformFloat("u_grassColor1Darkness", settings.color1DarknessFactor);
        OpenGL::SetUniformFloat("u_grassColor2Darkness", settings.color2DarknessFactor);
        OpenGL::SetUniformFloat("u_noiseSquareMultiplier", settings.noiseSquareMultiplier);
        OpenGL::SetUniformFloat("u_noiseMixMultiplier", settings.noiseMixMultiplier);
        OpenGL::SetUniformFloat("u_grassRoughness", settings.roughness);
        OpenGL::SetUniformFloat("u_grassSubSurfaceFactor", settings.subSurfaceFactor);

        glMemoryBarrier(GL_COMMAND_BARRIER_BIT);
        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, g_indirectBuffer);
        glDrawArraysIndirect(GL_TRIANGLES, 0);


        //float bladeCount = 360;
        //glm::mat4 projectionMatrix = viewportData[i].projection;
        //glm::mat4 viewMatrix = viewportData[i].view;
        //Transform transform;
        //transform.position = glm::vec3(17, -4.1, 19);
        //transform.scale = glm::vec3(5);
        //OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("SolidColor");
        //shader->Use();
        //shader->SetMat4("projection", projectionMatrix);
        //shader->SetMat4("view", viewMatrix);
        //shader->SetMat4("model", transform.to_mat4());
        //shader->SetBool("useUniformColor", false);
        //glBindVertexArray(g_grassGeometryMesh.GetVAO());
        //glEnable(GL_CULL_FACE);
        //glDrawElements(GL_TRIANGLES, bladeCount * 24, GL_UNSIGNED_INT, 0);

    }
}
