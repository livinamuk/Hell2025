#include "AStar.h"
#include "AStarMap.h"
#include "Hell/Backend/BackEnd.h"
#include "Timer.hpp"

void AStar::InitGrid() {
    ClearData();
    m_gridInitialized = false;

    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        return;
    }

    m_mapWidth = AStarMap::GetMapWidth();
    m_mapHeight = AStarMap::GetMapHeight();

    m_cells.clear();
    m_cells.resize(AStarMap::GetCellCount());

    for (int i = 0; i < m_cells.size(); i++) {
        int x = i % m_mapWidth;
        int y = i / m_mapWidth;

        Cell& cell = m_cells[i];
        cell.x = x;
        cell.y = y;
        cell.obstacle = AStarMap::IsCellObstacle(x, y);
        cell.neighbourCount = 0;
    }

    for (int i = 0; i < m_cells.size(); i++) {
        FindNeighbours(i);
    }

    m_openList.AllocateSpace(m_mapWidth * m_mapHeight);
    m_closedFlags.resize(m_mapWidth * m_mapHeight);
    m_gridInitialized = true;
}

void AStar::InitSearch(int startX, int startY, int destinationX, int destinationY) {
    //Timer timer("InitSearch()");

    ClearData();

    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        return;
    }

    if (!m_gridInitialized ||
        m_mapWidth != AStarMap::GetMapWidth() ||
        m_mapHeight != AStarMap::GetMapHeight()) {
        InitGrid();
    }

    if (!AStarMap::IsInBounds(startX, startY) ||
        !AStarMap::IsInBounds(destinationX, destinationY)) {
        return;
    }

    for (Cell& cell : m_cells) {
        cell.g = 99999;
        cell.f = -1;
        cell.parent = nullptr;
        cell.heapIndex = -1;

        // Precompute H
        int dx = std::abs(cell.x - destinationX);
        int dy = std::abs(cell.y - destinationY);
        cell.h = ORTHOGONAL_COST * (dx + dy) + (DIAGONAL_COST - 2 * ORTHOGONAL_COST) * std::min(dx, dy);
    }

    int idxStart = Index1D(startX, startY);
    int idxDestination = Index1D(destinationX, destinationY);

    m_start = &m_cells[idxStart];
    m_destination = &m_cells[idxDestination];
    m_current = m_start;
    m_start->g = 0;
    m_start->GetF(m_destination);
    m_searchInitilized = true;

    m_openList.Clear();
    m_openList.AddItem(m_start);

    // Init start cell
    m_start->g = 0;
    m_start->f = m_start->g + m_start->h;

    // initialize closed list to false
    m_closedFlags.assign(m_mapWidth * m_mapHeight, false);
}

void AStar::ClearData() {
    m_closedFlags.clear();
    m_finalPath.clear();
    m_openList.Clear();
    m_intersectionPoints.clear();
    m_gridPathFound = false;
    m_smoothPathFound = false;
    m_searchInitilized = false;
    m_smoothSearchIndex = 0;
    m_start = nullptr;
    m_destination = nullptr;
    m_current = nullptr;
}

bool AStar::GridPathFound() {
    return m_gridPathFound;
}

bool AStar::SmoothPathFound() {
    return m_smoothPathFound;
}

bool AStar::SearchInitilized() {
    return m_searchInitilized;
}

void AStar::FindPath() {
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        return;
    }

    if (m_destination && m_destination->obstacle) {
        return;
    }
    if (m_openList.IsEmpty()) {
        return;
    }
    if (m_gridPathFound) {
        return;
    }

    while (!m_openList.IsEmpty())
    {
        m_current = m_openList.RemoveFirst();
        if (IsDestination(m_current)) {
            m_gridPathFound = true;
            BuildFinalPath();
            //PrintPath();
            return;
        }

        // Add current cell to closed list
        int idxCurrent = Index1D(m_current->x, m_current->y);
        m_closedFlags[idxCurrent] = true;

        for (int i = 0; i < m_current->neighbourCount; ++i) {
            Cell* neighbour = m_current->neighbours[i];
            int new_g = IsOrthogonal(m_current, neighbour)
                ? m_current->g + ORTHOGONAL_COST
                : m_current->g + DIAGONAL_COST;

            int idxN = Index1D(neighbour->x, neighbour->y);

            if (m_closedFlags[idxN] || m_openList.Contains(neighbour)) {
                if (new_g < neighbour->g) {
                    neighbour->g = new_g;
                    neighbour->f = new_g + neighbour->GetH(m_destination);
                    neighbour->parent = m_current;
                }
            }
            else {
                neighbour->g = new_g;
                neighbour->f = new_g + neighbour->GetH(m_destination);
                neighbour->parent = m_current;
                if (!m_openList.Contains(neighbour)) {
                    m_openList.AddItem(neighbour);
                }
            }
        }
    }
}

