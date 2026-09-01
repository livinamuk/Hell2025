#pragma once

#include "Unloved/Objects/Renderables/MeshBufferOLD.h"

#include <glm/gtc/quaternion.hpp>

#include <vector>

enum class GizmoMode {
    TRANSLATE,
    ROTATE,
    SCALE
};

enum class GizmoFlag {
    NONE,
    TRANSLATE_X,
    TRANSLATE_Y,
    TRANSLATE_Z,
    ROTATE_X,
    ROTATE_Y,
    ROTATE_Z,
    SCALE_X,
    SCALE_Y,
    SCALE_Z,
    SCALE,
};

enum class GizmoAction {
    IDLE,
    DRAGGING
};

struct GizmoRenderItem {
    glm::mat4 modelMatrix;
    glm::vec4 color;
    GizmoFlag flag;
    int meshIndex;
};

namespace Gizmo {
    using namespace Unloved;

    void Init();
    void Update(bool allowInput = true, bool allowModeSwitching = true);
    void CancelInteraction();
    void SetPosition(const glm::vec3& position);
    void SetRotation(const glm::vec3& rotation);
    void SetRotation(const glm::quat& rotation);
    void SetMode(GizmoMode mode);
    void SetLocalAxes(bool enabled);
    void SetWorldRotationAxes(bool enabled);
    void SetSourceObjectOffeset(const glm::vec3& offset);
    void SetVisible(bool visible);
    void UpdateRenderItems();

    std::vector<GizmoRenderItem>& GetRenderItemsByViewportIndex(int index);
    MeshBufferOLD* GetMeshBufferByIndex(int index);
    const std::string GizmoFlagToString(const GizmoFlag& flag);
    const glm::vec3 GetPosition();
    const glm::vec3 GetRotation();
    const glm::quat GetRotationQuaternion();
    const bool HasHover();
    bool UsesLocalAxes();
    bool UsesWorldRotationAxes();
    float GetGizmoScalingFactorByViewportIndex(int viewportIndex);
    const GizmoAction GetAction();
    const GizmoMode GetMode();
}
