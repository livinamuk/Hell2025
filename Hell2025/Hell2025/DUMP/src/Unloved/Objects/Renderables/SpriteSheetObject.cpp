#include "SpriteSheetObject.h"
#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace Unloved {

SpriteSheetObject::SpriteSheetObject(const SpriteSheetObjectCreateInfo& createInfo) {
    Init(createInfo);
}

void SpriteSheetObject::Init(const SpriteSheetObjectCreateInfo& createInfo) {
    m_position = createInfo.position;
    m_rotation = createInfo.rotation;
    m_scale = createInfo.scale;
    m_loop = createInfo.loop;
    m_billboard = createInfo.billboard;
    m_textureName = createInfo.textureName;
    m_animationSpeed = createInfo.animationSpeed;
    m_renderingEnabled = createInfo.renderingEnabled;
    m_spriteSheetTexture = Hell::ResourceManager::GetSpriteSheetTexturePtr(m_textureName);
    m_uOffset = createInfo.uvOffset.x;
    m_vOffset = createInfo.uvOffset.y;
}

void SpriteSheetObject::Update(float deltaTime) {
    // Calculate animation
    uint32_t frameCount = m_spriteSheetTexture->GetFrameCount();
    float frameDuration = 1.0f / m_animationSpeed;
    float totalAnimationTime = frameCount * frameDuration;
    if (!m_loop) {
        m_time = std::min(m_time + deltaTime, totalAnimationTime);
    }
    else {
        m_time += deltaTime;
        if (m_time >= totalAnimationTime) {
            m_time = fmod(m_time, totalAnimationTime);
        }
    }
    float frameTime = m_time / frameDuration;
    m_mixFactor = fmod(frameTime, 1.0f);
    if (!m_loop && m_time >= totalAnimationTime) {
        m_frameIndex = frameCount - 1;
        m_frameIndexNext = m_frameIndex;
    }
    else {
        m_frameIndex = static_cast<int>(floor(frameTime)) % frameCount;
        m_frameIndexNext = m_loop ? (m_frameIndex + 1) % frameCount : std::min(m_frameIndex + 1, frameCount - 1);
    }
    m_animationComplete = !m_loop && m_time >= totalAnimationTime;
    m_timeAsPercentage = totalAnimationTime > 0.0f ? (m_time / totalAnimationTime) : 0.0f;

    // Construct render item
    if (m_renderingEnabled) {
        const int columnCount = m_spriteSheetTexture->GetColumnCount();
        const int rowCount = m_spriteSheetTexture->GetRowCount();
        const float frameWidth = 1.0f / columnCount;
        const float frameHeight = 1.0f / rowCount;

        const int frameX = m_frameIndex % columnCount;
        const int frameY = (m_frameIndex - (m_frameIndex % columnCount)) / columnCount;
        const int frameNextX = m_frameIndexNext % columnCount;
        const int frameNextY = (m_frameIndexNext - (m_frameIndexNext % columnCount)) / columnCount;

        Hell::Transform transform;
        transform.position = m_position;
        transform.rotation = m_rotation;
        transform.scale = m_scale;

        m_renderItem.modelMatrix = transform.to_mat4();
        m_renderItem.uvFrame = glm::vec4(frameX * frameWidth, frameY * frameHeight, frameWidth, frameHeight);
        m_renderItem.uvFrameNext = glm::vec4(frameNextX * frameWidth, frameNextY * frameHeight, frameWidth, frameHeight);
        m_renderItem.localOffset = glm::vec4(m_uOffset * 0.5f, m_vOffset * 0.5f, 0.0f, 0.0f);
        m_renderItem.mixFactor = m_mixFactor;
        m_renderItem.textureIndex = m_spriteSheetTexture->GetTextureIndex();
        m_renderItem.isBillboard = (int)m_billboard;
    }
}

void SpriteSheetObject::SetPosition(glm::vec3 position) {
    m_position = position;
}

void SpriteSheetObject::SetRotation(glm::vec3 rotation) {
    m_rotation = rotation;
}

void SpriteSheetObject::SetScale(glm::vec3 scale) {
    m_scale = scale;
}

void SpriteSheetObject::SetUOffset(float value) {
    m_uOffset = value;
}

void SpriteSheetObject::SetVOffset(float value) {
    m_vOffset = value;
}

void SpriteSheetObject::SetTime(float time) {
    m_time = time;
}

void SpriteSheetObject::SetSpeed(float speed) {
    m_animationSpeed= speed;
}

void SpriteSheetObject::EnableRendering() {
    m_renderingEnabled = true;
}

void SpriteSheetObject::DisableRendering() {
    m_renderingEnabled = false;
}
}
