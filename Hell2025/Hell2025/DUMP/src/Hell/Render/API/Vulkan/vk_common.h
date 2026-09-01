#pragma once
#include "volk/volk.h"

#include <cstdint>

inline constexpr uint32_t FRAME_OVERLAP = 2;

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#include "vk_mem_alloc.h"
