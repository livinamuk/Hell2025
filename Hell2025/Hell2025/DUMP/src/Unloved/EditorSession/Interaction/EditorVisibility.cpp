#include "EditorVisibility.h"

#include "Unloved/EditorSession/EditorSession.h"

#include "Unloved/ObjectId.h"
#include "Unloved/Objects/House/Wall.h"
#include "Unloved/Objects/Renderables/SkinnedGameObject.h"
#include "Unloved/Systems/WorldBVH/WorldBVH.h"
#include "Unloved/World/World.h"

#include <unordered_set>

namespace Unloved::EditorSession::Visibility {
    namespace {
        std::unordered_set<uint64_t> g_hiddenObjectIds;
    }

    bool Hide(uint64_t objectId) {
        if (objectId == 0 || !g_hiddenObjectIds.insert(objectId).second) return false;
        WorldBVH::MarkStaticSceneBvhDirty();
        return true;
    }

    bool UnhideAll() {
        if (g_hiddenObjectIds.empty()) return false;
        g_hiddenObjectIds.clear();
        WorldBVH::MarkStaticSceneBvhDirty();
        return true;
    }

    void Clear() {
        g_hiddenObjectIds.clear();
        WorldBVH::MarkStaticSceneBvhDirty();
    }

    bool ShouldHide(uint64_t objectId) {
        if (!EditorSession::IsActive() || objectId == 0) return false;
        if (g_hiddenObjectIds.find(objectId) != g_hiddenObjectIds.end()) return true;

        if (GetObjectIdType(objectId) == ObjectType::WALL_SEGMENT) {
            Wall* wall = World::GetWallByWallSegmentObjectId(objectId);
            return wall && g_hiddenObjectIds.find(wall->GetObjectId()) != g_hiddenObjectIds.end();
        }

        if (GetObjectIdType(objectId) == ObjectType::SKINNED_GAME_OBJECT) {
            SkinnedGameObject* skinnedGameObject = World::GetSkinnedGameObjectByObjectId(objectId);
            return skinnedGameObject && g_hiddenObjectIds.find(skinnedGameObject->GetOwnerObjectId()) != g_hiddenObjectIds.end();
        }

        return false;
    }
}
