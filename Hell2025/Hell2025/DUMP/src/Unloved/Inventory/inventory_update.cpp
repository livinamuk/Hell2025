#include "Inventory.h"

#include "Hell/Audio.h"
#include "Hell/Input.h"
#include "Hell/Logging.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/Renderer.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Session/Session.h"

namespace Audio = Hell::Audio;
namespace Input = Hell::Input;
namespace InputMulti = Hell::InputMulti;

namespace Unloved {


void Inventory::Update(float deltaTime) {
    //if (Input::KeyPressed(HELL_KEY_M)) {
    //    SetGridCountX(6);
    //}
    //if (Input::KeyPressed(HELL_KEY_7)) {
    //    MoveItem(5, 4, 2, true);
    //}

    //SetGridCountX(6);
    ////SetGridCountY(4);
    //MoveItem(5, 4, 2, true);

    if (m_state == InventoryState::MAIN_SCREEN) UpdateItemViewScreen(deltaTime);
    if (m_state == InventoryState::SHOP) UpdateItemViewScreen(deltaTime);
    if (m_state == InventoryState::EXAMINE_ITEM) UpdateExamineScreen(deltaTime);
}


void Inventory::UpdateItemViewScreen(float deltaTime) {
    Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(m_localPlayerIndex);
    if (!player) return;

    // WASD cell input
    if (player->PressedLeft(true))  StepDirection(-1, 0);
    if (player->PressedRight(true)) StepDirection(1, 0);
    if (player->PressedUp(true))    StepDirection(0, -1);
    if (player->PressedDown(true))  StepDirection(0, 1);

    //InventoryItemInfo* itemInfo = Bible::GetInventoryItemInfoByName(GetSelectedItemName());

	// Buttons
	if (player->PressedInventoryExamine() && GetSelectedItemIndex() != -1) {
		InitMeshNodesFromSelectedItem();
		SetState(InventoryState::EXAMINE_ITEM);
		Audio::PlayAudio(AUDIO_SELECT, 1.00f);
		m_examineRotationX = 0.0F;
		m_examineRotationY = 0.0f;
		m_examineZoom = 0.0f;
	}

    if (m_state == InventoryState::SHOP) {
        if (player->PressedInteract()) {
            player->PurchaseItem(GetSelectedItemName());
            player->ConsumeInteract();
        }
    }
	else {
		ItemInfo* itemInfo = Bible::GetItemInfoByName(GetSelectedItemName());
        if (itemInfo) {

			// Use item
			if (player->PressedInteract()) {
				if (player->CanUseItem(GetSelectedItemName())) {
					player->UseItem(GetSelectedItemName());
					player->LeaveShop();
					RemoveItemByIndex(GetSelectedItemIndex());
					Audio::PlayAudio(AUDIO_SELECT, 1.00f);
				}
				else {
					Logging::Debug() << "Player " << player->GetViewportIndex() << " tried to use '" << GetSelectedItemName() << "' but cannot\n";
				}
			}
			// Discard item
			if (player->PressedInventoryDiscard()) {
				if (itemInfo->IsDiscardable()) {
					player->DiscardItem(GetSelectedItemName());
                    RemoveItemByIndex(GetSelectedItemIndex());
					player->LeaveShop();
					Audio::PlayAudio("DiscardItem.wav", 1.50f);

					// If the item is a weapon, switch to knife, and remove it from your weapon states.
                    if (itemInfo->GetType() == ItemType::WEAPON) {
                        player->SwitchWeapon("Knife", WeaponAction::DRAW_BEGIN);

                        // TODO: wrap in a reliable function
                        for (WeaponState& weaponState : m_weaponStates) {
                            if (weaponState.name == itemInfo->GetName()) {
                                weaponState.has = false;
                                break;
                            }
                        }
                    }
				}
				else {
					Logging::Debug() << "Player " << player->GetViewportIndex() << " tried to use '" << GetSelectedItemName() << "' but cannot\n";
				}
			}
        }
    }
}

void Inventory::InitMeshNodesFromSelectedItem() {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(GetSelectedItemName());
    if (!itemInfo) return;

    Bible::ConfigureMeshNodesByItemName(NO_ID, itemInfo->m_name, &m_examineItemMeshNodes, false);
}

void Inventory::UpdateExamineScreen(float deltaTime) {
    ItemInfo* itemInfo = Bible::GetItemInfoByName(GetSelectedItemName());
    Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(m_localPlayerIndex);

    if (!itemInfo) return;
    if (!player) return;

    Model* model = Hell::ResourceManager::GetModelByName(itemInfo->GetModelName());
    if (!model) {
        m_examineItemMeshNodes.CleanUp();
        return;
    }

    if (Input::KeyPressed(HELL_KEY_ENTER)) {
        Bible::Init();
    }

    float rotateSensitivity = 0.25f;
    float zoomSensitivity = 10.0f;
    float verticalRotationLimit = HELL_PI * 0.75f;

    float zoomMinLimit = 1.0f;
    float zoomMaxLimit = itemInfo->m_examineInfo.maxZoom;

    // Rotate
    if (player->PressingFire()) {
        m_examineRotationX += InputMulti::GetMouseYOffset(player->m_mouseIndex) * deltaTime * rotateSensitivity;
        m_examineRotationY -= InputMulti::GetMouseXOffset(player->m_mouseIndex) * deltaTime * rotateSensitivity;
    }

    // Zoom
    if (Input::MouseWheelUp()) {
        m_examineZoom += deltaTime * zoomSensitivity;
    }
    if (Input::MouseWheelDown()) {
        m_examineZoom -= deltaTime * zoomSensitivity;
    }

    // Clamp vertical rotation
    m_examineRotationX = glm::clamp(m_examineRotationX, -verticalRotationLimit, verticalRotationLimit);

    // Clamp zoom rotation
    m_examineZoom = glm::clamp(m_examineZoom, zoomMinLimit, zoomMaxLimit);

    Transform rotX;
    rotX.rotation.x = m_examineRotationX;

    Transform rotY;
    rotY.rotation.y = m_examineRotationY;

    Transform zoomTransform;
    zoomTransform.scale = glm::vec3(m_examineZoom);

    const glm::vec3 aabbMin = model->GetAABBMin();
    const glm::vec3 aabbMax = model->GetAABBMax();
    const glm::vec3 center = 0.5f * (aabbMin + aabbMax);
    const glm::mat4 centeringMatrix = glm::translate(glm::mat4(1.0f), -center);

    Transform initialTransform;
    initialTransform.position = itemInfo->m_examineInfo.translation;
    initialTransform.rotation = itemInfo->m_examineInfo.rotation;
    initialTransform.scale = itemInfo->m_examineInfo.scale;

    glm::mat4 modelMatrix = rotX.to_mat4() * rotY.to_mat4() * initialTransform.to_mat4() * zoomTransform.to_mat4() * centeringMatrix;

    //Renderer::DrawItemExamineAABB(AABB(aabbMin, aabbMax), YELLOW);
    m_examineItemMeshNodes.Update(modelMatrix);
}

void Inventory::StepDirection(int dx, int dy) {
    Audio::PlayAudio(AUDIO_SELECT, 1.0f);

    int startX = m_selectedCellX;
    int startY = m_selectedCellY;
    int startIndex = InBounds(startX, startY) ? m_itemIndex2DArray[startX][startY] : -1;

    int x = (startX + dx + m_gridCountX) % m_gridCountX;
    int y = (startY + dy + m_gridCountY) % m_gridCountY;

    // Only skip contiguous cells if we STARTED on an item
    if (startIndex != -1) {
        // Skip all cells that belong to the same item
        while (m_itemIndex2DArray[x][y] == startIndex) {
            if (x == startX && y == startY) break; // safety
            x = (x + dx + m_gridCountX) % m_gridCountX;
            y = (y + dy + m_gridCountY) % m_gridCountY;
        }
    }

    m_selectedCellX = x;
    m_selectedCellY = y;
}

}
