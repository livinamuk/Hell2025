#pragma once
#include "Unloved/Common/Types.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Math/Transform.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Road.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/Exterior/Tree.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Props/BulletCasing.h"
#include "Unloved/Objects/Props/GameObject.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Objects/Props/GenericBouncable.h"
#include "Unloved/Objects/Props/GenericStatic.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/House.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/TrimSet.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Renderables/MeshBufferOLD.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/Maps/Map.h"

#include <vector>

namespace Unloved::LegacyWorld {

    void BeginFrame();
    void EndFrame();

    void ResetWorld();
    void ClearAllObjects();

    DDGIVolume& GetTestDDGIVolume();

    void LoadMapsHeightMapData(const std::vector<MapCreateInfo>& mapCreateInfoSet);

    bool ChunkExists(int x, int z);
    const uint32_t GetChunkCountX();
    const uint32_t GetChunkCountZ();
    const uint32_t GetChunkCount();
    const HeightMapChunk* GetChunk(int x, int z);

    void PrintObjectCounts();

    // Creation
    void CreateGameObject();

    const float GetWorldSpaceWidth();
    const float GetWorldSpaceDepth();

    // Map
    const std::string& GetCurrentMapName();

    std::vector<HeightMapChunk>& GetHeightMapChunks();
    std::vector<Road>& GetRoads();
}
