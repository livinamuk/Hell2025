#pragma once
#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
    #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#endif

#ifndef GLM_FORCE_SILENT_WARNINGS
    #define GLM_FORCE_SILENT_WARNINGS
#endif

#ifndef GLM_ENABLE_EXPERIMENTAL
    #define GLM_ENABLE_EXPERIMENTAL
#endif
    
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ostream>

inline std::ostream& operator<<(std::ostream& os, const glm::vec3& vec) {
    os << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const glm::ivec2& vec) {
    os << "(" << vec.x << ", " << vec.y << ")";
    return os;
}
