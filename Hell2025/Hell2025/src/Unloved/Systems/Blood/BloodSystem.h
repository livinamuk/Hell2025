#pragma once

#include "Unloved/Objects/Renderables/VATInstance.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <vector>


struct BloodPoolDecal {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    float scale = 0.5f;
    float lifeTime = 0.0f;

    void Update();
};



struct TestBloodDecal {
   glm::mat4 modelMatrix;
   glm::mat4 inverseModelMatrix;
   uint32_t textureIdx;
};

struct TestParticle {
    TestParticle() = default;
    TestParticle(const glm::vec3& position, const glm::vec3& velocity, const Hell::LocalFrame& emitterLocalFrame);
    void Update(float deltaTime);
    void DebugDraw(int32_t randomSeed);
    Hell::LocalFrame GetDecalLocalFrame() const;

    glm::vec3 m_position = glm::vec3(0.0f);
    glm::vec3 m_positionPrev = glm::vec3(0.0f);
    glm::vec3 m_velocity = glm::vec3(0.0f);
    glm::vec3 m_finalHitNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    float m_gravity = -9.8f;
    float m_lifeTime = 0.0f;
    bool m_stopped = false;
    Hell::LocalFrame m_localFrame;
};

namespace Unloved::BloodSystem {

    void BeginFrame();
    void Update();

    void SpawnBloodPoolDecal(const glm::vec3& postion, const glm::vec3& normal);
    const std::vector<BloodPoolDecal>& GetBloodPoolDecals();





    // OLD BROKEN WIP BULLSHIIT BELOW

    void SpawnVatBlood(const glm::vec3& position, const glm::vec3& forward, float scale, uint64_t parentHitObjectId);

    const std::vector<VATRenderItem>& GetVATRenderItems();
    const std::vector<TestBloodDecal>& GetBloodDecals();
    const std::vector<TestParticle>& GetTestParticles();

}
