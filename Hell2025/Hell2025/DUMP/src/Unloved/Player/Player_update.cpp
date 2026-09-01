#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Time.h"

#include "Unloved/Render/Renderer.h" // For ragdoll draw settings. TODO: get me out of here

#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;

namespace Unloved {

void Player::Update(float deltaTime) {
    m_moving = false;

    // Bail if in editor
    if (EditorSession::IsActive()) return;


    if (Input::KeyPressed(HELL_KEY_G) && m_viewportIndex == 0) {
        TriggerVignette(glm::vec3(0.6f, 0.0f, 0.0f), 0.4f);
    }

    // Update exclusive/ignored viewport indices
    if (SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject()) {
        viewWeapon->SetExclusiveViewportIndex(m_viewportIndex);
    }
    if (SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject()) {
        characterModel->SetIgnoredViewportIndex(m_viewportIndex);
        if (Ragdoll* ragdoll = GetRagdoll()) {
            if (Renderer::GetCurrentRendererSettings().debugDrawRagdolls) {
                characterModel->DisableRendering();
            }
            else {
                characterModel->EnableRendering();
            }
        }
    }

    // Toggle inventory
    if (PressedToggleInventory() && m_shopInventory.IsClosed()) {

        // Was the inventory closed? Then open it
        if (m_inventory.IsClosed()) {
            m_inventory.OpenInventory();
        }
        else {
            // Viewing items in the main screen? well close it
            if (GetInvetoryState() == InventoryState::MAIN_SCREEN) {
                m_inventory.CloseInventory();
            }
            // Examining an item? Well return to main screen
            if (GetInvetoryState() == InventoryState::EXAMINE_ITEM) {
                m_inventory.GoToMainScreen();
            }
        }

        // Hack to also exit shop if the inventory is being used to display your items when in shop to SELL
        m_shopInventory.CloseInventory();
        m_isInShop = false;
        m_shopMermaidObjectId = 0;

        Audio::PlayAudio(AUDIO_SELECT, 1.00f);
    }

    // Close the shop
    if (PressedToggleInventory() && IsInShop()) {
        LeaveShop();
    }

    // Shop hack test
    //if (m_viewportIndex == 0 && Input::KeyPressed(HELL_KEY_U)) {
    //    EnterShop();
    //}

    if (IsInShop()) {
        UpdateShop(deltaTime);
    }

    //if (ViewportIsVisible()) {
    //    std::cout << "Pos:" << GetFootPosition() << " cam: " << GetCameraRotation() << "\n";
    //
    //    //std::cout << "Shop: " << m_shopInventory.IsOpen() << " Inv: " << m_inventory.IsOpen() << "\n";
    //}

    // This may break code elsewhere in the player logic like anywhere
    if (m_inventory.IsOpen()) {
        DisableControl();
        m_inventory.Update(deltaTime);
    }
    if (m_shopInventory.IsOpen()) {
        DisableControl();
        m_shopInventory.Update(deltaTime);
    }

    // Inside or outside?
    glm::vec3 rayOrigin = GetCameraPosition();
    glm::vec3 rayDirection = glm::vec3(0.0f, -1.0f, 0.0f);
    float rayLength = 100.0f;
    PhysXRayResult rayResult = Hell::Physics::CastPhysXRayStaticEnvironment(rayOrigin, rayDirection, rayLength);
    m_feetAboveHeightField = (rayResult.hitFound && rayResult.userData.physicsType == PhysicsType::HEIGHT_FIELD);

    // Running
    m_running = PressingRun() && !m_crouching;

    //m_running = false; // REMOVE ME TO ENABLE SPRINTING

    // Respawn
    if (IsAwaitingSpawn()) {
        Respawn();
        return;
    }
    if (IsDead() && m_timeSinceDeath > 3.25) {
        if (PressedFire() ||
            PressedReload() ||
            PressedCrouch() ||
            PressedInteract() ||
            PressingJump() ||
            PressedNextWeapon()) {
            Respawn();
            Audio::PlayAudio("RE_Beep.wav", 0.5);
        }
    }

    UpdateVignette(deltaTime);;
    UpdateMovement(deltaTime);
    UpdateHeadBob(deltaTime);
    UpdateBreatheBob(deltaTime);
    UpdateCamera(deltaTime);
    UpdateCursorRays();
    UpdateInteract();
    UpdateWeaponLogic(deltaTime);
    UpdateViewWeapon(deltaTime);
    UpdateWeaponSlide();
    UpdateAudio(deltaTime);
    //UpdateUI(deltaTime);
    UpdateFlashlight(deltaTime);
    UpdateFlashlightFrustum();
    UpdatePlayingPiano(deltaTime);
    UpdateCharacterModelHacks();
    UpdateMeleeAttack();
    CalculateMuzzleFlashSpawnPosition();

    if (PressedInteract() && !IsInShop()) {
        const uint64_t mermaidObjectId = GetFacingMermaidObjectId();
        if (mermaidObjectId) EnterShop(mermaidObjectId);
    }

    UpdateViewWeaponVisibility();

    if (World::HasOcean()) {
        float feetHeight = GetFootPosition().y;
        float waterHeight = Ocean::GetWaterHeightAtPlayer(m_viewportIndex);
        m_waterState.feetUnderWaterPrevious = m_waterState.feetUnderWater;
        m_waterState.cameraUnderWaterPrevious = m_waterState.cameraUnderWater;
        m_waterState.wadingPrevious = m_waterState.wading;
        m_waterState.swimmingPrevious = m_waterState.swimming;
        m_waterState.cameraUnderWater = GetCameraPosition().y < waterHeight;
        m_waterState.feetUnderWater = GetFootPosition().y < waterHeight;
        m_waterState.heightAboveWater = (GetFootPosition().y > waterHeight) ? (GetFootPosition().y - waterHeight) : 0.0f;
        m_waterState.heightBeneathWater = (GetFootPosition().y < waterHeight) ? (waterHeight - GetFootPosition().y) : 0.0f;
        m_waterState.swimming = m_waterState.cameraUnderWater && IsMoving();
        m_waterState.wading = !m_waterState.cameraUnderWater && m_waterState.feetUnderWater && IsMoving() && feetHeight < waterHeight - 0.5f;
    }
    else {
        m_waterState.feetUnderWaterPrevious = false;
        m_waterState.cameraUnderWaterPrevious = false;
        m_waterState.wadingPrevious = false;
        m_waterState.swimmingPrevious = false;
        m_waterState.cameraUnderWater = false;
        m_waterState.feetUnderWater = false;
        m_waterState.heightAboveWater = 0.0f;
        m_waterState.heightBeneathWater = 0.0f;
        m_waterState.swimming = false;
        m_waterState.wading = false;
    }

    // Weapon audio frequency (for under water)
    m_weaponAudioFrequency = CameraIsUnderwater() ? 0.4f : 1.0f;

    if (m_infoTextTimer > 0) {
        m_infoTextTimer -= deltaTime;
    }
    else {
        m_infoTextTimer = 0;
        m_infoText = "";
    }

    if (IsAlive()) {
        m_timeSinceDeath = 0.0f;
    }
    else {
        m_timeSinceDeath += deltaTime;
        SetFootPosition(glm::vec3(0, -10, 0));
    }

    //if (Input::KeyPressed(HELL_KEY_Q)) {
    //    std::cout << GetFootPosition() << "\n";
    //    std::cout << GetCamera().GetEulerRotation() << "\n\n";
    //}

   //if (Input::KeyPressed(HELL_KEY_N) && m_viewportIndex == 0) {
   //    auto* viewWeapon = GetViewWeaponSkinnedGameObject();
   //    viewWeapon->PrintNodeNames();
   //}

    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    if (viewport->IsVisible()) {

        //std::cout << "Facing: " << IsFacingClosestMermaid() << " " << "Dot: " << DotToClosestToMermaid() << "\n";

        if (Input::KeyPressed(HELL_KEY_8)) {
            //std::cout << "\nPlayer " << m_viewportIndex << " inventory:\n";
            //m_inventory.PrintGridOccupiedStateToConsole();

            //std::cout << "glm::(" << GetCameraPosition().x << ", " << GetCameraPosition().z << "\n";


            std::cout << "POS:" << GetCameraPosition() << " ROT: " << GetCameraRotation() << "\n";
        }
    }
}

void Player::UpdateVignette(float deltaTime) {
    m_vignetteTimer -= deltaTime;

    float progress = m_vignetteTimer / m_vignetteDuration;
    m_vignetteIntensityScalar = std::sin(progress);

    if (m_vignetteTimer <= 0.0f) {
        m_vignetteColor = glm::vec3(0.0f);
        m_vignetteIntensityScalar = 0.0f;
    }

    m_vignetteTimer = std::max(0.0f, m_vignetteTimer);
}

void Player::PostWorldUpdate() {
    UpdateWeaponAttachments();
    UpdateSpriteSheets(Hell::Time::DeltaTime());
}

} // namespace Unloved
