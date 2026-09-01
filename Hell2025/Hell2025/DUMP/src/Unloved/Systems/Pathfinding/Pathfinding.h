/*#pragma once
#include <vector>
#include <list>
#include <glm/glm.hpp>
#include "../Timer.hpp"

#define CELL_SIZE 32

#include "AStar.h"

namespace Pathfinding {

    void Init();
    void Update(float deltaTime);
    void ClearMap();
    void SaveMap();
    void LoadMap();
    void SetStart(int x, int y);;
    void SetTarget(int x, int y);
    void SetObstacle(int x, int y, bool value);
    bool IsInBounds(int x, int y);
    bool IsObstacle(int x, int y);
    bool HasLineOfSight(glm::vec2 startPosition, glm::vec2 endPosition);
    int GetMouseX();
    int GetMouseY();
    int GetMouseCellX();
    int GetMouseCellY();
    int GetMapWidth();
    int GetMapHeight();
    int GetStartX();
    int GetStartY();
    int GetTargetX();
    int GetTargetY();
    bool SlowModeEnabled();

    // Inline functions
    extern int g_mapWidth;
    extern int g_mapHeight;
    inline int Index1D(int x, int y)    { return y * g_mapWidth + x; }
    inline int GetCellCount()           { return g_mapWidth * g_mapHeight; }

    AStar& GetAStar();
}*/