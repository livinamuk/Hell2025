#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Math/Math.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Config/FlashlightConfig.h"
#include "Unloved/Render/RenderDataManager.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"
#include "Unloved/Objects/Lighting/SpotLight.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>

namespace Audio = Hell::Audio;

namespace {
    constexpr float DEFAULT_FLASHLIGHT_SHADOW_HALF_ANGLE = 30.0f;
    constexpr float FLASHLIGHT_SHADOW_PADDING_DEGREES = 1.5f;
    constexpr float FLASHLIGHT_VISIBLE_IES_THRESHOLD = 0.00001f;

    float GetFlashlightIESHalfAngle(const IESProfile* profile, float contrast) {
        if (!profile) return DEFAULT_FLASHLIGHT_SHADOW_HALF_ANGLE;

        const int32_t verticalCount = profile->GetVerticalAngleCount();
        const int32_t horizontalCount = profile->GetHorizontalAngleCount();
        const std::vector<float>& candelaValues = profile->GetCandelaValues();
        const size_t expectedValueCount = static_cast<size_t>(verticalCount) * static_cast<size_t>(horizontalCount);
        if (verticalCount <= 0 || horizontalCount <= 0 || candelaValues.size() != expectedValueCount) {
            return DEFAULT_FLASHLIGHT_SHADOW_HALF_ANGLE;
        }

        // Match ApplyFlashlightIESProfile's theta-zero horizontal slice,
        // including the first/last interval extrapolation used for IES files
        // whose horizontal data does not begin at zero degrees.
        const float horizontalPosition = profile->GetHBias() * static_cast<float>(std::max(horizontalCount - 1, 0));
        int32_t horizontal0 = 0;
        int32_t horizontal1 = 0;
        float horizontalInterpolation = 0.0f;
        if (horizontalCount > 1) {
            if (horizontalPosition <= 0.0f) {
                horizontal1 = 1;
                horizontalInterpolation = horizontalPosition;
            }
            else if (horizontalPosition >= static_cast<float>(horizontalCount - 1)) {
                horizontal0 = horizontalCount - 2;
                horizontal1 = horizontalCount - 1;
                horizontalInterpolation = horizontalPosition - static_cast<float>(horizontal0);
            }
            else {
                horizontal0 = static_cast<int32_t>(std::floor(horizontalPosition));
                horizontal1 = horizontal0 + 1;
                horizontalInterpolation = horizontalPosition - static_cast<float>(horizontal0);
            }
        }

        const float safeContrast = std::max(contrast, 0.001f);
        int32_t lastVisibleVerticalIndex = 0;
        for (int32_t verticalIndex = 0; verticalIndex < verticalCount; ++verticalIndex) {
            const float value0 = candelaValues[horizontal0 * verticalCount + verticalIndex];
            const float value1 = candelaValues[horizontal1 * verticalCount + verticalIndex];
            const float squaredValue0 = value0 * value0;
            const float squaredValue1 = value1 * value1;
            const float interpolatedValue = squaredValue0 + (squaredValue1 - squaredValue0) * horizontalInterpolation;
            const float adjustedValue = std::pow(std::max(interpolatedValue, 0.0f), safeContrast);
            if (adjustedValue > FLASHLIGHT_VISIBLE_IES_THRESHOLD) {
                lastVisibleVerticalIndex = verticalIndex;
            }
        }

        const float verticalT = verticalCount > 1
            ? static_cast<float>(lastVisibleVerticalIndex) / static_cast<float>(verticalCount - 1)
            : 0.0f;
        const float profileAngle = profile->GetMinVerticalAngle() + profile->GetVerticalAngleRange() * verticalT;
        return std::clamp(profileAngle, 1.0f, 89.0f);
    }
}

