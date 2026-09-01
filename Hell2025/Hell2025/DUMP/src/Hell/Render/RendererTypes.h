#pragma once

#include <cstdint>

struct BlitRegion {
    int32_t originX = 0;
    int32_t originY = 0;
    int32_t width = 0;
    int32_t height = 0;
};

struct BlitRect {
    int32_t x0 = 0;
    int32_t y0 = 0;
    int32_t x1 = 0;
    int32_t y1 = 0;
};
