#pragma once

#include <cstdint>

struct DrawIndexedIndirectCommand {
    uint32_t indexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstIndex = 0;
    int32_t  baseVertex = 0;
    uint32_t baseInstance = 0;
};

struct DrawArraysIndirectCommand {
    uint32_t vertexCount = 0;
    uint32_t instanceCount = 0;
    uint32_t firstVertex = 0;
    uint32_t baseInstance = 0;
};

struct DispatchIndirectCommand {
    uint32_t num_groups_x;
    uint32_t num_groups_y;
    uint32_t num_groups_z;
};
