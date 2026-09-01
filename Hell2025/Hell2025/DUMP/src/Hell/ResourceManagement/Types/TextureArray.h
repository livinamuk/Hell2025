#pragma once

#include "Hell/Common.h"
#include "Hell/Render/API/OpenGL/Types/GL_texture_array.h"
#include "Hell/Render/TextureTypes.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace Hell {

struct TextureArray {
    TextureArray() = default;
    TextureArray(const std::string& name);
    TextureArray(const TextureArray&) = delete;
    TextureArray& operator=(const TextureArray&) = delete;
    TextureArray(TextureArray&&) noexcept = default;
    TextureArray& operator=(TextureArray&&) noexcept = default;
    ~TextureArray() = default;

    void AllocateMemory(uint32_t width, uint32_t height, uint32_t internalFormat, uint32_t mipmapLevelCount, uint32_t count);
    void CleanUp();
    void SetLayerDataR16(uint32_t layerIndex, const std::vector<float>& data);
    void GenerateMipmaps();
    void SetWrapMode(TextureWrapMode wrapMode);
    void SetMinFilter(TextureFilter filter);
    void SetMagFilter(TextureFilter filter);
    void Clear(float r, float g, float b, float a);
    void ClearLayer(float r, float g, float b, float a, int layerIndex);
    void ClearAllMipLevels(float r, float g, float b, float a);

    OpenGLTextureArray& GetGLTextureArray();

    uint32_t GetHandle();
    uint32_t GetWidth();
    uint32_t GetHeight();
    uint32_t GetCount();
    uint32_t GetInternalFormat();
    uint32_t GetMipmapLevelCount();
    size_t GetGPUAllocatedByteCount() const;

    const std::string& GetName() const { return m_name; }
    uint64_t GetOpenGLId() const { return m_openGLId; }

private:
    std::string m_name = UNDEFINED_STRING;
    uint64_t m_openGLId = 0;
    uint64_t m_vulkanId = 0;
};

} // namespace Hell
