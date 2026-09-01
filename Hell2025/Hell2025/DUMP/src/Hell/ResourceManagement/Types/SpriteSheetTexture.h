#pragma once
#include "Hell/File/FileInfo.h"

#include <cstdint>
#include <string>

struct Texture;

struct SpriteSheetTexture {
    SpriteSheetTexture() = default;
    void Init(Texture& texture);
    void SetFileInfo(FileInfo fileInfo);

    std::string GetDebugInfo();
    FileInfo GetFileInfo()  { return m_fileInfo; }
    int GetColumnCount()    { return m_columns; }
    int GetFrameCount()     { return m_frameCount; }
    int GetRowCount()       { return m_rows; }
    int GetTextureIndex()   { return m_textureIndex; }

private:
    FileInfo m_fileInfo;
    std::string m_textureName;
    uint32_t m_textureIndex = 0;
    uint32_t m_rows = 0;
    uint32_t m_columns = 0;
    int32_t m_fullWidth = 0;
    int32_t m_fullHeight = 0;
    int32_t m_channelCount = 0;
    uint32_t m_frameWidth = 0;
    uint32_t m_frameHeight = 0;
    uint32_t m_frameCount = 0;
};
