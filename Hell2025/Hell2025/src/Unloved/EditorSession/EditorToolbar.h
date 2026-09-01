#pragma once

namespace Unloved::EditorSession::Toolbar {

    void Init();
    void Reset();
    void Update(bool allowInput);
    void Render();

    bool WantsMouseCapture();
}
