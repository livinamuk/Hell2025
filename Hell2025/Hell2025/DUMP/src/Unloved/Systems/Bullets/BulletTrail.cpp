#include "BulletTrail.h"

#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"

#include "Unloved/Systems/Bullets/BulletSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <vector>

BulletTrail::BulletTrail(uint64_t id, BulletCreateInfo& createInfo) {
    m_objectId = id;
    m_createInfo = createInfo;

    m_position = createInfo.origin;
    m_forward = createInfo.direction;
    m_speed = 440.0f;
    m_distanceTraveled = 0.0f;
    m_maxDistance = 100.0f;
    m_phaseAccumulator = 0.0f;
    m_randomPhaseOffset = (static_cast<float>(rand()) / RAND_MAX) * 100.0f;
}

void BulletTrail::Update(float deltaTime) {

    float spiralFrequency = 1.0f;
    float spiralScale = 0.005f;
    float particleSpacing = 0.0025f;

    // TODO:
    // update bullet.maxDistance with any BVH of PhysX scene hit, plus some threshold so bullets still register

    // Determine remaining distance
    float remainingDistance = m_maxDistance - m_distanceTraveled;

    // Determine step distance
    glm::vec3 translation = m_forward * m_speed * deltaTime;
    float desiredStepDistance = glm::length(translation);
    float stepDistance = std::min(desiredStepDistance, std::max(0.0f, remainingDistance));

    // Determine new position
    glm::vec3 oldPos = m_position;
    glm::vec3 translationDir = m_forward;
    m_position += translationDir * stepDistance;
    m_distanceTraveled += stepDistance;

    // Determine up and right vectors
    // TODO: do it when creating the bullet trail, it doesn't change
    glm::vec3 tempUp = (glm::abs(m_forward.y) < 0.999f) ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
    glm::vec3 right = glm::normalize(glm::cross(m_forward, tempUp));
    glm::vec3 up = glm::cross(right, m_forward);

    float traveledThisFrame = 0.0f;

    while (traveledThisFrame < stepDistance) {
        m_phaseAccumulator += particleSpacing;

        // Determine corkscrew spiral offset
        float segmentRatio = traveledThisFrame / stepDistance;
        glm::vec3 interpolatingPos = glm::mix(oldPos, m_position, segmentRatio);
        float theta = (m_phaseAccumulator + m_randomPhaseOffset) * spiralFrequency;
        float radius = (std::sin(m_phaseAccumulator * 0.4f) * 0.5f + 0.5f) * spiralScale;
        glm::vec3 offset = (right * std::cos(theta) + up * std::sin(theta)) * radius;

        // Create the particle
        BulletTrailParticle particle;
        particle.position = interpolatingPos + offset;

        // Jitter particle spawn position
        float jitterX = Hell::Random::Float(-1.0f, 1.0f);
        float jitterY = Hell::Random::Float(-1.0f, 1.0f);
        float jitterZ = Hell::Random::Float(-1.0f, 1.0f);
        particle.position += glm::vec3(jitterX, jitterY, jitterZ) * 0.0015f;

        // Particle velocity
        float driftSpeed = Hell::Random::Float(0.05f, 0.2f);
        float recoilSpeed = Hell::Random::Float(0.1f, 0.3f);
        glm::vec3 drift = glm::normalize(offset) * driftSpeed;
        glm::vec3 recoil = -m_forward * recoilSpeed;
        particle.velocity = drift + recoil;
        particle.rotation = Hell::Random::Float(0.0f, HELL_PI * 2.0f);
        particle.rotationalVelocity = Hell::Random::Float(-25.0f, 25.0f);

        std::vector<BulletTrailParticle>& bulletTrailParticles = Unloved::BulletSystem::GetBulletTrailParticles();
        bulletTrailParticles.emplace_back(particle);

        // Increment particle spacing
        traveledThisFrame += particleSpacing;
    }

    // Segment this bullet trail traveled this frame
    glm::vec3 p1 = oldPos;
    glm::vec3 p2 = m_position;
    //DebugDraw::DrawLine(p1, p2, YELLOW);

    // Create a regular bullet
    BulletCreateInfo createInfo = m_createInfo;
    createInfo.origin = p1;
    createInfo.rayLength = glm::distance(p1, p2);
    Unloved::BulletSystem::AddBullet(createInfo, m_objectId);
}

void BulletTrail::CleanUp() {

}
