#pragma once

#include "Hell/Render/API/OpenGL/Types/GL_rasterizer_state.h"

#include <string>

namespace OpenGL::RasterizerStateManager {
    OpenGLRasterizerState* CreateRasterizerState(const std::string& name);
    OpenGLRasterizerState* GetRasterizerState(const std::string& name);
    void ForceRasterizerState(const std::string& name);
    void ForceRasterizerState(const OpenGLRasterizerState& rasterizerState);
    void SetRasterizerState(const std::string& name);
    void SetRasterizerState(const OpenGLRasterizerState& rasterizerState);
    void VerifyStateCache();
}
