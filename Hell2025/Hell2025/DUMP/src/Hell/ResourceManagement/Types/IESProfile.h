#pragma once
#include "Hell/Common.h"
#include "Hell/File/FileInfo.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct IESProfile;

namespace Hell::AssetLoader {
    IESProfile LoadIES(const std::string& path);
}

struct IESProfile {
    IESProfile() = default;
    IESProfile(const std::string& name);

    void SetFileInfo(const FileInfo& fileInfo) { m_fileInfo = fileInfo; }
    void SetTextureIndex(int32_t textureIndex) { m_textureIndex = textureIndex; }
    void PrintDebugInfo();

    float GetMinVerticalAngle() const;
    float GetMaxVerticalAngle() const;
    float GetMinHorizontalAngle() const;
    float GetMaxHorizontalAngle() const;
    float GetVerticalAngleRange() const;
    float GetHorizontalAngleRange() const;

    const std::string& GetName() const                 { return m_name; }
    const FileInfo& GetFileInfo() const                { return m_fileInfo; }
    const std::vector<float>& GetCandelaValues() const { return m_candelaValues; }
    int32_t GetTextureIndex() const                    { return m_textureIndex; }
    int32_t GetHorizontalAngleCount() const            { return m_horizontalAngleCount; }
    int32_t GetVerticalAngleCount() const              { return m_verticalAngleCount; }
    float GetMaxIntensity() const                      { return m_maxIntensity; }
    float GetVScale() const                            { return m_vScale; }
    float GetVBias() const                             { return m_vBias; }
    float GetHScale() const                            { return m_hScale; }
    float GetHBias() const                             { return m_hBias; }
    size_t GetCPUAllocatedByteCount() const;

private:
    friend IESProfile Hell::AssetLoader::LoadIES(const std::string& path);

    void RecalculateDerivedValues();

    std::string m_name = UNDEFINED_STRING;
    FileInfo m_fileInfo;
    std::vector<float> m_verticalAngles;
    std::vector<float> m_horizontalAngles;
    std::vector<float> m_candelaValues;
    int32_t m_horizontalAngleCount = 0;
    int32_t m_verticalAngleCount = 0;
    int32_t m_textureIndex = -1;
    float m_maxIntensity = 0.0f;
    float m_vScale = 0.0f;
    float m_vBias = 0.0f;
    float m_hScale = 0.0f;
    float m_hBias = 0.0f;
};
