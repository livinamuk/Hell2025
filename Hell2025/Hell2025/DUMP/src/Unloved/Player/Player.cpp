#include "Player.h"

#include "Hell/Audio.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"
#include "Hell/Logging.h"

#include "Unloved/Common/Constants.h"
#include "Unloved/Render/Renderer.h"
#include "Legacy/World/LegacyWorld.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Ocean/Ocean.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/World/World.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <limits>
#include <vector>

namespace Unloved {

void Player::Init(uint64_t playerId, const glm::vec3& position, const glm::vec3& rotation, int32_t viewportIndex) {
    m_playerId = playerId;
    m_flashlightSpotLightId = World::AddSpotLight(playerId, viewportIndex);

    m_camera.SetPosition(position + glm::vec3(0.0f, m_viewHeightStanding, 0.0f));
    m_camera.SetEulerRotation(rotation);
    m_viewportIndex = viewportIndex;

    m_inventory.SetLocalPlayerIndex(m_viewportIndex);
    m_shopInventory.SetLocalPlayerIndex(m_viewportIndex);

    m_characterModelSkinnedGameObjectId = World::CreateSkinnedGameObject();
    m_characterModelAnimatorInstanceId = Animator::CreateAnimatorInstance();

    m_viewWeaponSkinnedGameObjectId = World::CreateSkinnedGameObject();
    m_viewWeaponAnimatorInstanceId = Animator::CreateAnimatorInstance();

    AnimatorInstance* characterModelAnimatorInstance = GetCharacterModelAnimatorInstance();
    AnimatorInstance* viewWeaponAnimatorInstance = GetViewWeaponAnimatorInstance();

    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();

    if (!characterModel|| !viewWeapon || !characterModelAnimatorInstance || !viewWeaponAnimatorInstance) {
        Logging::Editor() << "Player::Init(..) some core shit fucked up\n";
        __debugbreak();
    }

    // Register the character skeleton before creating its animation layer
    characterModelAnimatorInstance->RegisterSkinnedModels({ "UniSexGuyScaled" });
    m_characterModelAnimationLayerIndex = characterModelAnimatorInstance->CreateAnimationLayer();
    characterModel->SetAnimatorInstanceId(m_characterModelAnimatorInstanceId);

    // The view weapon skeleton is registered when the first weapon is equipped
    m_viewWeaponAnimationLayerIndex = viewWeaponAnimatorInstance->CreateAnimationLayer();
    viewWeapon->SetAnimatorInstanceId(m_viewWeaponAnimatorInstanceId);
    viewWeapon->SetViewWeapon(true);
    viewWeapon->SetExclusiveViewportIndex(viewportIndex);
    viewWeapon->DisableShadows();
    viewWeapon->SetExcludeFromVulkanTLAS(true);

    SpriteSheetObjectCreateInfo createInfo;
    createInfo.textureName = "MuzzleFlash_4x5";
    createInfo.loop = false;
    createInfo.billboard = true;
    createInfo.renderingEnabled = false;
    m_muzzleFlash.Init(createInfo);

    CreateCharacterController(position);
    InitCharacterModel();
    InitRagdoll();
}

void Player::CleanUp() {
    Animator::RemoveAnimatorInstance(m_characterModelAnimatorInstanceId);
    Animator::RemoveAnimatorInstance(m_viewWeaponAnimatorInstanceId);

    World::RemoveObjectById(m_characterModelSkinnedGameObjectId);
    World::RemoveObjectById(m_viewWeaponSkinnedGameObjectId);
    World::RemoveObjectById(m_flashlightSpotLightId);

    Hell::Physics::MarkCharacterControllerForRemoval(m_characterControllerId);

    m_supressor.CleanUp();
    m_redDot.CleanUp();
    m_p90MagMeshNodes.CleanUp();
    m_inventory.CleanUp();
    m_shopInventory.CleanUp();

    m_playerId = 0;
    m_characterControllerId = 0;
    m_characterModelAnimatorInstanceId = 0;
    m_characterModelAnimationLayerIndex = 0;
    m_characterModelSkinnedGameObjectId = 0;
    m_viewWeaponAnimatorInstanceId = 0;
    m_viewWeaponAnimationLayerIndex = 0;
    m_viewWeaponSkinnedGameObjectId = 0;
    m_flashlightSpotLightId = 0;
    m_pianoId = 0;
    m_shopMermaidObjectId = 0;
    m_isPlayingPiano = false;
}

void Player::BeginFrame() {
    m_interactFound = false;
    m_interactObjectId = 0;
    m_interactOpenableId = 0;
}

void Player::EnterShop(uint64_t mermaidObjectId) {
    if (!World::GetMermaidByObjectId(mermaidObjectId)) return;

    m_shopMermaidObjectId = mermaidObjectId;
    m_isInShop = true;
    m_shopInventory.OpenAsShop();
    m_inventory.CloseInventory();

    const std::string& text = Bible::MermaidShopGreeting();
    m_typeWriter.DisplayText(text);

    m_flashlightOn = true;
    Hell::Audio::PlayAudio(AUDIO_SELECT, 1.00f);

    ConsumeInteract();

    //InputMulti::ClearKeyStates();
}

void Player::LeaveShop() {
    m_shopMermaidObjectId = 0;
    m_isInShop = false;
    m_inventory.CloseInventory();
    m_shopInventory.CloseInventory();
}

static float WrapPi(float a) {
    const float twoPi = 6.28318530718f;
    a = std::fmod(a + 3.14159265359f, twoPi);
    if (a < 0.0f) a += twoPi;
    return a - 3.14159265359f;
}

static float LerpAngle(float a, float b, float t) {
    float delta = WrapPi(b - a);
    return a + delta * t;
}

void Player::UpdateShop(float deltaTime) {
    Mermaid* mermaid = World::GetMermaidByObjectId(m_shopMermaidObjectId);
    if (!mermaid) {
        LeaveShop();
        return;
    }

    const glm::vec3 cameraOffsetFromFeet = GetCameraPosition() - GetFootPosition();
    const glm::vec3 targetPosition = mermaid->GetShopTeleportPosition() - cameraOffsetFromFeet;
    const glm::vec3& targetCamEuler = mermaid->GetShopTeleportEuler();
    glm::vec3 currentPosition = GetFootPosition();
    glm::vec3 currentCamEuler = m_camera.GetEulerRotation();

    float positionInterpolationSpeed = 25.0f;
    float rotationInterpolationSpeed = 50.0f;

    float positionT = 1.0f - std::exp(-positionInterpolationSpeed * deltaTime);
    float rotationT = 1.0f - std::exp(-rotationInterpolationSpeed * deltaTime);

    glm::vec3 newPosition = glm::mix(currentPosition, targetPosition, positionT);

    glm::vec3 newCamEuler = currentCamEuler;
    newCamEuler.x = LerpAngle(currentCamEuler.x, targetCamEuler.x, rotationT); // pitch
    newCamEuler.y = LerpAngle(currentCamEuler.y, targetCamEuler.y, rotationT); // yaw
    newCamEuler.z = 0.0f;

    SetFootPosition(newPosition);
    m_camera.SetEulerRotation(newCamEuler);
}

void Player::DiscardItem(const std::string& itemName) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(itemName);
    if (!itemInfo) {
        Logging::Error() << "Player::DiscardItem(..) failed to drop item '" << itemName << "'\n";
        return;
    }

