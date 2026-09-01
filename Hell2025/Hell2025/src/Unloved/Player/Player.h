#pragma once
#include "PlayerTypes.h"

#include "Hell/BVH/Types.h"
#include "Hell/Math/AABB.h"
#include "Hell/Math/Transform.h"
#include "Hell/Math/VecXZ.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Common/Types.h"
#include "Unloved/Bible/Bible_enums.h"

#include "Unloved/Camera/Camera.h"
#include "Unloved/Camera/Frustum.h"
#include "Unloved/Characters/Humanoid/AnimatedHumanoid.h"
#include "Unloved/Inventory/Inventory.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Objects/Renderables/SpriteSheetObject.h"
#include "Unloved/Systems/Animator/Animator.h"
#include "Unloved/Systems/TypeWriter/TypeWriter.h"
#include "Unloved/Weapons/WeaponCommon.h"

namespace Unloved {

enum class PlayerMovementMode {
    NONE,
    WALKING,
    SWIMMING,
    LADDER,
    LADDER_TRANSITION
};

enum class LadderDismountStatus {
    NONE,
    WAITING_FOR_ENDPOINT,
    NO_ASSOCIATED_POINTS,
    NO_NEARBY_POINTS,
    DESTINATION_BLOCKED_OR_RESERVED,
    NOT_FACING_POINT,
    STRICT_REJECTED,
    TRANSITION_REJECTED,
    STARTED
};

struct Ladder;
struct Mermaid;

struct Player {
public:
    struct MeleeAttackState {
        bool active = false;
        MeleeAttackInfo attackInfo;
        WeaponAction weaponAction = WeaponAction::IDLE;
        std::string weaponName;
        int32_t lastSampledFrame = -1;
        uint64_t hitGroupId = 0;
    };

    // Lifecycle
    void Init(uint64_t playerId, const glm::vec3& position, const glm::vec3& rotation, int32_t viewportIndex);
    void CleanUp();
    void BeginFrame();
    void Update(float deltaTime);
    void PostWorldUpdate();
    void Respawn();
    void Kill(bool wasHeadShot);
    void GiveDamage(int damage, uint64_t enemyId);
    void EnableControl();
    void DisableControl();

    // Frame updates
    void UpdateCamera(float deltaTime);
    void UpdateMovement(float deltaTime);
    void UpdateCursorRays();
    void UpdateInteract();
    void UpdateViewWeapon(float deltaTime);
    void UpdateWeaponLogic(float deltaTime);
    void UpdateWeaponSlide();
    void UpdateWeaponAttachments();
    void UpdateViewWeaponVisibility();
    void UpdateHeadBob(float deltaTime);
    void UpdateBreatheBob(float deltaTime);
    void UpdateAudio(float deltaTime);
    void UpdateFlashlight(float deltaTime);
    void UpdateFlashlightFrustum();
    void UpdateUI(float deltaTime);
    void UpdateSpriteSheets(float deltaTime);
    void UpdatePlayingPiano(float deltaTime);
    void UpdateShop(float deltaTime);
    void UpdateVignette(float deltaTime);

    // Player setup
    const void SetName(const std::string& name);
    void SetCameraHeightModifier(float modifier) { m_cameraHeightModifier = modifier; }
    void CreateCharacterController(const glm::vec3& position);
    void SetFootPosition(glm::vec3 position);
    void AddHorizontalImpulse(glm::vec3 direction, float force);
    void AddVerticalImpulse(float force);
    void SimulateVelocityMovement(float deltaTime, const glm::vec3& inputDirection, float maxSpeed, float accelerationStrength, float damping, bool applyGravity, float gravity);

    // Input setup
    void SetKeyboardIndex(int32_t index);
    void SetMouseIndex(int32_t index);
    void ConsumeInteract();

    // Inventory and shop
    void DiscardItem(const std::string& itemName);
    void UseItem(const std::string& itemName);
    void EnterShop(uint64_t mermaidObjectId);
    void LeaveShop();
    void GiveCash(int amount);
    void SubtractCash(int amount);

    // Weapon inventory and selection
    void GiveDefaultLoadout();
    void GiveWeapon(const std::string& name);
    void GiveSight(const std::string& weaponName);
    void GiveSilencer(const std::string& weaponName);
    void SwitchWeapon(const std::string& name, WeaponAction weaponAction);
    void NextWeapon();
    void DropWeapons();
    void DropItems();

    // View weapon animation
    void PlayViewWeaponAnimation(Bible::AnimationSlot animationSlot, float speed);
    void PlayAndLoopViewWeaponAnimation(Bible::AnimationSlot animationSlot, float speed);

