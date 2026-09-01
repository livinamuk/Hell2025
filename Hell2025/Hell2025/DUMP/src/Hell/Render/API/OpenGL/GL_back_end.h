#pragma once

#include "Hell/Common.h"
#include "Hell/Render/VertexAttributes.h"

#include "Unloved/Common/Types.h"
#include "Types/GL_texture.h"
#include "Hell/ResourceManagement/Types/Texture.h"
#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <string>
#include <vector>

namespace OpenGL::BackEnd {
    // Core
    void Init();
	void BeginFrame();

    // Textures
    void AllocateTextureMemory(Texture& texture);
    const std::vector<GLuint64>& GetBindlessTextureIDs();

    // Buffers
    void AllocateSkinnedVertexBufferSpace(uint32_t vertexCount);

    void SetDepthClearValue(float value);


    GLuint GetSkinnedVertexDataVAO();
    GLuint GetSkinnedVertexDataVBO();
    GLuint GetPreviousSkinnedPositionBuffer();
}
