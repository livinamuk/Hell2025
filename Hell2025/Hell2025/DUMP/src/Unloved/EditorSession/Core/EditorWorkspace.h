#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <cstdint>
#include <string>

namespace Unloved::EditorSession::Workspace {

    bool Open(EditorSessionMode mode);
    bool OpenHouse(const std::string& name);
    bool OpenMap(const std::string& name);
    bool OpenBoneMask(const std::string& path, std::string& error);
    bool OpenRagdoll(const std::string& path, std::string& error);
    bool ImportRagdoll(const std::string& path, std::string& error);
    bool SaveBoneMask(std::string& error);
    bool SaveRagdoll(std::string& error);
    bool SaveRagdollAs(const std::string& name, std::string& error);
    bool NewHouse(const std::string& name);
    bool NewMap(const std::string& name);
    bool NewBoneMask(const std::string& name, const std::string& skinnedModelName, std::string& error);
    bool NewRagdoll(const std::string& name, const std::string& skinnedModelName, std::string& error);
    void Close();
    void Save();
    void Discard();
    bool RevertHouse();
    bool RevertMap();
    bool ResetHeightMap();

    bool HasMode();
    bool IsWorldBacked();
    EditorSessionMode GetMode();
    const std::string& GetName();
    bool NameExists(EditorSessionMode mode, const std::string& name);
    bool SetHouseName(const std::string& name);
    bool SetMapName(const std::string& name);
    uint32_t GetMapChunkWidth();
    uint32_t GetMapChunkDepth();
    bool ResizeMap(uint32_t chunkWidth, uint32_t chunkDepth);
}
