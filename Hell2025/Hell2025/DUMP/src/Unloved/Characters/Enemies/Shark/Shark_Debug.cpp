#include "Shark.h"

#include "Unloved/Debug/DebugDraw.h"

namespace Unloved {

    std::string Shark::GetDebugInfoAsString() {
        return "\nShark\n";
    }

    void Shark::DrawDebug() {
        for (const glm::vec3& point : m_path) {
            DebugDraw::DrawPoint(point, RED);
        }

        const glm::vec3& p1 = m_spine.GetLeadPosition();
        DebugDraw::DrawPoint(p1, YELLOW);

        // Forward vector
        glm::vec3 p2 = p1 + m_forward;
        DebugDraw::DrawLine(p1, p2, YELLOW);
        DebugDraw::DrawPoint(p2, YELLOW);

        // Direction to target
        glm::vec3 a = p1 * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 b = m_targetPosition * glm::vec3(1.0f, 0.0f, 1.0f);
        glm::vec3 dirToTarget = glm::normalize(b - a);
        glm::vec3 p3 = p1 + dirToTarget;
        DebugDraw::DrawLine(p1, p3, GREEN);
        DebugDraw::DrawPoint(p3, GREEN);

        // Target XZ
        glm::vec3 p4 = m_targetPosition;
        p4.y = m_path[0].y;
        DebugDraw::DrawPoint(p4, WHITE);
    }

    void Shark::DrawSpinePoints() {
        for (uint32_t i = 1; i < m_spine.GetSegmentCount(); ++i) {
            DebugDraw::DrawPoint(m_spine.GetPosition(i), RED);
        }
        DebugDraw::DrawPoint(m_spine.GetLeadPosition(), WHITE);
    }
}
