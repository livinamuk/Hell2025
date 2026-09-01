#pragma once

#include "EditorSessionTypes.h"

namespace Unloved::EditorSession::Inspector {

    void RenderProperties(const EditorRect& rect);

    bool HasTools();
    void RenderTools(const EditorRect& rect);

    bool HasBrushes();
    void RenderBrushes(const EditorRect& rect);

    bool HasMaterials();
    void RenderMaterials(const EditorRect& rect);
}