    glm::vec3 spawnPosition = GetCameraPosition() + (GetCameraForward() * 0.5f);

	PickUpCreateInfo createInfo;
	createInfo.position = spawnPosition;
	createInfo.rotation.x = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.rotation.y = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.rotation.z = Hell::Random::Float(-HELL_PI, HELL_PI);
	createInfo.name = itemName;
	createInfo.saveToFile = false;
	createInfo.disablePhysicsAtSpawn = false;
	createInfo.respawn = false;
	createInfo.type = Bible::GetItemType(itemName);

	Unloved::World::AddPickUp(createInfo);
}

bool Player::PurchaseItem(const std::string& itemName) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(itemName);
    if (!itemInfo) return false;

    const ItemType& itemType = itemInfo->GetType();
    const int itemCost = itemInfo->GetCost();

    // Is it a weapon that you already have
    if (itemInfo->GetType() == ItemType::WEAPON && HasWeapon(itemName)) {
		m_typeWriter.DisplayText("You already got one Darlin'.");
        Hell::Audio::PlayAudio("ShopDenied.wav", 1.0f);
        return false;
    }

    // Can you afford it?
	if (m_cash >= itemCost) {
		if (itemType == ItemType::WEAPON) {
			m_inventory.GiveWeapon(itemName);
			m_inventory.GiveAmmo(itemName, itemCost);
			SwitchWeapon(itemName, DRAW_BEGIN);
			SubtractCash(itemCost);
		}
		if (itemType == ItemType::HEAL) {
			m_inventory.AddInventoryItem(itemName);
			SubtractCash(itemCost);
		}
        else {
            Logging::ToDo() << "Bro, Player::PurchaseItem(...) is missing this item type's implementation";
        }

        m_typeWriter.DisplayText(Bible::MermaidShopWeaponPurchaseConfirmationText());
        Hell::Audio::PlayAudio("ShopPurchase2.wav", 1.0f);
        LeaveShop();

        return true;
    }

    // Denied coz you couldn't afford it
	m_typeWriter.DisplayText(Bible::MermaidShopFailedPurchaseText());
    Hell::Audio::PlayAudio("ShopDenied.wav", 1.0f);
	return false;
}