void AStar::PrintPath() {

    for (int y = 0; y < m_mapHeight; ++y) {
        for (int x = 0; x < m_mapWidth; ++x) {

            bool inPath = false;
            for (Cell* cell : m_finalPath) {
                if (cell->x == x && cell->y == y) {
                    inPath = true;
                    break;
                }
            }

            int idx = Index1D(x, y);

            if (x == m_start->x && y == m_start->y) {
                std::cout << "A";
            }
            else if (x == m_destination->x && y == m_destination->y) {
                std::cout << "B";
            }
            else if (inPath) {
                std::cout << ".";
            }
            else if (AStarMap::IsCellObstacle(x, y)) {
                std::cout << char(219);
            }
            else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
}

MinHeap& AStar::GetOpenList() {
    return m_openList;
}

std::vector<Cell*>& AStar::GetPathCellVersion() {
    return m_finalPath;
}

// Optimize me!!!!
std::vector<glm::ivec2> AStar::GetPath() {
    std::vector<glm::ivec2> path;
    for (Cell* cell : m_finalPath) {
        path.push_back(glm::ivec2(cell->x, cell->y));
    }
    return path;
}

bool AStar::IsDestination(Cell* cell) {
    return cell == m_destination;
}

void AStar::BuildFinalPath() {
    Cell* cell = m_destination;
    while (cell != m_start) {
        m_finalPath.push_back(cell);
        cell = cell->parent;
    }
    std::reverse(m_finalPath.begin(), m_finalPath.end());

    // Init smooth search
    m_smoothPathFound = false;
    m_smoothSearchIndex = 2;
    m_intersectionPoints.clear();
    glm::vec2 startPoint = glm::vec2(m_start->x + 0.5f, m_start->y + 0.5f);
    glm::vec2 endPoint = glm::vec2(m_destination->x + 0.5f, m_destination->y + 0.5f);
    m_intersectionPoints.push_back(startPoint);
    for (int j = 0; j < m_finalPath.size(); j++) {
        Cell* cell = m_finalPath[j];
        m_intersectionPoints.push_back(glm::vec2(cell->x, cell->y));
    }
    m_intersectionPoints.push_back(endPoint);
}

//void AStar::FindSmoothPath() {
//    if (!m_gridPathFound) {
//        return;
//    }
//    if (m_smoothSearchIndex >= m_intersectionPoints.size()) {
//        m_smoothPathFound = true;
//    }
//    if (m_smoothPathFound) {
//        return;
//    }
//    // Remove points with line of sight
//    glm::vec2 currentPosition = m_intersectionPoints[m_smoothSearchIndex];
//    glm::vec2 queryPosition = m_intersectionPoints[m_smoothSearchIndex - 2];
//    glm::vec2 intersectionPointOut;
//    if (Pathfinding::HasLineOfSight(currentPosition, queryPosition)) {
//        m_intersectionPoints.erase(m_intersectionPoints.begin() + m_smoothSearchIndex - 1);
//    }
//    else {
//        m_smoothSearchIndex++;
//    }
//    if (!Pathfinding::SlowModeEnabled()) {
//        FindSmoothPath();
//    }
//}

bool AStar::IsOrthogonal(Cell* cellA, Cell* cellB) {
    return (cellA->x == cellB->x || cellA->y == cellB->y);
}

bool AStar::IsInClosedList(Cell* cell) {
    int idx = Index1D(cell->x, cell->y);
    return m_closedFlags[idx];
}

void AStar::FindNeighbours(int idx) {
    int w = m_mapWidth;
    int h = m_mapHeight;
    Cell& cell = m_cells[idx];

    // If already cached, skip
    if (cell.neighbourCount != 0)
        return;

    int x = idx % w;
    int y = idx / w;

    cell.neighbourCount = 0;

    // North
    if (y > 0) {
        int idxN = idx - w;
        if (!m_cells[idxN].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxN];
    }
    // South
    if (y + 1 < h) {
        int idxS = idx + w;
        if (!m_cells[idxS].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxS];
    }
    // West
    if (x > 0) {
        int idxW = idx - 1;
        if (!m_cells[idxW].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxW];
    }
    // East
    if (x + 1 < w) {
        int idxE = idx + 1;
        if (!m_cells[idxE].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxE];
    }
    // Northwest
    if (x > 0 && y > 0) {
        int idxNW = idx - w - 1;
        if (!m_cells[idxNW].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxNW];
    }
    // Northeast
    if (x + 1 < w && y > 0) {
        int idxNE = idx - w + 1;
        if (!m_cells[idxNE].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxNE];
    }
    // Southwest
    if (x > 0 && y + 1 < h) {
        int idxSW = idx + w - 1;
        if (!m_cells[idxSW].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxSW];
    }
    // Southeast
    if (x + 1 < w && y + 1 < h) {
        int idxSE = idx + w + 1;
        if (!m_cells[idxSE].obstacle)
            cell.neighbours[cell.neighbourCount++] = &m_cells[idxSE];
    }
}
