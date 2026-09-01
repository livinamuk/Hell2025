#include "Player.h"

#include "Hell/Input.h"
#include "Hell/Math/Math.h"

#include "Unloved/Render/Renderer.h"
#include "Unloved/Render/RenderDataManager.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Bible/Bible.h"
#include "Unloved/Systems/Animator/Animator.h"

namespace InputMulti = Hell::InputMulti;

namespace Unloved {

void Player::UpdateViewWeapon(float deltaTime) {
    SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    if (!viewWeapon) return;

    SkinnedModel* skinnedModel = viewWeapon->GetSkinnedModel();

    glm::mat4 dmMaster = glm::mat4(1);
    glm::mat4 cameraMatrix = glm::mat4(1);
    glm::mat4 cameraLocalBindTransform = glm::mat4(1);
    glm::mat4 root = glm::mat4(1);

    for (int i = 0; i < skinnedModel->m_nodes.size(); i++) {
        if (skinnedModel->m_nodes[i].name == "camera") {
            cameraLocalBindTransform = skinnedModel->m_nodes[i].localBindTransform;
        }
    }

    cameraLocalBindTransform = skinnedModel->GetLocalBindTransform("camera");

    // Weapon sway
    float xMax = 5.0;
    float SWAY_AMOUNT = 0.125f;
    float SMOOTH_AMOUNT = 4.0f;
    float SWAY_MIN_X = -xMax;
    float SWAY_MAX_X = xMax;
    float SWAY_MIN_Y = -2;
    float SWAY_MAX_Y = 0.95f;
    float xOffset = (float)InputMulti::GetMouseXOffset(m_mouseIndex);
    float yOffset = (float)InputMulti::GetMouseYOffset(m_mouseIndex);
    float movementX = xOffset * SWAY_AMOUNT;
    float movementY = -yOffset * SWAY_AMOUNT;

    if (GetCurrentWeaponInfo()->itemInfoName == "AKS74U") {
        xMax = 10.0f;
    }

    movementX = std::min(movementX, SWAY_MAX_X);
    movementX = std::max(movementX, SWAY_MIN_X);
    movementY = std::min(movementY, SWAY_MAX_Y);
    movementY = std::max(movementY, SWAY_MIN_Y);

    if (HasControl()) {
        m_weaponSwayX = Hell::Math::InterpTo(m_weaponSwayX, movementX, deltaTime, SMOOTH_AMOUNT);
        m_weaponSwayY = Hell::Math::InterpTo(m_weaponSwayY, movementY, deltaTime, SMOOTH_AMOUNT);
    }

    if (ViewportIsVisible()) {
       //Debug::AddText("m_weaponSwayX: " + std::to_string(m_weaponSwayX));
       //Debug::AddText("m_weaponSwayY: " + std::to_string(m_weaponSwayY));
       //Debug::AddText("xOffset: " + std::to_string(xOffset));
       //Debug::AddText("yOffset: " + std::to_string(yOffset));
    }

    float weaponScale = 0.001f;
    float weaponSwayScale = 0.001f;

    //weaponScale = 0.01f;

    //weaponScale = 0.005f;

    // NEW_RIG_FILE
    // HACK because the old weapons are fucked for scale
    if (GetCurrentWeaponInfo()->itemInfoName == "Knife" ||
        GetCurrentWeaponInfo()->itemInfoName == "Tokarev" ||
        GetCurrentWeaponInfo()->itemInfoName == "Glock" ||
        GetCurrentWeaponInfo()->itemInfoName == "GoldenGlock" ||
        GetCurrentWeaponInfo()->itemInfoName == "SPAS" ||
        GetCurrentWeaponInfo()->itemInfoName == "P90" ||
        GetCurrentWeaponInfo()->itemInfoName == "Remington870" ||
        GetCurrentWeaponInfo()->itemInfoName == "AKS74U"
        ) {
        weaponScale *= 100.0;
    }

    //if (Input::KeyPressed(HELL_KEY_E) && GetCurrentWeaponInfo()->itemInfoName == "P90") {
    //    std::cout << "\nP90\n";
    //    SkinnedModel* skinnedModel = viewWeapon->GetSkinnedModel();
    //    Hell::MeshBuffer& meshBuffer = Hell::ResourceManager::GetMeshBuffer("AssetGeometry");
    //    for (uint32_t meshId : skinnedModel->GetMeshIndices()) {
    //        Mesh* skinnedMesh = meshBuffer.GetMeshById(meshId);
    //        Hell::SkinnedMeshMetadata* metadata = meshBuffer.GetSkinnedMeshMetadataByMeshId(meshId);
    //        if (metadata && metadata->nonDeformingBoneIndex != -1) {
    //            std::cout << skinnedMesh->GetName() << " " << metadata->nonDeformingBoneIndex << "\n";
    //        }
    //    }
    //    std::cout << "\n";
    //    viewWeapon->PrintMeshNames();
    //    viewWeapon->PrintNodeNames();
    //
    //    std::cout << "\n";
    //    std::cout << "GetNodeIndex: " << viewWeapon->GetNodeIndex("Magazine") << "\n";
    //    std::cout << "GetBoneIndex: " << viewWeapon->GetBoneIndex("Magazine") << "\n";
    //
    //
    //    std::vector<Node>& m_nodes = skinnedModel->m_nodes;
    //
    //    std::cout << "\nNodes\n";
    //    for (int i = 0; i < m_nodes.size(); i++) {
    //        Node& node = m_nodes[i];
    //        std::cout << i << " " << node.name << " " << node.parentIndex << "\n";
    //    }
    //}
    //
    //glm::vec3 pos = viewWeapon->GetBoneWorldMatrix("Magazine")[3];
    //glm::vec3 pos2 = viewWeapon->GetBoneWorldMatrix("Weapon")[3];
    //DebugDraw::DrawPoint(pos, RED);
    //DebugDraw::DrawPoint(pos2, GREEN);

    // Final transform
    Transform transform;
    transform.position = m_camera.GetPosition();
    transform.position += (m_weaponSwayX * weaponSwayScale) * m_camera.GetRight();
    transform.position += (m_weaponSwayY * weaponSwayScale) * m_camera.GetUp();
    transform.rotation.x = m_camera.GetEulerRotation().x;
    transform.rotation.y = m_camera.GetEulerRotation().y;
    transform.scale = glm::vec3(weaponScale);

    // HACK because the knife vs non-knife scale mismatch fucks weaponsway
    if (m_weaponAction == WeaponAction::DRAWING || m_weaponAction == WeaponAction::DRAWING_FIRST) {
        m_weaponSwayX = 0.0f;
        m_weaponSwayY = 0.0f;
    }

    viewWeapon->SetCameraMatrix(transform.to_mat4() * glm::inverse(cameraLocalBindTransform) * glm::inverse(dmMaster));
    viewWeapon->EnableModelMatrixOverride();
}

void Player::CalculateMuzzleFlashSpawnPosition() {
	SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
    WeaponInfo* weaponInfo = GetCurrentWeaponInfo();

	if (!viewWeapon) return;
	if (!weaponInfo) return;

	const std::string& boneName = weaponInfo->muzzleFlashBoneName;
	m_muzzleFlashSpawnPosition = viewWeapon->GetNodeWorldPosition(boneName);
}

void Player::UpdateViewWeaponVisibility() {
	SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
	if (!viewWeapon) return;

	bool shouldRenderViewWeapon = true;

	if (InventoryIsOpen() && GetInvetoryState() == InventoryState::EXAMINE_ITEM) shouldRenderViewWeapon = false;
	if (InventoryIsOpen() && GetInvetoryState() == InventoryState::EXAMINE_ITEM) shouldRenderViewWeapon = false;
	if (IsInShop())                                                              shouldRenderViewWeapon = false;

	if (shouldRenderViewWeapon) {
		viewWeapon->EnableRendering();
	}
	else {
		viewWeapon->DisableRendering();
	}

	// Temporarily always render for all viewports
	viewWeapon->SetExclusiveViewportIndex(-1);
}


void Player::SubmitP90MagsRenderItems() {
	SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
	if (!viewWeapon) return;

	SkinnedModel* skinnedModel = viewWeapon->GetSkinnedModel();
	if (!skinnedModel) return;

	// Do you have P90 out?
	if (viewWeapon->GetSkinnedModel()->GetName() == "P90") {

		int ammo = GetCurrentWeaponMagAmmo();

        // Iterate all bullets and hide appropriately
		for (int i = 1; i <= 49; i++) {
            std::string meshName = "";

            int j = 50 - i;

			if (j < 10) {
				meshName = "Bullet_0" + std::to_string(j);
			}
			else {
				meshName = "Bullet_" + std::to_string(j);
			}

            bool hide = (ammo - i > 0);

			if (!hide) {
				m_p90MagMeshNodes.SetBlendingModeByMeshName(meshName, BlendingMode::DO_NOT_RENDER);
			}
            else {
				m_p90MagMeshNodes.SetBlendingModeByMeshName(meshName, BlendingMode::DEFAULT);
			}
		}

		m_p90MagMeshNodes.SetBlendingModeByMeshName("P90_Magazine", BlendingMode::PLASTIC);

        // Update mesh world matrices and submit render items
		glm::mat4 globalBlendedNodeTransform = viewWeapon->GetNodeModelSpaceMatrix("Magazine");
		glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Magazine");
		glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();
		glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset;
		m_p90MagMeshNodes.Update(finalMatrix);
		RenderDataManager::SubmitMeshNodes(m_p90MagMeshNodes, true);

        // Now do it all again for the second mag
        {
			Transform offset;
			offset.position = glm::vec3(0.0f, 0.0f, -1.000003f);
			glm::mat4 offsetMatrix = offset.to_mat4();

			glm::mat4 globalBlendedNodeTransform = viewWeapon->GetNodeModelSpaceMatrix("Magazine2");
			glm::mat4 boneOffset = skinnedModel->GetBoneOffset("Magazine2");
			glm::mat4 modelMatrix = viewWeapon->GetModelMatrix();
			glm::mat4 finalMatrix = modelMatrix * globalBlendedNodeTransform * boneOffset * offsetMatrix;

            m_p90MagMeshNodes.Update(finalMatrix);
            RenderDataManager::SubmitMeshNodes(m_p90MagMeshNodes, true);
        }

		//RenderDataManager::SubmitRenderItems(m_p90MagMeshNodes.GetRenderItems());
		//RenderDataManager::SubmitRenderItemsPlastic(m_p90MagMeshNodes.GetRenderItemsPlastic());

		//if (m_viewportIndex == 0) {
        //    std::cout << "Player 0 has " << m_p90MagMeshNodes.GetRenderItemsPlastic().size() << " plastic render items\n";
		//}
	}
}

} // namespace Unloved