void Player::Respawn() {
    m_inventory.Init();
    m_shopInventory.Init();
    m_health = 100;
    m_isInShop = false;
    m_shopMermaidObjectId = 0;
    m_alive = true;
    m_flashlightOn = false;
    m_awaitingSpawn = false;

    // Get random spawn point
    const SpawnPoint& spawnPoint = Session::GetGameMode() == GameMode::DEATH_MATCH ? Session::GetRandomDeathmatchSpawnPoint() : Session::GetRandomCampaignSpawnPoint();
    glm::vec3 spawnPosition = spawnPoint.GetPosition() - glm::vec3(0.0f, 1.60, 0.0f);

    // Set position and camera rotation to spawn point
    SetFootPosition(spawnPosition);
    m_camera.SetEulerRotation(spawnPoint.GetCameraEuler());

    //if (m_viewportIndex == 0) {
    //    SetFootPosition(glm::vec3(36.18, 31, 37.26));
    //    m_camera.SetEulerRotation(glm::vec3(-0.15, -0.02, 0));
    //}

   // else {
   //     if (m_viewportIndex == 1) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 45.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   //     if (m_viewportIndex == 2) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 55.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   //     if (m_viewportIndex == 3) {
   //         SetFootPosition(glm::vec3(12.5f, 30.6f, 605.5f));
   //         m_camera.SetEulerRotation(glm::vec3(0, 0, 0));
   //     }
   // }

    GiveDefaultLoadout();
    SwitchWeapon("Glock", WeaponAction::DRAW_BEGIN);

    m_camera.Update();
    m_flashlightDirection = m_camera.GetForward();

    if (IsInShop()) {
        m_flashlightDirection += glm::vec3(0.0f, -0.1f, 0.0f);
        m_flashlightDirection = glm::normalize(m_flashlightDirection);
    }

    // Are you inside? Turn flash light on
    float maxRayDistance = 2000;
    glm::vec3 rayOrigin = GetFootPosition() + glm::vec3(0, 2, 0);
    glm::vec3 rayDir = glm::vec3(0, 1, 0);
    PhysXRayResult physxRayResult = Hell::Physics::CastPhysXRay(rayOrigin, rayDir, maxRayDistance, true, std::vector<physx::PxRigidActor*>());
    if (!physxRayResult.hitFound) {
        m_flashlightOn = true;
    }

    // Make character model animated again (aka not ragdoll)
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (characterModel) {
        characterModel->SetAnimationModeToAnimated();
    }

    if (Ragdoll* Ragdoll = GetRagdoll()) {
        Ragdoll->DisableSimulation();
    }

    m_respawnCount++;

    //Logging::Debug() << "Spawned player " << m_viewportIndex << " at " << spawnPosition;
}


void Player::EnableControl() {
    m_controlEnabled = true;
}
void Player::DisableControl() {
    m_controlEnabled = false;
}

const bool Player::IsAwaitingSpawn() {
    return m_awaitingSpawn;
}

