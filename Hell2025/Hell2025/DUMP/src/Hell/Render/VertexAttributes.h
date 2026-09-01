#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

enum class VertexAttributeType {
    Float,
    Int,
    UnsignedInt
};

struct VertexAttribute {
    uint32_t location = 0;
    int32_t componentCount = 0;
    VertexAttributeType type = VertexAttributeType::Float;
    bool normalized = false;
    size_t offset = 0;
};

struct VertexLayoutDescription {
    size_t stride = 0;
    std::span<const VertexAttribute> attributes;
};

struct Vertex2D {
    glm::vec2 position;
    glm::vec2 uv = glm::vec2(0);
    glm::vec4 color = glm::vec4(1);

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 3> attributes = {
            VertexAttribute { 0, 2, VertexAttributeType::Float, false, offsetof(Vertex2D, position) },
            VertexAttribute { 1, 2, VertexAttributeType::Float, false, offsetof(Vertex2D, uv) },
            VertexAttribute { 2, 4, VertexAttributeType::Float, false, offsetof(Vertex2D, color) }
        };

        return { sizeof(Vertex2D), attributes };
    }
};

struct VertexPN {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 2> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(VertexPN, position) },
            VertexAttribute { 1, 3, VertexAttributeType::Float, false, offsetof(VertexPN, normal) }
        };

        return { sizeof(VertexPN), attributes };
    }
};

struct Vertex {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 normal = glm::vec3(0);
    glm::vec2 uv = glm::vec2(0);
    glm::vec3 tangent = glm::vec3(0);

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 4> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(Vertex, position) },
            VertexAttribute { 1, 3, VertexAttributeType::Float, false, offsetof(Vertex, normal) },
            VertexAttribute { 2, 2, VertexAttributeType::Float, false, offsetof(Vertex, uv) },
            VertexAttribute { 3, 3, VertexAttributeType::Float, false, offsetof(Vertex, tangent) }
        };

        return { sizeof(Vertex), attributes };
    }

    static VertexLayoutDescription GetPositionUVLayout() {
        static constexpr std::array<VertexAttribute, 2> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(Vertex, position) },
            VertexAttribute { 2, 2, VertexAttributeType::Float, false, offsetof(Vertex, uv) }
        };

        return { sizeof(Vertex), attributes };
    }
};

struct VertexWeight {
    glm::ivec4 boneID = glm::ivec4(0);
    glm::vec4 weight = glm::vec4(0);
};

struct DebugVertex3D {
    glm::vec3 position = glm::vec3(0);
    glm::vec3 color = glm::vec3(0);
    glm::ivec2 pixelOffset = glm::ivec2(0);
    int32_t depthEnabled = 0;
    int32_t exclusiveViewportIndex = -1;
    int32_t ignoredViewportIndex = -1;

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 6> attributes = {
            VertexAttribute { 0, 3, VertexAttributeType::Float, false, offsetof(DebugVertex3D, position) },
            VertexAttribute { 1, 3, VertexAttributeType::Float, false, offsetof(DebugVertex3D, color) },
            VertexAttribute { 2, 2, VertexAttributeType::Int, false, offsetof(DebugVertex3D, pixelOffset) },
            VertexAttribute { 3, 1, VertexAttributeType::Int, false, offsetof(DebugVertex3D, depthEnabled) },
            VertexAttribute { 4, 1, VertexAttributeType::Int, false, offsetof(DebugVertex3D, exclusiveViewportIndex) },
            VertexAttribute { 5, 1, VertexAttributeType::Int, false, offsetof(DebugVertex3D, ignoredViewportIndex) }
        };

        return { sizeof(DebugVertex3D), attributes };
    }
};

struct DebugVertex2D {
    glm::ivec2 position;
    glm::vec3 color;

    static VertexLayoutDescription GetLayout() {
        static constexpr std::array<VertexAttribute, 2> attributes = {
            VertexAttribute { 0, 2, VertexAttributeType::Int, false, offsetof(DebugVertex2D, position) },
            VertexAttribute { 1, 3, VertexAttributeType::Float, false, offsetof(DebugVertex2D, color) }
        };

        return { sizeof(DebugVertex2D), attributes };
    }
};
