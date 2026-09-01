#pragma once
#include "Unloved/Camera/Frustum.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Hell/ResourceManagement/Types/Model.h"
#include "Unloved/Common/CreateInfo.h"
#include "Unloved/Common/Types.h"

namespace Unloved {


struct LightFlicker {
    float m_baseIntensity = 1.0f;
    float m_amplitude = 0.25f; // 0..1
    float m_responseHz = 12.0f; // smoothing speed
    float m_slowFrequencyHz = 2.0f;
    float m_midFrequencyHz = 6.5f;
    float m_fastFrequencyHz = 11.0f;
    float m_slowWeight = 0.60f;
    float m_midWeight = 0.30f;
    float m_fastWeight = 0.10f;
    float m_shapePower = 1.8f; // The higher this value, the lower the peaks
    glm::vec3 m_lowColor = glm::vec3(1.00f, 0.35f, 0.10f);
    glm::vec3 m_highColor = glm::vec3(1.00f, 0.75f, 0.35f);
    glm::vec3 m_currentColor = glm::vec3(1.0f);
    float m_currentFlicker = 0.8f; // internal state in [0,1]

    void Update(float deltaTime, float timeSeconds);
};

struct Light {
    Light() = default;
    Light(uint64_t id, const LightCreateInfo& createInfo, const SpawnOffset& spawnOffset);
    Light(const Light&) = delete;
    Light& operator=(const Light&) = delete;
    Light(Light&&) noexcept = default;
    Light& operator=(Light&&) noexcept = default;
    ~Light() = default;

    void Update(float deltaTime);
    void CleanUp();

    void RaycastWorldBounds();
    void SetPosition(const glm::vec3& position);
    void SetPositionX(float x);
    void SetPositionY(float y);
    void SetPositionZ(float z);
    void SetRotation(const glm::vec3& rotation);
    void SetRotationX(float x);
    void SetRotationY(float y);
    void SetRotationZ(float z);
    void SetColor(const glm::vec3& color);
    void SetColorR(float r);
    void SetColorG(float g);
    void SetColorB(float b);
    void SetForward(const glm::vec3& forward);
    void SetForwardX(float x);
    void SetForwardY(float y);
    void SetForwardZ(float z);
    void SetTwist(float twist);
    void SetRadius(float radius);
    void SetStrength(float strength);
    void SetType(LightType type);
    void SetIESExposure(float exposure);
    void SetIESProfileType(IESProfileType type);
    void UpdateMatricesAndFrustum();
    void ForceDirty();

    void ConfigureMeshNodes();

    void SetRaytracingDirtyFlag(bool value) { m_dirtyForRaytracing = value; }
    bool ConsumeForcedDirtyFlag() { bool value = m_forcedDirty; m_forcedDirty = false; return value; }

    bool IsForcedDirty() const             { return m_forcedDirty; }

    Unloved::Frustum* GetFrustumByFaceIndex(uint32_t faceIndex);

    MeshNodes& GetMeshNodes()                                  { return m_meshNodes; }
    LightType GetType() const                                  { return m_createInfo.type; }
    const glm::mat4 GetProjectionView(int index) const         { return m_projectionTransforms[index]; }
	const glm::mat4 GetProjectionViewReverseZ(int index) const { return m_projectionTransformsReverseZ[index]; }
    const bool IsDirtyForRaytracing() const                    { return m_dirtyForRaytracing; }
    const float GetRadius() const                              { return m_createInfo.radius; }
    const float GetStrength() const                            { return m_createInfo.strength; }
    const float GetTwist() const                               { return m_createInfo.twist; }
    const glm::vec3& GetPosition() const                       { return m_createInfo.position; }
    const glm::vec3& GetRotation() const                       { return m_createInfo.rotation; }
    const glm::vec3& GetForward() const                        { return m_createInfo.forward; }
    const glm::vec3& GetColor() const                          { return m_createInfo.color; }
    const uint64_t GetObjectId() const                         { return m_objectId; }
    const LightCreateInfo& GetCreateInfo() const               { return m_createInfo; };
    const std::vector<RenderItem>& GetRenderItems() const      { return m_meshNodes.GetRenderItems(); }
    const IESProfileType GetIESProfileType() const             { return m_createInfo.iesProfileType; }
    const float GetIESExposure() const                         { return m_createInfo.iesExposure; }

    // Remove me
    const glm::vec3& GetWorldBoundsMin() const { return m_worldBoundsMin; }
    const glm::vec3& GetWorldBoundsMax() const { return m_worldBoundsMax; }
    // Remove me

    bool m_doFlicker = false;
    LightFlicker m_lightFlicker;

private:
	MeshNodes m_meshNodes;
    bool m_forcedDirty = false;
    bool m_dirtyForRaytracing = true;
    uint64_t m_objectId = 0;
    std::vector<RenderItem> m_renderItems;
    LightCreateInfo m_createInfo;
	Unloved::Frustum m_frustum[6];
	glm::mat4 m_projectionTransforms[6];
	glm::mat4 m_projectionTransformsReverseZ[6];
    glm::mat4 m_viewMatrix[6];

    glm::vec3 m_worldBoundsMin = glm::vec3(0.0f);
    glm::vec3 m_worldBoundsMax = glm::vec3(0.0f);

    // Your light is basically made of 3 things. A main model/transform which for most lights is the main model.
    // Hanging lights have 2 other models, the cord, and the cord mount.
};
}
