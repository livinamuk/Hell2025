#pragma once

#include <glad/gl.h>

struct OpenGLRasterizerState {
    // Blending
    GLboolean blendEnable = false;
    GLenum blendFuncSrcfactor = GL_SRC_ALPHA;
    GLenum blendFuncDstfactor = GL_ONE_MINUS_SRC_ALPHA;

    // Color
    GLboolean colorMask = true;

    // Depth
    GLboolean depthTestEnabled = true;
    GLboolean depthMask = true;
    GLenum depthFunc = GL_LESS;

    // Misc
    GLboolean cullfaceEnable = true;
    GLenum cullfaceMode = GL_BACK;
    GLfloat pointSize = 1.0f;

    // Stencil
    GLboolean stencilTestEnabled = false;
    GLenum stencilFunc = GL_ALWAYS;
    GLint stencilRef = 0;
    GLuint stencilReadMask = 0xFF;
    GLuint stencilWriteMask = 0xFF;
    GLenum stencilFailOp = GL_KEEP;
    GLenum stencilDepthFailOp = GL_KEEP;
    GLenum stencilPassOp = GL_KEEP;
};
