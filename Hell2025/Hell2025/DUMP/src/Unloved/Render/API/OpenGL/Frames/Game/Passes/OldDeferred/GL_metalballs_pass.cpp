#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Hell/Render/API/OpenGL/GL_back_end.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Hell/Common/Random.h"
#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Viewport/ViewportManager.h"

#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"
#include "Hell/Time.h"


// remove me
#include "Unloved/Session/Session.h"
#include "Hell/Physics/Physics.h"
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <execution>
#include "Hell/Input.h"
namespace Input = Hell::Input;
// remove me

namespace OpenGL::Renderer {

    GLuint CreateTiling3DNoiseTexture(int resolution = 64);
    void FluidParticleTest();
    void BloodFluidTest(const std::vector<glm::vec3>& positions);

    struct GPUMetaBall {
        glm::vec3 m_position = glm::vec3(0.0f);
        float m_invSigma2 = 0;
    };

    struct MetaBall {
        MetaBall() = default;

        MetaBall(const glm::vec3& position, float radius) {
            m_position = position;
            m_radius = radius;
            m_invSigma2 = 1.0f / std::max(radius * radius, 1e-8f);
            m_stepSize = std::max(m_radius * 0.25f, 1e-4f);
            m_proxyRadius = m_radius * m_proxyScale;
        }

        void Update() {
            m_proxyModelMatrix = glm::translate(glm::mat4(1.0f), m_position) * glm::scale(glm::mat4(1.0f), glm::vec3(m_proxyRadius));

            m_gpuMetaBall.m_position = m_position;
            m_gpuMetaBall.m_invSigma2 = m_invSigma2;
        }

        void DebugDraw() {
            DebugDraw::DrawSphere(m_position, m_proxyRadius, YELLOW);
        }

        float GetStepSize() const                      { return m_stepSize; }
        float GetProxyRadius() const                   { return m_proxyRadius; }
        const GPUMetaBall GetGPUMetaBall() const       { return m_gpuMetaBall; }
        const glm::mat4& GetProxyModelMatrix() const   { return m_proxyModelMatrix; }

    private:
        glm::vec3 m_position = glm::vec3(0.0f);
        glm::mat4 m_proxyModelMatrix = glm::mat4(1.0f);
        float m_invSigma2 = 0;
        float m_radius = 0;
        float m_proxyRadius = 0;
        float m_stepSize = 0;
        GPUMetaBall m_gpuMetaBall;
        const float m_proxyScale = 1.125f;
    };

    std::vector<MetaBall> metaBalls;

    void MetaBallsPass() {
        ProfilerOpenGLZoneFunction();

        metaBalls.clear();
        metaBalls.emplace_back(MetaBall(glm::vec3(36.0f, 32.00f, 36.00f), 0.5f));
        metaBalls.emplace_back(MetaBall(glm::vec3(36.5f, 32.75f, 36.25f), 0.5f));
        metaBalls.emplace_back(MetaBall(glm::vec3(37.0f, 32.25f, 37.00f), 0.5f));

        for (MetaBall& metaBall : metaBalls) {
            metaBall.Update();
            //metaBall.DebugDraw();
        }

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* gBuffer = OpenGL::ResourceManager::GetFrameBufferPtr("GBuffer");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("MetaBalls");
        Model* sphereModel = Hell::ResourceManager::GetModelByName("SphereLowRes");
        if (!sphereModel || sphereModel->GetMeshIndices().empty()) return;
        if (sphereModel->GetMeshCount() == 0) return;

        uint32_t meshId = sphereModel->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

        if (!gBuffer) return;
        if (!shader) return;
        if (!mesh) return;

        gBuffer->Bind();
        //gBuffer->DrawBuffers({ "BaseColor", "Normal", "RMA", "Emissive" });
        gBuffer->DrawBuffers({ "BaseColorMetallic", "NormalXYRoughnessMisc", "Emissive", "VelocityXYOcclusionSubSurface" });


        std::vector<GPUMetaBall> gpuMetaBalls;

        for (MetaBall& metaBall : metaBalls) {
            gpuMetaBalls.emplace_back(metaBall.GetGPUMetaBall());
        }

        OpenGL::UpdateSSBO("MetaBalls", metaBalls.size() * sizeof(GPUMetaBall), gpuMetaBalls.data());
        OpenGL::BindSSBO(SSBO_IDX_META_BALLS, "MetaBalls");


        OpenGL::RasterizerStateManager::ForceRasterizerState("GeometryPass_Default");

        OpenGL::BindShader("MetaBalls");
        OpenGL::BindSSBO(SSBO_IDX_RENDERER_DATA, "RendererData");
        OpenGL::BindSSBO(SSBO_IDX_VIEWPORT_DATA, "ViewportData");
        OpenGL::SetUniformInt("u_metaBallCount", metaBalls.size());

        static GLuint noiseTexture = 0;
        if (noiseTexture == 0) noiseTexture = CreateTiling3DNoiseTexture(64);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, noiseTexture);

