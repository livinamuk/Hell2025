#include "Light.h"

#include "Hell/Math/Ray.h"
#include "Hell/Noise/Noise.h"
#include "Hell/Physics/Physics.h"
#include "Hell/Projection/Projection.h"
#include "Hell/Time.h"

#include "Unloved/Render/RendererConstants.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"
#include "Unloved/World/World.h"

#include "Unloved/Render/Renderer.h"

#include "Timer.hpp"

namespace Unloved {

Light::Light(uint64_t id, const LightCreateInfo& createInfo, const SpawnOffset& spawnOffset) {
    m_createInfo = createInfo;
    m_createInfo.position += spawnOffset.translation;
    m_createInfo.rotation.y += spawnOffset.yRotation;

	m_objectId = id;
    ConfigureMeshNodes();
    ForceDirty();
}

void Light::Update(float deltaTime) {
    UpdateMatricesAndFrustum();

    if (m_doFlicker) {
        m_lightFlicker.Update(Hell::Time::DeltaTime() * 10, Unloved::Session::GetSessionTime() * 10);
        SetColor(m_lightFlicker.m_currentColor * 1.5f);
    }
}

void Light::CleanUp() {
    // Nothing as of yet
}

void Light::RaycastWorldBounds() {
    glm::vec3 rayOrigin = GetPosition();
    float rayLength = GetRadius();
    int numRays =  500;

    std::vector<glm::vec3> rayDirs = Hell::Ray::GenerateSphereDirections(numRays);

    glm::vec3 minFound = rayOrigin;
    glm::vec3 maxFound = rayOrigin;

    for (const glm::vec3& rayDir : rayDirs) {
        BvhRayResult rayResult = Unloved::CoarseWorldBVH::ClosestHitWithoutDoors(rayOrigin, rayDir, rayLength);
        const glm::vec3 boundsPoint = rayResult.hitFound ? rayResult.hitPosition : rayOrigin + rayDir * rayLength;
        minFound = glm::min(minFound, boundsPoint);
        maxFound = glm::max(maxFound, boundsPoint);
    }

    // Clamp to actual light radius
    minFound = glm::max(minFound, rayOrigin - glm::vec3(GetRadius()));
    maxFound = glm::min(maxFound, rayOrigin + glm::vec3(GetRadius()));

    // Apply threshold
    float threshold = 0.025f;
    minFound -= glm::vec3(threshold);
    maxFound += glm::vec3(threshold);

    // Store it
    m_worldBoundsMin = minFound;
    m_worldBoundsMax = maxFound;

    // Debug draw
    //AABB worldBounds = AABB(GetWorldBoundsMin(), GetWorldBoundsMax());
    //DebugDraw::DrawPoint(rayOrigin, RED);
    //DebugDraw::DrawAABB(worldBounds, glm::vec4(GetColor(), 1.0f));
}


void Light::ConfigureMeshNodes() {
    // Mount position
    glm::vec3 mountPosition = m_createInfo.position;
	PhysXRayResult rayResult = Hell::Physics::CastPhysXRay(m_createInfo.position, glm::vec3(0.0f, 1.0f, 0.0f), 100.0f);
	if (rayResult.hitFound) {
        mountPosition = rayResult.hitPosition;
	}

    // Distance to roof
	float distanceToRoof = glm::distance(mountPosition, m_createInfo.position);

    // Transforms
    Transform worldTransform;
    worldTransform.position = m_createInfo.position;
    worldTransform.rotation = m_createInfo.rotation;

	Transform localMountTransform;
    localMountTransform.position = glm::vec3(0.0f, distanceToRoof, 0.0f);

	Transform localCordTransform;
    localCordTransform.scale = glm::vec3(1.0f, distanceToRoof, 1.0f);

	glm::mat4 worldMatrix = worldTransform.to_mat4();

	if (m_createInfo.type == LightType::HANGING_LIGHT) {
		std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

		MeshNodeCreateInfo& light = meshNodeCreateInfoSet.emplace_back();
		light.meshName = "Light";
		light.materialName = "Light";
		light.castShadows = false;
        light.emissiveColor = m_createInfo.color;

		MeshNodeCreateInfo& mount = meshNodeCreateInfoSet.emplace_back();
		mount.meshName = "Mount";
		mount.materialName = "Light";
		mount.castShadows = false;

		MeshNodeCreateInfo& cord = meshNodeCreateInfoSet.emplace_back();
		cord.meshName = "Cord";
		cord.materialName = "Light";
		cord.castShadows = false;

        m_meshNodes.Init(m_objectId, "LightHanging", meshNodeCreateInfoSet);
		m_meshNodes.SetTransformByMeshName("Mount", localMountTransform);
		m_meshNodes.SetTransformByMeshName("Cord", localCordTransform);
		m_meshNodes.Update(worldMatrix);
    }

    if (m_createInfo.type == LightType::WALL_LAMP) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& light = meshNodeCreateInfoSet.emplace_back();
        light.meshName = "LightWall";
        light.materialName = "LightWall";
        light.castShadows = false;
        light.emissiveColor = m_createInfo.color;

        m_meshNodes.Init(m_objectId, "LightWall", meshNodeCreateInfoSet);
        m_meshNodes.Update(worldMatrix);
    }

}

void Light::SetPosition(const glm::vec3& position) {
    m_createInfo.position = position;
    ConfigureMeshNodes();
    RaycastWorldBounds();
    ForceDirty();
}

void Light::SetPositionX(float x) {
    m_createInfo.position.x = x;
    ConfigureMeshNodes();
    RaycastWorldBounds();
    ForceDirty();
}

void Light::SetPositionY(float y) {
    m_createInfo.position.y = y;
    ConfigureMeshNodes();
    RaycastWorldBounds();
    ForceDirty();
}

