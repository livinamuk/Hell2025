#pragma once

#include <algorithm>
#include <cmath>

namespace Hell::Easing {

    inline float Clamp01(float t) {
        return std::clamp(t, 0.0f, 1.0f);
    }

    inline float EaseIn(float t, float exponent) {
        return std::pow(t, exponent);
    }

    inline float EaseOut(float t, float exponent) {
        return 1.0f - std::pow(1.0f - t, exponent);
    }

    inline float EaseInOut(float t, float exponent) {
        if (t < 0.5f) {
            return 0.5f * std::pow(t * 2.0f, exponent);
        }

        return 1.0f - 0.5f * std::pow((1.0f - t) * 2.0f, exponent);
    }

    inline float SmoothStep(float t) {
        t = Clamp01(t);
        return t * t * (3.0f - 2.0f * t);
    }

    inline float ReverseSmoothStep(float t) {
        return 1.0f - SmoothStep(t);
    }

    inline float CubicIn(float t) {
        t = Clamp01(t);
        return t * t * t;
    }

    inline float QuadraticOut(float t) {
        t = Clamp01(t);
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    inline float HermiteEaseInOut(float t) {
        return t * t * (3.0f - 2.0f * t);
    }
}
