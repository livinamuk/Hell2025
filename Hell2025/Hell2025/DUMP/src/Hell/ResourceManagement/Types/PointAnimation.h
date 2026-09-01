#pragma once

#include "Hell/Common.h"
#include "Hell/File/FileInfo.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace Hell {

    struct PointAnimationMetadata {
        float fps = 0.0f;
        int frameCount = 0;
        int pointCount = 0;
    };

    struct PointAnimation {
        PointAnimation() = default;
        PointAnimation(const std::string& name);

        void SetFileInfo(const FileInfo& fileInfo)                              { m_fileInfo = fileInfo; }
        void SetMetadata(const PointAnimationMetadata& metadata)                { m_metadata = metadata; }
        void SetFrames(std::vector<std::vector<glm::vec3>>&& frames);

        const std::string& GetName() const                                      { return m_name; }
        const FileInfo& GetFileInfo() const                                     { return m_fileInfo; }
        const PointAnimationMetadata& GetMetadata() const                       { return m_metadata; }
        const std::vector<std::vector<glm::vec3>>& GetFrames() const            { return m_frames; }
        const std::vector<glm::vec3>& GetFrame(int frameIndex) const;
        int GetFrameCount() const                                               { return m_metadata.frameCount; }
        int GetPointCount() const                                               { return m_metadata.pointCount; }
        bool HasFrames() const                                                  { return !m_frames.empty(); }
        size_t GetCPUAllocatedByteCount() const;

    private:
        std::string m_name = UNDEFINED_STRING;
        FileInfo m_fileInfo;
        PointAnimationMetadata m_metadata;
        std::vector<std::vector<glm::vec3>> m_frames;
    };

}
