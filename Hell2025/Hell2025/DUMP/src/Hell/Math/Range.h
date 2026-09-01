#pragma once

#include <type_traits>

namespace Hell::Math {

    template<typename In, typename MinIn, typename MaxIn, typename MinOut, typename MaxOut>
    inline float MapRange(In inValue, MinIn minInRange, MaxIn maxInRange, MinOut minOutRange, MaxOut maxOutRange) {
        static_assert(std::is_arithmetic<In>::value && std::is_arithmetic<MinIn>::value && std::is_arithmetic<MaxIn>::value && std::is_arithmetic<MinOut>::value && std::is_arithmetic<MaxOut>::value, "MapRange requires arithmetic types");
        float iv = static_cast<float>(inValue);
        float minI = static_cast<float>(minInRange);
        float maxI = static_cast<float>(maxInRange);
        float minO = static_cast<float>(minOutRange);
        float maxO = static_cast<float>(maxOutRange);
        float x = (iv - minI) / (maxI - minI);
        return minO + (maxO - minO) * x;
    }
}