    void PlayCharacterWeaponAnimation(Bible::AnimationSlot animationSlot);
    void PlayAndLoopCharacterWeaponAnimation(Bible::AnimationSlot animationSlot);

    // Weapon logic
    void UpdateMeleeLogic(float deltaTime);
    void UpdateGunLogic(float deltaTime);
    void UpdateShotgunGunLogic(float deltaTime);
    void BeginMeleeAttack(Bible::AnimationSlot animationSlot);
    void UpdateMeleeAttack();
    void SpawnMeleeHitSample(uint32_t animationFrame);

    // Melee
    void FireMelee();

    // Gun
    void FireGun();
    void ReloadGun();
    void EnterADS();
    void LeaveADS();
    void UpdateGunReloadLogic();
    void UpdateSlideLogic();
    void UpdateADSLogic(float deltaTime);
    void SpawnBullet(float variance);
    void SpawnUnderWaterBullet(float variance);

    // Shotgun
    void FireShotgun();
    void SecondaryMelee();
    void DryFireShotgun();
    void ReloadShotgun();
    void ToggleAutoShotgun();
    void UpdatePumpAudio();
    void UpdateShotgunReloadLogic();

    // Weapon rendering
    void SubmitP90MagsRenderItems();
    void SpawnMuzzleFlash(float speed, float scale);
    void SpawnCasing();
    void CalculateMuzzleFlashSpawnPosition();

    // Piano
    void SitAtPiano(uint64_t pianoId);

    // Feedback
    void DisplayInfoText(const std::string& text);
    void TriggerHealVignette();
    void TriggerDamageVignette();
    void TriggerVignette(const glm::vec3& color, float duration);

    // Player queries
    bool RespawnAllowed();
    const std::string& GetName() const { return m_name; }
    const uint64_t GetPlayerId() { return m_playerId; }
    const bool IsAlive() { return m_alive; }
    const bool IsDead() { return !m_alive; }
    const bool IsAwaitingSpawn();
    const bool HasControl();
    const bool IsLocal() const;
    const bool IsOnline() const;
    bool IsMoving();
    bool IsGrounded();
    bool IsCrouching();
    const bool IsRunning() { return m_running; }
    const bool AreFeetAboveHeightField() { return m_feetAboveHeightField; }

    // Camera and viewport queries
    const glm::mat4& GetViewMatrix() const;
    const glm::mat4& GetInverseViewMatrix() const;
    const glm::vec3& GetCameraPosition() const;
    const glm::vec3& GetCameraRotation() const;
    const glm::vec3& GetCameraForward() const;
    const glm::vec3& GetCameraRight() const;
    const glm::vec3& GetCameraUp() const;
    const int32_t GetViewportIndex() const;
    Unloved::Camera& GetCamera();
    float GetCameraHeightModifier() const { return m_cameraHeightModifier; }
    glm::vec3 GetViewportColorTint();
    float GetViewportContrast();
    const float GetFov();
    glm::ivec2 GetViewportCenter();
    glm::mat4& GetAnimatedCameraMatrix();
    glm::mat4& GetCSMViewMatrix();
    bool ViewportIsVisible();

    // Movement and physics queries
    const glm::vec3 GetFootPosition() const;
    PxShape* GetCharacterControllerShape() const;
    PxRigidDynamic* GetCharacterControllerActor() const;
    const uint64_t GetcharacterControllerId() { return m_characterControllerId; }
    const AABB& GetCharacterControllerAABB() { return m_characterControllerAABB; }
    float GetTargetWalkingSpeed();
    Hell::ivecXZ GetChunkPos() { return m_chunkPos; }

    // Input queries
    int32_t GetKeyboardIndex();
    int32_t GetMouseIndex();
    bool PressingWalkForward();
    bool PressingWalkBackward();
    bool PressingWalkLeft();
    bool PressingWalkRight();
    bool PressingCrouch();
    bool PressedUp(bool allowKeyRepeat = false);
    bool PressedDown(bool allowKeyRepeat = false);
    bool PressedLeft(bool allowKeyRepeat = false);
    bool PressedRight(bool allowKeyRepeat = false);
    bool PressedWalkForward(bool allowKeyRepeat = false);
    bool PressedWalkBackward(bool allowKeyRepeat = false);
    bool PressedWalkLeft(bool allowKeyRepeat = false);
    bool PressedWalkRight(bool allowKeyRepeat = false);
    bool PressedInteract();
    bool PressingInteract();
    bool PressedReload();
    bool PressedFire();
    bool PressingFire();
    bool PressingJump();
    bool PressingRun();
    bool PressedRun();
    bool PressedCrouch();
    bool PressedWeaponMiscFunction();
    bool PressedNextWeapon();
    bool PressingADS();
    bool PressedADS();
    bool PressedEscape();
    bool PressedMelee();
    bool PressedFlashlight();
    bool PressedToggleInventory();
    bool PressedInventoryExamine();
    bool PressedInventoryDiscard();
    bool PressedFullscreen();
    bool PressedOne();
    bool PressedTwo();
    bool PressedThree();
    bool PressedFour();

