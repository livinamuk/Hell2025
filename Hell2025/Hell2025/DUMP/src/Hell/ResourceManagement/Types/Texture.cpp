#include "Texture.h"

#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/MaterialBuilder.h"

#include <algorithm>
#include <cmath>
#include <iostream> // TODO clean up logging
#include <utility>

using namespace Hell;

void Texture::CleanUp() {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId != 0) {
            OpenGL::ResourceManager::RemoveTexture(m_openGLId);
            m_openGLId = 0;
        }
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        if (m_vulkanId != 0) {
            VulkanResourceManager::RemoveTexture(m_vulkanId);
            m_vulkanId = 0;
        }
    }
}

OpenGLTexture& Texture::GetGLTexture() {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId == 0) {
            m_openGLId = OpenGL::ResourceManager::CreateTexture();
        }

        return OpenGL::ResourceManager::GetTexture(m_openGLId);
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }

    Logging::Error() << "Texture::GetGLTexture() was called but API is not OpenGL\n";
    static OpenGLTexture invalid;
    return invalid;
}

VulkanTexture& Texture::GetVKTexture() {
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        if (m_vulkanId == 0) {
            m_vulkanId = VulkanResourceManager::CreateTexture();
        }

        return VulkanResourceManager::GetTexture(m_vulkanId);
    }

    Logging::Error() << "Texture::GetVKTexture() was called but API is not Vulkan\n";
    static VulkanTexture invalid;
    return invalid;
}

void Texture::FreeCPUMemory() {
    for (TextureMip& mip : m_imageData.mips) {
        mip.data.clear();
        mip.data.shrink_to_fit();
    }
}

const int Texture::GetWidth() {
    return GetMipMapWidth(0);
}

const int Texture::GetHeight() {
    return GetMipMapHeight(0);
}

const int Texture::GetMipMapWidth(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].width;
    }
    else {
        std::cout << "Texture::GetMipMapWidth(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const int Texture::GetMipMapHeight(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].height;
    }
    else {
        std::cout << "Texture::GetMipMapHeight(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const void* Texture::GetData(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return m_imageData.mips[mipmapLevel].data.data();
    }
    else {
        std::cout << "Texture::GetData(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return nullptr;
    }
}

const int Texture::GetDataSize(int mipmapLevel) {
    if (mipmapLevel >= 0 && mipmapLevel < m_imageData.mips.size()) {
        return static_cast<int>(m_imageData.mips[mipmapLevel].data.size());
    }
    else {
        std::cout << "Texture::GetDataSize(int mipmapLevel) failed. mipmapLevel '" << mipmapLevel << "' out of range of size " << m_imageData.mips.size() << "\n";
        return 0;
    }
}

const int Texture::GetChannelCount() {
    return GetImageFormatChannelCount(m_imageData.format);
}

size_t Texture::GetCPUAllocatedByteCount() const {
    size_t byteCount = 0;

    for (const TextureMip& mip : m_imageData.mips) {
        byteCount += mip.data.capacity() * sizeof(std::byte);
    }

    return byteCount;
}

size_t Texture::GetGPUAllocatedByteCount() const {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (OpenGLTexture* texture = OpenGL::ResourceManager::GetTexturePtr(m_openGLId)) {
            return texture->GetAllocatedByteCount();
        }
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        if (VulkanTexture* texture = VulkanResourceManager::GetTexturePtr(m_vulkanId)) {
            return texture->GetAllocatedByteCount();
        }
    }

    return 0;
}

void Texture::SetTextureDataLevelBakeState(int index, BakeState state) {
    if (index >= 0 && m_textureDataLevelBakeStates.size() && index < m_textureDataLevelBakeStates.size()) {
        m_textureDataLevelBakeStates[index] = state;
    }
    else {
        std::cout << "Texture::SetTextureDataLevelBakeState(int index, BakeState state) failed. Index '" << index << "' out of range of size " << m_textureDataLevelBakeStates.size() << "\n";
    }
}

