#include "Debug_menu.h"

#include "Unloved/EditorSession/Ragdoll/EditorRagdollTest.h"

#include <cstdint>

namespace Debug::Menu::RagdollMenu {
    enum struct Setting : uint32_t {
        SIMULATE,
        BIND_POSE,
        TEST_ANIMATION,
        ELEVATE,
    };

    void BuildMenu() {
        AddAction(static_cast<uint32_t>(Setting::SIMULATE), "Simulate");
        AddAction(static_cast<uint32_t>(Setting::BIND_POSE), "Bind Pose");
        AddAction(static_cast<uint32_t>(Setting::TEST_ANIMATION), "Test Animation");
        AddAction(static_cast<uint32_t>(Setting::ELEVATE), "Elevate");
    }

    void ApplyEdit(uint32_t id, const Value&) {
        switch (static_cast<Setting>(id)) {
            case Setting::SIMULATE:       Unloved::EditorSession::RagdollTest::Simulate();           return;
            case Setting::BIND_POSE:      Unloved::EditorSession::RagdollTest::SetToBindPose();      return;
            case Setting::TEST_ANIMATION: Unloved::EditorSession::RagdollTest::SetToTestAnimation(); return;
            case Setting::ELEVATE:        Unloved::EditorSession::RagdollTest::Elevate();            return;
        }
    }

    void RegisterMenu() {
        RegisterRootPage("Ragdoll", "RAGDOLL", BuildMenu, ApplyEdit, Unloved::EditorSession::RagdollTest::IsActive);
    }

    Registrar g_registrar(RegisterMenu);
}
