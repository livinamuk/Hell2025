#include "GL_renderer.h"
#include "Hell/Backend/BackEnd.h"

#include "Hell/ResourceManagement/ResourceManager.h"

namespace OpenGL::Renderer {

    void BlitFrameBufferDepth(OpenGLFrameBuffer* srcFrameBuffer, OpenGLFrameBuffer* dstFrameBuffer, const Unloved::Viewport* viewport) {
        glm::vec2 position = viewport->GetPosition();
        glm::vec2 size = viewport->GetSize();
        BlitRect srcRect{ position.x * srcFrameBuffer->GetWidth(), position.y * srcFrameBuffer->GetHeight(), (position.x + size.x) * srcFrameBuffer->GetWidth(), (position.y + size.y) * srcFrameBuffer->GetHeight() };
        BlitRect dstRect{ position.x * dstFrameBuffer->GetWidth(), position.y * dstFrameBuffer->GetHeight(), (position.x + size.x) * dstFrameBuffer->GetWidth(), (position.y + size.y) * dstFrameBuffer->GetHeight() };
        OpenGL::BlitFrameBufferDepth(srcFrameBuffer, dstFrameBuffer, srcRect, dstRect);
    }

    GLint CreateQuadVAO() {
        GLuint vao = 0;
        GLuint vbo = 0;
        float vertices[] = {
            -1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f,
             1.0f,  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
        glBindVertexArray(0);
        return vao;
    }

    BlitRect BlitRectFromFrameBufferViewport(OpenGLFrameBuffer* framebuffer, Unloved::Viewport* viewport) {
        GLuint fbWidth = framebuffer->GetWidth();
        GLuint fbHeight = framebuffer->GetHeight();
        glm::vec2 pos = viewport->GetPosition();
        glm::vec2 size = viewport->GetSize();
        GLint x = static_cast<GLint>(pos.x * fbWidth);
        GLint y = static_cast<GLint>(pos.y * fbHeight);
        GLsizei w = static_cast<GLsizei>(size.x * fbWidth);
        GLsizei h = static_cast<GLsizei>(size.y * fbHeight);
        BlitRect blitRect;
        blitRect.x0 = x;
        blitRect.x1 = x + w;
        blitRect.y0 = y;
        blitRect.y1 = y + h;
        return blitRect;
    }

    void SetViewport(OpenGLFrameBuffer* framebuffer, Unloved::Viewport* viewport) {
        GLuint fbWidth = framebuffer->GetWidth();
        GLuint fbHeight = framebuffer->GetHeight();
        glm::vec2 pos = viewport->GetPosition();
        glm::vec2 size = viewport->GetSize();
        GLint x = static_cast<GLint>(pos.x * fbWidth);
        GLint y = static_cast<GLint>(pos.y * fbHeight);
        GLsizei w = static_cast<GLsizei>(size.x * fbWidth);
        GLsizei h = static_cast<GLsizei>(size.y * fbHeight);
        glViewport(x, y, w, h);
    }

    void ClearFrameBufferByViewport(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLfloat r, GLfloat g, GLfloat b, GLfloat a) {
        int xOffset = viewport->GetPosition().x * framebuffer->GetWidth();
        int yOffset = viewport->GetPosition().y * framebuffer->GetHeight();
        int width = viewport->GetSize().x * framebuffer->GetWidth();
        int height = viewport->GetSize().y * framebuffer->GetHeight();
        framebuffer->ClearAttachmenSubRegion(attachmentName, xOffset, yOffset, width, height, r, g, b, a);
    }

    void ClearFrameBufferByViewportInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLint r, GLint g, GLint b, GLint a) {
        int xOffset = viewport->GetPosition().x * framebuffer->GetWidth();
        int yOffset = viewport->GetPosition().y * framebuffer->GetHeight();
        int width = viewport->GetSize().x * framebuffer->GetWidth();
        int height = viewport->GetSize().y * framebuffer->GetHeight();
        framebuffer->ClearAttachmenSubRegionInt(attachmentName, xOffset, yOffset, width, height, r, g, b, a);
    }

    void ClearFrameBufferByViewportUInt(OpenGLFrameBuffer* framebuffer, const char* attachmentName, Unloved::Viewport* viewport, GLuint r, GLuint g, GLuint b, GLuint a) {
        int xOffset = viewport->GetPosition().x * framebuffer->GetWidth();
        int yOffset = viewport->GetPosition().y * framebuffer->GetHeight();
        int width = viewport->GetSize().x * framebuffer->GetWidth();
        int height = viewport->GetSize().y * framebuffer->GetHeight();

        float vx = viewport->GetPosition().x;
        float vy = viewport->GetPosition().y;
        float vw = viewport->GetSize().x;
        float vh = viewport->GetSize().y;

        framebuffer->ClearAttachmenSubRegionUInt(attachmentName, xOffset, yOffset, width, height, r, g, b, a);
    }

}
