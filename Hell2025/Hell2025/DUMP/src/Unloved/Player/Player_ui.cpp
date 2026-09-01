#include "Player.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Enum.h"
#include "Hell/Common/String.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"
#include "Hell/ResourceManagement/ResourceManager.h"

#include "Unloved/Render/RenderDataManager.h"
#include "Unloved/Render/Renderer.h"

#include "Unloved/Config/Config.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/DebugDraw.h"
#include "Unloved/EditorSession/EditorSession.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/LadderDismount.h"
#include "Unloved/Viewport/ViewportManager.h"
#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/ObjectId.h"
#include "Unloved/World/World.h"

#include <glm/geometric.hpp>
#include <vector>

namespace Unloved {

void Player::UpdateUI(float deltaTime) {
    if (EditorSession::IsActive()) return;

    Unloved::Viewport* viewport = Unloved::ViewportManager::GetViewportByIndex(m_viewportIndex);
    if (!viewport->IsVisible()) return;


    const Resolutions& resolutions = Config::GetResolutions();
    int width = resolutions.ui.x * viewport->GetSize().x;
    int height = resolutions.ui.y * viewport->GetSize().y;
    int xLeft = resolutions.ui.x * viewport->GetPosition().x;
    int xRight = xLeft + width;

    float viewportTop = viewport->GetPosition().y;
    int yTop = resolutions.ui.y * viewportTop;
    int yBottom = yTop + height;
    int centerX = xLeft + (width / 2);
    int centerY = yTop + (height / 2);
    int ammoX = xRight - (width * 0.17f);
    int ammoY = yBottom - (height * 0.145f) - TextBlitter::GetFontSpriteSheet("AmmoFont")->m_charHeight - -TextBlitter::GetFontSpriteSheet("AmmoFont")->m_lineSpacing;

    // Set type writer position based on viewport coords
    int margin = 100;
    glm::ivec2 typeWriteLocation;
    typeWriteLocation.x = xLeft + margin;
    typeWriteLocation.y = ammoY + 40;
    m_typeWriter.SetLocation(typeWriteLocation);

   //if (Input::KeyPressed(HELL_KEY_Q)) {
   //    std::string text = "SHIT PISS FUCK CUNT COCK SUCKER MOTHER FUCKER TITS FART TURD AND TWAT";
   //    m_typeWriter.DisplayText(text, 3);
   //}

    m_typeWriter.Update(deltaTime);

   //if (Debug::IsDebugTextVisible()) {
   //    return;
   //}

    // Info text
    int infoTextX = xLeft + (width * 0.1f);
    int infoTextY = ammoY;

    if (m_inventory.IsOpen()) {
        m_inventory.SubmitRenderItems();
    }
    if (m_shopInventory.IsOpen()) {
        m_shopInventory.SubmitRenderItems();
    }


    //std::string text = "Locked...";
    //UIBackEnd::BlitText("[COL=0.839,0.784,0.635]" + text, "RobotoCondensed", 150, 1080 - 150, Alignment::TOP_LEFT, 2.0f, TextureFilter::LINEAR);


//   glm::ivec2 location = glm::ivec2(centerX, centerY);
//   location = glm::ivec2(64, yTop + 64);
//   //glm::ivec2 size = glm::ivec2(-1, -1);
//
//   std::string texName = "inv10";
//   if (Input::KeyDown(HELL_KEY_Y)) {
//       texName = "inv11";
//   }
//   Texture* texture = Hell::ResourceManager::GetTextureByName(texName);
//   //glm::ivec2 size = glm::ivec2(-1, -1);
//   glm::ivec2 size = glm::ivec2(texture->GetWidth(0), texture->GetHeight(0));
//
//   UIBackEnd::BlitTexture(texName, location, Alignment::TOP_LEFT, WHITE, size, TextureFilter::NEAREST);
//
//   location = glm::ivec2(750, yTop + 64);
//   //UIBackEnd::BlitTexture("inv2", location, Alignment::TOP_LEFT, WHITE, size, TextureFilter::LINEAR);
//


    // Multiplayer Mode Text
    if (IsAlive() && Debug::GetDebugTextMode() == DebugTextMode::NONE) {
        std::string text = "Health: " + std::to_string(m_health) + "\n";
        text += "Cash: $" + std::to_string(m_cash) + "\n";
        text += "Kills: " + std::to_string(m_killCount) + "\n";
        text += "\n";

        UIBackEnd::BlitText(text, "StandardFont", xLeft, yTop, Alignment::TOP_LEFT, 2.0f);
    }


    // HUD
    if (IsAlive() && !IsInShop()) {
        // Cross hair texture
        std::string crosshairTexture = "CrosshairDot";
        if (m_interactFound) {
            crosshairTexture = "CrosshairSquare";
        }

        UIBackEnd::BlitText(m_infoText, "StandardFont", infoTextX, infoTextY, Alignment::TOP_LEFT, 2.0f);
        UIBackEnd::BlitTexture(crosshairTexture, glm::ivec2(centerX, centerY), Alignment::CENTERED, WHITE, glm::ivec2(128, 128));

        // Ammo
        if (GetCurrentWeaponType() != WeaponType::MELEE) {
            float scale = 1.3f;
            float smallScale = scale * 0.8f;

            int slashPadding = 10;
            std::string clipText = std::to_string(GetCurrentWeaponMagAmmo());
            std::string totalText = std::to_string(GetCurrentWeaponTotalAmmo());

            // Mag ammo color
            if (GetCurrentWeaponMagAmmo() == 0) {
                clipText = "[COL=0.8,0.05,0.05,1]" + clipText;
            }
            else {
                clipText = "[COL=0.16,0.78,0.23,1]" + clipText;
            }

            UIBackEnd::BlitText(clipText, "AmmoFont", ammoX - slashPadding, ammoY, Alignment::TOP_RIGHT, scale, TextureFilter::LINEAR);
            UIBackEnd::BlitText("/", "AmmoFont", ammoX, ammoY, Alignment::CENTERED_HORIZONTAL, scale, TextureFilter::LINEAR);
            UIBackEnd::BlitText(totalText, "AmmoFont", ammoX + slashPadding, ammoY, Alignment::TOP_LEFT, smallScale, TextureFilter::LINEAR);

            // SPAS AUTO
            WeaponState* weaponState = GetCurrentWeaponState();
            WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
            if (weaponInfo->hasAutoSwitch) {
                Texture* texture = Hell::ResourceManager::GetTextureByName("Weapon_Auto");
                if (GetCurrentWeaponType() == WeaponType::SHOTGUN && texture) {
                    int modifierPadding = 29;
                    int modifierX = TextBlitter::GetTextSize(totalText, "AmmoFont", smallScale).x + modifierPadding;
                    int gridSize = 10;
                    modifierX = (modifierX / 10) * 10;
                    int modifierScaleX = texture->GetWidth() * smallScale;
                    int modifierScaleY = texture->GetHeight() * smallScale;
                    glm::vec4 unselectedColor = glm::vec4(0.541, 0.51, 0.392, 0.5f);
                    glm::vec4 colorAuto = weaponState->shotgunInAutoMode ? WHITE : unselectedColor;
                    glm::vec4 colorPump = weaponState->shotgunInAutoMode ? unselectedColor : WHITE;
                    float padding = 4;
                    float autoY = ammoY;
                    float pumpY = autoY + ((texture->GetHeight() + padding) * smallScale);

                    int negHack = -9 + 9;
                    int hack = 7 + 9;

                    std::string shellTextureName = "ShotgunShellRed";
                    if (weaponState->shotgunSlug) {
                        shellTextureName = "ShotgunShellGreen";
                    }
                    Texture* texture = Hell::ResourceManager::GetTextureByName(shellTextureName);
                    int shellScaleX = texture->GetWidth() * smallScale;
                    int shellScaleY = texture->GetHeight() * smallScale * 1.1f;

                    UIBackEnd::BlitTexture(shellTextureName, glm::ivec2(ammoX + modifierX + negHack, autoY), Alignment::TOP_LEFT, WHITE, glm::ivec2(shellScaleX, shellScaleY), TextureFilter::LINEAR);
                    UIBackEnd::BlitTexture("Weapon_Auto", glm::ivec2(ammoX + modifierX + hack, autoY), Alignment::TOP_LEFT, colorAuto, glm::ivec2(modifierScaleX, modifierScaleY), TextureFilter::LINEAR);
                    UIBackEnd::BlitTexture("Weapon_Pump", glm::ivec2(ammoX + modifierX + hack, pumpY), Alignment::TOP_LEFT, colorPump, glm::ivec2(modifierScaleX, modifierScaleY), TextureFilter::LINEAR);
                }
            }
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER_WEAPON_INFO) {
            WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
            WeaponState* weaponstate = GetCurrentWeaponState();
            AmmoInfo* ammoInfo = GetCurrentAmmoInfo();
            AmmoState* ammoState = GetCurrentAmmoState();
            if (!weaponInfo || !weaponstate ||!ammoInfo || !ammoState) return;

            std::string text;
            text += "Weapon state name: " + weaponstate->name + "\n";
            text += "Item info name: " + weaponInfo->itemInfoName + "\n";
            text += "Ammo info name: " + weaponInfo->ammoInfoName + "\n";
            text += "Type: " + Hell::Enum::ToString(weaponInfo->type) + "\n";
            text += "\n";
            text += "Weapon action: " + Hell::Enum::ToString(m_weaponAction) + "\n";
            text += "Ammo in mag: " + std::to_string(weaponstate->ammoInMag) + "\n";
            text += "Ammo on hand: " + std::to_string(ammoState->ammoOnHand) + "\n";

            UIBackEnd::BlitText(text, "StandardFont", xLeft, yTop, Alignment::TOP_LEFT, 2.0f);
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER_LADDER_INFO) {
            const glm::vec3 footPosition = GetFootPosition();
            const float searchDistance = GetLadderSearchDistance();
            const LadderCandidate candidate = FindLadderCandidate();

            std::string text = "LADDER INFO\n";
            text += "Movement Mode: " + Hell::Enum::ToString(m_movementMode) + "\n";
            text += "World Ladders: " + std::to_string(World::GetLadders().size()) + "\n";
            text += "World Dismount Points: " + std::to_string(World::GetLadderDismounts().size()) + "\n";
            text += "Search Distance: " + Hell::String::FormatFloat(searchDistance) + "\n";
            text += "Foot Position: " + Hell::String::FormatVec3(footPosition) + "\n";
            text += "Active Ladder: ";
            text += m_ladderMoveData.ladderId ? std::to_string(m_ladderMoveData.ladderId) : "NONE";
            text += "\n";

            if (m_movementMode == PlayerMovementMode::LADDER) {
                text += "Ladder Velocity: " + Hell::String::FormatVec3(m_ladderMoveData.ladderVelocity) + "\n";
                text += "Ladder Speed: " + Hell::String::FormatFloat(glm::length(m_ladderMoveData.ladderVelocity)) + "\n";
                text += "Ladder T: " + Hell::String::FormatFloat(m_ladderMoveData.ladderParametricPosition) + "\n";
                text += "Move Direction: " + Hell::String::FormatFloat(m_ladderMoveData.ladderMoveDirection) + "\n";
                text += "Jump Held: " + Hell::String::FormatBool(PressingJump()) + "\n";

                if (Ladder* ladder = World::GetLadderByObjectId(m_ladderMoveData.ladderId)) {
                    const glm::vec3 ladderAxis = glm::normalize(ladder->GetTopPoint() - ladder->GetBottomPoint());
                    text += "View/Axis Dot: " + Hell::String::FormatFloat(glm::dot(GetCameraForward(), ladderAxis)) + "\n";
                    text += "Distance To Bottom: " + Hell::String::FormatFloat(glm::distance(footPosition, ladder->GetBottomPoint())) + "\n";
                    text += "Distance To Top: " + Hell::String::FormatFloat(glm::distance(footPosition, ladder->GetTopPoint())) + "\n";
                }
            }

            if (m_movementMode == PlayerMovementMode::LADDER_TRANSITION) {
                text += "Transition Type: ";
                text += m_ladderMoveData.mounting ? "MOUNT\n" : "DISMOUNT\n";
                text += "Transition Time: " + Hell::String::FormatFloat(m_ladderMoveData.transitionElapsedTime) +
                        " / " + Hell::String::FormatFloat(m_ladderMoveData.transitionDuration) + "\n";
                text += "Transition Start: " + Hell::String::FormatVec3(m_ladderMoveData.transitionStartPosition) + "\n";
                text += "Transition Goal: " + Hell::String::FormatVec3(m_ladderMoveData.transitionGoalPosition) + "\n";
                text += "Destination Reserved: " + Hell::String::FormatBool(m_ladderMoveData.destinationReserved) + "\n";

                DebugDraw::DrawLine(m_ladderMoveData.transitionStartPosition, m_ladderMoveData.transitionGoalPosition, RED);
                DebugDraw::DrawPoint(m_ladderMoveData.transitionGoalPosition, RED);
            }

            if (m_movementMode == PlayerMovementMode::LADDER || m_movementMode == PlayerMovementMode::LADDER_TRANSITION) {
                text += "Dismount Status: " + Hell::Enum::ToString(m_ladderMoveData.dismountStatus) + "\n";
                text += "Associated Dismounts: " + std::to_string(m_ladderMoveData.associatedDismountCount) + "\n";
                text += "Nearby Dismounts: " + std::to_string(m_ladderMoveData.nearbyDismountCount) + "\n";
                text += "Dismount Candidate: ";
                text += m_ladderMoveData.dismountCandidateId ? std::to_string(m_ladderMoveData.dismountCandidateId) : "NONE";
                text += "\n";

                if (m_ladderMoveData.dismountCandidateId) {
                    text += "Dismount Distance: " + Hell::String::FormatFloat(m_ladderMoveData.dismountCandidateDistance) + "\n";
                    text += "Dismount View Dot: " + Hell::String::FormatFloat(m_ladderMoveData.dismountCandidateViewDot) + "\n";

                    if (LadderDismount* dismount = World::GetLadderDismountByObjectId(m_ladderMoveData.dismountCandidateId)) {
                        DebugDraw::DrawLine(footPosition, dismount->GetPosition(), PINK);
                        DebugDraw::DrawPoint(dismount->GetPosition(), PINK);
                    }
                }
            }

            DebugDraw::DrawSphere(footPosition, searchDistance, BLUE);

            if (candidate.ladderId) {
                text += "Candidate Ladder: " + std::to_string(candidate.ladderId) + "\n";
                text += "Candidate Distance: " + Hell::String::FormatFloat(glm::distance(footPosition, candidate.closestPoint)) + "\n";
                text += "Closest Point: " + Hell::String::FormatVec3(candidate.closestPoint) + "\n";
                text += "Cone Mount Match: " + Hell::String::FormatBool(ShouldAutoMountLadderCone(candidate)) + "\n";
                text += "Endpoint Mount Match: " + Hell::String::FormatBool(ShouldAutoMountLadderEndpoint(candidate)) + "\n";
                text += "Mount Goal Clear: " + Hell::String::FormatBool(IsLadderTransitionGoalClear(candidate.closestPoint, candidate.ladderId)) + "\n";

                if (Ladder* ladder = World::GetLadderByObjectId(candidate.ladderId)) {
                    text += "Bottom Point: " + Hell::String::FormatVec3(ladder->GetBottomPoint()) + "\n";
                    text += "Top Point: " + Hell::String::FormatVec3(ladder->GetTopPoint()) + "\n";

                    DebugDraw::DrawLine(ladder->GetBottomPoint(), ladder->GetTopPoint(), GREEN);
                    DebugDraw::DrawLine(footPosition, candidate.closestPoint, WHITE);
                    DebugDraw::DrawPoint(ladder->GetBottomPoint(), GREEN);
                    DebugDraw::DrawPoint(ladder->GetTopPoint(), GREEN);
                    DebugDraw::DrawPoint(candidate.closestPoint, YELLOW);

                    const std::vector<RenderItem>& renderItems = ladder->GetRenderItems();
                    if (!renderItems.empty()) {
                        AABB ladderBounds;
                        for (const RenderItem& renderItem : renderItems) {
                            ladderBounds.Grow(glm::vec3(renderItem.aabbMin));
                            ladderBounds.Grow(glm::vec3(renderItem.aabbMax));
                        }
                        DebugDraw::DrawAABB(ladderBounds, YELLOW);
                    }
                }
            }
            else {
                text += "Candidate Ladder: NONE\n";
            }

            UIBackEnd::BlitText(text, "StandardFont", xLeft, yTop, Alignment::TOP_LEFT, 2.0f);
        }

        if (Debug::GetDebugTextMode() == DebugTextMode::PER_PLAYER || Debug::GetDebugRenderMode() == DebugRenderMode::BVH_CPU_PLAYER_RAYS) {

            std::string text = "";
            text += "Feet Pos: " + Hell::String::FormatVec3(GetFootPosition()) + "\n";
            text += "Cam Pos: " + Hell::String::FormatVec3(GetCameraPosition()) + "\n";
            text += "Cam Euler: " + Hell::String::FormatVec3(GetCameraRotation()) + "\n";

            text += "\n";

            SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
            SkinnedModel* model = viewWeapon->GetSkinnedModel();


			// Some matrices
			if (false) {
				const ViewportData& viewportData = RenderDataManager::GetViewportData()[0];
				text += "Projection Matrix:\n";
				text += Hell::String::FormatMat4(viewportData.projection, 10) + "\n\n";;
				text += "Projection Matrix Reverse Z:\n";
				text += Hell::String::FormatMat4(viewportData.projectionReverseZ, 10) + "\n\n";;
				text += "View Matrix:\n";
				text += Hell::String::FormatMat4(viewportData.view, 10) + "\n\n";
			}

            // Magazine matrices
            if (false) {
                text += "Magazine Local Bind Transform:\n";
                text += Hell::String::FormatMat4(viewWeapon->GetLocalBindTransformByNodeName("Magazine"), 10) + "\n\n";
                text += "Magazine Animated Transform:\n";
                text += Hell::String::FormatMat4(viewWeapon->GetNodeModelSpaceMatrix("Magazine"), 10) + "\n\n";
            }

            // Camera matrices
            if (false) {
                const ViewportData& viewportData = RenderDataManager::GetViewportData()[0];

                text += "Root:\n";
                text += Hell::String::FormatMat4(viewWeapon->GetLocalBindTransformByNodeName("root"), 10) + "\n\n";
                text += "Camera Local Bind Transform:\n";
                text += Hell::String::FormatMat4(viewWeapon->GetLocalBindTransformByNodeName("camera"), 10) + "\n\n";
                text += "Camera Animated Transform:\n";
                text += Hell::String::FormatMat4(viewWeapon->GetNodeModelSpaceMatrix("camera"), 10) + "\n\n";
                text += "View Matrix:\n";
                text += Hell::String::FormatMat4(viewportData.view, 10) + "\n\n";
            }

            // Kangaroos
            if (false) {
                if (Unloved::World::GetKangaroos().size()) {
                    Kangaroo& kangaroo = Unloved::World::GetKangaroos()[0];
                    text += kangaroo.GetDebugInfoString();
                }
            }

            // Inventory
            if (false) {
                text += "Unloved::Inventory State: " + Hell::Enum::ToString(m_inventory.GetInventoryState()) + "\n";
                for (auto item : m_inventory.GetItems()) {
                    text += "- " + item.m_name;
                    text += " [" + std::to_string(item.m_gridLocation.x) +"][" + std::to_string(item.m_gridLocation.y) + "]";
                    text += "\n";
                }
            }

            // Movement
            if (false) {
                text += "IsMoving: " + Hell::String::FormatBool(IsMoving()) + "\n";
            }

            // Weapons
            if (false) {
                text += "Weapon Action: " + Hell::Enum::ToString(GetCurrentWeaponAction()) + "\n";
            }

            // Interact
            //if (false) {
            //    text += "Interact object: " + Hell::Enum::ToString(m_interactObjectId) + " " + std::to_string(m_interactObjectId) + "\n";
            //}

            // Rays
            //if (true) {
            //    text += "BVH ray: " + Hell::Enum::ToString(Unloved::GetObjectIdType(m_bvhRayResult.objectId)) + " " + std::to_string(m_bvhRayResult.objectId) + "\n";
            //    text += "PhysX ray: " + Hell::Enum::ToString(m_physXRayResult.userData.objectType) + " " + std::to_string(m_physXRayResult.userData.objectId) + " " + Hell::Enum::ToString(m_physXRayResult.userData.physicsType) + " " + std::to_string(m_physXRayResult.userData.physicsId) + "\n";
            //    text += "Ray hit found: " + Hell::String::FormatBool(m_rayHitFound) + " " + Hell::Enum::ToString(m_rayHitObjectType) + " " + std::to_string(m_rayhitObjectId) + "\n";
            //    text += "Feet above height field: " + Hell::String::FormatBool(m_feetAboveHeightField) + "\n";
            //}


            // Render items
            if (false) {
                text += "RenderItems ALPHA_DISCARD: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::ALPHA_DISCARD)) + "\n";
                text += "RenderItems BLENDED: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::BLENDED)) + "\n";
                text += "RenderItems DEFAULT: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::DEFAULT)) + "\n";
                text += "RenderItems GLASS: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::GLASS)) + "\n";
                text += "RenderItems HAIR: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::HAIR)) + "\n";
                text += "RenderItems MIRROR: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::MIRROR)) + "\n";
                text += "RenderItems TOILET_WATER: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::TOILET_WATER)) + "\n";
                text += "RenderItems PLASTIC: " + std::to_string(RenderDataManager::GetRenderItemCount(BlendingMode::PLASTIC)) + "\n";
                text += "\n";
                text += "RenderItems Skinned ALPHA_DISCARD: " + std::to_string(RenderDataManager::GetSkinnedRenderItemCount(BlendingMode::ALPHA_DISCARD)) + "\n";
                text += "RenderItems Skinned BLENDED: " + std::to_string(RenderDataManager::GetSkinnedRenderItemCount(BlendingMode::BLENDED)) + "\n";
                text += "RenderItems Skinned DEFAULT: " + std::to_string(RenderDataManager::GetSkinnedRenderItemCount(BlendingMode::DEFAULT)) + "\n";
                text += "RenderItems Skinned HAIR: " + std::to_string(RenderDataManager::GetSkinnedRenderItemCount(BlendingMode::HAIR)) + "\n";
                text += "\n";
                text += "RenderItems PRODEDURAL: " + std::to_string(RenderDataManager::GetRenderItemIndicesProcedural().size()) + "\n";
            }

            // Movement
            if (false) {
                text += "Movement Dir: " + Hell::String::FormatVec3(m_movementDirection) + "\n";
                text += "Acceleration: " + std::to_string(m_acceleration) + "\n";
                text += "Y Velocity: " + std::to_string(m_yVelocity) + "\n";
            }

            // Physx Object Count
            if (false) {
                text += "\n";
                text += Hell::Physics::GetObjectCountsAsString();
                text += "\n";
            }

            // Shark
            if (false) {
                for (Shark& shark : Unloved::World::GetSharks()) {
                    text += shark.GetDebugInfoAsString();
                }
            }

            glm::vec3 rayOrigin = GetCameraPosition();
            glm::vec3 rayDir = GetCameraForward();
            float maxRayDistance = 100.0f;

            if (false) {
                text += "\n";
                text += "Flip normal map Y: " + Hell::String::FormatBool(OpenGL::Renderer::ShouldFlipNormalMapY()) + "\n";
            }

            // Override with BVH CPU RAYS if that render mode is set
            if (Debug::GetDebugRenderMode() == DebugRenderMode::BVH_CPU_PLAYER_RAYS) {
                text += "\nBVH ray hit: " + Hell::String::FormatBool(m_bvhRayResult.hitFound) + "\n";

                if (m_bvhRayResult.hitFound) {
                    MeshNode* meshNode = World::GetMeshNodeByObjectIdAndLocalNodeIndex(m_bvhRayResult.objectId, m_bvhRayResult.localMeshNodeIndex);

                    uint64_t hitId = m_bvhRayResult.objectId;
                    ObjectType hitType = Unloved::GetObjectIdType(hitId);
                    text += "- Hit pos: " + Hell::String::FormatVec3(m_bvhRayResult.hitPosition) + "\n";
                    text += "- Parent type: " + Hell::Enum::ToString(hitType) + "\n";
                    text += "- Mesh name: " + Hell::ResourceManager::GetMeshBuffer("AssetGeometry").GetMeshNameByMeshId(m_bvhRayResult.globalMeshIndex) + "\n";
                    text += "- Parent Id: " + std::to_string(Unloved::GetObjectIdLocal(m_bvhRayResult.objectId)) + "\n";
                    text += "- Openable Id: " + std::to_string(m_bvhRayResult.openableId) + "\n";
                    text += "- Custom Id: " + std::to_string(m_bvhRayResult.customId) + "\n";
                    text += "- Mesh node index: " + std::to_string(m_bvhRayResult.localMeshNodeIndex) + "\n";
                    text += "- Global mesh index: " + std::to_string(m_bvhRayResult.globalMeshIndex) + "\n";

                    if (meshNode) {
                        text += "- BlendingMode: " + Hell::Enum::ToString(meshNode->blendingMode) + "\n";
                        text += "- World AABB min: " + Hell::String::FormatVec3(meshNode->worldspaceAabb.GetBoundsMin()) + "\n";
                        text += "- World AABB max: " + Hell::String::FormatVec3(meshNode->worldspaceAabb.GetBoundsMax()) + "\n";
                    }

                    if (Openable* openable = Unloved::OpenableManager::GetOpenableByOpenableId(m_bvhRayResult.openableId)) {
                        text += "\n";
                        text += "Open state: " + Hell::Enum::ToString(openable->m_currentOpenState) + "\n";
                        text += "Value: " + std::to_string(openable->m_currentOpenValue) + "\n";
                        text += "Min: " + std::to_string(openable->m_minOpenValue) + "\n";
                        text += "Max: " + std::to_string(openable->m_maxOpenValue) + "\n";
                        text += "Dirty: " + Hell::String::FormatBool(openable->m_dirty) + "\n";
                        text += "Transform pos: " + Hell::String::FormatVec3(openable->m_transform.position) + "\n";
                        text += "Transform rot: " + Hell::String::FormatVec3(openable->m_transform.rotation) + "\n";

                    }
                }
            }

            UIBackEnd::BlitText(text, "StandardFont", xLeft, yTop, Alignment::TOP_LEFT, 2.0f);
        }

    }

    // Press Start
    if (RespawnAllowed()) {
        static Texture* texture = Hell::ResourceManager::GetTextureByName("PressStart");
        if (texture) {
            static int width = texture->GetWidth() * 2;
            static int height = texture->GetHeight() * 2;
            glm::ivec2 location = glm::ivec2(centerX, centerY);
            glm::ivec2 size = glm::ivec2(width, height);
            UIBackEnd::BlitTexture("PressStart", location, Alignment::CENTERED, RED, size, TextureFilter::LINEAR);
        }
    }

    //std::string name = "FontTest_LockedFromTheOtherSide";
    ////name = "FontTest_LockedWithAKey";
    ////name = "FontTest_YouUnlockedIt";
    //
    //if (Texture* texture = Hell::ResourceManager::GetTextureByName(name)) {
    //    int width = texture->GetWidth() * 2;
    //    int height = texture->GetHeight() * 2;
    //    int marginX = 100;
    //    int marginY = 120;
    //    glm::ivec2 location = glm::ivec2(marginX, yBottom - marginY);
    //    glm::ivec2 size = glm::ivec2(width, height);
    //    UIBackEnd::BlitTexture(name, location, Alignment::BOTTOM_LEFT, WHITE, size, TextureFilter::NEAREST);
    //}
}

} // namespace Unloved
