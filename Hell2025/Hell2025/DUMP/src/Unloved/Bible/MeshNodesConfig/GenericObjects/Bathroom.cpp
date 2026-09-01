#include "Unloved/Bible/Bible.h"
#include "Hell/ResourceManagement/ResourceManager.h"

namespace Bible {
    using namespace Unloved;

    void ConfigureMeshNodesToilet(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
        body.meshName = "Body";
        body.materialName = "Toilet";

        MeshNodeCreateInfo& lid = meshNodeCreateInfoSet.emplace_back();
        lid.materialName = "Toilet";
        lid.meshName = "Lid";
        lid.openable.isOpenable = true;
        lid.openable.openAxis = Unloved::OpenAxis::ROTATE_X;
        lid.openable.initialOpenState = Unloved::OpenState::OPEN;
        lid.openable.minOpenValue = 0.0f;
        lid.openable.maxOpenValue = 1.7f;
        lid.openable.openSpeed = 6.825f;
        lid.openable.closeSpeed = 6.825f;
        lid.openable.openingAudio = "ToiletLidUp.wav";
        lid.openable.closingAudio = "ToiletLidDown.wav";
        lid.openable.audioVolume = 0.75f;
        lid.openable.prerequisiteClosedMeshName = "Seat";

        MeshNodeCreateInfo& seat = meshNodeCreateInfoSet.emplace_back();
        seat.materialName = "Toilet";
        seat.meshName = "Seat";
        seat.openable.isOpenable = true;
        seat.openable.openAxis = Unloved::OpenAxis::ROTATE_X;
        seat.openable.initialOpenState = Unloved::OpenState::CLOSED;
        seat.openable.minOpenValue = 0.0f;
        seat.openable.maxOpenValue = 1.7;
        seat.openable.openSpeed = 7.25f;
        seat.openable.closeSpeed = 7.25f;
        seat.openable.openingAudio = "ToiletSeatUp.wav";
        seat.openable.closingAudio = "ToiletSeatDown.wav";
        seat.openable.audioVolume = 0.75f;
        seat.openable.prerequisiteOpenMeshName = "Lid";

        meshNodes->Init(id, "Toilet", meshNodeCreateInfoSet);
    }

    void ConfigureMeshNodesBathroomBasin(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& basin = meshNodeCreateInfoSet.emplace_back();
        basin.meshName = "BathroomBasin";
        basin.materialName = "BathroomBasin";

        MeshNodeCreateInfo& tapHot = meshNodeCreateInfoSet.emplace_back();
        tapHot.meshName = "BathroomBasinTapHot";
        tapHot.materialName = "BathroomBasin";

        MeshNodeCreateInfo& tapCold = meshNodeCreateInfoSet.emplace_back();
        tapCold.meshName = "BathroomBasinTapCold";
        tapCold.materialName = "BathroomBasin";

        meshNodes->Init(id, "BathroomBasin", meshNodeCreateInfoSet);

        Model* model = Hell::ResourceManager::GetModelByName(meshNodes->GetModelName());
    }

    void ConfigureMeshNodesBathroomCabinet(uint64_t id, MeshNodes* meshNodes) {
        std::vector<MeshNodeCreateInfo> meshNodeCreateInfoSet;

        MeshNodeCreateInfo& body = meshNodeCreateInfoSet.emplace_back();
        body.meshName = "CabinetBody";
        body.materialName = "BathroomCabinet";

        MeshNodeCreateInfo& door = meshNodeCreateInfoSet.emplace_back();
        door.meshName = "CabinetDoor";
        door.materialName = "BathroomCabinet";
        door.openable.isOpenable = true;
        door.openable.openAxis = Unloved::OpenAxis::ROTATE_Y;
        door.openable.initialOpenState = Unloved::OpenState::CLOSED;
        door.openable.minOpenValue = 0.0f;
        door.openable.maxOpenValue = 1.7;
        door.openable.openSpeed = 7.25f;
        door.openable.closeSpeed = 7.25f;
        door.openable.openingAudio = "BathroomCabinetOpen.wav";
        door.openable.closingAudio = "BathroomCabinetClose.wav";
        door.openable.additionalTriggerMeshNames = { "CabinetMirror" };

        MeshNodeCreateInfo& mirror = meshNodeCreateInfoSet.emplace_back();
        mirror.meshName = "CabinetMirror";
        mirror.materialName = "BathroomCabinetMirror";
        mirror.blendingMode = BlendingMode::MIRROR;
        mirror.decalType = DecalType::GLASS;

        meshNodes->Init(id, "BathroomCabinet", meshNodeCreateInfoSet);

        Model* model = Hell::ResourceManager::GetModelByName(meshNodes->GetModelName());
    }

    
}
