#pragma once

#include "Hell/Render/TextureTypes.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct OpenGLTextureArray {
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

    uint32_t GetHandle()           { return m_handle; }
    uint32_t GetWidth()            { return m_width; }
    uint32_t GetHeight()           { return m_height; }
    uint32_t GetCount()            { return m_count; }
    uint32_t GetInternalFormat()   { return m_internalFormat; }
    uint32_t GetMipmapLevelCount() { return m_mipmapLevelCount; }
    size_t GetGPUAllocatedByteCount() const;

private:
    uint32_t m_handle = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_count = 0;
    uint32_t m_format = 0;
    uint32_t m_internalFormat = 0;
    uint32_t m_mipmapLevelCount = 0;
    uint32_t m_type = 0;
    bool m_memoryAllocated = false;
};
