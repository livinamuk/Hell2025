#include "Debug_menu.h"

#include "Unloved/Bible/Bible.h"
#include "Unloved/Objects/Props/GenericAnimatedObject.h"
#include "Unloved/World/World.h"

#include <cstdint>
#include <string>
#include <vector>

namespace Debug::Menu::GenericAnimatedObjectMenu {

    enum struct Action : uint32_t {
        CROUCH_BLEND = 0,
        MOVEMENT_BLEND,
        ARMS_HEAD_ANIMATION,
        DEBUG_DRAW,
        DEBUG_DRAW_EJECTION_PORT,
    };

    const std::vector<Unloved::AnimationSlot> ARMS_HEAD_ANIMATION_SLOTS = {
        Unloved::AnimationSlot::FIRE_1,
        Unloved::AnimationSlot::FIRE_2,
        Unloved::AnimationSlot::FIRE_3,
        Unloved::AnimationSlot::RELOAD,
        Unloved::AnimationSlot::RELOAD_EMPTY
    };

    void BuildMenu();
    void ApplyEdit(uint32_t id, const Value& value);

    void RegisterMenu() {
        RegisterRootPage("Generic Animated Object", "GENERIC ANIMATED OBJECT", BuildMenu, ApplyEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMenu() {
        float crouchBlend = 0.0f;
        float movementBlend = 0.0f;
        int32_t armsHeadAnimationIndex = 0;
        bool debugDraw = false;
        bool debugDrawEjectionPort = false;
        std::vector<std::string> armsHeadAnimationNames = { "None" };

        for (Unloved::AnimationSlot animationSlot : ARMS_HEAD_ANIMATION_SLOTS) {
            armsHeadAnimationNames.push_back(Bible::GetAnimation(Unloved::AnimationProfile::RAT_KING_GLOCK, animationSlot));
        }

        for (const Unloved::GenericAnimatedObject& genericAnimatedObject : Unloved::World::GetGenericAnimatedObjects()) {
            crouchBlend = genericAnimatedObject.GetCrouchBlend();
            movementBlend = genericAnimatedObject.GetMovementBlend();
            debugDraw = genericAnimatedObject.GetDebugDraw();
            debugDrawEjectionPort = genericAnimatedObject.GetDebugDrawEjectionPort();

            for (int32_t i = 0; i < static_cast<int32_t>(ARMS_HEAD_ANIMATION_SLOTS.size()); i++) {
                if (genericAnimatedObject.GetWeaponAnimationName() == Bible::GetAnimation(Unloved::AnimationProfile::RAT_KING_GLOCK, ARMS_HEAD_ANIMATION_SLOTS[i])) {
                    armsHeadAnimationIndex = i + 1;
                    break;
                }
            }
            break;
        }

        AddFloat(static_cast<uint32_t>(Action::CROUCH_BLEND), "Crouch blend", crouchBlend, 0.0f, 1.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Action::MOVEMENT_BLEND), "Movement blend", movementBlend, 0.0f, 1.0f, 0.05f, 2);
        AddEnum(static_cast<uint32_t>(Action::ARMS_HEAD_ANIMATION), "Arms and head animation", armsHeadAnimationIndex, armsHeadAnimationNames);
        AddBool(static_cast<uint32_t>(Action::DEBUG_DRAW), "Debug draw", debugDraw);
        AddBool(static_cast<uint32_t>(Action::DEBUG_DRAW_EJECTION_PORT), "Debug draw ejection port", debugDrawEjectionPort);
    }

    void ApplyEdit(uint32_t id, const Value& value) {
        // Bail if there are no Generic Animated Objects
        if (Unloved::World::GetGenericAnimatedObjects().empty()) return;

        // Otherwise get object 0
        Unloved::GenericAnimatedObject& genericAnimatedObject = Unloved::World::GetGenericAnimatedObjects()[0];

        // Animation
        if (id == static_cast<uint32_t>(Action::ARMS_HEAD_ANIMATION)) {
            // If somehow out of range..
            if (value.intValue < 0 || value.intValue > static_cast<int32_t>(ARMS_HEAD_ANIMATION_SLOTS.size())) {
                return;
            }

            // If selection wraps...
            if (value.intValue == 0) {
                genericAnimatedObject.SetWeaponAnimationName(Bible::GetAnimation(Unloved::AnimationProfile::RAT_KING_GLOCK, Unloved::AnimationSlot::IDLE));
            }

            // Apply selection 
            if (value.intValue > 0) {
                genericAnimatedObject.SetWeaponAnimationName(Bible::GetAnimation(Unloved::AnimationProfile::RAT_KING_GLOCK, ARMS_HEAD_ANIMATION_SLOTS[value.intValue - 1]));
            }
        } 

        // Crouch blend
        if (id == static_cast<uint32_t>(Action::CROUCH_BLEND)) {
            genericAnimatedObject.SetCrouchBlend(value.floatValue);
        }

        // Movement blend
        if (id == static_cast<uint32_t>(Action::MOVEMENT_BLEND)) { 
            genericAnimatedObject.SetMovementBlend(value.floatValue);
        }

        // Debug draw
        if (id == static_cast<uint32_t>(Action::DEBUG_DRAW)) {
            genericAnimatedObject.SetDebugDraw(value.boolValue);
        }

        if (id == static_cast<uint32_t>(Action::DEBUG_DRAW_EJECTION_PORT)) {
            genericAnimatedObject.SetDebugDrawEjectionPort(value.boolValue);
        }
    }
}
