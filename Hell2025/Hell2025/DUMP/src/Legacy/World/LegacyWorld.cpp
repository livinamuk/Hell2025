#include "LegacyWorld.h"

#include "Hell/Audio.h"
#include "Hell/Common/Enum.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"
#include "Hell/Physics/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/ObjectId.h"
#include "Unloved/Systems/Mirrors/MirrorManager.h"
#include "Unloved/Systems/Bullets/BulletSystem.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Systems/House/HouseBuilder.h"
#include "Unloved/Systems/HeightMap/HeightMap.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Common/Types.h"

using namespace Hell;

namespace {
    std::vector<Vertex> CreateHeightMapChunkVertices(int chunkX, int chunkZ) {
        std::vector<Vertex> vertices(VERTICES_PER_CHUNK);

        for (int z = 0; z <= HEIGHT_MAP_CHUNK_PIXEL_SIZE; z++) {
            for (int x = 0; x <= HEIGHT_MAP_CHUNK_PIXEL_SIZE; x++) {
                int vertexIndex = z * (HEIGHT_MAP_CHUNK_PIXEL_SIZE + 1) + x;
                Vertex& vertex = vertices[vertexIndex];
                vertex.position = glm::vec3(chunkX * HEIGHT_MAP_CHUNK_PIXEL_SIZE + x, 0.0f, chunkZ * HEIGHT_MAP_CHUNK_PIXEL_SIZE + z);
                vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f);
                vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
            }
        }

        return vertices;
    }

    std::vector<uint32_t> CreateHeightMapChunkIndices() {
        std::vector<uint32_t> indices;
        indices.reserve(INDICES_PER_CHUNK);

        for (int z = 0; z < HEIGHT_MAP_CHUNK_PIXEL_SIZE; z++) {
            for (int x = 0; x < HEIGHT_MAP_CHUNK_PIXEL_SIZE; x++) {
                uint32_t v00 = z * (HEIGHT_MAP_CHUNK_PIXEL_SIZE + 1) + x;
                uint32_t v10 = v00 + 1;
                uint32_t v01 = (z + 1) * (HEIGHT_MAP_CHUNK_PIXEL_SIZE + 1) + x;
                uint32_t v11 = v01 + 1;

                indices.push_back(v00);
                indices.push_back(v01);
                indices.push_back(v10);
                indices.push_back(v10);
                indices.push_back(v01);
                indices.push_back(v11);
            }
        }

        return indices;
    }
}

namespace Unloved::LegacyWorld {

    std::vector<HeightMapChunk> g_heightMapChunks;
    std::vector<Road> g_roads;

    std::vector<GPULight> g_gpuLightsLowRes;
    std::vector<GPULight> g_gpuLightsMidRes;
    std::vector<GPULight> g_gpuLightsHighRes;

    std::map<ivecXZ, int> g_validChunks;

    std::string g_mapName = "";
    uint32_t g_worldMapChunkCountX = 0;
    uint32_t g_worldMapChunkCountZ = 0;

    void LoadMapsHeightMapData(const std::vector<MapCreateInfo>& mapCreateInfoSet) {
        World::GetMaps().clear();
        World::RefreshOceanPhysics();
        HeightMap::Clear();
        g_worldMapChunkCountX = 0;
        g_worldMapChunkCountZ = 0;

        // Load height map data from all maps
        for (const MapCreateInfo& mapCreateInfo : mapCreateInfoSet) {
            int32_t mapIndex = MapManager::GetMapDataIndexByName(mapCreateInfo.mapName);
            MapData* mapData = MapManager::GetMapDataByName(mapCreateInfo.mapName);
            if (!mapData) {
                Logging::Error() << "LegacyWorld::LoadMapsHeightMapData() failed coz '" << mapCreateInfo.mapName << "' was not found";
                return;
            }

            Map& map = World::GetMaps().emplace_back();
            map.m_mapIndex = mapIndex;
            map.spawnOffsetChunkX = mapCreateInfo.spawnOffsetChunkX;
            map.spawnOffsetChunkZ = mapCreateInfo.spawnOffsetChunkZ;

            uint32_t reachX = map.spawnOffsetChunkX + mapData->GetChunkCountX();
            uint32_t reachZ = map.spawnOffsetChunkZ + mapData->GetChunkCountZ();

            g_worldMapChunkCountX = std::max(g_worldMapChunkCountX, reachX);
            g_worldMapChunkCountZ = std::max(g_worldMapChunkCountZ, reachZ);
        }

        World::RefreshOceanPhysics();
        HeightMap::BuildWorldHeightData(g_worldMapChunkCountX, g_worldMapChunkCountZ);

        // Create heightmap chunks
        g_heightMapChunks.clear();
        g_validChunks.clear();

        MeshBuffer& heightMapMeshBuffer = ResourceManager::GetMeshBuffer("HeightMapGeometry");
        heightMapMeshBuffer.Reset();

        size_t chunkCount = static_cast<size_t>(g_worldMapChunkCountX) * static_cast<size_t>(g_worldMapChunkCountZ);
        if (chunkCount > 0) {
            heightMapMeshBuffer.PreAllocate(chunkCount * VERTICES_PER_CHUNK, chunkCount * INDICES_PER_CHUNK);
        }

        const std::vector<uint32_t> chunkIndices = CreateHeightMapChunkIndices();

        for (int x = 0; x < g_worldMapChunkCountX; x++) {
            for (int z = 0; z < g_worldMapChunkCountZ; z++) {
                HeightMapChunk& chunk = g_heightMapChunks.emplace_back();
                chunk.coord.x = x;
                chunk.coord.z = z;
                chunk.meshId = heightMapMeshBuffer.AddMesh(CreateHeightMapChunkVertices(x, z), chunkIndices, "HeightMapChunk_" + std::to_string(x) + "_" + std::to_string(z));

                g_validChunks[chunk.coord] = g_heightMapChunks.size() - 1;
            }
        }

        Renderer::RecalculateAllHeightMapData(true);
    }

