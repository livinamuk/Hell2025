#pragma once

#include "Unloved/Systems/Animator/AnimatorInstance.h"

#include <cstdint>

namespace Unloved::Animator {

    uint64_t CreateAnimatorInstance();
    void RemoveAnimatorInstance(uint64_t animatorInstanceId);
    AnimatorInstance* GetAnimatorInstanceByObjectId(uint64_t animatorInstanceId);

    void Update(float deltaTime);
    void CleanUp();
}
