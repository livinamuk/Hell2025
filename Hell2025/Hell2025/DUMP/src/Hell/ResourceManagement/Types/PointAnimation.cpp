#include "PointAnimation.h"

#include <algorithm>
#include <utility>

namespace Hell {

    PointAnimation::PointAnimation(const std::string& name) {
        m_name = name;
    }

    void PointAnimation::SetFrames(std::vector<std::vector<glm::vec3>>&& frames) {
        m_frames = std::move(frames);

        if (m_metadata.frameCount == 0) {
            m_metadata.frameCount = static_cast<int>(m_frames.size());
        }

        if (m_metadata.pointCount == 0) {
            for (const std::vector<glm::vec3>& frame : m_frames) {
                m_metadata.pointCount = std::max(m_metadata.pointCount, static_cast<int>(frame.size()));
            }
        }
    }

    const std::vector<glm::vec3>& PointAnimation::GetFrame(int frameIndex) const {
        static const std::vector<glm::vec3> emptyFrame;

        if (frameIndex < 0 || static_cast<size_t>(frameIndex) >= m_frames.size()) {
            return emptyFrame;
        }

        return m_frames[frameIndex];
    }

    size_t PointAnimation::GetCPUAllocatedByteCount() const {
        size_t byteCount = m_name.capacity() +
            m_fileInfo.path.capacity() +
            m_fileInfo.name.capacity() +
            m_fileInfo.ext.capacity() +
            m_fileInfo.dir.capacity() +
            m_frames.capacity() * sizeof(std::vector<glm::vec3>);

        for (const std::vector<glm::vec3>& frame : m_frames) {
            byteCount += frame.capacity() * sizeof(glm::vec3);
        }

        return byteCount;
    }

}
