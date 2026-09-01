#include "BloodScreenSpaceDecal.h"
#include "Hell/Common/Constants.h"
#include "Hell/Common/Random.h"

void BloodScreenSpaceDecal::Init(BloodScreenSpaceDecalCreateInfo createInfo) {
    Transform transform;
    transform.position = createInfo.position;
    transform.rotation.y = Hell::Random::Float(0.0f, HELL_PI * 2);
    transform.scale = glm::vec3(2.0f);

    m_modelMatrix = transform.to_mat4();
    m_inverseModelMatrix = glm::inverse(m_modelMatrix);

    m_type = Hell::Random::Int(0, 3);
}

