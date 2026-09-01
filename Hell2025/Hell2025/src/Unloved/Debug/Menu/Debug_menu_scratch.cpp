#include "Debug_menu.h"

#include "Hell/Logging.h"
#include "Hell/Serialization/Json.h"
#include "Hell/Physics/Physics.h"

#include "Unloved/Debug/Debug.h"
#include "Unloved/Debug/Scratch.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/CoarseWorldBVH/CoarseWorldBVH.h"
#include "Unloved/Systems/Blood/BloodSystem.h"
#include "Unloved/World/World.h"

#include <cstdint>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace Debug::Menu::Scratch {

    enum struct BindingType {
        BOOL,
        INT,
        UINT,
        FLOAT,
        STRING_LIST,
    };

    struct Binding {
        BindingType type;
        std::string name;
    };

    enum struct Action : uint32_t {
        SAVE_TELEPORT = std::numeric_limits<uint32_t>::max() - 1,
        LOAD_TELEPORT = std::numeric_limits<uint32_t>::max(),
    };

    const std::string SCRATCH_CONFIG_FILE_PATH = "res/config/scratch.json";
    constexpr uint32_t SAVED_TELEPORT_PLAYER_COUNT = 4;
    std::vector<Binding> g_bindings;

    void BuildMenu();
    void ApplyEdit(uint32_t id, const Value& value);
    nlohmann::json CreateTeleportJson(Unloved::Player* player);
    void ApplyTeleportJson(const nlohmann::json& teleport, Unloved::Player& player);
    bool SaveTeleport();
    bool LoadTeleport();
    void AddScratchBool(const std::string& name, bool defaultValue);
    void AddScratchStringList(const std::string& name, std::vector<std::string> values, int32_t defaultIndex = 0);
    bool ApplyScratchEdit(uint32_t id, const Value& value);

    void RegisterMenu() {
        RegisterRootPage("Scratch", "SCRATCH", BuildMenu, ApplyEdit);
    }

    Registrar g_registrar(RegisterMenu);

    void BuildMenu() {
        g_bindings.clear();

        AddScratchStringList("Glass Mode", { "0", "1", "2", "3" }, 3);
        AddScratchBool("Glass Shadows", true);
        AddScratchBool("New Grass", true);
        AddScratchBool("Grass HiZ Culling", true);
        AddScratchBool("Mermaid Top", true);

        AddLineBreak();
        AddAction(static_cast<uint32_t>(Action::SAVE_TELEPORT), "Save Teleport");
        AddAction(static_cast<uint32_t>(Action::LOAD_TELEPORT), "Load Teleport");

        AddOpenGLFunctionTiming("GrassPass");
        AddOpenGLFunctionTiming("OcclusionHiZPass");
        AddOpenGLFunctionTiming("NewGrassCacheGeneration");
        AddOpenGLFunctionTiming("NewGrassChunkCulling");
        AddOpenGLFunctionTiming("NewGrassCulling");
        AddOpenGLFunctionTiming("NewGrassDraw");
        AddOpenGLFunctionTiming("GlassPass");
        AddOpenGLFunctionTiming("EmissivePass");

        Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(1);
        if (!player) {
            AddText("Player 2 is not in the session");
            return;
        }

        Unloved::Light& light = Unloved::World::GetLights()[5];

        std::string text;
        text += "Player pos: " + Hell::String::FormatVec3(player->GetCameraPosition()) + "\n";
        text += "Light pos: " + Hell::String::FormatVec3(light.GetPosition()) + "\n";
        text += "Light col: " + Hell::String::FormatVec3(light.GetColor()) + "\n";

        bool lineOfSight = Unloved::CoarseWorldBVH::AnyHitWithDoors(player->GetCameraPosition(), light.GetPosition());
        text += "Hit blocked: " + Hell::String::FormatBool(lineOfSight) + "\n";

        uint64_t physicsId = player->GetRagdoll()->GetPhysicsIdByBoneName("CC_Base_Head");
        PhysicsContactResult result = Hell::Physics::GetContactResult(physicsId, CollisionGroup::ENVIROMENT_OBSTACLE);

        text += "\n";
        text += "Hit found: " + Hell::String::FormatBool(result.hitFound) + "\n";
        text += "Hit position: " + Hell::String::FormatVec3(result.hitPosition) + "\n";
        text += "Hit normal: " + Hell::String::FormatVec3(result.hitNormal) + "\n";

        if (result.hitFound && result.hitNormal.y > 0.1f) {

            glm::vec3 headCenter = player->GetRagdoll()->GetRigidWorldSpaceCenter("CC_Base_Head");

            glm::vec3 spawnPos = result.hitPosition;
            spawnPos.x = headCenter.x;
            spawnPos.z = headCenter.z;

            Logging::Debug() << "Head hit ground\n";

            Unloved::BloodSystem::SpawnBloodPoolDecal(spawnPos, result.hitNormal);
        }

        AddText(text);
    }

    void ApplyEdit(uint32_t id, const Value& value) {
        if (ApplyScratchEdit(id, value)) return;

        switch (static_cast<Action>(id)) {
            case Action::SAVE_TELEPORT: {
                const bool saved = SaveTeleport();
                Debug::BlitQuickDebugMessage(saved ? "Saved teleport to " + SCRATCH_CONFIG_FILE_PATH : "Failed to save scratch teleport");
                break;
            }
            case Action::LOAD_TELEPORT: {
                const bool loaded = LoadTeleport();
                Debug::BlitQuickDebugMessage(loaded ? "Loaded teleport from " + SCRATCH_CONFIG_FILE_PATH : "Failed to load scratch teleport");
                break;
            }
            default: break;
        }
    }

    nlohmann::json CreateTeleportJson(Unloved::Player* player) {
        glm::vec3 position(0.0f);
        glm::vec3 cameraEuler(0.0f);
        float cameraHeightModifier = 0.0f;

        if (player) {
            position = player->GetFootPosition();
            cameraEuler = player->GetCamera().GetEulerRotation();
            cameraHeightModifier = player->GetCameraHeightModifier();
        }

        return {
            { "position", { position.x, position.y, position.z } },
            { "cameraEuler", { cameraEuler.x, cameraEuler.y, cameraEuler.z } },
            { "cameraHeightModifier", cameraHeightModifier }
        };
    }

    void ApplyTeleportJson(const nlohmann::json& teleport, Unloved::Player& player) {
        const glm::vec3 position = teleport.at("position").get<glm::vec3>();
        const glm::vec3 cameraEuler = teleport.at("cameraEuler").get<glm::vec3>();
        const float cameraHeightModifier = teleport.value("cameraHeightModifier", player.GetCameraHeightModifier());
        player.SetFootPosition(position);
        player.GetCamera().SetEulerRotation(cameraEuler);
        player.SetCameraHeightModifier(cameraHeightModifier);
    }

    bool SaveTeleport() {
        nlohmann::json json = nlohmann::json::object();
        if (std::filesystem::exists(SCRATCH_CONFIG_FILE_PATH) && !Hell::Json::LoadFromFile(json, SCRATCH_CONFIG_FILE_PATH)) {
            return false;
        }
        if (!json.is_object()) json = nlohmann::json::object();

        nlohmann::json playerTeleports = nlohmann::json::array();
        for (uint32_t playerIndex = 0; playerIndex < SAVED_TELEPORT_PLAYER_COUNT; playerIndex++) {
            playerTeleports.push_back(CreateTeleportJson(Unloved::Session::GetLocalPlayerByViewportIndex(playerIndex)));
        }

        // Retain the original player-one fields so existing scratch files and
        // older builds can still read the primary teleport.
        json["teleport"] = playerTeleports[0];
        json["teleport"]["players"] = std::move(playerTeleports);

        std::error_code errorCode;
        std::filesystem::create_directories(std::filesystem::path(SCRATCH_CONFIG_FILE_PATH).parent_path(), errorCode);
        if (errorCode) {
            Logging::Error() << "SaveTeleport() failed to create config directory: " << errorCode.message() << "\n";
            return false;
        }

        return Hell::Json::SaveToFile(json, SCRATCH_CONFIG_FILE_PATH);
    }

    bool LoadTeleport() {
        nlohmann::json json;
        if (!Hell::Json::LoadFromFile(json, SCRATCH_CONFIG_FILE_PATH)) return false;

        try {
            const nlohmann::json& teleport = json.at("teleport");
            const auto playerTeleports = teleport.find("players");

            // Legacy files contain only the top-level player-one teleport.
            if (playerTeleports == teleport.end()) {
                if (Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(0)) {
                    ApplyTeleportJson(teleport, *player);
                }
                return true;
            }

            if (!playerTeleports->is_array()) return false;
            for (uint32_t playerIndex = 0; playerIndex < SAVED_TELEPORT_PLAYER_COUNT; playerIndex++) {
                Unloved::Player* player = Unloved::Session::GetLocalPlayerByViewportIndex(playerIndex);
                if (!player || playerIndex >= playerTeleports->size()) continue;
                ApplyTeleportJson(playerTeleports->at(playerIndex), *player);
            }
        }
        catch (const nlohmann::json::exception& e) {
            Logging::Error() << "LoadTeleport() failed to read '" << SCRATCH_CONFIG_FILE_PATH << "': " << e.what() << "\n";
            return false;
        }

        return true;
    }

    uint32_t AddBinding(BindingType type, const std::string& name) {
        const uint32_t id = static_cast<uint32_t>(g_bindings.size());
        g_bindings.push_back({ type, name });
        return id;
    }

    void AddScratchBool(const std::string& name, bool defaultValue) {
        Debug::Scratch::InitBool(name, defaultValue);
        AddBool(AddBinding(BindingType::BOOL, name), name, Debug::Scratch::GetBool(name));
    }

    void AddScratchInt(const std::string& name, int32_t defaultValue, int32_t minimum, int32_t maximum, int32_t increment) {
        Debug::Scratch::InitInt(name, defaultValue);
        AddInt(AddBinding(BindingType::INT, name), name, Debug::Scratch::GetInt(name), minimum, maximum, increment);
    }

    void AddScratchUInt(const std::string& name, uint32_t defaultValue, uint32_t minimum, uint32_t maximum, uint32_t increment) {
        Debug::Scratch::InitUInt(name, defaultValue);
        AddUInt(AddBinding(BindingType::UINT, name), name, Debug::Scratch::GetUInt(name), minimum, maximum, increment);
    }

    void AddScratchFloat(const std::string& name, float defaultValue, float minimum, float maximum, float increment, int32_t precision = 3, bool scientific = false) {
        Debug::Scratch::InitFloat(name, defaultValue);
        AddFloat(AddBinding(BindingType::FLOAT, name), name, Debug::Scratch::GetFloat(name), minimum, maximum, increment, precision, scientific);
    }

    void AddScratchStringList(const std::string& name, std::vector<std::string> values, int32_t defaultIndex) {
        Debug::Scratch::InitStringList(name, std::move(values), defaultIndex);
        AddEnum(AddBinding(BindingType::STRING_LIST, name), name, Debug::Scratch::GetStringListIndex(name), Debug::Scratch::GetStringList(name));
    }

    bool ApplyScratchEdit(uint32_t id, const Value& value) {
        if (id >= g_bindings.size()) return false;

        const Binding& binding = g_bindings[id];
        switch (binding.type) {
        case BindingType::BOOL:        Debug::Scratch::SetBool(binding.name, value.boolValue); break;
        case BindingType::INT:         Debug::Scratch::SetInt(binding.name, value.intValue); break;
        case BindingType::UINT:        Debug::Scratch::SetUInt(binding.name, value.uintValue); break;
        case BindingType::FLOAT:       Debug::Scratch::SetFloat(binding.name, value.floatValue); break;
        case BindingType::STRING_LIST: Debug::Scratch::SetStringListIndex(binding.name, value.intValue); break;
        }
        return true;
    }
}
