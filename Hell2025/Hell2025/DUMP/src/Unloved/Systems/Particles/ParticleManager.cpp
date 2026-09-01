#include "ParticleManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Input.h"
#include "Hell/Time.h"
namespace Input = Hell::Input;


namespace ParticleManager {
    std::vector<Particle> g_particles;

    void SpawnParticle(const glm::vec3& position);

    void Update(float deltaTime) {

        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0);
        if (!player) return;

        const glm::vec3& cameraPosition = player->GetCameraPosition();
        const glm::vec3& cameraForward = player->GetCameraForward();
        const glm::vec3& cameraRight = player->GetCameraRight();
        const glm::vec3& cameraUp = player->GetCameraUp();
        const float yVel = player->m_yVelocity;

        float g_bubbleSpawnCooldownTimer = 0.0f;
        float g_bubbleSpawnCooldownMax = 0.4f;

        //std::cout << "up: " << cameraUp << "\n";

        //if (player->IsMoving()) {
        //    g_bubbleSpawnCooldownTimer = g_bubbleSpawnCooldownMax;
        //}

        if (g_bubbleSpawnCooldownTimer <= 0) {
            g_bubbleSpawnCooldownTimer = g_bubbleSpawnCooldownMax;
        }

        if (player->GetCameraPosition().y > Ocean::GetOceanOriginY()) {
            g_bubbleSpawnCooldownTimer = 0.0f;
        }

        if (g_bubbleSpawnCooldownTimer > 0) {
            g_bubbleSpawnCooldownTimer -= Hell::Time::DeltaTime();

            bool spawn = Hell::Random::Int(0, 3) == 1;

            if (spawn) {


                glm::vec3 spawnPos = cameraPosition;
                //spawnPos += (cameraForward * 0.4f) + Hell::Random::Float(0.0125f, 0.5f);
                //spawnPos += (cameraRight * Hell::Random::Float(-0.1f, 0.1f));
                //spawnPos -= cameraUp * Hell::Random::Float(0.0f, 0.1f);


                spawnPos += cameraForward * 0.2f;
                spawnPos += cameraUp * -0.05f;


                SpawnParticle(spawnPos);

                //spawnPos = cameraPosition;
                //spawnPos += (cameraForward * 0.4f) + Hell::Random::Float(-0.05f, 0.05f);
                //spawnPos -= (cameraRight * 0.25f) + Hell::Random::Float(-0.1f, 0.1f);
                //SpawnParticle(spawnPos);
            }
        }

        // Spawn bubbles when entering water

        if (player->CameraIsUnderwater() && yVel < 0.0f) {
            float falloffExponent = 0.75f;
            float normalizedVelocity = glm::clamp(glm::abs(yVel) / 7.0f, 0.0f, 1.0f);
            float spawnChance = glm::pow(normalizedVelocity, falloffExponent);
            bool spawn = Hell::Random::Float(0.0f, 1.0f) < spawnChance;

            if (spawn) {
                const glm::vec3& cameraPosition = player->GetCameraPosition();
                const glm::vec3& cameraForward = player->GetCameraForward();
                const glm::vec3& cameraRight = player->GetCameraRight();
                const glm::vec3& cameraUp = player->GetCameraUp();

                glm::vec3 spawnPos = cameraPosition;
                spawnPos += cameraForward * 0.1f;
                spawnPos -= (cameraRight * Hell::Random::Float(-0.05f, 0.05f));
                SpawnParticle(spawnPos);
            }
        }

        g_bubbleSpawnCooldownTimer = std::max(g_bubbleSpawnCooldownTimer, 0.0f);

        if (Input::KeyDown(HELL_KEY_4)) {
            glm::vec3 position = glm::vec3(36.25, 32.0, 37.0);
            SpawnParticle(position);
        }

        for (Particle& particle : g_particles) {
            particle.position += particle.velocity * deltaTime;
            particle.alphaFade -= deltaTime * 1.0f;
            particle.scale -= deltaTime * 0.075f;

            particle.scale = std::max(particle.scale, 0.0f);
            particle.alphaFade = std::max(particle.alphaFade, 0.0f);
        }

        //std::cout << "Particle count: " << g_particles.size() << "\n";

        for (int i = 0; i < g_particles.size(); i++) {
            if (g_particles[i].position.y > Ocean::GetOceanOriginY()) {
                g_particles.erase(g_particles.begin() + i);
                i--;
            }
        }

        if (player->CameraIsUnderwater()) {
            //std::cout << player->m_yVelocity << "\n";
        }
    }

    void SpawnParticle(const glm::vec3& position) {
        Particle& particle = g_particles.emplace_back();
        particle.position = position;
        particle.velocity.x = Hell::Random::Float(-0.1, 0.1);
        particle.velocity.y = Hell::Random::Float(0.25, 0.5);
        particle.velocity.z = Hell::Random::Float(-0.1, 0.1);
        particle.rotation = Hell::Random::Float(0, HELL_PI * 2.0f);
        particle.scale = Hell::Random::Float(0.025, 0.075);
        particle.alphaFade = 1.0f;
    }

    std::vector<Particle>& GetParticles() {
        return g_particles;
    }
}
