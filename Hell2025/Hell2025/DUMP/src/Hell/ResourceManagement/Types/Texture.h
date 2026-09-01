#pragma once

#include "Hell/Render/API/OpenGL/Types/GL_texture.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include "Hell/Common.h"
#include "Hell/File.h"
#include "Hell/Math/GLM.h"
#include "Hell/Render/TextureTypes.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>

struct Texture {
public:
    Texture() = default;
    void CleanUp();
    void SetUploadState(UploadState value);
    void SetFileInfo(FileInfo fileInfo);
    void SetImageData(ImageData imageData);
    void SetImageDataType(ImageDataType imageDataType);
    void SetTextureWrapMode(TextureWrapMode wrapMode);
    void SetTextureWrapModeS(TextureWrapMode wrapMode);
    void SetTextureWrapModeT(TextureWrapMode wrapMode);
    void SetBorderColor(float r, float g, float b, float a);
    void SetMinFilter(TextureFilter filter);
    void SetMagFilter(TextureFilter filter);
    void SetBindlessIndex(int32_t index) { m_bindlessIndex = index; }
    void SetTextureDataLevelBakeState(int index, BakeState state);
    void RequestMipmaps();
    void FreeCPUMemory();
    const void PrintDebugInfo();
    void CheckForBakeCompletion();
    const bool MipmapsAreRequested();
    const bool BakeComplete();
    const int GetTextureDataCount();
    const int GetWidth();
    const int GetHeight();
    const int GetMipMapWidth(int mipmapLevel);
    const int GetMipMapHeight(int mipmapLevel);
    const int GetDataSize(int mipmapLevel);
    const int GetChannelCount();
    const void* GetData(int mipmapLevel);
    size_t GetCPUAllocatedByteCount() const;
    size_t GetGPUAllocatedByteCount() const;
    const ImageData& GetImageData() const { return m_imageData; }
    const ImageFormat GetImageFormat() const { return m_imageData.format; }
    const BakeState GetTextureDataLevelBakeState(int index);

    OpenGLTexture& GetGLTexture();
    VulkanTexture& GetVKTexture();

    const int GetMipmapLevelCount()                  { return m_mipmapLevelCount; }
    const std::string& GetFileName() const           { return m_fileInfo.name; }
    const std::string& GetFilePath() const           { return m_fileInfo.path; }
    const FileInfo GetFileInfo() const               { return m_fileInfo; }
    const ImageDataType GetImageDataType() const     { return m_imageDataType; }
    const TextureWrapMode GetTextureWrapModeS() const { return m_wrapModeS; }
    const TextureWrapMode GetTextureWrapModeT() const { return m_wrapModeT; }
    const TextureFilter GetMinFilter() const         { return m_minFilter; }
    const TextureFilter GetMagFilter() const         { return m_magFilter; }
    const glm::vec4& GetBorderColor() const          { return m_borderColor; }
    uint64_t GetOpenGLId() const                     { return m_openGLId; }
    uint64_t GetVulkanId() const                     { return m_vulkanId; }
    int32_t GetBindlessIndex() const                 { return m_bindlessIndex; }
    UploadState GetUploadState() const               { return m_uploadState; }

private:
    UploadState m_uploadState = UploadState::NOT_REQUESTED;
    ImageDataType m_imageDataType = ImageDataType::UNDEFINED;
    TextureWrapMode m_wrapModeS = TextureWrapMode::REPEAT;
    TextureWrapMode m_wrapModeT = TextureWrapMode::REPEAT;
    TextureFilter m_minFilter = TextureFilter::NEAREST;
    TextureFilter m_magFilter = TextureFilter::NEAREST;
    FileInfo m_fileInfo;
    ImageData m_imageData;
    std::vector<BakeState> m_textureDataLevelBakeStates;
    uint64_t m_openGLId = 0;
    uint64_t m_vulkanId = 0;
    int32_t m_bindlessIndex = -1;
    int m_mipmapLevelCount = 0;
    bool m_mipmapsRequested = false;
    bool m_bakeComplete = false;
    glm::vec4 m_borderColor = glm::vec4(0.0f);
};
