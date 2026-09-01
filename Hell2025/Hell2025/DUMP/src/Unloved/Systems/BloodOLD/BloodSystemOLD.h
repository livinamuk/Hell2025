#pragma once

#include "Unloved/Systems/BloodOLD/BloodScreenSpaceDecal.h"
#include "Unloved/Systems/BloodOLD/BloodVAT.h"

#include <vector>

namespace Unloved::BloodSystemOLD {
    void AddBloodVAT(const glm::vec3& position, const glm::vec3& direction);
    void AddBloodScreenSpaceDecal(BloodScreenSpaceDecalCreateInfo createInfo);

    std::vector<BloodScreenSpaceDecal>& GetBloodScreenSpaceDecals();
    std::vector<BloodVAT>& GetBloodVAT();

    void Update(float deltaTime);
    void CleanUp();
}