    void BeginFrame() {
        for (GameObject& gameObject : Unloved::World::GetGameObjects()) {
            gameObject.BeginFrame();
        }
        //for (Tree& tree : g_trees) {
        //    tree.BeginFrame();
        //}
    }

    void EndFrame() {
        // Nothing as of yet
    }


    void CreateGameObject() {
        Unloved::World::AddGameObject(GameObjectCreateInfo());
    }


    void ResetWorld() {
        std::cout << "Reset world()\n";

        // Clear height map data
        g_heightMapChunks.clear();
        g_validChunks.clear();
        World::GetMaps().clear();
        World::RefreshOceanPhysics();
        HeightMap::Clear();

        MeshBuffer& proceduralMeshBuffer = ResourceManager::GetMeshBuffer("Procedural");
        proceduralMeshBuffer.Reset();

        MeshBuffer& heightMapMeshBuffer = ResourceManager::GetMeshBuffer("HeightMapGeometry");
        heightMapMeshBuffer.Reset();

        ClearAllObjects();
    }

    void ClearAllObjects() {
        Unloved::HouseBuilder::RemoveAllWeatherBoards();
        Unloved::MirrorManager::CleanUp();
        Unloved::BulletSystem::CleanUp();
        Unloved::BloodSystemOLD::CleanUp();
        Unloved::World::CleanUpAll();
        Hell::Physics::FlushPendingRemovals();
    }

    DDGIVolume& GetTestDDGIVolume() {
        static DDGIVolume invalid;

        if (Unloved::World::GetDDGIVolumes().size() > 1) {
            Logging::Fatal() << "LegacyWorld::GetTestDDGIVolume() fucked up, you have more than one LightVolume and ALL your code assumes you only have one\n";
            return invalid;
        }
        if (Unloved::World::GetDDGIVolumes().size() == 1) {
            for (DDGIVolume& ddgiVolume : Unloved::World::GetDDGIVolumes()) {
                return ddgiVolume;
            }
        }
        else {
            Logging::Fatal() << "LegacyWorld::GetTestDDGIVolume() fucked up, you have zero LightVolumes and ALL your code assumes you only have one\n";
            return invalid;
        }

        return invalid;
    }

    void AddMap(const std::string& mapName, int32_t spawnOffsetChunkX, int32_t spawnOffsetChunkZ) {

    }

    std::vector<HeightMapChunk>& GetHeightMapChunks() {
        return g_heightMapChunks;
    }

    const uint32_t GetChunkCountX() {
        return g_worldMapChunkCountX;
    }

    const uint32_t GetChunkCountZ() {
        return g_worldMapChunkCountZ;
    }

    const uint32_t GetChunkCount() {
        return (uint32_t)g_heightMapChunks.size();
    }

    bool ChunkExists(int x, int z) {
        return g_validChunks.contains(ivecXZ(x, z));
    }

    const HeightMapChunk* GetChunk(int x, int z) {
        if (!ChunkExists(x, z)) return nullptr;

        int index = g_validChunks[ivecXZ(x, z)];
        return &g_heightMapChunks[index];
    }

    const std::string& GetCurrentMapName() {
        return g_mapName;
    }


    const float GetWorldSpaceWidth() {
        return g_worldMapChunkCountX * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
    }

    const float GetWorldSpaceDepth() {
        return g_worldMapChunkCountZ * HEIGHT_MAP_CHUNK_WORLD_SPACE_SIZE;
    }

    void PrintObjectCounts() {
        Logging::Debug()
            << "Doors:          " << Unloved::World::GetDoors().size() << "\n"
            << "Lights:         " << Unloved::World::GetLights().size() << "\n"
            << "Pickups:        " << Unloved::World::GetPickUps().size() << "\n"
            << "Pianos:         " << Unloved::World::GetPianos().size() << "\n"
            << "Picture Frames: " << Unloved::World::GetPictureFrames().size() << "\n"
            << "Planes:         " << Unloved::World::GetWorldPlanes().size() << "\n"
            //<< "Trees:          " << g_trees.size() << "\n"
            << "Walls:          " << Unloved::World::GetWalls().size() << "\n"
            << "Windows:        " << Unloved::World::GetWindows().size() << "\n"
            << "";

    }

    std::vector<Road>& GetRoads()                                       { return g_roads; }

}
