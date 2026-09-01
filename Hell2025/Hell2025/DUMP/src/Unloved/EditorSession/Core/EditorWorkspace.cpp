#include "EditorWorkspace.h"

#include "EditorViewports.h"

#include "Unloved/EditorSession/BoneMask/EditorBoneMask.h"
#include "Unloved/EditorSession/Ragdoll/EditorRagdoll.h"

#include "Hell/Audio.h"
#include "Hell/Common/String.h"
#include "Hell/File/File.h"

#include "Legacy/World/LegacyWorld.h"
#include "Unloved/Common/Constants.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Maps/Map.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/House/HouseManager.h"
#include "Unloved/Systems/Map/MapManager.h"
#include "Unloved/World/World.h"

namespace Unloved::EditorSession::Workspace {
    namespace {
        EditorSessionMode g_mode = EditorSessionMode::HOUSE;
        std::string g_houseName;
        std::string g_mapName;
        uint64_t g_worldGeneration = 0;
        bool g_hasMode = false;

        bool IsWorldCurrent() {
            return IsWorldBacked() && g_worldGeneration == World::GetGeneration();
        }

        bool FileNameExists(const std::string& directory, const std::string& extension, const std::string& name) {
            const std::string lowerName = Hell::String::ToLower(name);
            for (const FileInfo& fileInfo : Hell::File::IterateDirectory(directory, { extension })) {
                if (Hell::String::ToLower(fileInfo.name) == lowerName) return true;
            }
            return false;
        }

        const std::string* GetStoredName(EditorSessionMode mode) {
            switch (mode) {
                case EditorSessionMode::HOUSE:     return &g_houseName;
                case EditorSessionMode::MAP:       return &g_mapName;
                case EditorSessionMode::RAGDOLL:   return nullptr;
                case EditorSessionMode::BONE_MASK: return nullptr;
            }

            return nullptr;
        }

        bool PrepareHouse(const std::string& houseName) {
            HouseData* houseData = HouseManager::GetHouseDataByName(houseName);
            if (!houseData) return false;

            Viewports::PrepareInitialView(houseData->GetCreateInfoCollection());
            World::LoadSingleHouse(houseName);

            // Reset player 0 if they are too high
            if (Player* player = Session::GetLocalPlayerByViewportIndex(0)) {
                if (player->GetFootPosition().y > 10.0f) {
                    player->SetFootPosition(glm::vec3(2.25f, 0.0f, 1.68f));
                    player->GetCamera().SetEulerRotation(glm::vec3(-0.2f, 0.0f, 0.0f));
                }
            }

            return true;
        }

        bool PrepareMap(const std::string& mapName) {
            MapData* mapData = MapManager::GetMapDataByName(mapName);
            if (!mapData) return false;

            Viewports::PrepareInitialMapView(mapData->GetChunkCountX(), mapData->GetChunkCountZ(), MapData::DEFAULT_HEIGHT);
            MapCreateInfo mapCreateInfo;
            mapCreateInfo.mapName = mapName;

            World::ResetWorld();

            // Load terrain through the legacy path
            LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });
            World::LoadMapObjects(*mapData, SpawnOffset());

