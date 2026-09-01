#pragma once

#include <cstdint>

enum class API {
    OPENGL,
    VULKAN,
    UNDEFINED
};

enum class WindowedMode {
    WINDOWED,
    FULLSCREEN
};

enum class Alignment {
    CENTERED,
    CENTERED_HORIZONTAL,
    CENTERED_VERTICAL,
    TOP_LEFT,
    TOP_RIGHT,
    BOTTOM_LEFT,
    BOTTOM_RIGHT
};

enum class BakeState {
    AWAITING_BAKE,
    BAKING_IN_PROGRESS,
    BAKE_COMPLETE,
    UNDEFINED
};

enum class Axis {
    X,
    Y,
    Z,
    NONE,
};

enum class CameraView {
    PERSPECTIVE,
    ORTHO,
    FRONT,
    BACK,
    RIGHT,
    LEFT,
    TOP,
    BOTTOM,
    UNDEFINED
};

enum class LoadState : uint8_t {
    NOT_REQUESTED,
    QUEUED,
    LOADING,
    LOADED,
    FAILED
};

enum class UploadState : uint8_t {
    NOT_REQUESTED,
    QUEUED,
    UPLOADING,
    UPLOADED,
    FAILED
};
