#include "Player.h"

namespace Unloved {

void Player::InitCharacterModel() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetSkinnedModel("UniSexGuyScaled");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Body", "UniSexGuyBody");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Eye", "UniSexGuyBody");
    characterModel->SetMeshMaterialByMeshName("Biker_Jeans", "UniSexGuyJeans");
    characterModel->SetMeshMaterialByMeshName("CC_Base_Eye", "UniSexGuyEyes");
    characterModel->SetMeshMaterialByMeshName("Glock", "Glock");
    characterModel->SetMeshMaterialByMeshName("SM_Knife_01", "Knife");
    characterModel->SetMeshMaterialByMeshName("Shotgun_Mesh", "Shotgun");
    characterModel->SetMeshMaterialByMeshIndex(13, "UniSexGuyHead");
    characterModel->SetMeshMaterialByMeshIndex(14, "UniSexGuyLashes");
    //characterModel->EnableBlendingByMeshIndex(14);
    characterModel->SetMeshMaterialByMeshName("FrontSight_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("Receiver_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("BoltCarrier_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("SafetySwitch_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("MagRelease_low", "AKS74U_0");
    characterModel->SetMeshMaterialByMeshName("Pistol_low", "AKS74U_2");
    characterModel->SetMeshMaterialByMeshName("Trigger_low", "AKS74U_1");
    characterModel->SetMeshMaterialByMeshName("Magazine_Housing_low", "AKS74U_3");
    characterModel->SetMeshMaterialByMeshName("BarrelTip_low", "AKS74U_4");
}

void Player::UpdateCharacterModelHacks() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
    WeaponState* weaponState = GetCurrentWeaponState();

    if (!characterModel) return;
    if (!weaponInfo) return;
    if (!weaponState) return;


    characterModel->SetAllMeshBlendingModes(BlendingMode::DEFAULT);

    if (IsAlive()) {

        if (weaponInfo->type == WeaponType::MELEE) {
            HideAKS74UMesh();
            HideGlockMesh();
            HideShotgunMesh();
            if (IsMoving()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Knife_Walk", 1.0f);
            }
            else {
                PlayAndLoopCharacterAnimation("UnisexGuy_Knife_Idle", 1.0f);
            }
            if (IsCrouching()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Knife_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::PISTOL) {
            HideAKS74UMesh();
            HideShotgunMesh();
            HideKnifeMesh();
            if (IsMoving()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Glock_Walk", 1.0f);
            }
            else {
                PlayAndLoopCharacterAnimation("UnisexGuy_Glock_Idle", 1.0f);
            }
            if (IsCrouching()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Glock_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::AUTOMATIC) {
            HideShotgunMesh();
            HideKnifeMesh();
            HideGlockMesh();
            if (IsMoving()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_AKS74U_Walk", 1.0f);
            }
            else {
                PlayAndLoopCharacterAnimation("UnisexGuy_AKS74U_Idle", 1.0f);
            }
            if (IsCrouching()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_AKS74U_Crouch", 1.0f);
            }
        }
        if (weaponInfo->type == WeaponType::SHOTGUN) {
            HideAKS74UMesh();
            HideKnifeMesh();
            HideGlockMesh();
            if (IsMoving()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Shotgun_Walk", 1.0f);
            }
            else {
                PlayAndLoopCharacterAnimation("UnisexGuy_Shotgun_Idle", 1.0f);
            }
            if (IsCrouching()) {
                PlayAndLoopCharacterAnimation("UnisexGuy_Shotgun_Crouch", 1.0f);
            }
        }

        characterModel->SetPosition(GetFootPosition());
        characterModel->SetRotationY(m_camera.GetEulerRotation().y + HELL_PI);

        // Push the body behind the camera so it cannot shadow the view weapon
        glm::vec3 cameraForwardXZ = m_camera.GetForward() * glm::vec3(1.0f, 0.0f, 1.0f);
        float cameraForwardXZLengthSquared = glm::dot(cameraForwardXZ, cameraForwardXZ);
        
        if (cameraForwardXZLengthSquared > 0.0001f) {
            cameraForwardXZ = glm::normalize(cameraForwardXZ);
        
            bool foundEyeCenter = false;
            glm::vec3 eyeCenter = glm::vec3(0.0f);
        
            if (characterModel->GetNodeIndex("CC_Base_L_Eye") != -1 && characterModel->GetNodeIndex("CC_Base_R_Eye") != -1) {
                glm::vec3 leftEyePosition = characterModel->GetNodeWorldPosition("CC_Base_L_Eye");
                glm::vec3 rightEyePosition = characterModel->GetNodeWorldPosition("CC_Base_R_Eye");
                eyeCenter = (leftEyePosition + rightEyePosition) * 0.5f;
                foundEyeCenter = true;
            }
            else if (characterModel->GetNodeIndex("CC_Base_Head") != -1) {
                eyeCenter = characterModel->GetNodeWorldPosition("CC_Base_Head");
                foundEyeCenter = true;
            }
        
            if (foundEyeCenter) {
                float eyeDepth = glm::dot(eyeCenter - m_camera.GetPosition(), cameraForwardXZ);
                float desiredEyeDepth = -0.05f;
        
                if (eyeDepth > desiredEyeDepth) {
                    glm::vec3 correction = cameraForwardXZ * (desiredEyeDepth - eyeDepth);
                    characterModel->SetPosition(characterModel->GetPosition() + correction);
                }
            }
        }
    }
    else {
        HideKnifeMesh();
        HideGlockMesh();
        HideShotgunMesh();
        HideAKS74UMesh();
    }
}

void Player::HideKnifeMesh() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetBlendingModeByMeshName("SM_Knife_01", BlendingMode::DO_NOT_RENDER);
}

void Player::HideGlockMesh() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetBlendingModeByMeshName("Glock", BlendingMode::DO_NOT_RENDER);
}

void Player::HideShotgunMesh() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetBlendingModeByMeshName("Shotgun_Mesh", BlendingMode::DO_NOT_RENDER);
}

void Player::HideAKS74UMesh() {
    SkinnedGameObject* characterModel = GetCharacterModelSkinnedGameObject();
    if (!characterModel) return;

    characterModel->SetBlendingModeByMeshName("FrontSight_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("Receiver_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("BoltCarrier_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("SafetySwitch_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("MagRelease_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("Pistol_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("Trigger_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("Magazine_Housing_low", BlendingMode::DO_NOT_RENDER);
    characterModel->SetBlendingModeByMeshName("BarrelTip_low", BlendingMode::DO_NOT_RENDER);
}

} // namespace Unloved
