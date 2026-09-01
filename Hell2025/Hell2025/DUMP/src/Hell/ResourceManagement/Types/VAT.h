#pragma once

#include "Hell/Common.h"
#include "Hell/File/FileInfo.h"

#include <glm/glm.hpp>

#include <cstddef>
#include <cstdint>
#include <string>

namespace Hell {

    struct VATMetadata {
        int frameCount = 0;
        float fps = 0.0f;
        glm::vec3 boundsMin = glm::vec3(0.0f);
        glm::vec3 boundsMax = glm::vec3(0.0f);
        std::string positionTexture;
        std::string rotationTexture;
        std::string lookupTexture;
        std::string model;
    };

    struct Vat {
        Vat() = default;
        Vat(const std::string& name);

        void SetFileInfo(const FileInfo& fileInfo)                  { m_fileInfo = fileInfo; }
        void SetMetadata(const VATMetadata& metadata)               { m_metadata = metadata; }
        void SetModelId(uint32_t modelId)                           { m_modelId = modelId; }

        const std::string& GetName() const                          { return m_name; }
        const FileInfo& GetFileInfo() const                         { return m_fileInfo; }
        const VATMetadata& GetMetadata() const                      { return m_metadata; }
        uint32_t GetModelId() const                                 { return m_modelId; }
        size_t GetCPUAllocatedByteCount() const;

    private:
        std::string m_name = UNDEFINED_STRING;
        FileInfo m_fileInfo;
        VATMetadata m_metadata;
        uint32_t m_modelId = 0;
    };

}
