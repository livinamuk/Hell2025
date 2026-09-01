#include "HouseBuilder.h"

#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Systems/NavMesh/NavMesh.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"
#include "Unloved/World/World.h"

#include <algorithm>

namespace Unloved::HouseBuilder {

namespace {
    bool g_dirty = false;
    std::vector<Vertex> g_weatherBoardVertices;
    std::vector<uint32_t> g_weatherBoardIndices;
    std::vector<std::string> g_takenLargePictureFrameMaterials;
}

void Init() {
    g_weatherBoardVertices.clear();
    g_weatherBoardIndices.clear();

    Model* model = Hell::ResourceManager::GetModelByName("WeatherBoard");
    if (!model || model->GetMeshIndices().empty()) return;

    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    Mesh* mesh = meshBuffer.GetMeshById(model->GetMeshIndices()[0]);
    if (!mesh) return;

    g_weatherBoardVertices.assign(meshBuffer.GetVertices().begin() + mesh->baseVertex, meshBuffer.GetVertices().begin() + mesh->baseVertex + mesh->vertexCount);
    g_weatherBoardIndices.assign(meshBuffer.GetIndices().begin() + mesh->baseIndex, meshBuffer.GetIndices().begin() + mesh->baseIndex + mesh->indexCount);
    if (g_weatherBoardVertices.empty()) return;

    // Store the model UV as one whole board
    float minV = g_weatherBoardVertices.front().uv.y;
    float maxV = minV;
    for (const Vertex& vertex : g_weatherBoardVertices) {
        minV = std::min(minV, vertex.uv.y);
        maxV = std::max(maxV, vertex.uv.y);
    }
    const float uvHeight = maxV - minV;
    if (uvHeight > 0.0f) {
        for (Vertex& vertex : g_weatherBoardVertices) vertex.uv.y = (maxV - vertex.uv.y) / uvHeight;
    }
}

void RebuildIfDirty() {
    if (IsDirty()) {
        RebuildAll();
    }
}

void RebuildAll() {
    RecreateAllProceduralWallMesh();
    RecreateAllProcedularWorldPlaneMesh();
    RecreateAllWeatherBoards();
    RecreateAllHangingLightCords();
    RecreateAllWallTrims();

    CoarseWorldBVH::Rebuild();

    for (Light& light : Unloved::World::GetLights()) {
        light.RaycastWorldBounds();
        light.ForceDirty();
    }

    NavMeshManager::MarkStaticDirty();
    g_dirty = false;
}

void RecreateAllWallTrims() {
    Hell::SlotMap<TrimSet>& trimSets = Unloved::World::GetTrimSets();
    trimSets.clear();

    for (Wall& wall : Unloved::World::GetWalls()) {
        if (wall.GetWallType() == WallType::WEATHER_BOARDS) continue;

        const WallCreateInfo& createInfo = wall.GetCreateInfo();

        // Ceiling trim
        TrimSetCreateInfo createInfoCeiling;
        for (const SequencePoint& sequencePoint : createInfo.sequencePoints) {
            glm::vec3 trimPoint = sequencePoint.position + glm::vec3(0.0f, sequencePoint.customFloat, 0.0f);
            trimPoint.y -= 0.01f; // safety threshold
            createInfoCeiling.points.push_back(trimPoint);
            createInfoCeiling.type = TrimSetType::CEILING_FANCY;
            createInfoCeiling.trimScale = 0.95f;
        }
        if (createInfo.useReversePointOrder) std::reverse(createInfoCeiling.points.begin(), createInfoCeiling.points.end());

        Unloved::World::AddTrimSet(createInfoCeiling, SpawnOffset());
    }
}

void RecreateAllProceduralWallMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    // Door/window clipping volumes are owned by those objects, so CSG can use them directly

    for (Wall& wall : Unloved::World::GetWalls()) {

        // Update CSG and trims
        wall.UpdateSegmentsTrimsAndVertexData();

        for (WallSegment& wallSegment : wall.GetWallSegments()) {
            // Remove old mesh
            meshBuffer.RemoveMesh(wallSegment.GetMeshId());

            // Create new mesh
            uint32_t meshId = meshBuffer.AddMesh(wallSegment.GetVertices(), wallSegment.GetIndices(), "WallSegment");

            // Update mesh Id
            wallSegment.SetMeshId(meshId);
        }
    }
}

void RecreateAllProcedularWorldPlaneMesh() {
    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("Procedural");

    for (WorldPlane& worldPlane : Unloved::World::GetWorldPlanes()) {
        // Remove old mesh
        meshBuffer.RemoveMesh(worldPlane.GetMeshId());

        // Create new mesh
        uint32_t meshId = meshBuffer.AddMesh(worldPlane.GetVertices(), worldPlane.GetIndices(), "WorldPlane");

        // Update mesh Id
        worldPlane.SetMeshId(meshId);
    }
}

void RecreateAllWeatherBoards() {
    for (Wall& wall : Unloved::World::GetWalls()) {
        wall.RecreateWeatherBoardMesh();
    }
}

void RemoveAllWeatherBoards() {
    for (Wall& wall : Unloved::World::GetWalls()) {
        wall.CleanUpWeatherBoardMesh();
    }
}

void RecreateAllHangingLightCords() {
    for (Light& light : Unloved::World::GetLights()) {
        light.ConfigureMeshNodes();
    }
}

void MarkDirty() {
    g_dirty = true;
}

bool IsDirty() {
    return g_dirty;
}

const std::vector<Vertex>& GetWeatherBoardVertices() {
    return g_weatherBoardVertices;
}

const std::vector<uint32_t>& GetWeatherBoardIndices() {
    return g_weatherBoardIndices;
}

// Picture Frames

const std::vector<std::string> GetLargePictureFrameMaterialNames() {
    const static std::vector<std::string> materials = {
        //"Picture_RainbowMage_ALB",
        "Picture_SHNakedLady",
        "Picture_Minotaur"
    };

    return materials;
}

void ResetPictureFrameImageList() {
    g_takenLargePictureFrameMaterials.clear();
}

void TakeLargePictureFrameMaterial(const std::string& name) {
    g_takenLargePictureFrameMaterials.push_back(name);
}

std::string GetNextRandomLargePictureFrameMaterial() {
    // Pool
    const std::vector<std::string>& materialPool = GetLargePictureFrameMaterialNames();

    // Get available list
    std::vector<std::string> avaliableMaterials;
    for (const std::string& material : materialPool) {
        if (std::find(g_takenLargePictureFrameMaterials.begin(), g_takenLargePictureFrameMaterials.end(), material) == g_takenLargePictureFrameMaterials.end()) {
            avaliableMaterials.push_back(material);
        }
    }

    // Clear pool if all used
    if (avaliableMaterials.empty()) {
        g_takenLargePictureFrameMaterials.clear();
        return GetNextRandomLargePictureFrameMaterial();
    }

    // Return random material from available pool
    const int32_t index = Hell::Random::Int(0, static_cast<int32_t>(avaliableMaterials.size()) - 1);
    const std::string material = avaliableMaterials[index];
    TakeLargePictureFrameMaterial(material);
    return material;
}

}
