#pragma once
#include "Unloved/Systems/BloodOLD/Blood_types.h"

#include "Unloved/Common/Types.h"

struct BloodScreenSpaceDecal {
    void Init(BloodScreenSpaceDecalCreateInfo createInfo);

    int GetType()                       { return m_type; }
    glm::mat4 GetModelMatrix()          { return m_modelMatrix; }
    glm::mat4 GetInverseModelMatrix()   { return m_inverseModelMatrix; }

    int m_type = 0;

private:
    glm::mat4 m_modelMatrix = glm::mat4(1.0f);
    glm::mat4 m_inverseModelMatrix = glm::mat4(1.0f);
};