            return true;
        }

        void Commit() {
            switch (g_mode) {
                case EditorSessionMode::HOUSE:
                    HouseManager::UpdateCreateInfoCollectionFromWorld(g_houseName);
                    break;
                case EditorSessionMode::MAP:
                    MapManager::UpdateCreateInfoCollectionFromWorld(g_mapName);
                    break;
                case EditorSessionMode::RAGDOLL:
                case EditorSessionMode::BONE_MASK:
                    break;
            }
        }
    }

    bool Open(EditorSessionMode mode) {
        if (mode == EditorSessionMode::BONE_MASK && BoneMaskEditor::HasDocument()) {
            g_mode = EditorSessionMode::BONE_MASK;
            g_worldGeneration = 0;
            g_hasMode = true;
            return true;
        }

        if (mode == EditorSessionMode::RAGDOLL && RagdollEditor::HasDocument()) {
            g_mode = EditorSessionMode::RAGDOLL;
            g_worldGeneration = 0;
            g_hasMode = true;
            return true;
        }

        const std::string* name = GetStoredName(mode);
        if (name && !name->empty()) {
            switch (mode) {
                case EditorSessionMode::HOUSE:     return OpenHouse(*name);
                case EditorSessionMode::MAP:       return OpenMap(*name);
                case EditorSessionMode::RAGDOLL:   break;
                case EditorSessionMode::BONE_MASK: break;
            }
        }

        g_mode = mode;
        g_worldGeneration = 0;
        g_hasMode = false;

        return false;
    }

    bool OpenBoneMask(const std::string& path, std::string& error) {
        if (!BoneMaskEditor::Open(path, error)) return false;

        g_mode = EditorSessionMode::BONE_MASK;
        g_worldGeneration = 0;
        g_hasMode = true;
        return true;
    }

    bool ImportRagdoll(const std::string& path, std::string& error) {
        if (!RagdollEditor::ImportLegacy(path, error)) {
            return false;
        }

        g_mode = EditorSessionMode::RAGDOLL;
        g_worldGeneration = 0;
        g_hasMode = true;

        return true;
    }

    bool OpenRagdoll(const std::string& path, std::string& error) {
        if (!RagdollEditor::OpenNative(path, error)) return false;

        g_mode = EditorSessionMode::RAGDOLL;
        g_worldGeneration = 0;
        g_hasMode = true;
        return true;
    }

    bool SaveBoneMask(std::string& error) {
        if (!g_hasMode || g_mode != EditorSessionMode::BONE_MASK) {
            error = "No bone mask is open";
            return false;
        }
        if (!BoneMaskEditor::Save(error)) return false;

        Debug::BlitQuickDebugMessage("Bone mask saved");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        return true;
    }

    bool SaveRagdoll(std::string& error) {
        if (!g_hasMode || g_mode != EditorSessionMode::RAGDOLL) {
            error = "No ragdoll is open";
            return false;
        }
        if (!RagdollEditor::Save(error)) return false;

        Debug::BlitQuickDebugMessage("Ragdoll saved");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        return true;
    }

    bool SaveRagdollAs(const std::string& name, std::string& error) {
        if (!g_hasMode || g_mode != EditorSessionMode::RAGDOLL) {
            error = "No ragdoll is open";
            return false;
        }
        if (NameExists(EditorSessionMode::RAGDOLL, name)) {
            error = "Ragdoll '" + name + "' already exists";
            return false;
        }
        if (!RagdollEditor::SaveAs(name, error)) return false;

        Debug::BlitQuickDebugMessage("Ragdoll saved as '" + name + "'");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        return true;
    }

    bool OpenHouse(const std::string& name) {
        if (name.empty()) return false;

        const bool alreadyOpen = g_hasMode && g_mode == EditorSessionMode::HOUSE && g_houseName == name && IsWorldCurrent();
        if (alreadyOpen) return true;

        HouseManager::LoadHouseData(name);
        if (!PrepareHouse(name)) return false;

        g_houseName = name;
        g_mode = EditorSessionMode::HOUSE;
        g_worldGeneration = World::GetGeneration();
        g_hasMode = true;

        return true;
    }

    bool OpenMap(const std::string& name) {
        if (name.empty()) return false;

        const bool alreadyOpen = g_hasMode && g_mode == EditorSessionMode::MAP && g_mapName == name && IsWorldCurrent();
        if (alreadyOpen) return true;

        MapManager::LoadMapData(name);
        if (!PrepareMap(name)) return false;

        g_mapName = name;
        g_mode = EditorSessionMode::MAP;
        g_worldGeneration = World::GetGeneration();
        g_hasMode = true;

        return true;
    }

    bool NewHouse(const std::string& name) {
        if (name.empty() || NameExists(EditorSessionMode::HOUSE, name)) return false;
        if (!HouseManager::NewHouse(name)) return false;
        if (!PrepareHouse(name)) return false;

        g_houseName = name;
        g_mode = EditorSessionMode::HOUSE;
        g_worldGeneration = World::GetGeneration();
        g_hasMode = true;

        HouseManager::SaveHouse(name);

        return true;
    }

    bool NewMap(const std::string& name) {
        constexpr int32_t DEFAULT_CHUNK_COUNT = 8;

        if (name.empty() || NameExists(EditorSessionMode::MAP, name)) return false;
        if (!MapManager::NewMap(name, DEFAULT_CHUNK_COUNT, DEFAULT_CHUNK_COUNT, MapData::DEFAULT_HEIGHT)) return false;
        if (!PrepareMap(name)) return false;

        g_mapName = name;
        g_mode = EditorSessionMode::MAP;
        g_worldGeneration = World::GetGeneration();
        g_hasMode = true;

        MapManager::SaveMap(name);

        return true;
    }

    bool NewBoneMask(const std::string& name, const std::string& skinnedModelName, std::string& error) {
        if (name.empty()) {
            error = "Enter a bone mask name";
            return false;
        }
        if (NameExists(EditorSessionMode::BONE_MASK, name)) {
            error = "Bone mask '" + name + "' already exists";
            return false;
        }
        if (!BoneMaskEditor::New(name, skinnedModelName, error)) return false;

        g_mode = EditorSessionMode::BONE_MASK;
        g_worldGeneration = 0;
        g_hasMode = true;
        return true;
    }

    bool NewRagdoll(const std::string& name, const std::string& skinnedModelName, std::string& error) {
        if (name.empty()) {
            error = "Enter a ragdoll name";
            return false;
        }
        if (NameExists(EditorSessionMode::RAGDOLL, name)) {
            error = "Ragdoll '" + name + "' already exists";
            return false;
        }
        if (!RagdollEditor::New(name, skinnedModelName, error)) return false;

        g_mode = EditorSessionMode::RAGDOLL;
        g_worldGeneration = 0;
        g_hasMode = true;
        return true;
    }

    void Close() {
        if (!IsWorldCurrent()) return;
        Commit();
    }

    void Save() {
        if (!IsWorldCurrent()) return;
        Commit();

        if (g_mode == EditorSessionMode::HOUSE) {
            HouseManager::SaveHouse(g_houseName);
            Debug::BlitQuickDebugMessage("House saved");
        }

        if (g_mode == EditorSessionMode::MAP) {
            MapManager::SaveMap(g_mapName);
            Debug::BlitQuickDebugMessage("Map saved");
        }

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
    }

    void Discard() {
        if (g_mode == EditorSessionMode::RAGDOLL) {
            RagdollEditor::Reset();
        }
        if (g_mode == EditorSessionMode::BONE_MASK) {
            BoneMaskEditor::Reset();
        }

        g_hasMode = false;
        g_worldGeneration = 0;
    }

    bool RevertHouse() {
        if (!IsWorldCurrent() || g_mode != EditorSessionMode::HOUSE) return false;
        if (!HouseManager::ReloadHouseData(g_houseName)) return false;

        World::LoadSingleHouse(g_houseName);
        g_worldGeneration = World::GetGeneration();
        Debug::BlitQuickDebugMessage("House reverted");

        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);
        return true;
    }

    bool RevertMap() {
        if (!IsWorldCurrent() || g_mode != EditorSessionMode::MAP) return false;
        if (!MapManager::ReloadMapData(g_mapName)) return false;

        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        if (!mapData) return false;

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = g_mapName;
        World::ResetWorld();
        LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });
        World::LoadMapObjects(*mapData, SpawnOffset());
        g_worldGeneration = World::GetGeneration();

        Debug::BlitQuickDebugMessage("Map reverted");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);

        return true;
    }

    bool ResetHeightMap() {
        if (!IsWorldCurrent() || g_mode != EditorSessionMode::MAP) return false;

        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        if (!mapData) return false;

        mapData->ClearToHeight(MapData::DEFAULT_HEIGHT);
        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = g_mapName;

        LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });

        Debug::BlitQuickDebugMessage("Height map reset");
        Hell::Audio::PlayAudio(AUDIO_SELECT, 1.0f);

        return true;
    }

    bool HasMode() {
        return g_hasMode;
    }

    bool IsWorldBacked() {
        return g_hasMode && (g_mode == EditorSessionMode::HOUSE || g_mode == EditorSessionMode::MAP);
    }

    EditorSessionMode GetMode() {
        return g_mode;
    }

    const std::string& GetName() {
        if (g_mode == EditorSessionMode::RAGDOLL) {
            return RagdollEditor::GetName();
        }
        if (g_mode == EditorSessionMode::BONE_MASK) {
            return BoneMaskEditor::GetName();
        }

        static const std::string emptyName;
        const std::string* name = GetStoredName(g_mode);
        return name ? *name : emptyName;
    }

    bool NameExists(EditorSessionMode mode, const std::string& name) {
        if (name.empty()) return false;

        const std::string lowerName = Hell::String::ToLower(name);
        const std::string* currentName = GetStoredName(mode);

        if (g_hasMode && g_mode == mode && currentName && Hell::String::ToLower(*currentName) == lowerName) {
            return true;
        }

        switch (mode) {
            case EditorSessionMode::HOUSE:     return FileNameExists("res/houses", "house", name);
            case EditorSessionMode::MAP:       return FileNameExists("res/maps", "map", name);
            case EditorSessionMode::RAGDOLL:   return FileNameExists("res/ragdolls", "ragdoll", name);
            case EditorSessionMode::BONE_MASK: return FileNameExists("res/bone_masks", "bonemask", name);
        }

        return false;
    }

    bool SetHouseName(const std::string& name) {
        if (!g_hasMode || g_mode != EditorSessionMode::HOUSE || name.empty()) return false;
        if (name == g_houseName) return true;
        if (FileNameExists("res/houses", "house", name)) return false;

        HouseData* houseData = HouseManager::GetHouseDataByName(g_houseName);
        if (!houseData) return false;

        houseData->SetFilename(name);
        g_houseName = name;
        return true;
    }

    bool SetMapName(const std::string& name) {
        if (!g_hasMode || g_mode != EditorSessionMode::MAP || name.empty()) return false;
        if (name == g_mapName) return true;
        if (FileNameExists("res/maps", "map", name)) return false;

        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        if (!mapData) return false;

        mapData->SetFilename(name);
        g_mapName = name;
        return true;
    }

    uint32_t GetMapChunkWidth() {
        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        return mapData ? mapData->GetChunkCountX() : 0;
    }

    uint32_t GetMapChunkDepth() {
        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        return mapData ? mapData->GetChunkCountZ() : 0;
    }

    bool ResizeMap(uint32_t chunkWidth, uint32_t chunkDepth) {
        if (!IsWorldCurrent() || g_mode != EditorSessionMode::MAP || chunkWidth == 0 || chunkDepth == 0) return false;

        MapData* mapData = MapManager::GetMapDataByName(g_mapName);
        if (!mapData) return false;
        if (chunkWidth == mapData->GetChunkCountX() && chunkDepth == mapData->GetChunkCountZ()) return true;

        if (!mapData->Resize(chunkWidth, chunkDepth)) return false;

        MapCreateInfo mapCreateInfo;
        mapCreateInfo.mapName = g_mapName;
        LegacyWorld::LoadMapsHeightMapData({ mapCreateInfo });
        return true;
    }
}
