#include "Kangaroo.h"
#include "Unloved/Debug/DebugDraw.h"

namespace Unloved {

    void Kangaroo::DebugDraw() {
        // Forward vector
        DebugDraw::DrawLine(m_position, m_position + m_forward, WHITE);
        DebugDraw::DrawPoint(m_position, RED);
        DebugDraw::DrawPoint(m_position + m_forward, RED);

        //std::cout << "Kangaroo forward: " << m_forward << "\n";
        //std::cout << "m_position: " << m_position << "\n";
        //std::cout << "m_position + m_forward: " << m_position + m_forward << "\n";
    }
}