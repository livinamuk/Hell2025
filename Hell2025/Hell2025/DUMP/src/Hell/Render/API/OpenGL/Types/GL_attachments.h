#pragma once
#include <glad/gl.h>
#include <glm/vec4.hpp>
#include <string>

struct ColorAttachment {
    std::string name = "undefined";
    GLuint handle = 0;
    GLenum internalFormat = GL_NONE;
    GLenum format = GL_NONE;
    GLenum type = GL_NONE;
    GLenum target = GL_TEXTURE_2D;
    GLenum minFilter = GL_LINEAR;
    GLenum magFilter = GL_LINEAR;
    GLenum wrapFilter = GL_CLAMP_TO_EDGE;
    bool allocateMips = false;
};

struct DepthAttachment {
    GLuint handle = 0;
    GLenum internalFormat = GL_NONE;
    GLenum minFilter = GL_LINEAR;
    GLenum magFilter = GL_LINEAR;
    GLint wrapFilter = GL_CLAMP_TO_EDGE;
    glm::vec4 borderColor = glm::vec4(1.0f);
};