const bool Player::HasControl() {
    return m_controlEnabled;
}

const bool Player::IsLocal() const {
    return m_viewportIndex != -1;
}

const bool Player::IsOnline() const {
    return m_viewportIndex == -1;
}

const glm::mat4& Player::GetViewMatrix() const {
    return m_camera.GetViewMatrix();
}

const glm::mat4& Player::GetInverseViewMatrix() const {
    return m_camera.GetInverseViewMatrix();
}

const glm::vec3& Player::GetCameraPosition() const {
    return m_camera.GetPosition();
}

const glm::vec3& Player::GetCameraRotation() const {
    return m_camera.GetEulerRotation();
}

const glm::vec3& Player::GetCameraForward() const {
    return m_camera.GetForward();
}

const glm::vec3& Player::GetCameraRight() const {
    return m_camera.GetRight();
}

const glm::vec3& Player::GetCameraUp() const {
    return m_camera.GetUp();
}

const int32_t Player::GetViewportIndex() const {
    return m_viewportIndex;
}

const glm::vec3 Player::GetFootPosition() const {
    // FIND ME
    PxController* m_characterController = nullptr;
    CharacterController* characterControler = Hell::Physics::GetCharacterControllerById(m_characterControllerId);
    if (characterControler) {
        m_characterController = characterControler->GetPxController();
        PxExtendedVec3 pxPos = m_characterController->getFootPosition();
        return glm::vec3(
            static_cast<float>(pxPos.x),
            static_cast<float>(pxPos.y),
            static_cast<float>(pxPos.z)
        );
    }
    else {
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }
}

Unloved::Camera& Player::GetCamera() {
    return m_camera;
}


AnimatorInstance* Player::GetCharacterModelAnimatorInstance() {
    return Animator::GetAnimatorInstanceByObjectId(m_characterModelAnimatorInstanceId);
}

AnimatorInstance* Player::GetViewWeaponAnimatorInstance() {
    return Animator::GetAnimatorInstanceByObjectId(m_viewWeaponAnimatorInstanceId);
}

SkinnedGameObject* Player::GetCharacterModelSkinnedGameObject() {
    return World::GetSkinnedGameObjectByObjectId(m_characterModelSkinnedGameObjectId);
}

SkinnedGameObject* Player::GetViewWeaponSkinnedGameObject() {
    return World::GetSkinnedGameObjectByObjectId(m_viewWeaponSkinnedGameObjectId);
}

bool Player::ViewportIsVisible() {
    Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    if (!viewport) return false;

    return viewport->IsVisible();
}

float Player::GetWeaponAudioFrequency() {
    return m_weaponAudioFrequency;
}

glm::mat4& Player::GetAnimatedCameraMatrix() {
    return m_animatedCameraMatrix;
}

glm::mat4& Player::GetCSMViewMatrix() {
    return m_csmViewMatrix;
}

void Player::DisplayInfoText(const std::string& text) {
    m_infoTextTimer = 2.0f;
    m_infoText = text;
}

const float Player::GetFov() {
    return m_cameraZoom;
}

void Player::GiveDamage(int damage, uint64_t enemyId) {
    m_health -= damage;
    if (m_health <= 0) {
        m_health = 0;
        Kill(false);
    }
    else {
        TriggerDamageVignette();
    }
}

float Player::DotToClosestToMermaid() {
    Mermaid* mermaid = World::GetMermaidByObjectId(GetFacingMermaidObjectId());
    return mermaid ? glm::dot(mermaid->GetWorldForward(), GetCameraForward()) : 0.0f;
}

void Player::GiveCash(int amount) {
    m_cash += amount;
}

void Player::SubtractCash(int amount) {
    m_cash -= amount;
}

