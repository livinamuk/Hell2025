#pragma once

#include "Hell/Math/Transform.h"
#include "Hell/ResourceManagement/Types/Model.h"
#include "Hell/ResourceManagement/Types/Material.h"

#include "Unloved/Common/Types.h"
#include "Unloved/Render/RendererTypes.h"

namespace Unloved {

struct Trim {
    void Init(Transform transform, const std::string& modelName, const std::string& materialName);
    void SubmitRenderItem();

private:
    Hell::Transform m_transform;
    int32_t m_materialIndex = -1;
    Model* m_model;
    RenderItem m_renderItem;
    uint64_t m_objectId = 0;

    void UpdateRenderItem();
};
}
