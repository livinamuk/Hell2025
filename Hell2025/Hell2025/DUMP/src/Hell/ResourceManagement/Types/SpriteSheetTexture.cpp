#include "SpriteSheetTexture.h"
#include "Hell/ResourceManagement/Types/Texture.h"

void SpriteSheetTexture::Init(Texture& texture) {
    size_t lastUnderscore = m_fileInfo.name.find_last_of('_');
    m_textureName = m_fileInfo.name;
    size_t dot = m_fileInfo.name.find_last_of('.');
    std::string dims = m_fileInfo.name.substr(lastUnderscore + 1, dot - lastUnderscore - 1);
    size_t x = dims.find('x');
    m_columns = std::stoi(dims.substr(0, x));
    m_rows = std::stoi(dims.substr(x + 1));

    m_fullWidth = texture.GetWidth();
    m_fullHeight = texture.GetHeight();
    m_frameWidth = m_fullWidth / m_columns;
    m_frameHeight = m_fullHeight / m_rows;
    m_frameCount = m_rows * m_columns;
    m_textureIndex = texture.GetBindlessIndex();
}

void SpriteSheetTexture::SetFileInfo(FileInfo fileInfo) {
    m_fileInfo = fileInfo;
}

std::string SpriteSheetTexture::GetDebugInfo() {
    std::string text = m_textureName + "\n";
    text += "-rows: " + std::to_string(m_rows) + "\n";
    text += "-columns: " + std::to_string(m_columns) + "\n";
    text += "-fullWidth: " + std::to_string(m_fullWidth) + "\n";
    text += "-fullHeight: " + std::to_string(m_fullHeight) + "\n";
    text += "-frameWidth: " + std::to_string(m_frameWidth) + "\n";
    text += "-frameHeight: " + std::to_string(m_frameHeight) + "\n";
    text += "-frameCount: " + std::to_string(m_frameCount) + "\n";
    return text;
}
