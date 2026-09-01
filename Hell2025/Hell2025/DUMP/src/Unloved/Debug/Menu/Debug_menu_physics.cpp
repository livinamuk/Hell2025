#include "Debug_menu.h"

#include "Unloved/Config/PhysicsConfig.h"
#include "Unloved/Debug/Debug.h"
#include "Unloved/EditorSession/EditorSession.h"

#include <cstdint>

namespace Debug::Menu::PhysicsMenu {

    enum struct Setting : uint32_t {
        GRAVITY_ENABLED,
        SIMULATE_TEST_RAGDOLL,
        TEST_RAGDOLL_BIND_POSE,
        TEST_RAGDOLL_ANIMATION,
        ELEVATE_TEST_RAGDOLL,
        SUBSTEPS,
        POSITION_ITERATIONS,
        VELOCITY_ITERATIONS,
        BULLET_IMPACT_IMPULSE,
        SHOTGUN_PELLET_IMPACT_IMPULSE,
        MELEE_IMPACT_IMPULSE,
        RAGDOLL_IMPACT_TRANSLATION,
        RAGDOLL_IMPACT_ROTATION,
        DEFAULT_MATERIAL_STATIC_FRICTION,
        DEFAULT_MATERIAL_DYNAMIC_FRICTION,
        DEFAULT_MATERIAL_RESTITUTION,
        RESET_DEFAULTS,
        SAVE_TO_DISK,
        LOAD_FROM_DISK,
    };

    void BuildMenu();
    void ApplyEdit(uint32_t id, const Value& value);

    PageId RegisterMenu(PageId parentPage) {
        return RegisterPage("PHYSICS", parentPage, BuildMenu, ApplyEdit);
    }

    void BuildMenu() {
        const Config::Physics::Settings& settings = Config::Physics::GetSettings();

        AddBool(static_cast<uint32_t>(Setting::GRAVITY_ENABLED), "Gravity Enabled", settings.gravityEnabled);
        AddAction(static_cast<uint32_t>(Setting::SIMULATE_TEST_RAGDOLL), "Simulate Test Ragdoll");
        AddAction(static_cast<uint32_t>(Setting::TEST_RAGDOLL_BIND_POSE), "Test Ragdoll: Bind Pose");
        AddAction(static_cast<uint32_t>(Setting::TEST_RAGDOLL_ANIMATION), "Test Ragdoll: Test Animation");
        AddAction(static_cast<uint32_t>(Setting::ELEVATE_TEST_RAGDOLL), "Elevate Ragdoll");

        AddLineBreak();
        AddUInt(static_cast<uint32_t>(Setting::SUBSTEPS), "Substeps", settings.substeps, 1, 16, 1);

        AddLineBreak();
        AddUInt(static_cast<uint32_t>(Setting::POSITION_ITERATIONS), "Position Iteration Multiplier (respawn required)", settings.positionIterations, 1, 255, 1);
        AddUInt(static_cast<uint32_t>(Setting::VELOCITY_ITERATIONS), "Velocity Iteration Multiplier (respawn required)", settings.velocityIterations, 1, 255, 1);

        AddLineBreak();
        AddFloat(static_cast<uint32_t>(Setting::BULLET_IMPACT_IMPULSE), "Bullet Impact Impulse", settings.bulletImpactImpulse, 0.0f, 100.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::SHOTGUN_PELLET_IMPACT_IMPULSE), "Shotgun Pellet Impact Impulse", settings.shotgunPelletImpactImpulse, 0.0f, 100.0f, 0.25f, 2);
        AddFloat(static_cast<uint32_t>(Setting::MELEE_IMPACT_IMPULSE), "Melee Impact Impulse", settings.meleeImpactImpulse, 0.0f, 100.0f, 0.5f, 2);
        AddFloat(static_cast<uint32_t>(Setting::RAGDOLL_IMPACT_TRANSLATION), "Ragdoll Impact Translation (%)", settings.ragdollImpactTranslationScale * 100.0f, 0.0f, 1000.0f, 25.0f, 0);
        AddFloat(static_cast<uint32_t>(Setting::RAGDOLL_IMPACT_ROTATION), "Ragdoll Impact Rotation (%)", settings.ragdollImpactRotationScale * 100.0f, 0.0f, 1000.0f, 5.0f, 0);

