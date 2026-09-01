#include "Kangaroo.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Pathfinding/AStarMap.h"

namespace Unloved {

    void Kangaroo::FindPathToTarget() {
        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);;
        glm::vec3 playerPosition = player->GetCameraPosition();

        glm::ivec2 start = AStarMap::GetCellCoordsFromWorldSpacePosition(m_position);
        glm::ivec2 end = AStarMap::GetCellCoordsFromWorldSpacePosition(playerPosition);
        m_aStar.InitSearch(start.x, start.y, end.x, end.y);
        m_aStar.FindPath();
    }
}
