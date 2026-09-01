#pragma once
#include <vector>

#define ORTHOGONAL_COST 10
#define DIAGONAL_COST 14

struct Cell {
    int x, y;
    bool obstacle;
    int g = 99999;  // G cost: distance from starting node
    int h = -1;     // H cost: distance from end node. Aka the heuristic.
    int f = -1;     // F cost: g + f
    Cell* neighbours[8];
    uint8_t neighbourCount = 0;
    Cell* parent = nullptr;
    int heapIndex = -1;

    inline int GetH(const Cell*) const { return h; }

    inline int GetF(const Cell* dest) {
        return g + GetH(dest);
    }
};