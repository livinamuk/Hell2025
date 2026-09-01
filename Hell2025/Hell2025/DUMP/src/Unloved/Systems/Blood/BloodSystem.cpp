#include "BloodSystem.h"

#include "Hell/Common/Color.h"
#include "Hell/Common/Random.h"
#include "Hell/Common/String.h"
#include "Hell/Debug/DebugDraw.h"
#include "Hell/Input.h"
#include "Hell/Physics.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"
#include "Hell/Transform.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Objects/Renderables/VATInstance.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"

namespace {
    constexpr float FRAME_EPSILON = 0.000001f;

    std::vector<TestBloodDecal> g_bloodDecals;
    std::vector<TestParticle> g_particles;
    std::vector<VATInstance> g_vatInstances;
    std::vector<VATRenderItem> g_vatRenderItems;
    int32_t g_magic = 0;

    Hell::LocalFrame TransportLocalFrame(const Hell::LocalFrame& currentFrame, const glm::vec3& newForwardVector) {
        const float forwardLengthSquared = glm::dot(newForwardVector, newForwardVector);
        if (forwardLengthSquared <= FRAME_EPSILON) {
            return currentFrame;
        }

        Hell::LocalFrame transportedFrame;
        transportedFrame.forward = newForwardVector / glm::sqrt(forwardLengthSquared);

        glm::vec3 transportedRight = currentFrame.right -
            transportedFrame.forward * glm::dot(currentFrame.right, transportedFrame.forward);
        const float rightLengthSquared = glm::dot(transportedRight, transportedRight);

        if (rightLengthSquared > FRAME_EPSILON) {
            transportedFrame.right = transportedRight / glm::sqrt(rightLengthSquared);
        }
        else {
            glm::vec3 transportedUp = currentFrame.up -
                transportedFrame.forward * glm::dot(currentFrame.up, transportedFrame.forward);
            const float upLengthSquared = glm::dot(transportedUp, transportedUp);

            if (upLengthSquared <= FRAME_EPSILON) {
                return Hell::LocalFrame(transportedFrame.forward);
            }

            transportedUp /= glm::sqrt(upLengthSquared);
            transportedFrame.right = glm::normalize(glm::cross(transportedUp, transportedFrame.forward));
        }

        transportedFrame.up = glm::normalize(glm::cross(transportedFrame.forward, transportedFrame.right));
        return transportedFrame;
    }

    void CreateBloodDecal(TestParticle& particle, const glm::vec3& hitPosition, const glm::vec3& hitNormal) {
        particle.m_position = hitPosition;
        particle.m_finalHitNormal = hitNormal;
        particle.m_stopped = true;

        constexpr float decalScale = 0.3f;
        const Hell::LocalFrame decalLocalFrame = particle.GetDecalLocalFrame();
        const Hell::QuatTransform transform(hitPosition, decalLocalFrame, glm::vec3(decalScale));

        Texture* texture = Hell::ResourceManager::GetTextureByName("BloodDecal_0");

        TestBloodDecal& decal = g_bloodDecals.emplace_back();
        decal.modelMatrix = transform.ToMat4();
        decal.inverseModelMatrix = glm::inverse(decal.modelMatrix);
        decal.textureIdx = static_cast<uint32_t>(texture->GetBindlessIndex());
    }
}

namespace Unloved::BloodSystem {

    const std::vector<VATRenderItem>& GetVATRenderItems() { return g_vatRenderItems; }
    const std::vector<TestBloodDecal>& GetBloodDecals() { return g_bloodDecals; }
    const std::vector<TestParticle>& GetTestParticles()   { return g_particles; }

    void UpdateVATInstances();
    void UpdateParticles();

    void BeginFrame() {
        g_vatRenderItems.clear();

        for (int i = (int)g_vatInstances.size() - 1; i >= 0; i--) {
            if (g_vatInstances[i].IsAnimationComplete()) {
                g_vatInstances.erase(g_vatInstances.begin() + i);
            }
        }
    }

    void Update() {
        return;
        UpdateVATInstances();
        UpdateParticles();
    }

    void UpdateVATInstances() {
        for (VATInstance& vatInstance : g_vatInstances) {
            vatInstance.Update(Hell::Time::DeltaTime());
            g_vatRenderItems.emplace_back(vatInstance.CreateRenderItem());
        }
    }