    // Interaction queries
    bool InteractFound() { return m_interactFound; }
    uint64_t GetInteractObjectId() { return m_interactObjectId; }
    const glm::vec3& GetInteractHitPosition() { return m_interactHitPosition; }
    const glm::vec3& GetInteractHitNormal() { return m_interactHitNormal; }
    uint64_t GetClosestMirrorId();
    uint64_t GetFacingMermaidObjectId() const;
    bool IsFacingClosestMermaid();
    float DotToClosestToMermaid();

    // Inventory and shop queries
    bool PurchaseItem(const std::string& itemName);
    bool CanUseItem(const std::string& itemName);
    bool InventoryIsOpen();
    bool InventoryIsClosed();
    bool ShopInventoryIsOpen();
    bool ShopInventoryIsClosed();
    bool IsInShop() { return m_isInShop; }
    const InventoryState& GetInvetoryState() { return m_inventory.GetInventoryState(); }
    Unloved::Inventory& GetInventory() { return m_inventory; }
    glm::mat4 GetItemExamineModelMatrix() { return m_inventory.GetItemExamineModelMatrix(); }
    int GetCash() { return m_cash; }

    // Weapon queries
    bool HasWeapon(const std::string& weaponName);
    const std::string& GetSelectedWeaponName();
    int GetCurrentWeaponMagAmmo();
    int GetCurrentWeaponTotalAmmo();
    bool IsShellInShotgunChamber();
    WeaponAction& GetWeaponAction();
    WeaponType GetCurrentWeaponType();
    WeaponAction GetCurrentWeaponAction();
    WeaponInfo* GetCurrentWeaponInfo();
    WeaponState* GetWeaponStateByName(const std::string &name);
    WeaponState* GetCurrentWeaponState();
    AmmoState* GetCurrentAmmoState();
    const AmmoInfo* GetCurrentAmmoInfo();
    float GetWeaponAudioFrequency();
    bool IsViewWeaponAnimationComplete();
    uint32_t GetViewWeaponAnimationFrameNumber();
    bool IsViewWeaponAnimationPastFrameNumber(uint32_t frameNumber);
    glm::mat4 GetViewWeaponBoneWorldMatrix(const std::string& boneName);
    bool CanFireMelee();
    bool CanFireGun();
    bool CanReloadGun();
    bool CanEnterADS();
    bool CanLeaveADS();
    bool IsInADS();
    bool CanToggleShotgunAuto();
    bool CanFireShotgun();
    bool CanDryFireShotgun();
    bool CanSecondaryMelee();
    bool CanReloadShotgun();
    bool ShotgunRequiresPump();
    glm::vec3& GetMuzzleFlashSpawnPosition() { return m_muzzleFlashSpawnPosition; }

    // Animation ish
    AnimatorInstance* GetViewWeaponAnimatorInstance();
    SkinnedGameObject* GetViewWeaponSkinnedGameObject();
    Ragdoll* GetRagdoll();
    uint64_t GetRagdollId();

    AnimatedHumanoid& GetAnimatedHumanoid();

    // Flashlight queries
    Unloved::Frustum& GetFlashlightFrustum() { return m_flashlightFrustum; }
    const glm::vec3 GetFlashlightPosition() { return m_flashlightPosition; }
    const glm::vec3 GetFlashlightDirection() { return m_flashlightDirection; };
    const glm::mat4 GetFlashlightProjectionView() { return m_flashlightProjectionView; };
    const float GetFlashLightModifer() { return m_flashLightModifier; }
    const uint64_t GetFlashlightSpotLightId() { return m_flashlightSpotLightId; }

    // Water queries
    bool FeetEnteredUnderwater();
    bool FeetExitedUnderwater();
    bool CameraEnteredUnderwater();
    bool CameraExitedUnderwater();
    bool IsSwimming();
    bool IsWading();
    bool CameraIsUnderwater();
    bool FeetBelowWater();
    bool StartedSwimming();
    bool StoppedSwimming();
    bool StartingWading();
    bool StoppedWading();
    float GetFeetDistanceBeneathWater();

