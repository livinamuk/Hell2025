#include "BloodVAT.h"

BloodVAT::BloodVAT(glm::vec3 position, glm::vec3 forward) {
    // Cycle through the 4 types
    static unsigned int typeCounter = 0;
    switch (typeCounter) {
        case 0: m_type = 7; break;
        case 1: m_type = 9; break;
        case 2: m_type = 6; break;
        case 3: m_type = 4; break;
    }
    typeCounter++;
    if (typeCounter == 4) {
        typeCounter = 0;
    }

    // Each blood mesh annoyingly requires a different local offset
    glm::mat4 meshOffset = glm::mat4(1.0f);
    switch (m_type) {
        case 0: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.08f, -0.23f, -0.155f)); break;
        case 7: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.2300000042f, -0.5000000000f, -0.2249999940f)); break;
        case 6: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.0839999989f, -0.3799999952f, -0.1500000060f)); break;
        case 8: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1700000018f, -0.2290000021f, -0.1770000011f)); break;
        case 9: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.0500000007f, -0.2549999952f, -0.1299999952f)); break;
        case 4: meshOffset = glm::translate(glm::mat4(1.0f), glm::vec3(-0.0500000045f, -0.4149999917f, -0.1900000125f)); break;
    }

    forward = glm::normalize(forward);

    glm::vec3 worldUp = glm::abs(glm::dot(forward, glm::vec3(0, 1, 0))) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
    glm::vec3 right = glm::normalize(glm::cross(worldUp, forward));
    glm::vec3 up = glm::cross(forward, right);

    glm::mat4 rotationBasis = glm::mat4(glm::mat3(right, up, forward));
    glm::mat4 rotationY90 = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0, 1, 0));
    glm::vec3 scale = glm::vec3(3.0f);

    m_modelMatrix = glm::mat4(1.0f);
    m_modelMatrix = glm::translate(m_modelMatrix, position);
    m_modelMatrix *= rotationBasis;
    m_modelMatrix *= rotationY90;
    m_modelMatrix = glm::scale(m_modelMatrix, scale);

    m_modelMatrix *= meshOffset;
}

void BloodVAT::Update(float deltaTime) {
    m_lifetime += deltaTime * 1.5f;
}