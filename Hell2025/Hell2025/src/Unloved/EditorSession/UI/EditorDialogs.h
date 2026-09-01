#pragma once

#include "Unloved/EditorSession/Editor_enums.h"
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
    void New(NewFileDialogType type);
    void SaveRagdollAs(const std::string& initialName);
    void Close();
    void Render();

    bool IsOpen();
    bool IsNameInputOpen();
    std::string ConsumeSelectedFile();
    std::string ConsumeImportedRagdoll();
}