    // Piano queries
    bool IsPlayingPiano();

    // Feedback queries
    const glm::vec3& GetVignetteColor() const { return m_vignetteColor; }
    float GetVignetteDuration() const { return m_vignetteDuration; }
    float GetVignetteTimer() const { return m_vignetteTimer; }
    float GetVignettIntensityScalar() const { return m_vignetteIntensityScalar; }

    Inventory m_inventory;
    Inventory m_shopInventory;
    TypeWriter m_typeWriter;
    WaterState m_waterState;
    MeleeAttackState m_meleeAttackState;

    MeshNodes m_supressor;
    MeshNodes m_redDot;
    MeshNodes m_p90MagMeshNodes;
    Hell::Transform m_weaponSwayTransform;

    glm::mat4 m_deathCamViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_weaponSwayMatrix = glm::mat4(1);
    glm::mat4 m_animatedCameraMatrix;
    glm::mat4 m_csmViewMatrix;

    glm::vec3 m_movementDirection = glm::vec3(0.0f);

    std::string m_infoText = "";

    uint64_t m_pianoId = 0;

    float m_timeSinceDeath = 0.0f;
    float m_ladderFootstepAudioTimer = 0;
    float m_ladderFootstepAudioLoopLength = 0.5;
    float m_acceleration = 0.0f;
    float m_infoTextTimer = 0;
    float _muzzleFlashTimer = 0;
    float m_weaponAudioFrequency = 1.0f;
    float m_walkTiltTimer = 0.0f;
    float m_yVelocity = 0;
    float m_weaponSwayX = 0;
    float m_weaponSwayY = 0;
    float m_swimVerticalAcceleration = 0.0f;
    float m_smoothedWaterY;

    int m_killCount = 0;
    int m_health = 100;
    int m_mouseIndex = -1;
    int m_keyboardIndex = -1;
    int m_cash = 0;

    bool m_isPlayingPiano = false;
    bool m_isInShop = false;
    uint64_t m_shopMermaidObjectId = 0;
    bool m_pistolAwaitingFireReleased = false;

private:
    struct LadderMoveData {
        uint64_t ladderId = 0;
        uint64_t dismountCandidateId = 0;
        glm::vec3 ladderVelocity = glm::vec3(0.0f);
        glm::vec3 transitionStartPosition = glm::vec3(0.0f);
        glm::vec3 transitionGoalPosition = glm::vec3(0.0f);
        float ladderParametricPosition = 0.0f;
        float ladderMoveDirection = 0.0f;
        float dismountCandidateDistance = 0.0f;
        float dismountCandidateViewDot = 0.0f;
        float transitionElapsedTime = 0.0f;
        float transitionDuration = 0.0f;
        int32_t associatedDismountCount = 0;
        int32_t nearbyDismountCount = 0;
        LadderDismountStatus dismountStatus = LadderDismountStatus::NONE;
        bool mounting = false;
        bool destinationReserved = false;
        bool startedThisFrame = false;
        bool simulationShapeWasEnabled = false;
        bool sceneQueryShapeWasEnabled = false;
    };

    struct LadderCandidate {
        uint64_t ladderId = 0;
        uint64_t dismountId = 0;
        glm::vec3 closestPoint = glm::vec3(0.0f);
    };

    // Ladder movement
    void LeaveLadder(const glm::vec3& velocity);
    void UpdateMovementMode();
    void UpdateWalkingMovement(float deltaTime);
    void UpdateSwimmingMovement(float deltaTime);
    void UpdateLadderMovement(float deltaTime);
    void UpdateLadderTransition(float deltaTime);
    float ScaleSourceLadderDistance(float sourceDistance) const;
    float GetLadderSearchDistance() const;
    LadderCandidate FindLadderCandidate(uint64_t skipLadderId = 0) const;
    bool ShouldAutoMountLadderCone(const LadderCandidate& candidate);
    bool ShouldAutoMountLadderEndpoint(const LadderCandidate& candidate);
    bool TryAutoMountLadder();
    bool IsLadderTransitionGoalClear(const glm::vec3& goalPosition, uint64_t ladderId) const;
    bool IsLadderTransitionGoalReserved(const glm::vec3& goalPosition) const;
    bool StartLadderTransition(bool mounting, const LadderCandidate& candidate);
    bool TryExitLadderViaDismountNode(Ladder& ladder, bool strict);

    const std::string& GetViewWeaponModelName();

