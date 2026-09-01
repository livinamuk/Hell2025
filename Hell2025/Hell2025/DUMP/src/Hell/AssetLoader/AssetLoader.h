#pragma once
#include "Hell/AssetFormats/AssetData.h"
#include "Hell/File.h"

#include <cstdint>
#include <string>
#include <vector>

struct RagdollAsset;
struct Animation;
struct BoneMask;

namespace Hell::AssetLoader {

    void Init(uint32_t maxCompressedTextureResolution);
    void DiscoverAssets();
    void Update();
    void LoadMinimumRequiredAssets(); // TODO: rename to something that doesn't fell like a boolean
    bool LoadingComplete();
    void OnLoadingComplete();
    void AddLoadLogItem(std::string text);
    std::vector<std::string>& GetLoadLog();

    bool LoadAnimation(const FileInfo& fileInfo, Animation& outAnimation);
    bool LoadBoneMask(const std::string& path, BoneMask& boneMask, std::string& error);
    bool SaveBoneMask(const std::string& path, const BoneMask& boneMask, std::string& error);
    void LoadBoneMasks();
    bool LoadRagdollAsset(const std::string& path, RagdollAsset& asset, std::string& error);
    bool SaveRagdollAsset(const std::string& path, const RagdollAsset& asset, std::string& error);
    void LoadRagdollAssets();
    void CreateSpriteSheets();
    void LoadIESFiles();
    void LoadMidiFiles();
    void LoadPointAnimations();
    void LoadSoundFonts();
    void LoadVATFiles();
}