namespace Unloved {

void Player::UpdateFlashlight(float deltaTime) {
    // Toggle on/off
    if (InventoryIsClosed() && PressedFlashlight() && IsAlive()) {
        Audio::PlayAudio("Flashlight.wav", 1.5f);
        m_flashlightOn = !m_flashlightOn;
    }
    // Modifier
    if (!m_flashlightOn) {
        m_flashLightModifier = 0.0f;
    }
    else {
        m_flashLightModifier = Hell::Math::InterpTo(m_flashLightModifier, 1.0f, deltaTime, 10.5f);
    }

    // Prevent NAN direction, which is the case on first spawn
    if (Hell::Math::IsNan(m_flashlightDirection)) {
        m_flashlightDirection = GetCameraForward();
    }

    glm::vec3 rayHitPosition = m_bvhRayResult.hitPosition;

    float distanceToRayHit = glm::distance(rayHitPosition, GetCameraPosition());

    // Centered pos/dir
    glm::vec3 centeredFlashlightPosition = GetCameraPosition();

    //centeredFlashlightPosition += GetCameraUp() * glm::vec3(-0.01f);
    //centeredFlashlightPosition += GetCameraRight() * glm::vec3(0.01f);
    //centeredFlashlightPosition += GetCameraForward() * glm::vec3(0.05f);
    centeredFlashlightPosition += GetCameraForward() * glm::vec3(-0.15f);

    glm::vec3 centeredFlashlightDirection = GetCameraForward();

    // Offset pos/dir
    glm::vec3 offsetFlashlightPosition = centeredFlashlightPosition;
    offsetFlashlightPosition += GetCameraRight() * glm::vec3(0.1f);
    offsetFlashlightPosition -= GetCameraUp() * glm::vec3(m_bobOffsetY * 2);
    glm::vec3 offsetFlashlightDirection = glm::normalize(rayHitPosition - offsetFlashlightPosition);

    // Compute lerp factor
    float maxDistance = 1.0f;
    float t = glm::clamp(distanceToRayHit / maxDistance, 0.0f, 0.75f);

    // Mix between centered and offset based on distance to cam hit
    glm::vec3 flashlightPositionTarget = glm::mix(centeredFlashlightPosition, offsetFlashlightPosition, t);
    glm::vec3 flashlightDirectionTarget = glm::mix(centeredFlashlightDirection, offsetFlashlightDirection, t);

    // If no hit was found then default back to centered
    if (!m_rayHitFound) {
    //if (textureHitPos == glm::vec3(0.0f) && physxRayHitPos == glm::vec3(0.0f)) {
        flashlightPositionTarget = centeredFlashlightPosition;
        flashlightDirectionTarget = centeredFlashlightDirection;
    }

    // Lerp between last pos/dir to the new ones
    float interSpeed = 40;
    m_flashlightPosition = Hell::Math::InterpTo(m_flashlightPosition, flashlightPositionTarget, deltaTime, interSpeed);
    m_flashlightDirection = Hell::Math::InterpTo(m_flashlightDirection, flashlightDirectionTarget, deltaTime, interSpeed);

    m_flashlightPosition = flashlightPositionTarget;

    float aspectRatio = 1.0f;
    //if (RenderDataManager::GetViewportData().size()) {
    //    float viewportWidth = RenderDataManager::GetViewportData()[m_viewportIndex].width;
    //    float viewportHeight = RenderDataManager::GetViewportData()[m_viewportIndex].height;
    //    aspectRatio = viewportWidth / viewportHeight;
    //}

    if (IsInShop()) {
        if (Mermaid* mermaid = World::GetMermaidByObjectId(m_shopMermaidObjectId)) {
            m_flashlightDirection = glm::normalize(mermaid->GetPosition() - GetFootPosition());
        }
    }

    // Projection view matrix. At scale 1 the IES data owns the cone, while an
    // optional outer angle overrides it. Cone Scale then changes the shadow
    // cone and the inset IES lobe together.
    const Config::Flashlight::Settings& flashlightSettings = Config::Flashlight::GetSettings();
    const IESProfile* iesProfile = Hell::ResourceManager::GetIESProfilePtr(flashlightSettings.iesProfile);
    const float flashlightRange = flashlightSettings.range;
    const float iesConeScale = flashlightSettings.iesConeScale;
    const float iesOuterAngle = flashlightSettings.iesOuterAngle;
    const float iesContrast = flashlightSettings.iesContrast;

    const float safeConeScale = std::clamp(iesConeScale, 0.001f, 1.2f);
    float lightRadius = std::max(flashlightRange, 0.05f);
    const float unscaledShadowHalfAngleDegrees = iesOuterAngle > 0.0f
        ? iesOuterAngle
        : GetFlashlightIESHalfAngle(iesProfile, iesContrast);
    float shadowHalfAngleDegrees = unscaledShadowHalfAngleDegrees * safeConeScale;

    shadowHalfAngleDegrees = std::clamp(shadowHalfAngleDegrees + FLASHLIGHT_SHADOW_PADDING_DEGREES, 1.0f, 89.0f);
    const float shadowHalfAngle = glm::radians(shadowHalfAngleDegrees);
    const glm::vec3 flashlightTargetPosition = m_flashlightPosition + m_flashlightDirection;
    glm::mat4 flashlightViewMatrix = glm::lookAt(m_flashlightPosition, flashlightTargetPosition, GetCameraUp());
    glm::mat4 spotlightProjection = glm::perspective(shadowHalfAngle * 2.0f, aspectRatio, 0.05f, lightRadius);
    m_flashlightProjectionView = spotlightProjection * flashlightViewMatrix;

    // Prevent NAN bugs
    if (Hell::Math::IsNan(m_flashlightPosition)) {
        m_flashlightPosition = flashlightPositionTarget;
    }

    // The world owns flashlight spotlights. The player retains only the stable
    // world ID and recreates the light if a world reset invalidated it.
    SpotLight* spotLight = World::GetSpotLightByObjectId(m_flashlightSpotLightId);
    if (!spotLight) {
        m_flashlightSpotLightId = World::AddSpotLight(m_playerId, m_viewportIndex);
        spotLight = World::GetSpotLightByObjectId(m_flashlightSpotLightId);
    }

    if (spotLight) {
        SpotLightData data;
        data.projectionView = m_flashlightProjectionView;
        data.position = m_flashlightPosition;
        data.direction = m_flashlightDirection;
        data.range = flashlightSettings.range;
        data.modifier = m_flashLightModifier;
        data.shadowHalfAngleDegrees = shadowHalfAngleDegrees;
        data.castsShadows = true;
        data.skipOwnerShadow = IsInShop();
        data.useFlashlightViewDistanceScale = true;
        spotLight->SetData(data);
    }
}

void Player::UpdateFlashlightFrustum() {
    const Resolutions& resolutions = Config::GetResolutions();
    int renderTargetWidth = resolutions.gBuffer.x;
    int renderTargetHeight = resolutions.gBuffer.y;
    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    float viewportWidth = viewport->GetSize().x * renderTargetWidth;
    float viewportHeight = viewport->GetSize().y * renderTargetHeight;
    float aspect = viewportWidth / viewportHeight;
    float nearPlane = 0.01f;
    float farPlane = 10.0f;
    glm::mat4 perspectiveMatrix = glm::perspective(m_cameraZoom, aspect, nearPlane, farPlane);
    glm::mat4 projectionView = perspectiveMatrix * m_camera.GetViewMatrix();
    m_flashlightFrustum.Update(m_flashlightProjectionView);
}

} // namespace Unloved
