#pragma once
#include "ProgramState.h"

namespace Unloved {
    bool Init();
    void CleanUp();

    void BeginFrame();
    void Update();
    void Render();
    void EndFrame();

    void SetProgramState(ProgramState programState);
    ProgramState GetProgramState();
}
