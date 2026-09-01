#include "ResourceManager.h"

#include "Hell/Logging.h"
#include "Hell/MemoryTracker/MemoryTracker.h"
#include "Hell/Physics/Ragdoll/RagdollAsset.h"

#include <algorithm>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Hell::ResourceManager {

    namespace {
        std::unordered_map<std::string, Animation> g_animations;
        std::unordered_map<std::string, BoneMask> g_boneMasks;
        std::unordered_map<std::string, GenericMesh> g_genericMeshes;
        std::unordered_map<std::string, IESProfile> g_iesProfiles;
        std::unordered_map<std::string, MeshBuffer> g_meshBuffers;
        std::unordered_map<std::string, MidiFile> g_midiFiles;
        std::unordered_map<std::string, uint32_t> g_modelIdsByName;
        std::unordered_map<std::string, PointAnimation> g_pointAnimations;
        std::unordered_map<std::string, RagdollAsset> g_ragdollAssets;
        std::unordered_map<std::string, uint32_t> g_skinnedModelIdsByName;
        std::unordered_map<std::string, SoundFont> g_soundFonts;
        std::unordered_map<std::string, SpriteSheetTexture> g_spriteSheetTextures;
        std::unordered_map<std::string, Texture> g_textures;
        std::unordered_map<std::string, TextureArray> g_textureArrays;
        std::unordered_map<std::string, Vat> g_vats;

        std::vector<Material> g_materials;
        std::vector<std::string> g_materialNamesByIndex;
        std::unordered_map<std::string, int32_t> g_materialIndices;

        std::unordered_map<uint32_t, Model> g_models;
        std::unordered_map<uint32_t, SkinnedModel> g_skinnedModels;

        std::vector<std::string> g_textureNamesByBindlessIndex;

        uint32_t g_nextModelId = 0;
        uint32_t g_nextSkinnedModelId = 0;

        Material CreateDefaultMaterial() {
            Material material;
            material.m_basecolor = GetTextureBindlessIndexByName("CheckerBoard_ALB");
            material.m_normal = GetTextureBindlessIndexByName("DefaultNRM");
            material.m_rma = GetTextureBindlessIndexByName("DefaultRMA");
            material.m_emissive = GetTextureBindlessIndexByName("Black");
            material.m_opacity = GetTextureBindlessIndexByName("White");
            material.m_hairMaps = GetTextureBindlessIndexByName("Black");
            return material;
        }
    }

    void Init() {
        CreateGenericMesh("DebugLines2D");
        CreateGenericMesh("DebugLines3D");
        CreateGenericMesh("DebugPoints2D");
        CreateGenericMesh("DebugPoints3D");
        CreateGenericMesh("DebugMeshItemExamineLines");
        CreateGenericMesh("UI");

        CreateMeshBuffer("AssetGeometry");
        CreateMeshBuffer("HeightMapGeometry");
        CreateMeshBuffer("PhysicsShapeGeometry");
        CreateMeshBuffer("Procedural");
    }

    void CleanUp() {
        for (auto& object : g_genericMeshes) { object.second.CleanUp(); } g_genericMeshes.clear();
        for (auto& object : g_meshBuffers)   { object.second.CleanUp(); } g_meshBuffers.clear();
        for (auto& object : g_textures)      { object.second.CleanUp(); } g_textures.clear();
        for (auto& object : g_textureArrays) { object.second.CleanUp(); } g_textureArrays.clear();

        g_animations.clear();
        g_boneMasks.clear();
        g_iesProfiles.clear();
        g_materials.clear();
        g_materialIndices.clear();
        g_materialNamesByIndex.clear();
        g_midiFiles.clear();
        g_models.clear();
        g_modelIdsByName.clear();
        g_nextModelId = 0;
        g_pointAnimations.clear();
        g_ragdollAssets.clear();
        g_skinnedModels.clear();
        g_skinnedModelIdsByName.clear();
        g_nextSkinnedModelId = 0;
        g_soundFonts.clear();
        g_spriteSheetTextures.clear();
        g_textureNamesByBindlessIndex.clear();
        g_vats.clear();
    }

    // Animation

    Animation& CreateAnimation(const std::string& name) {
        auto it = g_animations.find(name);

        if (it != g_animations.end()) {
            Logging::Fatal() << "ResourceManager::CreateAnimation(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_animations.emplace(name, Animation());
        return result.first->second;
    }

    std::unordered_map<std::string, Animation>& GetAnimations() {
        return g_animations;
    }

    Animation& GetAnimation(const std::string& name) {
        auto it = g_animations.find(name);

        if (it == g_animations.end()) {
            Logging::Error() << "ResourceManager::GetAnimation(..) failed: '" << name << "' does not exist\n";

            static Animation invalid;
            return invalid;
        }

        return it->second;
    }

    Animation* GetAnimationPtr(const std::string& name) {
        auto it = g_animations.find(name);

        if (it == g_animations.end()) {
            Logging::Error() << "ResourceManager::GetAnimationPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Bone Mask

    BoneMask& CreateBoneMask(BoneMask&& boneMask) {
        const std::string name = boneMask.name;
        auto it = g_boneMasks.find(name);

        if (it != g_boneMasks.end()) {
            Logging::Fatal() << "ResourceManager::CreateBoneMask(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_boneMasks.emplace(name, std::move(boneMask));
        return result.first->second;
    }

    BoneMask* GetBoneMaskPtr(const std::string& name) {
        auto it = g_boneMasks.find(name);

        if (it == g_boneMasks.end()) {
            Logging::Error() << "ResourceManager::GetBoneMaskPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Generic Mesh

    GenericMesh& CreateGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it != g_genericMeshes.end()) {
            Logging::Fatal() << "ResourceManager::CreateGenericMesh(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_genericMeshes.emplace(name, GenericMesh(name));
        return result.first->second;
    }

    GenericMesh& GetGenericMesh(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMesh(..) failed: '" << name << "' does not exist\n";

            static GenericMesh invalid;
            return invalid;
        }

        return it->second;
    }

    GenericMesh* GetGenericMeshPtr(const std::string& name) {
        auto it = g_genericMeshes.find(name);

        if (it == g_genericMeshes.end()) {
            Logging::Error() << "ResourceManager::GetGenericMeshPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // IES Profile

    IESProfile& CreateIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it != g_iesProfiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateIESProfile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_iesProfiles.emplace(name, IESProfile(name));
        return result.first->second;
    }

    IESProfile& CreateIESProfile(IESProfile&& iesProfile) {
        const std::string name = iesProfile.GetName();
        auto it = g_iesProfiles.find(name);

        if (it != g_iesProfiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateIESProfile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_iesProfiles.emplace(name, std::move(iesProfile));
        return result.first->second;
    }

    IESProfile& GetIESProfile(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfile(..) failed: '" << name << "' does not exist\n";

            static IESProfile invalid;
            return invalid;
        }

        return it->second;
    }

    IESProfile* GetIESProfilePtr(const std::string& name) {
        auto it = g_iesProfiles.find(name);

        if (it == g_iesProfiles.end()) {
            Logging::Error() << "ResourceManager::GetIESProfilePtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // VAT

    Vat& CreateVAT(const std::string& name) {
        auto it = g_vats.find(name);

        if (it != g_vats.end()) {
            Logging::Fatal() << "ResourceManager::CreateVAT(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_vats.emplace(name, Vat(name));
        return result.first->second;
    }

    Vat& CreateVAT(Vat&& vat) {
        const std::string name = vat.GetName();
        auto it = g_vats.find(name);

        if (it != g_vats.end()) {
            Logging::Fatal() << "ResourceManager::CreateVAT(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_vats.emplace(name, std::move(vat));
        return result.first->second;
    }

    Vat& GetVAT(const std::string& name) {
        auto it = g_vats.find(name);

        if (it == g_vats.end()) {
            Logging::Error() << "ResourceManager::GetVAT(..) failed: '" << name << "' does not exist\n";

            static Vat invalid;
            return invalid;
        }

        return it->second;
    }

    Vat* GetVATPtr(const std::string& name) {
        auto it = g_vats.find(name);

        if (it == g_vats.end()) {
            Logging::Error() << "ResourceManager::GetVATPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Point Animation

    PointAnimation& CreatePointAnimation(const std::string& name) {
        auto it = g_pointAnimations.find(name);

        if (it != g_pointAnimations.end()) {
            Logging::Fatal() << "ResourceManager::CreatePointAnimation(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_pointAnimations.emplace(name, PointAnimation(name));
        return result.first->second;
    }

    PointAnimation& CreatePointAnimation(PointAnimation&& pointAnimation) {
        const std::string name = pointAnimation.GetName();
        auto it = g_pointAnimations.find(name);

        if (it != g_pointAnimations.end()) {
            Logging::Fatal() << "ResourceManager::CreatePointAnimation(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_pointAnimations.emplace(name, std::move(pointAnimation));
        return result.first->second;
    }

    PointAnimation& GetPointAnimation(const std::string& name) {
        auto it = g_pointAnimations.find(name);

        if (it == g_pointAnimations.end()) {
            Logging::Error() << "ResourceManager::GetPointAnimation(..) failed: '" << name << "' does not exist\n";

            static PointAnimation invalid;
            return invalid;
        }

        return it->second;
    }

    PointAnimation* GetPointAnimationPtr(const std::string& name) {
        auto it = g_pointAnimations.find(name);

        if (it == g_pointAnimations.end()) {
            Logging::Error() << "ResourceManager::GetPointAnimationPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Mesh

    Mesh* GetModelMeshByName(const std::string& modelName, const std::string& meshName) {
        const uint32_t meshId = GetModelMeshIdByName(modelName, meshName);
        if (meshId == 0) return nullptr;

        return GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
    }

    uint32_t GetModelMeshIdByName(const std::string& modelName, const std::string& meshName) {
        Model* model = GetModelByName(modelName);
        if (!model) return 0;

        const int32_t meshId = model->GetGlobalMeshIndexByMeshName(meshName);
        if (meshId < 0) return 0;

        return static_cast<uint32_t>(meshId);
    }

    Mesh* GetQuadMesh() {
        return GetModelMeshByName("Primitives", "Quad");
    }

    uint32_t GetQuadMeshId() {
        return GetModelMeshIdByName("Primitives", "Quad");
    }

    // Ragdoll assets

    RagdollAsset& CreateRagdollAsset(RagdollAsset&& asset) {
        const std::string name = asset.name;
        auto it = g_ragdollAssets.find(name);
        if (it != g_ragdollAssets.end()) {
            Logging::Fatal() << "ResourceManager::CreateRagdollAsset(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_ragdollAssets.emplace(name, std::move(asset));
        return result.first->second;
    }

    RagdollAsset* GetRagdollAssetByName(const std::string& name) {
        auto it = g_ragdollAssets.find(name);
        if (it == g_ragdollAssets.end()) {
            Logging::Error() << "ResourceManager::GetRagdollAssetByName(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Material

    Material& CreateMaterial(const std::string& name) {
        auto it = g_materialIndices.find(name);

        if (it != g_materialIndices.end()) {
            Logging::Fatal() << "ResourceManager::CreateMaterial(..) failed: '" << name << "' already exists\n";
            return g_materials[it->second];
        }

        const int32_t index = static_cast<int32_t>(g_materials.size());
        g_materials.push_back(CreateDefaultMaterial());
        g_materialIndices.emplace(name, index);
        g_materialNamesByIndex.push_back(name);
        return g_materials.back();
    }

    std::vector<Material>& GetMaterials() {
        return g_materials;
    }

    std::vector<std::string> GetMaterialNames() {
        return g_materialNamesByIndex;
    }

    Material* GetDefaultMaterial() {
        auto it = g_materialIndices.find("CheckerBoard");
        if (it == g_materialIndices.end()) {
            Logging::Error() << "ResourceManager::GetDefaultMaterial() failed: 'CheckerBoard' does not exist\n";
            return nullptr;
        }

        return &g_materials[it->second];
    }

    Material* GetMaterialByIndex(int32_t index) {
        if (index >= 0 && static_cast<size_t>(index) < g_materials.size()) {
            return &g_materials[index];
        }

        Logging::Error() << "ResourceManager::GetMaterialByIndex(..) failed: index '" << index << "' is out of range\n";
        return GetDefaultMaterial();
    }

    Material* GetMaterialByName(const std::string& name) {
        auto it = g_materialIndices.find(name);
        if (it != g_materialIndices.end()) {
            return GetMaterialByIndex(it->second);
        }

        Logging::Error() << "ResourceManager::GetMaterialByName(..) failed: '" << name << "' does not exist\n";
        return GetDefaultMaterial();
    }

    int32_t GetMaterialIndexByName(const std::string& name) {
        auto it = g_materialIndices.find(name);
        return it != g_materialIndices.end() ? it->second : -1;
    }

    std::string GetMaterialNameByIndex(int32_t index) {
        if (index >= 0 && static_cast<size_t>(index) < g_materialNamesByIndex.size()) {
            return g_materialNamesByIndex[index];
        }

        Logging::Error() << "ResourceManager::GetMaterialNameByIndex(..) failed: index '" << index << "' is out of range\n";
        return UNDEFINED_STRING;
    }

    // Mesh Buffer

    MeshBuffer& CreateMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it != g_meshBuffers.end()) {
            Logging::Fatal() << "ResourceManager::CreateMeshBuffer(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_meshBuffers.emplace(name, MeshBuffer(name));
        if (name == "HeightMapGeometry" || name == "Procedural") {
            result.first->second.SetCreateVulkanBlasForNewMeshes(false);
        }
        return result.first->second;
    }

    MeshBuffer& GetMeshBuffer(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBuffer(..) failed: '" << name << "' does not exist\n";

            static MeshBuffer invalid;
            return invalid;
        }

        return it->second;
    }

    MeshBuffer* GetMeshBufferPtr(const std::string& name) {
        auto it = g_meshBuffers.find(name);

        if (it == g_meshBuffers.end()) {
            Logging::Error() << "ResourceManager::GetMeshBufferPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // MIDI Files

    MidiFile& CreateMidiFile(const std::string& name) {
        auto it = g_midiFiles.find(name);

        if (it != g_midiFiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateMidiFile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_midiFiles.emplace(name, MidiFile(name));
        return result.first->second;
    }

    MidiFile& CreateMidiFile(MidiFile&& midiFile) {
        const std::string name = midiFile.GetName();
        auto it = g_midiFiles.find(name);

        if (it != g_midiFiles.end()) {
            Logging::Fatal() << "ResourceManager::CreateMidiFile(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_midiFiles.emplace(name, std::move(midiFile));
        return result.first->second;
    }

    MidiFile& GetMidiFile(const std::string& name) {
        auto it = g_midiFiles.find(name);

        if (it == g_midiFiles.end()) {
            Logging::Error() << "ResourceManager::GetMidiFile(..) failed: '" << name << "' does not exist\n";

            static MidiFile invalid;
            return invalid;
        }

        return it->second;
    }

    MidiFile* GetMidiFilePtr(const std::string& name) {
        auto it = g_midiFiles.find(name);

        if (it == g_midiFiles.end()) {
            Logging::Error() << "ResourceManager::GetMidiFilePtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Models

    Model& CreateModel(const std::string& name) {
        auto nameIt = g_modelIdsByName.find(name);

        if (nameIt != g_modelIdsByName.end()) {
            Logging::Fatal() << "ResourceManager::CreateModel(..) failed: '" << name << "' already exists\n";
            return g_models[nameIt->second];
        }

        if (g_nextModelId == std::numeric_limits<uint32_t>::max()) {
            Logging::Fatal() << "ResourceManager::CreateModel(..) failed: model ID space exhausted\n";
            static Model invalid;
            return invalid;
        }

        g_nextModelId++;

        Model& model = g_models[g_nextModelId];
        model.SetModelId(g_nextModelId);
        model.SetName(name);
        g_modelIdsByName[name] = g_nextModelId;

        return model;
    }

    std::unordered_map<uint32_t, Model>& GetModels() {
        return g_models;
    }

    Model* GetModelById(uint32_t modelId) {
        auto it = g_models.find(modelId);
        if (it != g_models.end()) {
            return &it->second;
        }

        Logging::Error() << "ResourceManager::GetModelById(..) failed: model id '" << modelId << "' does not exist\n";
        return nullptr;
    }

    Model* GetModelByName(const std::string& name) {
        const uint32_t modelId = GetModelIdByName(name);
        if (modelId == 0) {
            return nullptr;
        }

        return GetModelById(modelId);
    }

    uint32_t GetModelIdByName(const std::string& name) {
        auto it = g_modelIdsByName.find(name);
        if (it != g_modelIdsByName.end()) {
            return it->second;
        }

        Logging::Error() << "ResourceManager::GetModelIdByName(..) failed: model '" << name << "' does not exist\n";
        //__debugbreak();
        return 0;
    }

    void SetModelName(uint32_t modelId, const std::string& name) {
        Model* model = GetModelById(modelId);
        if (!model) {
            return;
        }

        auto existingNameIt = g_modelIdsByName.find(name);
        if (existingNameIt != g_modelIdsByName.end() && existingNameIt->second != modelId) {
            Logging::Fatal() << "ResourceManager::SetModelName(..) failed: model name '" << name << "' already exists\n";
            return;
        }

        const std::string oldName = model->GetName();
        auto oldNameIt = g_modelIdsByName.find(oldName);
        if (oldNameIt != g_modelIdsByName.end() && oldNameIt->second == modelId) {
            g_modelIdsByName.erase(oldNameIt);
        }

        model->SetName(name);
        g_modelIdsByName[name] = modelId;
    }

    // Skinned Models

    SkinnedModel& CreateSkinnedModel(const std::string& name) {
        auto nameIt = g_skinnedModelIdsByName.find(name);

        if (nameIt != g_skinnedModelIdsByName.end()) {
            Logging::Fatal() << "ResourceManager::CreateSkinnedModel(..) failed: '" << name << "' already exists\n";
            return g_skinnedModels[nameIt->second];
        }

        if (g_nextSkinnedModelId == std::numeric_limits<uint32_t>::max()) {
            Logging::Fatal() << "ResourceManager::CreateSkinnedModel(..) failed: skinned model ID space exhausted\n";
            static SkinnedModel invalid;
            return invalid;
        }

        g_nextSkinnedModelId++;

        SkinnedModel& skinnedModel = g_skinnedModels[g_nextSkinnedModelId];
        skinnedModel.SetSkinnedModelId(g_nextSkinnedModelId);
        skinnedModel.SetName(name);
        g_skinnedModelIdsByName[name] = g_nextSkinnedModelId;

        return skinnedModel;
    }

    std::unordered_map<uint32_t, SkinnedModel>& GetSkinnedModels() {
        return g_skinnedModels;
    }

    SkinnedModel* GetSkinnedModelById(uint32_t skinnedModelId) {
        auto it = g_skinnedModels.find(skinnedModelId);
        if (it != g_skinnedModels.end()) {
            return &it->second;
        }

        Logging::Error() << "ResourceManager::GetSkinnedModelById(..) failed: skinned model id '" << skinnedModelId << "' does not exist\n";
        return nullptr;
    }

    SkinnedModel* GetSkinnedModelByName(const std::string& name) {
        const uint32_t skinnedModelId = GetSkinnedModelIdByName(name);
        if (skinnedModelId == 0) {
            return nullptr;
        }

        return GetSkinnedModelById(skinnedModelId);
    }

    uint32_t GetSkinnedModelIdByName(const std::string& name) {
        auto it = g_skinnedModelIdsByName.find(name);
        if (it != g_skinnedModelIdsByName.end()) {
            return it->second;
        }

        Logging::Error() << "ResourceManager::GetSkinnedModelIdByName(..) failed: skinned model '" << name << "' does not exist\n";
        return 0;
    }

    // Sound Fonts

    SoundFont& CreateSoundFont(const std::string& name) {
        auto it = g_soundFonts.find(name);

        if (it != g_soundFonts.end()) {
            Logging::Fatal() << "ResourceManager::CreateSoundFont(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_soundFonts.emplace(name, SoundFont(name));
        return result.first->second;
    }

    SoundFont& CreateSoundFont(SoundFont&& soundFont) {
        const std::string name = soundFont.GetName();
        auto it = g_soundFonts.find(name);

        if (it != g_soundFonts.end()) {
            Logging::Fatal() << "ResourceManager::CreateSoundFont(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_soundFonts.emplace(name, std::move(soundFont));
        return result.first->second;
    }

    SoundFont& GetSoundFont(const std::string& name) {
        auto it = g_soundFonts.find(name);

        if (it == g_soundFonts.end()) {
            Logging::Error() << "ResourceManager::GetSoundFont(..) failed: '" << name << "' does not exist\n";

            static SoundFont invalid;
            return invalid;
        }

        return it->second;
    }

    SoundFont* GetSoundFontPtr(const std::string& name) {
        auto it = g_soundFonts.find(name);

        if (it == g_soundFonts.end()) {
            Logging::Error() << "ResourceManager::GetSoundFontPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    // Sprite Sheet Texture

    SpriteSheetTexture& CreateSpriteSheetTexture(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);

        if (it != g_spriteSheetTextures.end()) {
            Logging::Fatal() << "ResourceManager::CreateSpriteSheetTexture(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_spriteSheetTextures.try_emplace(name);
        return result.first->second;
    }

    SpriteSheetTexture& GetSpriteSheetTexture(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);

        if (it == g_spriteSheetTextures.end()) {
            Logging::Error() << "ResourceManager::GetSpriteSheetTexture(..) failed: '" << name << "' does not exist\n";

            static SpriteSheetTexture invalid;
            return invalid;
        }

        return it->second;
    }

    SpriteSheetTexture* GetSpriteSheetTexturePtr(const std::string& name) {
        auto it = g_spriteSheetTextures.find(name);
        return it != g_spriteSheetTextures.end() ? &it->second : nullptr;
    }

    // Texture

    Texture& CreateTexture(const std::string& name) {
        auto it = g_textures.find(name);

        if (it != g_textures.end()) {
            Logging::Fatal() << "ResourceManager::CreateTexture(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_textures.try_emplace(name);
        Texture& texture = result.first->second;
        const int32_t bindlessIndex = static_cast<int32_t>(g_textureNamesByBindlessIndex.size());

        texture.SetBindlessIndex(bindlessIndex);

        g_textureNamesByBindlessIndex.push_back(name);

        return texture;
    }

    std::unordered_map<std::string, Texture>& GetTextures() {
        return g_textures;
    }

    Texture* GetTextureByName(const std::string& name) {
        auto it = g_textures.find(name);

        if (it == g_textures.end()) {
            Logging::Error() << "ResourceManager::GetTextureByName() failed because texture '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    Texture* GetTextureByBindlessIndex(int32_t bindlessIndex) {
        if (bindlessIndex < 0 || static_cast<size_t>(bindlessIndex) >= g_textureNamesByBindlessIndex.size()) {
            Logging::Error() << "ResourceManager::GetTextureByBindlessIndex() failed because bindless index '" << bindlessIndex << "' does not exist\n";
            return nullptr;
        }

        return GetTextureByName(g_textureNamesByBindlessIndex[bindlessIndex]);
    }

    int32_t GetTextureBindlessIndexByName(const std::string& name, bool ignoreWarning) {
        auto it = g_textures.find(name);
        if (it != g_textures.end()) {
            return it->second.GetBindlessIndex();
        }

        if (!ignoreWarning) {
            Logging::Fatal() << "ResourceManager::GetTextureBindlessIndexByName() failed because texture '" << name << "' does not exist\n";
        }

        return -1;
    }

    void FreeTextureCPUMemory() {
        for (auto& [name, texture] : g_textures) {
            texture.FreeCPUMemory();
        }
    }

    // Texture Array

    TextureArray& CreateTextureArray(const std::string& name) {
        auto it = g_textureArrays.find(name);

        if (it != g_textureArrays.end()) {
            Logging::Fatal() << "ResourceManager::CreateTextureArray(..) failed: '" << name << "' already exists\n";
            return it->second;
        }

        auto result = g_textureArrays.emplace(name, TextureArray(name));
        return result.first->second;
    }

    TextureArray& GetTextureArray(const std::string& name) {
        auto it = g_textureArrays.find(name);

        if (it == g_textureArrays.end()) {
            Logging::Error() << "ResourceManager::GetTextureArray(..) failed: '" << name << "' does not exist\n";

            static TextureArray invalid;
            return invalid;
        }

        return it->second;
    }

    TextureArray* GetTextureArrayPtr(const std::string& name) {
        auto it = g_textureArrays.find(name);

        if (it == g_textureArrays.end()) {
            Logging::Error() << "ResourceManager::GetTextureArrayPtr(..) failed: '" << name << "' does not exist\n";
            return nullptr;
        }

        return &it->second;
    }

    void RemoveTextureArrayByName(const std::string& name) {
        auto it = g_textureArrays.find(name);
        if (it == g_textureArrays.end()) return;

        it->second.CleanUp();
        g_textureArrays.erase(it);
    }

    // Memory Report

    void AppendMemoryReport(MemoryTracker::MemoryReport& report) {
        if (!g_genericMeshes.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Generic Meshes";
            category.entries.reserve(g_genericMeshes.size());

            for (const auto& [name, genericMesh] : g_genericMeshes) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = genericMesh.GetCPUAllocatedByteCount();
                entry.gpuBytes = genericMesh.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_meshBuffers.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Mesh Buffers";
            category.entries.reserve(g_meshBuffers.size());

            for (const auto& [name, meshBuffer] : g_meshBuffers) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = meshBuffer.GetCPUAllocatedByteCount();
                entry.gpuBytes = meshBuffer.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_iesProfiles.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "IES Profiles";
            category.entries.reserve(g_iesProfiles.size());

            for (const auto& [name, iesProfile] : g_iesProfiles) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = iesProfile.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_vats.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "VAT";
            category.entries.reserve(g_vats.size());

            for (const auto& [name, vat] : g_vats) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = vat.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_pointAnimations.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Point Animations";
            category.entries.reserve(g_pointAnimations.size());

            for (const auto& [name, pointAnimation] : g_pointAnimations) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = pointAnimation.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_models.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Models";
            category.entries.reserve(g_models.size());

            for (const auto& [modelId, model] : g_models) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = model.GetName();
                entry.cpuBytes = model.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_skinnedModels.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Skinned Models";
            category.entries.reserve(g_skinnedModels.size());

            for (const auto& [skinnedModelId, skinnedModel] : g_skinnedModels) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = skinnedModel.GetName();
                entry.cpuBytes = skinnedModel.GetCPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_textures.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Textures";
            category.entries.reserve(g_textures.size());

            for (const auto& [name, texture] : g_textures) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = texture.GetCPUAllocatedByteCount();
                entry.gpuBytes = texture.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        if (!g_textureArrays.empty()) {
            MemoryTracker::MemoryReportCategory& category = report.categories.emplace_back();
            category.name = "Texture Arrays";
            category.entries.reserve(g_textureArrays.size());

            for (const auto& [name, textureArray] : g_textureArrays) {
                MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = 0;
                entry.gpuBytes = textureArray.GetGPUAllocatedByteCount();
            }

            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }
    }
}
