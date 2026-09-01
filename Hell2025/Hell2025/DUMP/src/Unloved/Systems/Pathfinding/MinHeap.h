#pragma once
#include "Cell.h"
#include <vector>

struct MinHeap {
    std::vector<Cell*> items;
    int currentItemCount = 0;
    void AllocateSpace(int size);
    void AddItem(Cell* item);
    void Update(Cell* cell);
    bool Contains(Cell* cell);
    bool IsEmpty();
    int GetItemCount();
    void Clear();
    void SortUp(Cell* item);
    void Swap(Cell* cellA, Cell* cellB);
    Cell* RemoveFirst();
    void SortDown(Cell* cell);
};