uint64_t Player::GetFacingMermaidObjectId() const {
    const glm::vec3& cameraPosition = GetCameraPosition();
    const glm::vec3& cameraForward = GetCameraForward();
    constexpr float maximumInteractDistance = 2.0f;
    constexpr float minimumDistance = 0.0001f;

    uint64_t closestMermaidObjectId = 0;
    float closestDistance = std::numeric_limits<float>::max();

    for (Mermaid& mermaid : World::GetMermaids()) {
        const glm::vec3 toMermaid = mermaid.GetPosition() - cameraPosition;
        const float distanceToMermaid = glm::length(toMermaid);
        if (distanceToMermaid <= minimumDistance || distanceToMermaid > maximumInteractDistance) continue;

        const glm::vec3 directionToMermaid = toMermaid / distanceToMermaid;
        const bool mermaidInFrontOfCamera = glm::dot(cameraForward, directionToMermaid) > 0.0f;
        const bool cameraInFrontOfMermaid = glm::dot(mermaid.GetWorldForward(), -directionToMermaid) > 0.0f;
        if (!mermaidInFrontOfCamera || !cameraInFrontOfMermaid) continue;

        if (distanceToMermaid < closestDistance) {
            closestDistance = distanceToMermaid;
            closestMermaidObjectId = mermaid.GetObjectId();
        }
    }

    return closestMermaidObjectId;
}

bool Player::IsFacingClosestMermaid() {
    return GetFacingMermaidObjectId() != 0;
}

void Player::Kill(bool wasHeadShot) {
    if (m_alive) {
        m_flashlightOn = false;
        m_deathCount++;
        m_alive = false;
        m_inventory.CloseInventory();
        m_shopInventory.CloseInventory();

        if (SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject()) {
            if (GetRagdoll()) {
                characterModel->SetAnimationModeToRagdoll();
            }
        }

        Hell::Audio::PlayAudio("Death0.wav", 1.0f);
        DropWeapons();
        DropItems();
        m_cash /= 2;

        // HACK
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            if (i != m_viewportIndex) {
                if (Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i)) {
                    player->m_killCount++;
                    if (wasHeadShot) {
                        player->GiveCash(Bible::GetPlayerHeadShotCashReward());
                    }
                    else {
                        player->GiveCash(Bible::GetPlayerKillCashReward());
                    }
                }
            }
        }
    }
}

glm::vec3 Player::GetViewportColorTint() {
    glm::vec3 colorTint = glm::vec3(1, 1, 1);

    if (InventoryIsOpen() && m_inventory.GetInventoryState() == InventoryState::EXAMINE_ITEM) {
        colorTint = glm::vec3(0.325);
    }

    if (IsDead()) {
        colorTint.r = 2.0;
        colorTint.g = 0.2f;
        colorTint.b = 0.2f;

        float waitTime = 3;
        if (m_timeSinceDeath > waitTime) {
            float val = (m_timeSinceDeath - waitTime) * 10;
            colorTint.r -= val;
        }
    }

    //if (m_viewportIndex == 0) {
    //    std::cout << colorTint << "\n";
    //}

    return colorTint;
}

bool Player::HasWeapon(const std::string& weaponName) {
    return m_inventory.HasItem(weaponName);
}

const void Player::SetName(const std::string& name) {
    m_name = name;
}

bool Player::RespawnAllowed() {
    return IsDead() && m_timeSinceDeath > 3.25f;
}


float Player::GetViewportContrast() {
    if (IsAlive()) {
        return 1.0f;
    }
    else {
        return 1.1f;
    }
}

uint64_t Player::GetRagdollId() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return 0;

    return characterModel->GetRagdollId();
}

Ragdoll* Player::GetRagdoll() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    return characterModel ? characterModel->GetRagdoll() : nullptr;
}

bool Player::InventoryIsOpen() {
    return m_inventory.IsOpen();
}

bool Player::InventoryIsClosed() {
    return m_inventory.IsClosed();
}

bool Player::ShopInventoryIsOpen() {
    return m_shopInventory.IsOpen();
}

bool Player::ShopInventoryIsClosed() {
    return m_shopInventory.IsClosed();
}

void Player::TriggerHealVignette() {
	TriggerVignette(glm::vec3(0.0f, 0.2f, 0.0f), 0.4f);
}

void Player::TriggerDamageVignette() {
	TriggerVignette(glm::vec3(0.6f, 0.0f, 0.0f), 0.4f);
}

void Player::TriggerVignette(const glm::vec3& color, float duration) {
	m_vignetteColor = color;
	m_vignetteTimer = duration;
	m_vignetteDuration = duration;
}

} // namespace Unloved
