#pragma once
#include <vector>
#include "Cell.h"
#include "MinHeap.h"
#include "glm/glm.hpp"

struct AStar {
    void InitGrid();
    void InitSearch(int startX, int startY, int destinationX, int destinationY);
    void FindPath();
    void FindSmoothPath();
    void ClearData();
    bool GridPathFound();
    bool SmoothPathFound();
    bool SearchInitilized();
    void PrintPath();
    std::vector<Cell*>& GetPathCellVersion();
    MinHeap& GetOpenList();

    std::vector<glm::ivec2> GetPath();


    bool m_searchInProgress = false;
    int m_maxCellsSearchedPerFrame = 100;

    Cell* m_start = nullptr;
    Cell* m_destination = nullptr;
    Cell* m_current = nullptr;
    MinHeap m_openList;
    std::vector<Cell> m_cells;
    std::vector<Cell*> m_finalPath;
    std::vector<glm::vec2> m_intersectionPoints;
    std::vector<bool> m_closedFlags;

    const std::vector<bool>& GetClosedFlags() { return m_closedFlags; }

private:
    void BuildFinalPath();
    void FindNeighbours(int idx);
    bool IsDestination(Cell* cell);
    bool IsOrthogonal(Cell* cellA, Cell* cellB);
    bool IsInClosedList(Cell* cell);

    inline int Index1D(int x, int y) { return y * m_mapWidth + x; }

    int m_smoothSearchIndex = 0;
    bool m_gridPathFound = false;
    bool m_smoothPathFound = false;
    bool m_searchInitilized = false;
    bool m_gridInitialized = false;

    int m_mapWidth = 0;
    int m_mapHeight = 0;

};
