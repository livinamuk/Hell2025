#include "ObjectNames.h"

#include "Hell/Common/Constants.h"

#include "Unloved/Characters/Enemies/Dobermann/Dobermann.h"
#include "Unloved/Characters/Enemies/Kangaroo/Kangaroo.h"
#include "Unloved/Characters/Enemies/Shark/Shark.h"
#include "Unloved/Characters/Mermaids/Mermaid/Mermaid.h"
#include "Unloved/Objects/Exterior/Fence.h"
#include "Unloved/Objects/Exterior/Jetty.h"
#include "Unloved/Objects/Exterior/PowerPoleSet.h"
#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/House/Fireplace.h"
#include "Unloved/Objects/House/WorldPlane.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/House/Window.h"
#include "Unloved/Objects/Interior/Piano.h"
#include "Unloved/Objects/Interior/PictureFrame.h"
#include "Unloved/Objects/Lighting/Light.h"
#include "Unloved/Objects/Props/Christmas/ChristmasLights.h"
#include "Unloved/Objects/Props/Christmas/ChristmasTree.h"
#include "Unloved/Objects/Props/GenericObject.h"
#include "Unloved/Objects/Props/PickUp.h"
#include "Unloved/Objects/Traversal/Ladder.h"
#include "Unloved/Objects/Traversal/Staircase.h"
#include "Unloved/Systems/DDGI/DDGIVolume.h"
#include "Unloved/World/World.h"

namespace Unloved::EditorSession {

    std::vector<EditorObjectNameGroup> g_editorObjectNameGroups;

    // Create group by container
    template<typename Container>
    void AddEditorObjectNameGroup(const std::string& label, Container& objects) {
        if (objects.empty()) return;

        EditorObjectNameGroup& group = g_editorObjectNameGroups.emplace_back();
        group.label = label;
        group.entries.reserve(objects.size());

        for (auto& object : objects) {
            EditorObjectNameEntry& entry = group.entries.emplace_back();
            entry.name = object.GetCreateInfo().editorName;
            entry.objectId = object.GetObjectId();
        }
    }

    // Create type filtered group by container
    template<typename Container, typename Type>
    void AddEditorObjectNameGroup(const std::string& label, Container& objects, Type type) {
        EditorObjectNameGroup group;
        group.label = label;
        group.entries.reserve(objects.size());

        for (auto& object : objects) {
            if (object.GetType() != type) {
                continue;
            }

            EditorObjectNameEntry& entry = group.entries.emplace_back();
            entry.name = object.GetCreateInfo().editorName;
            entry.objectId = object.GetObjectId();
        }

        if (!group.entries.empty()) {
            g_editorObjectNameGroups.push_back(std::move(group));
        }
    }

    std::vector<EditorObjectNameGroup>& GetEditorObjectNameGroups() {
        g_editorObjectNameGroups.clear();

        AddEditorObjectNameGroup("Ceilings", World::GetWorldPlanes(), WorldPlaneType::CEILING);
        AddEditorObjectNameGroup("Christmas Trees", World::GetChristmasTrees());
        AddEditorObjectNameGroup("Christmas Lights", World::GetChristmasLightSets());
        AddEditorObjectNameGroup("DDGI Volumes", World::GetDDGIVolumes());
        AddEditorObjectNameGroup("Dobermann", World::GetDobermanns());
        AddEditorObjectNameGroup("Doors", World::GetDoors());
        AddEditorObjectNameGroup("Fences", World::GetFences());
        AddEditorObjectNameGroup("Fireplaces", World::GetFireplaces());
        AddEditorObjectNameGroup("Floors", World::GetWorldPlanes(), WorldPlaneType::FLOOR);
        AddEditorObjectNameGroup("Generic Objects", World::GetGenericObjects());
        AddEditorObjectNameGroup("Jetties", World::GetJetties());
        AddEditorObjectNameGroup("Kangaroos", World::GetKangaroos());
        AddEditorObjectNameGroup("Ladders", World::GetLadders());
        AddEditorObjectNameGroup("Lights", World::GetLights());
        AddEditorObjectNameGroup("Mermaids", World::GetMermaids());
        AddEditorObjectNameGroup("Pick Ups", World::GetPickUps());
        AddEditorObjectNameGroup("Picture Frames", World::GetPictureFrames());
        AddEditorObjectNameGroup("Pianos", World::GetPianos());
        AddEditorObjectNameGroup("Power Pole Sets", World::GetPowerPoleSets());
        AddEditorObjectNameGroup("Staircases", World::GetStaircases());
        AddEditorObjectNameGroup("Sharks", World::GetSharks());
        AddEditorObjectNameGroup("Walls", World::GetWalls());
        AddEditorObjectNameGroup("Windows", World::GetWindows());

        return g_editorObjectNameGroups;
    }
}