void Texture::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
    MaterialBuilder::RegisterTexture(*this);
}

void Texture::SetImageData(ImageData imageData) {
    m_imageData = std::move(imageData);
    m_imageDataType = m_imageData.type;
    m_bakeComplete = false;
    m_uploadState = UploadState::NOT_REQUESTED;

    // Calculate mipmap level count
    if (!m_imageData.mips.empty()) {
        m_mipmapLevelCount = 1 + static_cast<int>(std::log2(std::max(GetWidth(), GetHeight())));
    }
    else {
        m_mipmapLevelCount = 0;
    }

    // Initiate bake states
    m_textureDataLevelBakeStates.assign(m_imageData.mips.size(), BakeState::AWAITING_BAKE);
}

void Texture::SetImageDataType(ImageDataType imageDataType) {
    m_imageDataType = imageDataType;
}

void Texture::SetUploadState(UploadState uploadState) {
    m_uploadState = uploadState;
}

void Texture::SetTextureWrapMode(TextureWrapMode wrapMode) {
    m_wrapModeS = wrapMode;
    m_wrapModeT = wrapMode;
}

void Texture::SetTextureWrapModeS(TextureWrapMode wrapMode) {
    m_wrapModeS = wrapMode;
}

void Texture::SetTextureWrapModeT(TextureWrapMode wrapMode) {
    m_wrapModeT = wrapMode;
}

void Texture::SetBorderColor(float r, float g, float b, float a) {
    m_borderColor = glm::vec4(r, g, b, a);
}

void Texture::SetMinFilter(TextureFilter filter) {
    m_minFilter = filter;
}

void Texture::SetMagFilter(TextureFilter filter) {
    m_magFilter = filter;
}

const BakeState Texture::GetTextureDataLevelBakeState(int index) {
    if (index >= 0 && m_textureDataLevelBakeStates.size() && index < m_textureDataLevelBakeStates.size()) {
        return m_textureDataLevelBakeStates[index];
    }
    else {
        std::cout << "Texture::GetTextureDataLevelBakeState(int index) failed. Index '" << index << "' out of range of size " << m_textureDataLevelBakeStates.size() << "\n";
        return BakeState::UNDEFINED;
    }
}

void Texture::CheckForBakeCompletion() {
    if (m_bakeComplete) {
        return;
    }
    else if (m_textureDataLevelBakeStates.empty()) {
        return;
    }
    else {
        m_bakeComplete = true;
        for (BakeState& state : m_textureDataLevelBakeStates) {
            if (state != BakeState::BAKE_COMPLETE) {
                m_bakeComplete = false;
                return;
            }
        }
    }
}

const bool Texture::BakeComplete() {
    return m_bakeComplete;
}

const int Texture::GetTextureDataCount() {
    return m_imageData.mips.size();
}

void Texture::RequestMipmaps() {
    m_mipmapsRequested = true;
}

const bool Texture::MipmapsAreRequested() {
    return m_mipmapsRequested;
}

const void Texture::PrintDebugInfo() {
    std::cout << GetFileName() << "\n";
    std::cout << " - width: " << GetWidth() << "\n";
    std::cout << " - height: " << GetHeight() << "\n";
    std::cout << " - channel count: " << GetChannelCount() << "\n";
    std::cout << " - format: " << ImageFormatToString(GetImageFormat()) << "\n";
    std::cout << " - mipmap level count: " << GetMipmapLevelCount() << "\n";
    std::cout << " - mipmaps requested: " << (MipmapsAreRequested() ? "TRUE" : "FALSE") << "\n";
    std::cout << " - data size:\n";
    for (int i = 0; i < GetTextureDataCount(); i++) {
        std::cout << "   mip " << i << " " << GetDataSize(i) << " at " << GetData(i) << "\n";
    }
    std::cout << "\n";
}
