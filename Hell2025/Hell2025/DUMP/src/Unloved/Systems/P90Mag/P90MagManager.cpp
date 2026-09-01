#include "P90MagManager.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Render/Renderer.h"

#include "Hell/Math/GLM.h"

namespace P90MagManager {
    
    void SubmitMagForRender(const glm::mat4& worldTransform, uint32_t ammoInMag) {
        //DebugDraw::DrawPoint(worldTransform[3], RED);
    }
    
    void SubmitRenderItems() {
        for (int i = 0; i < Unloved::Session::GetLocalPlayerCount(); i++) {
            Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(i);
            if (!player) continue;

            if (player->GetSelectedWeaponName() == "P90") {

                //glm::mat4 test = player-> 

                Transform transform;
                transform.position = player->GetCameraPosition();

                SubmitMagForRender(transform.to_mat4(), 50);
            }
        }
        // check every player, see if they have a p90
        // if so submit a mag for rendering also

        // now, also check every "pickup"
        // if it's a p90, submit a mag for rendering also
    }
}