        AddLineBreak();
        AddFloat(static_cast<uint32_t>(Setting::DEFAULT_MATERIAL_STATIC_FRICTION), "Default Material Static Friction", settings.defaultMaterialStaticFriction, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DEFAULT_MATERIAL_DYNAMIC_FRICTION), "Default Material Dynamic Friction", settings.defaultMaterialDynamicFriction, 0.0f, 10.0f, 0.05f, 2);
        AddFloat(static_cast<uint32_t>(Setting::DEFAULT_MATERIAL_RESTITUTION), "Default Material Restitution", settings.defaultMaterialRestitution, 0.0f, 1.0f, 0.05f, 2);

        AddLineBreak();
        AddAction(static_cast<uint32_t>(Setting::RESET_DEFAULTS), "Reset defaults");
        AddAction(static_cast<uint32_t>(Setting::SAVE_TO_DISK), "Save to disk");
        AddAction(static_cast<uint32_t>(Setting::LOAD_FROM_DISK), "Load from disk");
    }

    void ApplyEdit(uint32_t id, const Value& value) {
        const Setting setting = static_cast<Setting>(id);

        if (setting == Setting::SIMULATE_TEST_RAGDOLL) {
            Unloved::EditorSession::SimulateRagdollTest();
            return;
        }

        if (setting == Setting::TEST_RAGDOLL_BIND_POSE) {
            Unloved::EditorSession::SetRagdollTestToBindPose();
            return;
        }

        if (setting == Setting::TEST_RAGDOLL_ANIMATION) {
            Unloved::EditorSession::SetRagdollTestToTestAnimation();
            return;
        }

        if (setting == Setting::ELEVATE_TEST_RAGDOLL) {
            Unloved::EditorSession::ElevateRagdollTest();
            return;
        }

        if (setting == Setting::SAVE_TO_DISK) {
            const bool saved = Config::Physics::SaveToDisk();
            Debug::BlitQuickDebugMessage(saved ? "Saved " + Config::Physics::GetFilePath() : "Failed to save physics config");
            return;
        }

        if (setting == Setting::LOAD_FROM_DISK) {
            const bool loaded = Config::Physics::LoadFromDisk();
            Debug::BlitQuickDebugMessage(loaded ? "Loaded " + Config::Physics::GetFilePath() : "Failed to load physics config");
            return;
        }

        if (setting == Setting::RESET_DEFAULTS) {
            Config::Physics::ResetSettings();
            Debug::BlitQuickDebugMessage("Reset physics config defaults");
            return;
        }

        Config::Physics::Settings settings = Config::Physics::GetSettings();
        switch (setting) {
            case Setting::GRAVITY_ENABLED:        settings.gravityEnabled = value.boolValue;                                      break;
            case Setting::SUBSTEPS:              settings.substeps = value.uintValue;                                            break;
            case Setting::POSITION_ITERATIONS:   settings.positionIterations = value.uintValue;                                 break;
            case Setting::VELOCITY_ITERATIONS:   settings.velocityIterations = value.uintValue;                                 break;
            case Setting::BULLET_IMPACT_IMPULSE: settings.bulletImpactImpulse = value.floatValue;                                break;
            case Setting::SHOTGUN_PELLET_IMPACT_IMPULSE: settings.shotgunPelletImpactImpulse = value.floatValue;                  break;
            case Setting::MELEE_IMPACT_IMPULSE: settings.meleeImpactImpulse = value.floatValue;                                  break;
            case Setting::RAGDOLL_IMPACT_TRANSLATION: settings.ragdollImpactTranslationScale = value.floatValue * 0.01f;          break;
            case Setting::RAGDOLL_IMPACT_ROTATION:    settings.ragdollImpactRotationScale = value.floatValue * 0.01f;             break;
            case Setting::DEFAULT_MATERIAL_STATIC_FRICTION:  settings.defaultMaterialStaticFriction = value.floatValue;          break;
            case Setting::DEFAULT_MATERIAL_DYNAMIC_FRICTION: settings.defaultMaterialDynamicFriction = value.floatValue;         break;
            case Setting::DEFAULT_MATERIAL_RESTITUTION:      settings.defaultMaterialRestitution = value.floatValue;             break;
            default: return;
        }

        Config::Physics::SetSettings(settings);
    }
}
