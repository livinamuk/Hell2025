#pragma once

#include "Unloved/EditorSession/EditorSessionTypes.h"

#include <string>

namespace Unloved::EditorSession::Dialog {

    void Open(const std::string& message);
    void Close();
    void Render();

    bool IsOpen();
}

namespace Unloved::EditorSession::FileDialog {

    void Open(EditorSessionMode mode, const std::string& selectedFileName);
    void ImportRagdoll(const std::string& selectedFileName);
    void New(EditorSessionMode mode);
    void SaveRagdollAs(const std::string& initialName);
    void Close();
    void Render();

    bool IsOpen();
    bool IsNameInputOpen();
    std::string ConsumeSelectedFile();
    std::string ConsumeImportedRagdoll();
    std::string ConsumeNewFileName();
    std::string ConsumeNewSkinnedModelName();
    std::string ConsumeRagdollSaveAsName();
}