        //glDisable(GL_CULL_FACE);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(gBuffer, viewport);

            OpenGL::SetUniformMat4("u_projectionView", viewportData[i].jitteredProjectionViewReverseZ);

            for (MetaBall& metaBall : metaBalls) {
                OpenGL::SetUniformFloat("u_stepSize", metaBall.GetStepSize());
                OpenGL::SetUniformMat4("u_model", metaBall.GetProxyModelMatrix());

                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, mesh->baseVertex, i);
            }
        }

        FluidParticleTest();
    }

    void FluidParticleTest() {

        struct Particle {
            glm::vec3 position;
            glm::vec3 velocity = glm::vec3(0);
            float density = 0;
            bool atRest = false;
            bool odd = false;
        };

        static std::vector<Particle> particles;

        // Hack to alternate skipping ever second particle math
        static bool odd = true;
        odd = !odd;

        // Simulation constants
        float gravity = -9.8f;
        float deltaTime = Hell::Time::DeltaTime();
        float radius = 0.12f;            // Slightly larger radius helps clumping
        float restDensity = 2.0f;
        float stiffness = 0.05f;
        float cohesionStrength = 1.0f;  // Glue factor
        float viscosityLimit = 0.4f;     // Syrup factor

        glm::vec3 origin = glm::vec3(36.0f, 32.5f, 36.0f);

        //glm::vec3 rayOrigin = Game::GetLocalPlayerByViewportIndex(0)->GetCameraPosition();
        //glm::vec3 rayDirection = Game::GetLocalPlayerByViewportIndex(0)->GetCameraForward();
        //float rayLength = 1.0f;;
        //PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(rayOrigin, rayDirection, rayLength);
        //
        //if (rayResult.hitFound) {
        //    origin = rayResult.hitPosition + (rayResult.hitNormal * 0.05f);
        //}
        //else {
        //    origin = rayOrigin + rayDirection;
        //}
        //
        //DebugDraw::DrawPoint(rayOrigin + rayDirection, GREEN);

        if (Input::KeyPressed(HELL_KEY_P)) {


            particles.clear();

            glm::vec3 bulletForward = Unloved::Session::GetLocalPlayerByViewportIndex(0)->GetCameraForward();

            float angle = atan2(bulletForward.z, bulletForward.x);
            float spacing = 0.005f;

            for (float x = -2.0f; x < 2.0f; x++) {
                for (float y = 0.0f; y < 8.0f; y++) {
                    for (float z = -2.0f; z < 2.0f; z++) {
                        float localVelocityX = Hell::Random::Float(0.0f, 1.0f);
                        float worldVelocityY = Hell::Random::Float(1.0f, 3.0f);
                        float localVelocityY = Hell::Random::Float(-0.5f, 0.5f);

                        float worldVelocityX = localVelocityX * cos(angle) - localVelocityY * sin(angle);
                        float worldVelocityZ = localVelocityX * sin(angle) + localVelocityY * cos(angle);

                        float offsetX = (x * spacing) * cos(angle) - (z * spacing) * sin(angle);
                        float offsetZ = (x * spacing) * sin(angle) + (z * spacing) * cos(angle);

                        Particle& particle = particles.emplace_back();
                        particle.velocity = glm::vec3(worldVelocityX, worldVelocityY, worldVelocityZ);
                        particle.position = origin + glm::vec3(offsetX, y * spacing, offsetZ);
                        particle.atRest = false;
                        particle.odd = odd;

                        odd = !odd;
                    }
                }
            }

            std::cout << "Spawned " << particles.size() << " particles\n";
        }

        glm::vec3 minBounds(FLT_MAX);
        glm::vec3 maxBounds(-FLT_MAX);

        for (const Particle& particle : particles) {
            minBounds = glm::min(minBounds, particle.position);
            maxBounds = glm::max(maxBounds, particle.position);
        }

        DebugDraw::DrawAABB(AABB(minBounds, maxBounds), YELLOW);


        // Calculate density
        //for (Particle& particle : particles) {
        //    if (particle.atRest) continue;
        //
        //    particle.density = 0;
        //    for (auto& other : particles) {
        //        float dist = glm::distance(particle.position, other.position);
        //        if (dist < radius) {
        //            particle.density += std::pow(radius - dist, 2);
        //        }
        //    }
        //}
        float radiusSq = radius * radius;

        // Calculate density in parallel
        std::for_each(std::execution::par, particles.begin(), particles.end(), [&](Particle& particle) {
            if (particle.atRest) return;

            float currentDensity = 0.0f;
            const glm::vec3 pos = particle.position;

            for (const auto& other : particles) {
                if (other.odd) continue;

                glm::vec3 diff = pos - other.position;
                float distSq = glm::dot(diff, diff);

                if (distSq < radiusSq) {
                    float dist = std::sqrt(distSq);
                    float gap = radius - dist;
                    currentDensity += gap * gap;
                }
            }
            particle.density = currentDensity;
            }
        );

        // Apply Forces
        //for (Particle& particle : particles) {
        //    if (particle.atRest) continue;
        //
        //    glm::vec3 pressureForce(0);
        //    glm::vec3 cohesionForce(0);
        //    glm::vec3 viscosityForce(0);
        //
        //    for (Particle& other : particles) {
        //        if (&particle == &other) continue;
        //
        //        glm::vec3 dir = particle.position - other.position;
        //        float dist = glm::length(dir);
        //
        //        if (dist < radius && dist > 0.0001f) {
        //            // Pressure (push apart)
        //            float pressure = stiffness * (particle.density + other.density - 2 * restDensity);
        //            pressureForce += glm::normalize(dir) * pressure * (radius - dist);
        //
        //            // Cohesion (pull together)
        //            cohesionForce -= glm::normalize(dir) * cohesionStrength * (radius - dist);
        //
        //            // Viscosity (drag each other by averaging velocities)
        //            glm::vec3 relativeVelocity = other.velocity - particle.velocity;
        //            viscosityForce += relativeVelocity * viscosityLimit * (radius - dist);
        //        }
        //    }
        //
        //    // Combine all forces
        //    particle.velocity += (pressureForce + cohesionForce + viscosityForce) * deltaTime;
        //    particle.velocity.y += gravity * deltaTime;
        //
        //    // Apply movement
        //    glm::vec3 movement = particle.velocity * deltaTime;
        //
        //    if (!particle.atRest) {
        //        glm::vec3 rayOrigin = particle.position;
        //        glm::vec3 rayDirection = glm::normalize(movement);
        //        float rayLength = glm::length(movement) * 1.1f;
        //        PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(rayOrigin, rayDirection, rayLength);
        //
        //        // Collision detected, so halt it
        //        if (rayResult.hitFound) {
        //            particle.position = rayResult.hitPosition;
        //            particle.velocity = glm::vec3(0.0f);
        //            particle.atRest = true;
        //        }
        //        // No hit, then move normally
        //        else {
        //            particle.position += movement;
        //        }
        //    }
        //}

        // Apply forces and movement in parallel
        // Apply forces and movement in parallel
        std::for_each(std::execution::par, particles.begin(), particles.end(), [&](Particle& particle) {
            if (particle.atRest) return;

            // Use a single accumulator and cache local variables to save register space
            glm::vec3 totalForce(0.0f);
            const glm::vec3 pos = particle.position;
            const glm::vec3 vel = particle.velocity;

            for (const auto& other : particles) {
                if (&particle == &other) continue;
                if (other.odd) continue;

                glm::vec3 dir = pos - other.position;
                float distSq = glm::dot(dir, dir);

                // Check squared distance first to early-out before the expensive sqrt
                if (distSq < radiusSq && distSq > 0.000001f) {
                    float dist = std::sqrt(distSq);
                    float invDist = 1.0f / dist;
                    float gap = radius - dist;

                    // Pressure & Cohesion combined:
                    // We multiply the direction vector by the combined scalar force
                    // instead of doing two separate vector normalizations and multiplications.
                    float pressure = stiffness * (particle.density + other.density - 2.0f * restDensity);
                    float combinedScalar = (pressure - cohesionStrength) * gap * invDist;
                    totalForce += dir * combinedScalar;

                    // Viscosity (drag each other by averaging velocities)
                    totalForce += (other.velocity - vel) * (viscosityLimit * gap);
                }
            }

            // Combine all forces and apply gravity
            particle.velocity += totalForce * deltaTime;
            particle.velocity.y += gravity * deltaTime;

            // Apply movement
            glm::vec3 movement = particle.velocity * deltaTime;

            // Collision detection
            // Note: Ensure Hell::Physics::CastPhysXRayStaticEnvironment is thread-safe!
            glm::vec3 rayDirection = glm::normalize(movement);
            float rayLength = glm::length(movement) * 1.1f;
            PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(pos, rayDirection, rayLength);

            // Collision detected, so halt it
            if (rayResult.hitFound) {
                particle.position = rayResult.hitPosition;
                particle.velocity = glm::vec3(0.0f);
                particle.atRest = true;
            }
            // No hit, then move normally
            else {
                particle.position += movement;
            }
            }
        );

        std::vector<glm::vec3> positions;

        //metaBalls.clear();
        for (Particle& particle : particles) {
            glm::vec4 color = YELLOW;
            //if (particle.odd) {
            //    color = RED;
            //}
            //metaBalls.emplace_back(MetaBall(particle.position, 0.005));
            //metaBalls.emplace_back(MetaBall(particle.position + glm::vec3(Hell::Random::Float(-0.005, 0.005)), 0.01));
            //metaBalls.emplace_back(MetaBall(particle.position + glm::vec3(Hell::Random::Float(-0.005, 0.005)), 0.01));
            DebugDraw::DrawPoint(particle.position, color);

            positions.push_back(particle.position);
        }

        BloodFluidTest(positions);
    }


    void BloodFluidTest(const std::vector<glm::vec3>& positions) {
        ProfilerOpenGLZoneFunction();

        const std::vector<ViewportData>& viewportData = Unloved::RenderDataManager::GetViewportData();

        OpenGLFrameBuffer* bloodFluidFbo = OpenGL::ResourceManager::GetFrameBufferPtr("BloodFluid");
        OpenGLShader* depthShader = OpenGL::ResourceManager::GetShaderPtr("BloodFluidDepth");
        OpenGLShader* thicknessShader = OpenGL::ResourceManager::GetShaderPtr("BloodFluidThickness");
        Model* sphereModel = Hell::ResourceManager::GetModelByName("SphereLowRes");
        if (!sphereModel || sphereModel->GetMeshIndices().empty()) return;
        if (sphereModel->GetMeshCount() == 0) return;

        uint32_t meshId = sphereModel->GetMeshIndices()[0];
        Mesh* mesh = Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshById(meshId);

        if (!bloodFluidFbo) return;
        if (!thicknessShader) return;
        if (!thicknessShader) return;
        if (!mesh) return;

        bloodFluidFbo->Bind();
        bloodFluidFbo->ClearAttachmentR("Depth", -1000.0f);
        bloodFluidFbo->ClearAttachmentR("Thickness", 0.0f);

        std::vector<glm::mat4> modelMatrices;
        for (const glm::vec3& position : positions) {
            Transform transform;
            transform.position = position;
            transform.scale = glm::vec3(0.0125f);
            modelMatrices.push_back(transform.to_mat4());
        }

        // Draw depth
        bloodFluidFbo->DrawBuffers({ "Depth" });
        OpenGL::BindShader("BloodFluidDepth");

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_GREATER);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(bloodFluidFbo, viewport);

            OpenGL::SetUniformMat4("u_projection", viewportData[i].projection);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);

            for (const glm::mat4& modelMatrix: modelMatrices) {
                OpenGL::SetUniformMat4("u_model", modelMatrix);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, mesh->baseVertex, i);
            }
        }

        // Draw thickness
        bloodFluidFbo->DrawBuffers({ "Thickness" });
        OpenGL::BindShader("BloodFluidThickness");

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);
        glDisable(GL_CULL_FACE);

        for (int i = 0; i < 4; i++) {
            Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGL::Renderer::SetViewport(bloodFluidFbo, viewport);

            OpenGL::SetUniformMat4("u_projection", viewportData[i].projection);
            OpenGL::SetUniformMat4("u_view", viewportData[i].view);

            for (const glm::mat4& modelMatrix : modelMatrices) {
                OpenGL::SetUniformMat4("u_model", modelMatrix);
                glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (GLvoid*)(mesh->baseIndex * sizeof(GLuint)), 1, mesh->baseVertex, i);
            }
        }

        // Cleanup
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);

        // Blur
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glm::ivec2 size = glm::ivec2(bloodFluidFbo->GetWidth(), bloodFluidFbo->GetHeight());

        OpenGLShader* blurShader = OpenGL::ResourceManager::GetShaderPtr("BloodFluidBlur");
        OpenGL::BindShader("BloodFluidBlur");
        OpenGL::SetUniformIVec2("u_screenSize", size);

        OpenGL::SetUniformVec2("u_dir", glm::vec2(0.0f, 1.0f));
        OpenGL::BindImageTexture(0, bloodFluidFbo->GetColorAttachmentHandleByName("Depth"), GL_READ_ONLY, GL_R32F);
        OpenGL::BindImageTexture(1, bloodFluidFbo->GetColorAttachmentHandleByName("BlurIntermediate"), GL_WRITE_ONLY, GL_R32F);
        OpenGL::DispatchCompute((size.x + 15) / 16, (size.y + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        OpenGL::SetUniformVec2("u_dir", glm::vec2(1.0f, 0.0f));
        OpenGL::BindImageTexture(0, bloodFluidFbo->GetColorAttachmentHandleByName("BlurIntermediate"), GL_READ_ONLY, GL_R32F);
        OpenGL::BindImageTexture(1, bloodFluidFbo->GetColorAttachmentHandleByName("Depth"), GL_WRITE_ONLY, GL_R32F);
        OpenGL::DispatchCompute((size.x + 15) / 16, (size.y + 15) / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }


    float Hash3(int x, int y, int z, int period) {
        x = (x % period + period) % period;
        y = (y % period + period) % period;
        z = (z % period + period) % period;

        int n = x + y * 57 + z * 113;
        n = (n << 13) ^ n;
        return (1.0f - ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 1073741824.0f);
    }

    float TilingValueNoise3D(float x, float y, float z, int period) {
        int ix = (int)std::floor(x);
        int iy = (int)std::floor(y);
        int iz = (int)std::floor(z);

        float fx = x - ix;
        float fy = y - iy;
        float fz = z - iz;

        float u = fx * fx * (3.0f - 2.0f * fx);
        float v = fy * fy * (3.0f - 2.0f * fy);
        float w = fz * fz * (3.0f - 2.0f * fz);

        // Sample eight corners of the surrounding cell
        float c000 = Hash3(ix, iy, iz, period);
        float c100 = Hash3(ix + 1, iy, iz, period);
        float c010 = Hash3(ix, iy + 1, iz, period);
        float c110 = Hash3(ix + 1, iy + 1, iz, period);
        float c001 = Hash3(ix, iy, iz + 1, period);
        float c101 = Hash3(ix + 1, iy, iz + 1, period);
        float c011 = Hash3(ix, iy + 1, iz + 1, period);
        float c111 = Hash3(ix + 1, iy + 1, iz + 1, period);

        // Interpolate along x
        float r00 = c000 * (1.0f - u) + c100 * u;
        float r10 = c010 * (1.0f - u) + c110 * u;
        float r01 = c001 * (1.0f - u) + c101 * u;
        float r11 = c011 * (1.0f - u) + c111 * u;

        // Interpolate along y
        float r0 = r00 * (1.0f - v) + r10 * v;
        float r1 = r01 * (1.0f - v) + r11 * v;

        // Interpolate along z
        return r0 * (1.0f - w) + r1 * w;
    }

    GLuint CreateTiling3DNoiseTexture(int resolution) {
        std::vector<unsigned char> data(resolution * resolution * resolution);

        // Adjust frequency to set noise density within the volume
        int period = 4;

        for (int z = 0; z < resolution; ++z) {
            for (int y = 0; y < resolution; ++y) {
                for (int x = 0; x < resolution; ++x) {
                    float px = (float)x / resolution * period;
                    float py = (float)y / resolution * period;
                    float pz = (float)z / resolution * period;

                    float noise = TilingValueNoise3D(px, py, pz, period);

                    // Normalize float to unsigned byte range
                    unsigned char val = (unsigned char)((noise * 0.5f + 0.5f) * 255.0f);

                    int index = x + y * resolution + z * resolution * resolution;
                    data[index] = val;
                }
            }
        }

        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_3D, textureID);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage3D(GL_TEXTURE_3D, 0, GL_R8, resolution, resolution, resolution, 0, GL_RED, GL_UNSIGNED_BYTE, data.data());

        return textureID;
    }

}
