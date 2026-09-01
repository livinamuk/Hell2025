#include "IESProfile.h"

#include "Hell/Logging.h"

IESProfile::IESProfile(const std::string& name) {
    m_name = name;
}

void IESProfile::RecalculateDerivedValues() {
    // Normalize values for GPU [0.0, 1.0]
    if (m_maxIntensity > 0.0f) {
        for (float& val : m_candelaValues) {
            val /= m_maxIntensity;
        }
    }

    float vRange = GetVerticalAngleRange();
    float hRange = GetHorizontalAngleRange();

    // Precompute Scale and Bias to turn (x - min) / range into (x * scale + bias)
    m_vScale = (vRange != 0.0f) ? 1.0f / vRange : 0.0f;
    m_vBias = -GetMinVerticalAngle() * m_vScale;

    m_hScale = (hRange != 0.0f) ? 1.0f / hRange : 0.0f;
    m_hBias = -GetMinHorizontalAngle() * m_hScale;
}

size_t IESProfile::GetCPUAllocatedByteCount() const {
    return (m_verticalAngles.capacity() * sizeof(float)) +
           (m_horizontalAngles.capacity() * sizeof(float)) +
           (m_candelaValues.capacity() * sizeof(float));
}

void IESProfile::PrintDebugInfo() {
    std::string symmetry = "";
    if (GetHorizontalAngleRange() == 180.0f) {
        symmetry = "Note:           Bilateral Symmetry detected (180 deg range).\n";
    }

    Logging::Debug() << "------------------------------------------\n"
    << "IES PROFILE LOADED: " << m_name << "\n"
    << "Data Points:    " << m_candelaValues.size() << "\n"
    << "Max Intensity:  " << m_maxIntensity << " cd\n"
    << "Grid Size:      " << m_verticalAngleCount << " (V) x " << m_horizontalAngleCount << " (H)\n"

    // Angle Ranges
    << "Vertical:       " << GetMinVerticalAngle() << " to " << GetMaxVerticalAngle() << " (Range: " << GetVerticalAngleRange() << ")\n"
    << "Horizontal:     " << GetMinHorizontalAngle() << " to " << GetMaxHorizontalAngle() << " (Range: " << GetHorizontalAngleRange() << ")\n"

    // Precomputed Shader Constants
    << "V-Scale/Bias:   " << m_vScale << " / " << m_vBias << "\n"
    << "H-Scale/Bias:   " << m_hScale << " / " << m_hBias << "\n"

    // Symmetry Detection
    << symmetry

    << "------------------------------------------\n\n";
}

float IESProfile::GetMinVerticalAngle() const {
    return m_verticalAngles.empty() ? 0.0f : m_verticalAngles.front();
}

float IESProfile::GetMaxVerticalAngle() const {
    return m_verticalAngles.empty() ? 0.0f : m_verticalAngles.back();
}

float IESProfile::GetMinHorizontalAngle() const {
    return m_horizontalAngles.empty() ? 0.0f : m_horizontalAngles.front();
}

float IESProfile::GetMaxHorizontalAngle() const {
    return m_horizontalAngles.empty() ? 0.0f : m_horizontalAngles.back();
}

float IESProfile::GetVerticalAngleRange() const {
    return GetMaxVerticalAngle() - GetMinVerticalAngle();
}

float IESProfile::GetHorizontalAngleRange() const {
    return GetMaxHorizontalAngle() - GetMinHorizontalAngle();
}
