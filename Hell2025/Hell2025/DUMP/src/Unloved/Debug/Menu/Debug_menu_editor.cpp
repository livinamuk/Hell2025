    #include "Debug_menu.h"

#include "Hell/Backend/BackEnd.h"
#include "Hell/Common/Enum.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/EditorSession/EditorSession.h"

#include <cstdint>
#include <limits>

namespace Debug::Menu::Editor {

    enum struct Setting : uint32_t {
        HOUSE,
        MAP,
        RAGDOLL,
        BONE_MASK,
    };

    PageId g_homepageId = ROOT_PAGE_ID;

    void BuildMainMenu();
    void ApplyEdit(uint32_t id, const Value& value);

    void RegisterMenu() {
        g_homepageId = RegisterRootPage("Editor", "EDITOR", BuildMainMenu, ApplyEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMainMenu() {
        AddAction(static_cast<uint32_t>(Setting::HOUSE), "House");
        AddAction(static_cast<uint32_t>(Setting::MAP), "Map");
        AddAction(static_cast<uint32_t>(Setting::RAGDOLL), "Ragdoll");
        AddAction(static_cast<uint32_t>(Setting::BONE_MASK), "Bone Mask");
    }

    void ApplyEdit(uint32_t id, const Value& value) {
        switch (static_cast<Setting>(id)) {
            case Setting::HOUSE:     Unloved::EditorSession::Open(Unloved::EditorSession::EditorSessionMode::HOUSE); Debug::HideMenu(); return;
            case Setting::MAP:       Unloved::EditorSession::Open(Unloved::EditorSession::EditorSessionMode::MAP); Debug::HideMenu(); return;
            case Setting::RAGDOLL:   Unloved::EditorSession::Open(Unloved::EditorSession::EditorSessionMode::RAGDOLL); Debug::HideMenu(); return;
            case Setting::BONE_MASK: Unloved::EditorSession::Open(Unloved::EditorSession::EditorSessionMode::BONE_MASK); Debug::HideMenu(); return;
            default: return;
        }
    }
}
