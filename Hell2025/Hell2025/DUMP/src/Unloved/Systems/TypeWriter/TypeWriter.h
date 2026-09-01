#pragma once
#include "Unloved/Common/Types.h"

struct TypeWriter {
    void Update(float deltaTime);
    void SetLocation(const glm::ivec2& location);
    void DisplayText(const std::string& text, float duration = 3.0f);
    void ClearText();
    
    const glm::ivec2& GetLocation() const   { return m_location; }
    const std::string& GetText() const      { return m_text; }

private:
    glm::ivec2 m_location = glm::ivec2(0, 0);
    std::string m_text = "";
    std::string m_textToBlit = "";
    float m_duration = 1.0f;
    float m_speed = 75.0f;
    float m_currentTime = 0.0f;

};