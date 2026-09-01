#include "TypeWriter.h"
#include "Hell/UI/TextBlitter.h"
#include "Hell/UI/UIBackEnd.h"

void TypeWriter::Update(float deltaTime) { 

    if (m_currentTime > m_duration) {
        m_textToBlit = "";
        return;
    }

    Alignment m_alignment = Alignment::TOP_LEFT;
    std::string m_fontName = "StandardFont";
    float m_scale = 2.0f;

    // REPLACE WITH HEADER THING WHEN DONE
    float speed = m_speed;
    speed = 75.0f;

    m_currentTime += deltaTime;

    size_t charCount = (int)(m_currentTime * speed);
    charCount = std::min(charCount, m_text.length());

    m_textToBlit = m_text.substr(0, charCount);

    UIBackEnd::BlitText(m_textToBlit, m_fontName, m_location, m_alignment, m_scale, TextureFilter::NEAREST);
}

void TypeWriter::SetLocation(const glm::ivec2& location) {
    m_location = location;
}

void TypeWriter::DisplayText(const std::string& text, float duration) {
    m_text = text;
    m_duration = duration;
    m_currentTime = 0.0f;
}

void TypeWriter::ClearText() {
    m_text = "";
    m_duration = 0;
    m_currentTime = 0;
}
