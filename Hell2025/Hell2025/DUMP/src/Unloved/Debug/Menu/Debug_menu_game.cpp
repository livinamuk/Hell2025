#include "Debug_menu.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Systems/BloodOLD/BloodSystemOLD.h"
#include "Unloved/World/World.h"

#include <cstdint>

namespace Debug::Menu::Game {

    enum struct Action : uint32_t {
        RESET_ALL_DOBERMANN,
        CLEAR_ALL_DECALS,
    };

    void BuildMenu() {
        AddAction(static_cast<uint32_t>(Action::RESET_ALL_DOBERMANN), "Reset All Dobermann");
        AddAction(static_cast<uint32_t>(Action::CLEAR_ALL_DECALS), "Clear All Decals");
    }

    void ApplyEdit(uint32_t id, const Value&) {
        const Action action = static_cast<Action>(id);

        if (action == Action::RESET_ALL_DOBERMANN) {
            for (Unloved::Dobermann& dobermann : Unloved::World::GetDobermanns()) dobermann.ResetToInitialState();
            return;
        }

        if (action == Action::CLEAR_ALL_DECALS) {
            Unloved::World::CleanUpCasings();
            Unloved::World::CleanUpDecals();
            Unloved::BloodSystemOLD::CleanUp();
        }
    }

    void RegisterMenu() {
        RegisterRootPage("Game", "GAME", BuildMenu, ApplyEdit);
    }

    Registrar g_registrar(RegisterMenu);
}
