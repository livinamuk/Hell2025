#pragma once
#include "Hell/ResourceManagement/Types/SpriteSheetTexture.h"

#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Render/RendererTypes.h"

#include <string>

namespace Unloved {

struct SpriteSheetObject {
public:
    SpriteSheetObject() = default;
    SpriteSheetObject(const SpriteSheetObjectCreateInfo& createInfo);

    void Init(const SpriteSheetObjectCreateInfo& createInfo);
    void Update(float deltaTime);
    void SetPosition(glm::vec3 position);
    void SetRotation(glm::vec3 position);
    void SetUOffset(float value);
    void SetVOffset(float value);
    void SetScale(glm::vec3 scale);
    void SetTime(float time);
    void SetSpeed(float speed);
    void EnableRendering();
    void DisableRendering();

    const bool IsRenderingEnabled()                 { return m_renderingEnabled; }
    const bool IsBillboard()                        { return m_billboard; }
    const bool IsComplete()                         { return m_animationComplete; }
    const float GetTime()                           { return m_time; }
    const float GetTimeAsPercentage()               { return m_timeAsPercentage; }
    const float GetMixFactor()                      { return m_mixFactor; }
    const std::string& GetTextureName()             { return m_textureName; }
    const glm::vec3 GetPosition()                   { return m_position; }
    const glm::vec3 GetRotation()                   { return m_rotation; }
    const glm::vec3 GetScale()                      { return m_scale; }
    const uint32_t GetFrameIndex()                  { return m_frameIndex; }
    const uint32_t GetNextFrameIndex()              { return m_frameIndexNext; }
    const SpriteSheetRenderItem& GetRenderItem()    { return m_renderItem; }

private:
    glm::vec3 m_position;
    glm::vec3 m_rotation;
    glm::vec3 m_scale;
    uint32_t m_frameIndex = 0;
    uint32_t m_frameIndexNext = 1;
    std::string m_textureName;
    bool m_animationComplete = false;
    bool m_billboard = true;
    bool m_loop = true;
    bool m_renderingEnabled = true;
    float m_animationSpeed = 3.5f;
    float m_mixFactor = 0.0f;
    float m_time = 0;
    float m_timeAsPercentage = 0.0f;
    float m_uOffset = 0.0f;
    float m_vOffset = 0.0f;
    SpriteSheetTexture* m_spriteSheetTexture;
    SpriteSheetRenderItem m_renderItem;
};
}
