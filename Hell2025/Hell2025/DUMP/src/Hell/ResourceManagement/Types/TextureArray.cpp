#include "TextureArray.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"

namespace Hell {

TextureArray::TextureArray(const std::string& name) {
    m_name = name;
}

void TextureArray::AllocateMemory(uint32_t width, uint32_t height, uint32_t internalFormat, uint32_t mipmapLevelCount, uint32_t count) {
    GetGLTextureArray().AllocateMemory(width, height, internalFormat, mipmapLevelCount, count);
}

void TextureArray::CleanUp() {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId != 0) {
            OpenGL::ResourceManager::RemoveTextureArray(m_openGLId);
            m_openGLId = 0;
        }
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        VulkanResourceManager::RemoveAllocatedImage(m_name);
        m_vulkanId = 0;
    }
}

void TextureArray::SetLayerDataR16(uint32_t layerIndex, const std::vector<float>& data) {
    GetGLTextureArray().SetLayerDataR16(layerIndex, data);
}

void TextureArray::GenerateMipmaps() {
    GetGLTextureArray().GenerateMipmaps();
}

void TextureArray::SetWrapMode(TextureWrapMode wrapMode) {
    GetGLTextureArray().SetWrapMode(wrapMode);
}

void TextureArray::SetMinFilter(TextureFilter filter) {
    GetGLTextureArray().SetMinFilter(filter);
}

void TextureArray::SetMagFilter(TextureFilter filter) {
    GetGLTextureArray().SetMagFilter(filter);
}

void TextureArray::Clear(float r, float g, float b, float a) {
    GetGLTextureArray().Clear(r, g, b, a);
}

void TextureArray::ClearLayer(float r, float g, float b, float a, int layerIndex) {
    GetGLTextureArray().ClearLayer(r, g, b, a, layerIndex);
}

void TextureArray::ClearAllMipLevels(float r, float g, float b, float a) {
    GetGLTextureArray().ClearAllMipLevels(r, g, b, a);
}

OpenGLTextureArray& TextureArray::GetGLTextureArray() {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId == 0) {
            m_openGLId = OpenGL::ResourceManager::CreateTextureArray(m_name);
        }

        return OpenGL::ResourceManager::GetTextureArrayById(m_openGLId);
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        // No OpenGL texture exists on Vulkan
    }

    Logging::Error() << "TextureArray::GetGLTextureArray() was called but API is not OpenGL\n";
    static OpenGLTextureArray invalid;
    return invalid;
}

uint32_t TextureArray::GetHandle() {
    return GetGLTextureArray().GetHandle();
}

uint32_t TextureArray::GetWidth() {
    return GetGLTextureArray().GetWidth();
}

uint32_t TextureArray::GetHeight() {
    return GetGLTextureArray().GetHeight();
}

uint32_t TextureArray::GetCount() {
    return GetGLTextureArray().GetCount();
}

uint32_t TextureArray::GetInternalFormat() {
    return GetGLTextureArray().GetInternalFormat();
}

uint32_t TextureArray::GetMipmapLevelCount() {
    return GetGLTextureArray().GetMipmapLevelCount();
}

size_t TextureArray::GetGPUAllocatedByteCount() const {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (OpenGLTextureArray* textureArray = OpenGL::ResourceManager::GetTextureArrayPtrById(m_openGLId)) {
            return textureArray->GetGPUAllocatedByteCount();
        }
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        if (VulkanResourceManager::AllocatedImageExists(m_name)) {
            AllocatedImage* image = VulkanResourceManager::GetAllocatedImage(m_name);
            return image ? image->GetGPUAllocatedByteCount() : 0;
        }
    }

    return 0;
}

} // namespace Hell
