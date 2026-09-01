#include "AssetLoader.h"

#include "Hell/AssetFormats/AssetFormats.h"
#include "Hell/Bvh/BVH.h"
#include "Hell/File.h"
#include "Hell/ImageTools/ImageTools.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/ResourceManagement/TextureUploader.h"
#include "Hell/UI/UIBackEnd.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <future>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace Hell::AssetLoader {

    void BakeModels();
    void BakeSkinnedModels();
    void BuildPrimitives();
    void CopyInAllLoadedModelBvhData();
    void PreAllocateAssetGeometry();
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax, int parentIndex, glm::mat4 localTransform, glm::mat4 inverseBindTransform);
    uint32_t CreateSkinnedMesh(const SkinnedMeshData& skinnedMeshData);

    namespace {

        struct LoadingComplete {
            bool animations = false;
            bool bakedModels = false;
            bool bakedSkinnedModels = false;
            bool modelBVHData = false;
            bool spriteSheets = false;
            bool allComplete = false;
        };

        struct LoadedModelData {
            ModelData modelData;
            ModelBvhData modelBvhData;
        };

        struct ModelDataLoadJob {
            uint32_t modelId = 0;
            FileInfo fileInfo;
            std::future<LoadedModelData> future;
            bool awaitingWork = true;
        };

        struct SkinnedModelDataLoadJob {
            uint32_t skinnedModelId = 0;
            FileInfo fileInfo;
            std::future<SkinnedModelData> future;
            bool awaitingWork = true;
        };

        struct AnimationLoadJob {
            std::string animationName;
            FileInfo fileInfo;
            std::future<Animation> future;
            bool awaitingWork = true;
        };

        struct TextureLoadJob {
            std::string textureName;
            std::string filePath;
            ImageDataType imageDataType = ImageDataType::UNDEFINED;
            std::future<ImageData> future;
            bool awaitingWork = true;
        };

        struct TextureUploadJob {
            std::string textureName;
        };

        LoadingComplete g_loadingComplete;
        uint32_t g_maxCompressedTextureResolution = 0;
        std::vector<std::string> g_loadLog = { "We are all alone on life's journey, held captive by the limitations of human consciousness.\n\n" };

        std::vector<AnimationLoadJob> g_animationLoadJobs;
        std::vector<ModelDataLoadJob> g_modelLoadJobs;
        std::vector<SkinnedModelDataLoadJob> g_skinnedModelLoadJobs;
        std::vector<TextureLoadJob> g_textureLoadJobs;
        std::vector<TextureUploadJob> g_textureUploadJobs;

        constexpr int MAX_ANIMATION_LOAD_JOBS_IN_FLIGHT = 8;
        constexpr int MAX_MODEL_LOAD_JOBS_IN_FLIGHT = 32;
        constexpr int MAX_SKINNED_MODEL_LOAD_JOBS_IN_FLIGHT = 32;
        constexpr int MAX_TEXTURE_LOAD_JOBS_IN_FLIGHT = 32;

        void CreateModelLoadJob(Model& model) {
            ModelDataLoadJob& modelLoadJob = g_modelLoadJobs.emplace_back();
            modelLoadJob.modelId = model.GetModelId();
            modelLoadJob.fileInfo = model.GetFileInfo();
        }

        void CreateSkinnedModelLoadJob(SkinnedModel& model) {
            SkinnedModelDataLoadJob& skinnedModelLoadJob = g_skinnedModelLoadJobs.emplace_back();
            skinnedModelLoadJob.skinnedModelId = model.GetSkinnedModelId();
            skinnedModelLoadJob.fileInfo = model.GetFileInfo();
        }

        void CreateTextureLoadJob(Texture& texture) {
            TextureLoadJob& textureLoadJob = g_textureLoadJobs.emplace_back();
            textureLoadJob.textureName = texture.GetFileName();
            textureLoadJob.filePath = texture.GetFilePath();
            textureLoadJob.imageDataType = texture.GetImageDataType();
        }

        LoadedModelData LoadModelData(FileInfo fileInfo) {
            LoadedModelData loadedData;
            const std::string modelPath = "res/models/" + fileInfo.name + "." + fileInfo.ext;
            const std::string bvhPath = "res/models/bvh/" + fileInfo.name + ".bvh";
            AssetFormats::LoadModel(modelPath, loadedData.modelData);
            AssetFormats::LoadModelBvh(bvhPath, loadedData.modelBvhData);
            return loadedData;
        }

        SkinnedModelData LoadSkinnedModelData(FileInfo fileInfo) {
            SkinnedModelData skinnedModelData;
            const std::string assetPath = "res/skinned_models/" + fileInfo.name + ".skinnedmodel";
            AssetFormats::LoadSkinnedModel(assetPath, skinnedModelData);
            return skinnedModelData;
        }

        Animation LoadAnimationFile(FileInfo fileInfo) {
            Animation animation;
            LoadAnimation(fileInfo, animation);
            return animation;
        }

        ImageData LoadTextureImageData(std::string path, ImageDataType imageDataType, uint32_t maxCompressedTextureResolution) {
            return ImageTools::LoadImageData(path, imageDataType, maxCompressedTextureResolution);
        }

        void PollAnimationJobs() {
            int activeJobCount = 0;

            for (const AnimationLoadJob& job : g_animationLoadJobs) {
                if (!job.awaitingWork) {
                    activeJobCount++;
                }
            }

            for (size_t i = 0; i < g_animationLoadJobs.size();) {
                AnimationLoadJob& job = g_animationLoadJobs[i];

                // Begin work if awaiting
                if (job.awaitingWork) {
                    if (activeJobCount >= MAX_ANIMATION_LOAD_JOBS_IN_FLIGHT) {
                        return;
                    }

                    job.awaitingWork = false;
                    job.future = std::async(std::launch::async, LoadAnimationFile, job.fileInfo);
                    activeJobCount++;
                    i++;
                    continue;
                }

                // Work is complete
                if (job.future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
                    Animation loadedAnimation = job.future.get();
                    Animation* animation = ResourceManager::GetAnimationPtr(job.animationName);

                    if (animation) {
                        if (!loadedAnimation.m_channels.empty()) {
                            *animation = std::move(loadedAnimation);
                            AddLoadLogItem("Loaded " + job.fileInfo.path);
                        }
                        else {
                            AddLoadLogItem("Failed to load " + job.fileInfo.path);
                        }
                    }
                    else {
                        Logging::Error() << "AssetLoader animation load job failed: animation '" << job.animationName << "' does not exist\n";
                    }

                    g_animationLoadJobs.erase(g_animationLoadJobs.begin() + i);
                    activeJobCount--;
                }
                else {
                    i++;
                }
            }
        }

        void PollModelJobs() {
            int activeJobCount = 0;

            for (const ModelDataLoadJob& job : g_modelLoadJobs) {
                if (!job.awaitingWork) {
                    activeJobCount++;
                }
            }

            for (size_t i = 0; i < g_modelLoadJobs.size();) {
                ModelDataLoadJob& job = g_modelLoadJobs[i];

                // Begin work if awaiting
                if (job.awaitingWork) {
                    if (activeJobCount >= MAX_MODEL_LOAD_JOBS_IN_FLIGHT) {
                        return;
                    }

                    job.awaitingWork = false;
                    job.future = std::async(std::launch::async, LoadModelData, job.fileInfo);
                    activeJobCount++;
                    i++;
                    continue;
                }

                // Work is complete
                if (job.future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
                    Model* model = ResourceManager::GetModelById(job.modelId);

                    if (model) {
                        LoadedModelData loadedData = job.future.get();
                        model->m_modelData = std::move(loadedData.modelData);
                        model->m_armatures = std::move(model->m_modelData.armatures);
                        model->m_modelBvhData = std::move(loadedData.modelBvhData);

                        const std::string red   = "";//[COL=1.0,0.0,0.0,1.0]";
                        const std::string white = "";//[COL=1.0,1.0,1.0,1.0]";
                        AddLoadLogItem(red + "Loaded " + job.fileInfo.path + white);
                    }
                    else {
                        Logging::Error() << "AssetLoader model load job failed: model id '" << job.modelId << "' does not exist\n";
                    }

                    g_modelLoadJobs.erase(g_modelLoadJobs.begin() + i);
                    activeJobCount--;
                }

                // Increment to check next job
                else {
                    i++;
                }
            }
        }

        void PollSkinnedModelJobs() {
            int activeJobCount = 0;

            for (const SkinnedModelDataLoadJob& job : g_skinnedModelLoadJobs) {
                if (!job.awaitingWork) {
                    activeJobCount++;
                }
            }

            for (size_t i = 0; i < g_skinnedModelLoadJobs.size();) {
                SkinnedModelDataLoadJob& job = g_skinnedModelLoadJobs[i];

                // Begin work if awaiting
                if (job.awaitingWork) {
                    if (activeJobCount >= MAX_SKINNED_MODEL_LOAD_JOBS_IN_FLIGHT) {
                        return;
                    }

                    job.awaitingWork = false;
                    job.future = std::async(std::launch::async, LoadSkinnedModelData, job.fileInfo);
                    activeJobCount++;
                    i++;
                    continue;
                }

                // Work is complete
                if (job.future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
                    SkinnedModel* skinnedModel = ResourceManager::GetSkinnedModelById(job.skinnedModelId);

                    if (skinnedModel) {
                        SkinnedModelData loadedData = job.future.get();
                        skinnedModel->m_skinnedModelData = std::move(loadedData);

                        const std::string green = "";//[COL=0.0,1.0,0.0,1.0]";
                        const std::string white = "";//[COL=1.0,1.0,1.0,1.0]";
                        AddLoadLogItem(green + "Loaded " + job.fileInfo.path + white);
                    }
                    else {
                        Logging::Error() << "AssetLoader skinned model load job failed: skinned model id '" << job.skinnedModelId << "' does not exist\n";
                    }

                    g_skinnedModelLoadJobs.erase(g_skinnedModelLoadJobs.begin() + i);
                    activeJobCount--;
                }

                // Increment to check next job
                else {
                    i++;
                }
            }
        }

        void PollTextureJobs() {
            int activeJobCount = 0;

            for (const TextureLoadJob& job : g_textureLoadJobs) {
                if (!job.awaitingWork) {
                    activeJobCount++;
                }
            }

            for (size_t i = 0; i < g_textureLoadJobs.size();) {
                TextureLoadJob& job = g_textureLoadJobs[i];

                // Begin work if awaiting
                if (job.awaitingWork) {
                    if (activeJobCount >= MAX_TEXTURE_LOAD_JOBS_IN_FLIGHT) {
                        return;
                    }

                    job.awaitingWork = false;
                    job.future = std::async(std::launch::async, LoadTextureImageData, job.filePath, job.imageDataType, g_maxCompressedTextureResolution);
                    activeJobCount++;
                    i++;
                    continue;
                }

                // Work is complete
                if (job.future.wait_for(std::chrono::seconds::zero()) == std::future_status::ready) {
                    Texture* texture = ResourceManager::GetTextureByName(job.textureName);

                    if (texture) {
                        ImageData imageData = job.future.get();

                        texture->SetImageData(std::move(imageData));

                        TextureUploader::QueueUpload(*texture);
                        g_textureUploadJobs.push_back(TextureUploadJob{ texture->GetFileName() });

                        const std::string blue =  "";//[COL=0.0,0.4,1.0,1.0]";
                        const std::string white = "";//[COL=1.0,1.0,1.0,1.0]";
                        AddLoadLogItem(blue + "Loaded " + job.filePath + white);
                    }
                    else {
                        Logging::Error() << "AssetLoader texture load job failed: texture '" << job.textureName << "' does not exist\n";
                    }

                    g_textureLoadJobs.erase(g_textureLoadJobs.begin() + i);
                    activeJobCount--;
                }

                // Increment to check next job
                else {
                    i++;
                }
            }
        }

        void PollTextureUploadJobs() {
            TextureUploader::ConsumeCompletedUploads();

            for (size_t i = 0; i < g_textureUploadJobs.size();) {
                Texture* texture = ResourceManager::GetTextureByName(g_textureUploadJobs[i].textureName);

                // Remove job if somehow the texture doesn't exist
                if (!texture) {
                    Logging::Error() << "AssetLoader texture upload job failed: texture '" << g_textureUploadJobs[i].textureName << "' does not exist\n";
                    g_textureUploadJobs.erase(g_textureUploadJobs.begin() + i);
                    continue;
                }

                // Remove job on success
                if (texture->GetUploadState() == UploadState::UPLOADED) {
                    AddLoadLogItem("Uploaded " + texture->GetFilePath());
                    g_textureUploadJobs.erase(g_textureUploadJobs.begin() + i);
                    continue;
                }

                // Remove job on failure
                if (texture->GetUploadState() == UploadState::FAILED) {
                    Logging::Error() << "Texture upload failed for'" << g_textureUploadJobs[i].textureName << "'\n";
                    g_textureUploadJobs.erase(g_textureUploadJobs.begin() + i);
                    continue;
                }

                // Increment to check next job
                ++i;
            }
        }

        void BlitLoadLog() {
            std::string text = "";
            int maxLinesDisplayed = 36;
            int endIndex = static_cast<int>(g_loadLog.size());
            int beginIndex = std::max(0, endIndex - maxLinesDisplayed);

            for (int i = beginIndex; i < endIndex; i++) {
                text += g_loadLog[i] + "\n";
            }

            UIBackEnd::BlitText(text, "StandardFont", 0, 0, Alignment::TOP_LEFT, 2.0f);
        }
    }

    void AddLoadLogItem(std::string text) {
        std::replace(text.begin(), text.end(), '\\', '/');
        g_loadLog.push_back(std::move(text));
    }

    std::vector<std::string>& GetLoadLog() {
        return g_loadLog;
    }

    void Init(uint32_t maxCompressedTextureResolution) {
        g_maxCompressedTextureResolution = maxCompressedTextureResolution;
        LoadMinimumRequiredAssets();
        DiscoverAssets();
        LoadBoneMasks();
        LoadIESFiles();
        LoadMidiFiles();
        LoadRagdollAssets();
        LoadSoundFonts();
        LoadVATFiles();
        LoadPointAnimations();
    }

    void OnLoadingComplete() {
        CreateSpriteSheets();
    }

    void LoadMinimumRequiredAssets() {
        // Required fonts
        for (FileInfo& fileInfo : File::IterateDirectory("res/fonts", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.SetImageData(Hell::ImageTools::LoadImageData(fileInfo.path, ImageDataType::UNCOMPRESSED));
            AddLoadLogItem("Loaded " + fileInfo.path);

            if (!TextureUploader::ImmediateUpload(texture)) {
                texture.SetUploadState(UploadState::FAILED);
                Logging::Error() << "AssetLoader::LoadRequired(..) failed to upload '" << fileInfo.path << "'\n";
                continue;
            }
        }

        // Required textures
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/required", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.SetImageData(Hell::ImageTools::LoadImageData(fileInfo.path, ImageDataType::UNCOMPRESSED));
            texture.RequestMipmaps();
            AddLoadLogItem("Loaded " + fileInfo.path);

            if (!TextureUploader::ImmediateUpload(texture)) {
                texture.SetUploadState(UploadState::FAILED);
                Logging::Error() << "AssetLoader::LoadRequired(..) failed to upload '" << fileInfo.path << "'\n";
                continue;
            }
        }

        // BRDF Texture for Indirect Specular
        FileInfo brdfFileInfo = File::GetInfo("res/textures/required/BrdfLut.dds");

        Texture& brdfTexture = ResourceManager::CreateTexture(brdfFileInfo.name);
        brdfTexture.SetFileInfo(brdfFileInfo);
        brdfTexture.SetImageDataType(ImageDataType::COMPRESSED);
        brdfTexture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
        brdfTexture.SetMinFilter(TextureFilter::LINEAR);
        brdfTexture.SetMagFilter(TextureFilter::LINEAR);

        ImageData brdfImageData = Hell::ImageTools::LoadImageData(brdfFileInfo.path, ImageDataType::COMPRESSED, g_maxCompressedTextureResolution);
        if (brdfImageData.format == ImageFormat::RGBA8_UNORM) {
            // FidelityFX SDK 1.1.4's Cauldron loader requests this DDS as sRGB
            // Overwriting format here so the sampled BRDF coefficients receive the same hardware sRGB to linear conversion the AMD demo
            brdfImageData.format = ImageFormat::RGBA8_SRGB;
        }
        brdfTexture.SetImageData(std::move(brdfImageData));
        AddLoadLogItem("Loaded " + brdfFileInfo.path);

        if (!TextureUploader::ImmediateUpload(brdfTexture)) {
            brdfTexture.SetUploadState(UploadState::FAILED);
            Logging::Error() << "AssetLoader::LoadRequired(..) failed to upload '" << brdfFileInfo.path << "'\n";
        }
    }

    void DiscoverAssets() {
        // Animations
        for (FileInfo& fileInfo : File::IterateDirectory("res/animations", { "fbx" })) {
            ResourceManager::CreateAnimation(fileInfo.name);

            AnimationLoadJob& animationLoadJob = g_animationLoadJobs.emplace_back();
            animationLoadJob.animationName = fileInfo.name;
            animationLoadJob.fileInfo = fileInfo;
        }

        // Models
        for (FileInfo& fileInfo : File::IterateDirectory("res/models")) {
            Model& model = ResourceManager::CreateModel(fileInfo.name);
            model.SetFileInfo(fileInfo);

            CreateModelLoadJob(model);
        }

        // Skinned models
        for (FileInfo& fileInfo : File::IterateDirectory("res/skinned_models")) {
            SkinnedModel& skinnedModel = ResourceManager::CreateSkinnedModel(fileInfo.name);
            skinnedModel.SetFileInfo(fileInfo);

            CreateSkinnedModelLoadJob(skinnedModel);
        }

        // Textures (uncompressed)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/uncompressed", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();

            CreateTextureLoadJob(texture);
        }

        // Textures (decals)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/decals", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_BORDER);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();

            CreateTextureLoadJob(texture);
        }

        // Textures (compressed)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/compressed", { "dds" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::COMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR_MIPMAP);
            texture.SetMagFilter(TextureFilter::LINEAR);
            texture.RequestMipmaps();

            CreateTextureLoadJob(texture);
        }

        // Textures (ui)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/ui", { "png", "jpg", })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            CreateTextureLoadJob(texture);
        }

        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/exr", { "exr" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::EXR);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::NEAREST);

            CreateTextureLoadJob(texture);
        }

        // Height map brush previews
        for (FileInfo fileInfo : File::IterateDirectory("res/textures/heightmap_brushes", { "exr" })) {
            const std::string textureName = "HeightMapBrush_" + fileInfo.name;
            fileInfo.name = textureName;
            Texture& texture = ResourceManager::CreateTexture(textureName);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::EXR);
            texture.SetTextureWrapMode(TextureWrapMode::CLAMP_TO_EDGE);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            CreateTextureLoadJob(texture);
        }

        // Textures (spritesheets)
        for (FileInfo& fileInfo : File::IterateDirectory("res/textures/spritesheets", { "png", "jpg", "tga" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::LINEAR);
            texture.SetMagFilter(TextureFilter::LINEAR);

            CreateTextureLoadJob(texture);
        }

        // VAT
        for (FileInfo& fileInfo : File::IterateDirectory("res/VAT/", { "exr" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::EXR);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::NEAREST);
            texture.SetMagFilter(TextureFilter::NEAREST);

            CreateTextureLoadJob(texture);
        }

        for (FileInfo& fileInfo : File::IterateDirectory("res/VAT/", { "png" })) {
            Texture& texture = ResourceManager::CreateTexture(fileInfo.name);
            texture.SetFileInfo(fileInfo);
            texture.SetImageDataType(ImageDataType::UNCOMPRESSED);
            texture.SetTextureWrapMode(TextureWrapMode::REPEAT);
            texture.SetMinFilter(TextureFilter::NEAREST);
            texture.SetMagFilter(TextureFilter::NEAREST);

            CreateTextureLoadJob(texture);
        }
    }

    void Update() {
        BlitLoadLog();

        if (g_loadingComplete.allComplete) {
            return;
        }

        PollAnimationJobs();
        PollModelJobs();
        PollSkinnedModelJobs();
        PollTextureJobs();
        PollTextureUploadJobs();

        const bool animationJobsComplete = g_animationLoadJobs.empty();
        const bool modelJobsComplete = g_modelLoadJobs.empty() && g_skinnedModelLoadJobs.empty();
        const bool textureJobsComplete = g_textureLoadJobs.empty() && g_textureUploadJobs.empty();

        if (animationJobsComplete && !g_loadingComplete.animations) {
            g_loadingComplete.animations = true;
            AddLoadLogItem("Loaded animations");
            Logging::Init() << "AssetLoader loaded animations";
            return;
        }

        if (modelJobsComplete) {
            if (!g_loadingComplete.bakedModels) {
                g_loadingComplete.bakedModels = true;
                BuildPrimitives();
                PreAllocateAssetGeometry();
                BakeModels();
                AddLoadLogItem("Baked models");
                Logging::Init() << "AssetLoader baked models";
                return;
            }

            if (!g_loadingComplete.bakedSkinnedModels) {
                g_loadingComplete.bakedSkinnedModels = true;
                BakeSkinnedModels();
                AddLoadLogItem("Baked skinned models");
                Logging::Init() << "AssetLoader baked skinned models";
                return;
            }

            if (!g_loadingComplete.modelBVHData) {
                g_loadingComplete.modelBVHData = true;
                CopyInAllLoadedModelBvhData();
                AddLoadLogItem("Loaded model BVH data");
                Logging::Init() << "AssetLoader loaded all BVH data";
                return;
            }
        }

        if (textureJobsComplete && !g_loadingComplete.spriteSheets) {
            g_loadingComplete.spriteSheets = true;
            AddLoadLogItem("Loaded textures");
            Logging::Init() << "AssetLoader loaded textures";
            OnLoadingComplete();
            return;
        }

        if (g_loadingComplete.animations &&
            g_loadingComplete.bakedModels &&
            g_loadingComplete.bakedSkinnedModels &&
            g_loadingComplete.modelBVHData &&
            g_loadingComplete.spriteSheets) {
            g_loadingComplete.allComplete = true;
        }
    }

    void PreAllocateAssetGeometry() {
        size_t vertexCount = 0;
        size_t indexCount = 0;
        size_t vertexWeightCount = 0;
        size_t morphDeltaCount = 0;

        for (const auto& modelEntry : Hell::ResourceManager::GetModels()) {
            const Model& model = modelEntry.second;
            for (const MeshData& meshData : model.m_modelData.meshes) {
                vertexCount += meshData.vertices.size();
                indexCount += meshData.indices.size();
            }
        }

        for (const auto& skinnedModelEntry : Hell::ResourceManager::GetSkinnedModels()) {
            const SkinnedModel& skinnedModel = skinnedModelEntry.second;
            for (const SkinnedMeshData& skinnedMeshData : skinnedModel.m_skinnedModelData.meshes) {
                vertexCount += skinnedMeshData.vertices.size();
                indexCount += skinnedMeshData.indices.size();

                if (skinnedMeshData.requiresSkinning) {
                    vertexWeightCount += skinnedMeshData.vertexWeights.size();
                }
                for (const MorphTargetData& morphTarget : skinnedMeshData.morphTargets) {
                    morphDeltaCount += morphTarget.positionDeltas.size();
                    morphDeltaCount += morphTarget.normalDeltas.size();
                    morphDeltaCount += morphTarget.tangentDeltas.size();
                }
            }
        }

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
        meshBuffer.PreAllocate(vertexCount, indexCount, vertexWeightCount, morphDeltaCount);
    }

    void BuildPrimitives() {
        Model* model = &Hell::ResourceManager::CreateModel("Primitives");
        model->m_modelData.name = "Primitives";
        model->m_modelData.aabbMin = glm::vec3(-0.5f, -0.5f, 0.0f);
        model->m_modelData.aabbMax = glm::vec3(0.5f, 0.5f, 0.0f);

        /* Quad */ {
            MeshData& meshData = model->m_modelData.meshes.emplace_back();
            meshData.name = "Quad";
            meshData.vertices = {
                // Position               Normal               UV            Tangent
                {{-0.5f, -0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-left
                {{ 0.5f, -0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Bottom-right
                {{ 0.5f,  0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}, // Top-right
                {{-0.5f,  0.5f, 0.0f},    {0.0f, 0.0f, 1.0f},  {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}}  // Top-left
            };
            meshData.indices = { 0, 1, 2, 2, 3, 0 };
            meshData.aabbMin = model->m_modelData.aabbMin;
            meshData.aabbMax = model->m_modelData.aabbMax;
            meshData.vertexCount = static_cast<uint32_t>(meshData.vertices.size());
            meshData.indexCount = static_cast<uint32_t>(meshData.indices.size());
        }
        model->m_modelData.meshCount = static_cast<uint32_t>(model->m_modelData.meshes.size());
    }

    void BakeModels() {
        std::vector<uint32_t> modelIds;
        for (const auto& [modelId, model] : Hell::ResourceManager::GetModels()) {
            modelIds.push_back(modelId);
        }
        std::sort(modelIds.begin(), modelIds.end());

        // Copy the vertices/indices into the asset manager
        for (uint32_t modelId : modelIds) {
            Model* modelPtr = Hell::ResourceManager::GetModelById(modelId);
            if (!modelPtr) {
                continue;
            }

            Model& model = *modelPtr;
            Hell::ResourceManager::SetModelName(model.GetModelId(), model.m_modelData.name);
            model.SetAABB(model.m_modelData.aabbMin, model.m_modelData.aabbMax);
            for (MeshData& meshData : model.m_modelData.meshes) {
                int meshId = CreateMesh(meshData.name, meshData.vertices, meshData.indices, meshData.aabbMin, meshData.aabbMax, meshData.parentIndex, meshData.localTransform, meshData.inverseBindTransform);
                model.AddMeshIndex(meshId);
            }
        }
    }

    void BakeSkinnedModels() {
        std::vector<uint32_t> skinnedModelIds;
        for (const auto& [skinnedModelId, skinnedModel] : Hell::ResourceManager::GetSkinnedModels()) {
            skinnedModelIds.push_back(skinnedModelId);
        }
        std::sort(skinnedModelIds.begin(), skinnedModelIds.end());

        for (uint32_t skinnedModelId : skinnedModelIds) {
            SkinnedModel* skinnedModelPtr = Hell::ResourceManager::GetSkinnedModelById(skinnedModelId);
            if (!skinnedModelPtr) {
                continue;
            }

            SkinnedModel& skinnedModel = *skinnedModelPtr;
            skinnedModel.BuildRuntimeData();

            for (SkinnedMeshData& skinnedMeshData : skinnedModel.m_skinnedModelData.meshes) {
                uint32_t meshId = CreateSkinnedMesh(skinnedMeshData);
                if (meshId != 0) {
                    skinnedModel.AddMeshIndex(meshId);
                }
            }
        }
    }

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax, int parentIndex, glm::mat4 localTransform, glm::mat4 inverseBindTransform) {
        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        const uint32_t meshId = meshBuffer.AddMesh(vertices, indices, name);
        Mesh* mesh = meshBuffer.GetMeshById(meshId);

        if (!mesh) {
            Logging::Error() << "AssetLoader::CreateMesh(..) failed to add mesh '" << name << "' to AssetGeometry\n";
            return -1;
        }

        mesh->aabbMin = aabbMin;
        mesh->aabbMax = aabbMax;
        mesh->extents = aabbMax - aabbMin;
        mesh->boundingSphereRadius = std::max(mesh->extents.x, std::max(mesh->extents.y, mesh->extents.z)) * 0.5f;
        mesh->parentIndex = parentIndex;
        mesh->localTransform = localTransform;
        mesh->inverseBindTransform = inverseBindTransform;

        return static_cast<int>(meshId);
    }

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        // Initialize AABB min and max with first vertex
        glm::vec3 aabbMin = vertices[0].position;
        glm::vec3 aabbMax = vertices[0].position;

        // Calculate AABB by iterating over all vertices
        for (const Vertex& v : vertices) {
            aabbMin = glm::min(aabbMin, v.position);
            aabbMax = glm::max(aabbMax, v.position);
        }

        return CreateMesh(name, vertices, indices, aabbMin, aabbMax, -1, glm::mat4(1.0f), glm::mat4(1.0f));
    }

    uint32_t CreateSkinnedMesh(const SkinnedMeshData& skinnedMeshData) {
        Hell::SkinnedMeshMetadata metadata;
        metadata.requiresSkinning = skinnedMeshData.requiresSkinning;
        metadata.nonDeformingBoneIndex = skinnedMeshData.nonDeformingBoneIndex;

        Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");

        const uint32_t meshId = meshBuffer.AddSkinnedMesh(skinnedMeshData.vertices, skinnedMeshData.indices, skinnedMeshData.vertexWeights, skinnedMeshData.morphTargets, metadata, skinnedMeshData.name);
        Mesh* mesh = meshBuffer.GetMeshById(meshId);

        if (!mesh) {
            Logging::Error() << "AssetLoader::CreateSkinnedMesh(..) failed to add mesh '" << skinnedMeshData.name << "' to AssetGeometry\n";
            return 0;
        }

        mesh->aabbMin = skinnedMeshData.aabbMin;
        mesh->aabbMax = skinnedMeshData.aabbMax;
        mesh->extents = mesh->aabbMax - mesh->aabbMin;
        mesh->boundingSphereRadius = std::max(mesh->extents.x, std::max(mesh->extents.y, mesh->extents.z)) * 0.5f;

        return meshId;
    }

    void CopyInAllLoadedModelBvhData() {
        std::vector<uint32_t> modelIds;
        for (const auto& [modelId, model] : Hell::ResourceManager::GetModels()) {
            modelIds.push_back(modelId);
        }
        std::sort(modelIds.begin(), modelIds.end());

        for (uint32_t modelId : modelIds) {
            Model* modelPtr = Hell::ResourceManager::GetModelById(modelId);
            if (!modelPtr) {
                continue;
            }

            Model& model = *modelPtr;
            // Skip primitives
            if (model.GetName() == "Primitives") continue;

            // Quick error check that bvh count matches mesh count
            if (model.m_modelBvhData.bvhs.size() != model.GetMeshCount()) {
                std::cout << "CopyInAllLoadedModelBvhData() error: bvh count does not equal mesh count for " << model.GetName() << "\n";
                continue;
            }

            // Iterate each preloaded MeshBvh and extract the data
            for (int i = 0; i < model.m_modelBvhData.bvhs.size(); i++) {
                MeshBvh& sourceMeshBvh = model.m_modelBvhData.bvhs[i];
                uint32_t meshId = model.GetMeshIndices()[i];

                Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);
                if (!mesh) {
                    std::cout << "CopyInAllLoadedModelBvhData() error: mesh with id " << meshId << " was invalid for " << model.GetName() << "\n";
                    continue;
                }

                // Swap data out of source MeshBvh and into the unordered map within BVH namespace, returning a new id
                mesh->meshBvhId = Hell::Bvh::CreateMeshBvhFromMeshBvh(sourceMeshBvh);
            }

            // Clean up
            model.m_modelBvhData.bvhs.clear();
        }
    }

    bool LoadingComplete() {
        return g_loadingComplete.allComplete;
    }
}