    // Identity and ownership
    std::string m_name = "PLAYER_NAME";
    uint64_t m_playerId = 0;
    uint64_t m_characterControllerId = 0;
    int m_playerIndex;
    int32_t m_viewportIndex = 0;
    bool m_alive = true;
    bool m_controlEnabled = true;
    bool m_awaitingSpawn = true;
    int32_t m_deathCount = 0;
    int32_t m_respawnCount = 0;

    AnimatedHumanoid m_animatedHumanoid;

    // View weapon
    uint64_t m_viewWeaponAnimatorInstanceId = 0;
    uint32_t m_viewWeaponAnimationLayerIndex = 0;
    uint64_t m_viewWeaponSkinnedGameObjectId = 0;

    // Input
    PlayerControls m_controls;
    InputType m_inputType = KEYBOARD_AND_MOUSE;

    // Camera
    Camera m_camera;
    float m_mouseSensitivity = 0.002f;
    float m_cameraZoom = 1.0f;
    float m_cameraHeightModifier = 0.0f;

    // Interact
    PhysXRayResult m_physXRayResult;
    BvhRayResult m_bvhRayResult;
    uint64_t m_interactObjectId = 0;
    uint32_t m_interactOpenableId = 0;
    uint32_t m_interactCustomId = 0;
    glm::vec3 m_interactHitPosition = glm::vec3(0.0f);
    glm::vec3 m_interactHitNormal = glm::vec3(0.0f, 0.0f, 1.0f);
    bool m_rayHitFound = false;
    bool m_interactFound = false;

    // Flashlight
    Frustum m_flashlightFrustum;
    glm::vec3 m_flashlightPosition;
    glm::vec3 m_flashlightDirection;
    glm::mat4 m_flashlightProjectionView;
    uint64_t m_flashlightSpotLightId = 0;
    bool m_flashlightOn = false;
    float m_flashLightModifier = 0.0f;

    // Movement
    AABB m_characterControllerAABB;
    Hell::ivecXZ m_chunkPos;
    PlayerMovementMode m_movementMode = PlayerMovementMode::WALKING;
    LadderMoveData m_ladderMoveData;
    bool m_moving = false;
    bool m_crouching = false;
    bool m_grounded = true;
    bool m_groundedLastFrame = true;
    bool m_jumpAnimationActive = false;
    bool m_stationaryJumpTailEligible = false;
    bool m_feetAboveHeightField = false;
    bool m_running = false;
    float m_currentSpeed = 0.0f;
    float m_walkingSpeed = 4.25f;
    float m_runningSpeed = 2.5f;
    float m_crouchingSpeed = 2.325f;
    float m_swimmingSpeed = 3.25f;
    float m_crouchDownSpeed = 17.5f;
    float m_viewHeightStanding = 1.65f;
    float m_viewHeightCrouching = 1.15f;
    float m_currentViewHeight = m_viewHeightStanding;
    float m_waterImpactVelocity = 0;

    // Camera motion and audio
    float m_bobOffsetX = 0.0f;
    float m_bobOffsetY = 0.0f;
    float m_headBobTime = 0.0f;
    float m_breatheBobTime = 0.0f;
    glm::vec3 m_headBob = glm::vec3(0.0f);
    glm::vec3 m_breatheBob = glm::vec3(0.0f);
    bool m_footstepPlayed = false;

    // Weapon state
    ShellEjectionState m_shellEjectionState;
    WeaponAction m_weaponAction = DRAW_BEGIN;
    int m_revolverReloadIterations = 0;
    int m_currentWeaponIndex = 0;
    bool m_firedThisFrame = false;
    bool _needsAmmoReloaded = false;
    bool m_revolverNeedsCocking = false;
    bool _glockSlideNeedsToBeOut = false;
    bool _needsShotgunFirstShellAdded = false;
    bool _needsShotgunSecondShellAdded = false;
    float m_accuracyModifer = 0;

    // Weapon rendering
    SpriteSheetObject m_muzzleFlash;
    glm::vec3 m_muzzleFlashSpawnPosition;
    float _muzzleFlashRotation = 0;
    glm::vec2 _weaponSwayFactor = glm::vec2(0);
    glm::vec3 _weaponSwayTargetPos = glm::vec3(0);
    float _muzzleFlashCounter = 0;

    // Vignette
    glm::vec3 m_vignetteColor = glm::vec3(0);
    float m_vignetteDuration = 0.0f;
    float m_vignetteTimer = 0.0f;
    float m_vignetteIntensityScalar = 0.0f;
};

}