    void UpdateParticles() {

        if (Hell::Input::KeyPressed(HELL_KEY_BACKSPACE)) {
            g_particles.clear();
            g_bloodDecals.clear();
        }

        if (Hell::Input::KeyPressed(HELL_KEY_R)) {
            if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {

                const glm::vec3& position = player->GetInteractHitPosition();
                const glm::vec3& forward = player->GetInteractHitNormal();

                Hell::LocalFrame localFrame = Hell::LocalFrame(forward);
                Hell::QuatTransform transform = Hell::QuatTransform(position, localFrame, glm::vec3(0.05f));

                glm::vec3 vel = localFrame.up +
                                localFrame.forward;

                g_particles.push_back(TestParticle(position, vel, localFrame));
            }
        }

        for (int i = static_cast<int>(g_particles.size()) - 1; i >= 0; i--) {
            g_particles[i].Update(Hell::Time::DeltaTime());

            //g_particles[i].DebugDraw(i);

            if (g_particles[i].m_stopped) continue;

            const glm::vec3 rayVector = g_particles[i].m_positionPrev - g_particles[i].m_position;
            const float rayLength = glm::length(rayVector);
            if (rayLength <= FRAME_EPSILON) continue;

            const glm::vec3& rayOrigin = g_particles[i].m_position;
            const glm::vec3 rayDir = rayVector / rayLength;

            if (g_particles[i].m_lifeTime < 0.2f) continue;

            // PhysX ray
            PhysXRayResult physXRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, rayLength, false);
            if (physXRayResult.hitFound) {
                CreateBloodDecal(g_particles[i], physXRayResult.hitPosition, physXRayResult.hitNormal);
                g_particles.erase(g_particles.begin() + i);
                continue;
            }

            // BVH ray
            BvhRayResult bvhRayResult = WorldBVH::ClosestHit(rayOrigin, rayDir, rayLength);
            if (bvhRayResult.hitFound) {
                CreateBloodDecal(g_particles[i], bvhRayResult.hitPosition, bvhRayResult.hitNormal);
                g_particles.erase(g_particles.begin() + i);
                continue;
            }
        }
    }

    void SpawnVatBlood(const glm::vec3& position, const glm::vec3& forward, float scale, uint64_t parentHitObjectId) {
        return;
        g_magic++;
        g_magic = g_magic % 6;

        VATInstance& vatInstance = g_vatInstances.emplace_back();

        VATInstanceCreateInfo createInfo;
        createInfo.playbackSpeed = 7.5f;
        createInfo.loop = false;
        createInfo.worldPosition = position;
        createInfo.worldForward = forward;
        createInfo.scale = scale;

        if (g_magic == 0) {
            createInfo.resourceName = "Blood19";
            createInfo.mirror = false;
        }
        if (g_magic == 1) {
            createInfo.resourceName = "Blood20";
            createInfo.mirror = false;
        }
        if (g_magic == 2) {
            createInfo.resourceName = "Blood22";
            createInfo.mirror = false;
        }
        if (g_magic == 3) {
            createInfo.resourceName = "Blood19";
            createInfo.mirror = true;
        }
        if (g_magic == 4) {
            createInfo.resourceName = "Blood20";
            createInfo.mirror = true;
        }
        if (g_magic == 5) {
            createInfo.resourceName = "Blood22";
            createInfo.mirror = true;
        }

        vatInstance.Init(createInfo);

        // TODO: figure out how you are gonna do the this thing: parentHitObjectId
        // TODO: figure out how you are gonna do the this thing: parentHitObjectId
        // TODO: figure out how you are gonna do the this thing: parentHitObjectId

        uint32_t particleCount = 20;

        Hell::LocalFrame localFrame = Hell::LocalFrame(forward);
        Hell::QuatTransform transform = Hell::QuatTransform(position, localFrame, glm::vec3(0.05f));

        for (uint32_t i = 0; i < particleCount; i++) {

            glm::vec3 vel = localFrame.right * Hell::Random::Float(-1.0f, 1.0f) +
                localFrame.up * Hell::Random::Float(0.1f, 1.0f) +
                localFrame.forward * Hell::Random::Float(0.0f, 0.5f);

            g_particles.push_back(TestParticle(position, vel, localFrame));
        }
    }
}

TestParticle::TestParticle(const glm::vec3& position, const glm::vec3& velocity, const Hell::LocalFrame& emitterLocalFrame) {
    m_position = position;
    m_positionPrev = position;
    m_velocity = velocity;
    m_localFrame = TransportLocalFrame(emitterLocalFrame, velocity);
}

void TestParticle::Update(float deltaTime) {
    m_positionPrev = m_position;
    m_lifeTime += deltaTime;

    if (!m_stopped) {
        m_velocity.y += m_gravity * deltaTime;
        m_localFrame = TransportLocalFrame(m_localFrame, m_velocity);
        m_position += m_velocity * deltaTime; \
    }
}

Hell::LocalFrame TestParticle::GetDecalLocalFrame() const {
    const glm::vec3 decalNormal = glm::normalize(m_finalHitNormal);
    glm::vec3 decalRight = m_localFrame.right -
        decalNormal * glm::dot(m_localFrame.right, decalNormal);

    Hell::LocalFrame decalLocalFrame = Hell::LocalFrame(decalNormal);
    const float decalRightLengthSquared = glm::dot(decalRight, decalRight);
    if (decalRightLengthSquared > 0.000001f) {
        decalLocalFrame.right = decalRight / glm::sqrt(decalRightLengthSquared);
        decalLocalFrame.up = glm::normalize(glm::cross(decalLocalFrame.forward, decalLocalFrame.right));
    }

    return decalLocalFrame;
}

void TestParticle::DebugDraw(int32_t randomSeed) {
    Hell::DebugDraw::DrawPoint(m_position, glm::vec4(Hell::Color::Random(randomSeed), 1.0f));
    Hell::DebugDraw::DrawLine(m_position, m_positionPrev, glm::vec4(Hell::Color::Random(randomSeed), 1.0f));

    if (m_stopped) {
        const Hell::LocalFrame decalLocalFrame = GetDecalLocalFrame();
        Hell::DebugDraw::DrawLine(
            m_position,
            m_position + decalLocalFrame.right * 0.25f,
            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f)
        );
    }
}
