#include "Animator.h"

#include "Hell/Containers/SlotMap.h"
#include "Hell/Logging.h"

#include "Unloved/ObjectId/ObjectId.h"

namespace Unloved::Animator {

    Hell::SlotMap<AnimatorInstance> g_animatorInstances;

    uint64_t CreateAnimatorInstance() {
        const uint64_t animatorInstanceId = GetNextObjectId(ObjectType::ANIMATOR);

        if (!g_animatorInstances.emplace_with_id(animatorInstanceId)) {
            Logging::Error() << "Animator::CreateAnimatorInstance() failed to insert animator instance " << animatorInstanceId << "\n";
            return 0;
        }

        return animatorInstanceId;
    }

    void RemoveAnimatorInstance(uint64_t animatorInstanceId) {
        g_animatorInstances.erase(animatorInstanceId);
    }

    AnimatorInstance* GetAnimatorInstanceByObjectId(uint64_t animatorInstanceId) {
        return g_animatorInstances.get(animatorInstanceId);
    }

    void Update(float deltaTime) {
        for (AnimatorInstance& animatorInstance : g_animatorInstances) {
            animatorInstance.Update(deltaTime);
        }
    }

    void CleanUp() {
        g_animatorInstances.clear();
    }
}
