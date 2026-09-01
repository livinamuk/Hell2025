#include "MinHeap.h"
#include <utility> // std::swap

void MinHeap::AllocateSpace(int size) {
    items = std::vector<Cell*>(size);
}

void MinHeap::AddItem(Cell* item) {
    item->heapIndex = currentItemCount;
    items[currentItemCount] = item;
    SortUp(item);
    currentItemCount++;
}

void MinHeap::Update(Cell* cell) {
    SortUp(cell);
}

bool MinHeap::Contains(Cell* cell) {
    return cell &&
           cell->heapIndex >= 0 &&
           cell->heapIndex < currentItemCount &&
           items[cell->heapIndex] == cell;
}

bool MinHeap::IsEmpty() {
    return (currentItemCount == 0);
}

int MinHeap::GetItemCount() {
    return currentItemCount;
}

void MinHeap::Clear() {
    currentItemCount = 0;
}

void MinHeap::SortUp(Cell* item) {
    int idx = item->heapIndex;
    int parent = (idx - 1) >> 1;
    while (idx > 0) {
        Cell* p = items[parent];
        if (p->f > item->f) {
            Swap(p, item);
            idx = item->heapIndex;
            parent = (idx - 1) >> 1;
        }
        else break;
    }
}

inline void MinHeap::Swap(Cell* a, Cell* b) {
    std::swap(items[a->heapIndex], items[b->heapIndex]);
    std::swap(a->heapIndex, b->heapIndex);
}

Cell* MinHeap::RemoveFirst() {
    Cell* firstItem = items[0];
    currentItemCount--;
    if (currentItemCount > 0) {
        items[0] = items[currentItemCount];
        items[0]->heapIndex = 0;
        SortDown(items[0]);
    }
    firstItem->heapIndex = -1;
    return firstItem;
}

void MinHeap::SortDown(Cell* cell) {
    int idx = cell->heapIndex;
    while (true) {
        int left = (idx << 1) + 1;
        int right = left + 1;
        int swapIdx = -1;
        if (left < currentItemCount) {
            swapIdx = left;
            if (right < currentItemCount
                && items[right]->f < items[left]->f)
                swapIdx = right;
            if (items[swapIdx]->f < cell->f) {
                Swap(items[swapIdx], cell);
                idx = cell->heapIndex;
                continue;
            }
        }
        break;
    }
}
