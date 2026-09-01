#include "Player.h"

#include "Unloved/Render/RenderDataManager.h"

#include <iostream> // TODO: cleanup logging

namespace Unloved {

void Player::UpdateSpriteSheets(float deltaTime) {
    // Muzzle flash
    if (ViewportIsVisible() && GetCurrentWeaponType() != WeaponType::MELEE) {
        SkinnedGameObject* viewWeapon = GetViewWeaponSkinnedGameObject();
        WeaponInfo* weaponInfo = GetCurrentWeaponInfo();
        if (!weaponInfo) {
            std::cout << "Player::UpdateSpriteSheets(float deltaTime) failed: weaponInfo was nullptr!\n";
            return;
        }

        m_muzzleFlash.SetPosition(m_muzzleFlashSpawnPosition);
        m_muzzleFlash.Update(deltaTime);

        //DebugDraw::DrawPoint(boneWorldPosition, YELLOW);

        if (m_muzzleFlash.IsRenderingEnabled() && m_muzzleFlash.GetTimeAsPercentage() < 0.1325f) {
            SpriteSheetRenderItem renderItem = m_muzzleFlash.GetRenderItem();
            renderItem.exclusiveViewportIndex = m_viewportIndex;

            RenderDataManager::SubmitSpriteSheetRenderItem(renderItem);
        }
    }
}

} // namespace Unloved