void Light::SetPositionZ(float z) {
    m_createInfo.position.z = z;
    ConfigureMeshNodes();
    RaycastWorldBounds();
    ForceDirty();
}

void Light::SetRotation(const glm::vec3& rotation) {
    m_createInfo.rotation = rotation;
    ConfigureMeshNodes();
}

void Light::SetRotationX(float x) {
    m_createInfo.rotation.x = x;
    ConfigureMeshNodes();
}

void Light::SetRotationY(float y) {
    m_createInfo.rotation.y = y;
    ConfigureMeshNodes();
}

void Light::SetRotationZ(float z) {
    m_createInfo.rotation.z = z;
    ConfigureMeshNodes();
}

void Light::SetForward(const glm::vec3& forward) {
    m_createInfo.forward = forward;
    ConfigureMeshNodes();
}

void Light::SetForwardX(float x) {
    m_createInfo.forward.x = x;
    ConfigureMeshNodes();
}

void Light::SetForwardY(float y) {
    m_createInfo.forward.y = y;
    ConfigureMeshNodes();
}

void Light::SetForwardZ(float z) {
    m_createInfo.forward.z = z;
    ConfigureMeshNodes();
}

void Light::SetTwist(float twist) {
    m_createInfo.twist = twist;
    ConfigureMeshNodes();
}

void Light::SetColor(const glm::vec3& color) {
    m_createInfo.color = color;
    ConfigureMeshNodes();
}

void Light::SetColorR(float r) {
    m_createInfo.color.r = r;
    ConfigureMeshNodes();
}

void Light::SetColorG(float g) {
    m_createInfo.color.g = g;
    ConfigureMeshNodes();
}

void Light::SetColorB(float b) {
    m_createInfo.color.b = b;
    ConfigureMeshNodes();
}

void Light::SetRadius(float radius) {
    m_createInfo.radius = radius;
    ConfigureMeshNodes();
    RaycastWorldBounds();
    ForceDirty();
}

void Light::SetStrength(float strength) {
    m_createInfo.strength = strength;
    ConfigureMeshNodes();
}

void Light::SetType(LightType type) {
    m_createInfo.type = type;
    ConfigureMeshNodes();
}

void Light::SetIESExposure(float exposure) {
    m_createInfo.iesExposure = exposure;
    ConfigureMeshNodes();
}

void Light::SetIESProfileType(IESProfileType type) {
    m_createInfo.iesProfileType = type;
    ConfigureMeshNodes();
}

Unloved::Frustum* Light::GetFrustumByFaceIndex(uint32_t faceIndex) {
    if (faceIndex < 0 || faceIndex >= 6) return nullptr;

    return &m_frustum[faceIndex];
}

void Light::ForceDirty() {
    m_forcedDirty = true;
}

void Light::UpdateMatricesAndFrustum() {
	float fovRadians = glm::radians(90.0f);
	float aspectRatio = 1.0f; // Square

	const glm::mat4 projectionMatrix = glm::perspective(fovRadians, aspectRatio, SHADOW_NEAR_PLANE, m_createInfo.radius);
	const glm::mat4 projectionMatrixReverseZ = Hell::Projection::ReverseZPerspective(fovRadians, aspectRatio, SHADOW_NEAR_PLANE);
	glm::mat4 faceYFlip = glm::mat4(1.0f);
	faceYFlip[1][1] = -1.0f;

	const glm::vec3 targets[6] = {
		glm::vec3(1.0f, 0.0f, 0.0f),  glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),  glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, 0.0f, -1.0f)
	};

	const glm::vec3 ups[6] = {
		glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),  glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)
	};

	for (int i = 0; i < 6; i++) {
		m_viewMatrix[i] = glm::lookAt(m_createInfo.position, m_createInfo.position + targets[i], ups[i]);
		m_projectionTransforms[i] = faceYFlip * projectionMatrix * m_viewMatrix[i];
		m_projectionTransformsReverseZ[i] = faceYFlip * projectionMatrixReverseZ * m_viewMatrix[i];
		m_frustum[i].Update(m_projectionTransforms[i]);
	}
}

void LightFlicker::Update(float deltaTime, float timeSeconds) {
    float tSlow = timeSeconds * m_slowFrequencyHz;
    float tMid = timeSeconds * m_midFrequencyHz + 12.3f;
    float tFast = timeSeconds * m_fastFrequencyHz + 41.7f;


    m_lowColor = glm::vec3(1.00f, 0.35f, 0.10f) * 0.5f;
    m_highColor = glm::vec3(1.00f, 0.75f, 0.35f) * 1.0f;

    int32_t seed = 0; // Random gen this if u want
    float nSlow = Hell::Noise::FractalNoise1D(tSlow, seed + 1);
    float nMid = Hell::Noise::FractalNoise1D(tMid, seed + 2);
    float nFast = Hell::Noise::FractalNoise1D(tFast, seed + 3);

    float rawFlicker01 = nSlow * m_slowWeight + nMid * m_midWeight + nFast * m_fastWeight;
    rawFlicker01 = glm::clamp(rawFlicker01, 0.0f, 1.0f);

    float shapedFlicker01 = std::pow(rawFlicker01, m_shapePower);

    float alpha = 1.0f - std::exp(-deltaTime * m_responseHz);
    m_currentFlicker = glm::mix(m_currentFlicker, shapedFlicker01, alpha);
    m_currentFlicker = glm::clamp(m_currentFlicker, 0.0f, 1.0f);

    float intensityScale = (1.0f - m_amplitude) + m_amplitude * m_currentFlicker;

    m_currentColor = glm::mix(m_lowColor, m_highColor, m_currentFlicker);
}

}
