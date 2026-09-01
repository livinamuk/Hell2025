#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string>
#include <vector>

struct AnimationVectorKey {
    glm::vec3 value = glm::vec3(0.0f);
    float time = 0.0f;
};

struct AnimationQuaternionKey {
    glm::quat value = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    float time = 0.0f;
};

struct AnimationChannel {
    std::string nodeName;
    std::vector<AnimationVectorKey> translationKeys;
    std::vector<AnimationQuaternionKey> rotationKeys;
    std::vector<AnimationVectorKey> scaleKeys;
};

struct ShapeAnimationChannel {
    std::string targetName;
    std::vector<std::string> meshObjectNames;
    std::vector<float> samples;
};

struct ShapeAnimationData {
    float framesPerSecond = 0.0f;
    float duration = 0.0f;
    int32_t frameStart = 0;
    int32_t frameEnd = 0;
    uint32_t sampleStepFrames = 1;
    std::vector<ShapeAnimationChannel> channels;
};

struct Animation {
    float m_duration = 0.0f;
    float m_ticksPerSecond = 25.0f;
    std::vector<AnimationChannel> m_channels;
    ShapeAnimationData m_shapeAnimation;
};
