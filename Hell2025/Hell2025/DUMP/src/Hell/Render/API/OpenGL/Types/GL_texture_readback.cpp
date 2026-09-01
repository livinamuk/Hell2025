#include "GL_texture_readback.h"
#include "Hell/Render/API/OpenGL/GL_util.h"
/*

    *****************

      Example usage

    ***************** (WARNING: still not async becoz pbo uses glBuffer usage instead of glBufferStorage... potentially stalling glReadPixels

    if (false) {
            static OpenGLTextureReadBackRGBA32F readback;
            static bool savedFile = false;
            if (Input::KeyPressed(HELL_KEY_SPACE)) {
                GLuint fboHandle = gBuffer.GetHandle();
                GLuint attachment = gBuffer.GetColorAttachmentSlotByName("WorldSpacePosition");
                int xOffset = 0;
                int yOffset = 0;
                int width = gBuffer.GetWidth();
                int height = gBuffer.GetHeight();
                readback.IssueDataRequest(fboHandle, attachment, xOffset, yOffset, width, height);
                savedFile = false;
            }
            if (readback.IsRequestInProgress()) {
                readback.Update();
            }
            if (readback.IsResultReady() && !savedFile) {
                ImageTools::SaveFloatArrayTextureAsBitmap(readback.GetData(), readback.GetWidth(), readback.GetHeight(), ImageFormat::RGBA32_SFLOAT, "testcunt.bmp");
                std::cout << "saved rgba32f file\n";
                savedFile = true;
            }
        }
        else {
            static OpenGLTextureReadBackR8 readback;
            static bool savedFile = false;
            if (Input::KeyPressed(HELL_KEY_SPACE)) {
                OpenGLFrameBuffer& heightMapFbo = OpenGL::ResourceManager::GetFrameBuffer("HeightMap");
                GLuint fboHandle = heightMapFbo.GetHandle();
                GLuint attachment = heightMapFbo.GetColorAttachmentSlotByName("Color");
                int xOffset = 0;
                int yOffset = 0;
                int width = heightMapFbo.GetWidth();
                int height = heightMapFbo.GetHeight();
                readback.IssueDataRequest(fboHandle, attachment, xOffset, yOffset, width, height);
                savedFile = false;
            }
            if (readback.IsRequestInProgress()) {
                readback.Update();
            }
            if (readback.IsResultReady() && !savedFile) {
                ImageTools::SaveFloatArrayTextureAsBitmap(readback.GetData(), readback.GetWidth(), readback.GetHeight(), ImageFormat::R8_UNORM, "testcunt.bmp");
                std::cout << "saved r8 file\n";
            }
        }

*/

void OpenGLTextureReadBack::IssueDataRequest(unsigned int framebufferHandle, unsigned int attachmentSlot, int offsetX, int offsetY, int width, int height, int internalFormat) {
    if (internalFormat == GL_RGBA32F) {
        m_channelCount = 4;
        m_bufferSize = width * height * sizeof(float) * m_channelCount;
        m_floatData.resize(width * height * m_channelCount);
        m_uint16Data.clear();
    }
    else if (internalFormat == GL_R16F) {
        m_channelCount = 1;
        m_bufferSize = width * height * sizeof(uint16_t) * m_channelCount;
        m_uint16Data.resize(width * height * m_channelCount);
        m_floatData.clear();
    }
    else {
        m_framebufferHandle = 0;
        m_attachmentSlot = 0;
        m_offsetX = 0;
        m_offsetY = 0;
        m_width = 0;
        m_height = 0;
        m_channelCount = 0;
        m_internalFormat = 0;
        m_resultReady = false;
        m_requestInProgress = false;
        m_bufferSize = 0;
        m_floatData.clear();
        m_uint16Data.clear();
        std::cout << "OpenGLTextureReadBack::IssueDataRequest() failed: " << OpenGL::Util::GLInternalFormatToString(internalFormat) << " is either invalid or not yet implemented!\n";
    }

    m_framebufferHandle = framebufferHandle;
    m_attachmentSlot = attachmentSlot;
    m_internalFormat = internalFormat;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
    m_width = width;
    m_height = height;
    m_resultReady = false;
    m_requestInProgress = false;
    Update();
}

void OpenGLTextureReadBack::Update() {
    // Job done? then leave
    if (m_resultReady) {
        return;
    }

    // Create/recreate the pbo if it doesn't exist or requested data size exceeds allocated pbo buffer size
    if (m_pbo.GetHandle() == 0 || m_bufferSize > m_pbo.GetAllocatedBufferSize()) {
        m_pbo.CleanUp();
        m_pbo.Init(m_bufferSize);
    }

    m_pbo.UpdateState();

    // Still waiting for PBO sync? then return early
    if (!m_pbo.IsSyncComplete()) {
        return;
    }

    // PBO work done? The access/store the data in the PBO
    if (m_requestInProgress && m_pbo.IsSyncComplete()) {
        if (m_internalFormat == GL_RGBA32F) {
            const float* mappedBuffer = reinterpret_cast<const float*>(m_pbo.GetPersistentBuffer());
            if (mappedBuffer) {
                m_floatData.assign(mappedBuffer, mappedBuffer + (m_width * m_height * m_channelCount));
                m_resultReady = true;
                m_requestInProgress = false;
            }
        }
        else if (m_internalFormat == GL_R16F) {
            const uint16_t* mappedBuffer = reinterpret_cast<const uint16_t*>(m_pbo.GetPersistentBuffer());
            if (mappedBuffer) {
                m_uint16Data.assign(mappedBuffer, mappedBuffer + (m_width * m_height));
                m_resultReady = true;
                m_requestInProgress = false;
            }
        }
        else {
            std::cout << "OpenGLTextureReadBack::Update() failed: " << OpenGL::Util::GLInternalFormatToString(m_internalFormat) << " is either invalid or not yet implemented!\n";
        }

    }

    // If you made it this far, then the job is not done, and the pbo is available for work, so issue the work
    if (!m_pbo.IsSyncInProgress()) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo.GetHandle());
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferHandle);
        glReadBuffer(m_attachmentSlot);
        if (m_internalFormat == GL_RGBA32F) {
            glReadPixels(m_offsetX, m_offsetY, m_width, m_height, GL_RGBA, GL_FLOAT, nullptr);      // maybe this should be 0?
        }
        else if (m_internalFormat == GL_R16F) {
            glReadPixels(m_offsetX, m_offsetY, m_width, m_height, GL_RED, GL_HALF_FLOAT, nullptr);  // maybe this should be 0?
        }
        else {
            std::cout << "OpenGLTextureReadBack::Update() failed: " << OpenGL::Util::GLInternalFormatToString(m_internalFormat) << " is either invalid or not yet implemented!\n";
            return;
        }
        m_pbo.SyncStart();
        m_pbo.UpdateState();
        m_requestInProgress = true;
    }
}

void OpenGLTextureReadBack::Reset() {
    m_resultReady = false;
    m_requestInProgress = false;
}

glm::vec4 OpenGLTextureReadBack::GetFloatPixel(int index) {
    if (index < 0 || index >= m_width * m_height) {
        std::cout << "vec4 OpenGLTextureReadBackRGBA32F::GetPixel() failed: Index out of bounds\n";
        return glm::vec4(-1.0f);
    }
    if (!m_resultReady) {
        std::cout << "vec4 OpenGLTextureReadBackRGBA32F::GetPixel() failed: Result not ready\n";
        return glm::vec4(-1.0f);
    }
    int baseIndex = index * m_channelCount;
    return glm::vec4(
        m_floatData[baseIndex],
        m_floatData[baseIndex + 1],
        m_floatData[baseIndex + 2],
        m_floatData[baseIndex + 3]
    );
}


void OpenGLTextureReadBackR8::IssueDataRequest(GLuint framebufferHandle, GLuint attachmentSlot, int offsetX, int offsetY, int width, int height) {
    m_framebufferHandle = framebufferHandle;
    m_attachmentSlot = attachmentSlot;
    m_offsetX = offsetX;
    m_offsetY = offsetY;
    m_width = width;
    m_height = height;
    m_data.resize(m_width * m_height);
    m_bufferSize = m_width * m_height * sizeof(uint8_t);
    m_resultReady = false;
    m_requestInProgress = false;
    Update();
}

void OpenGLTextureReadBackR8::Update() {
    // Job done? then leave
    if (m_resultReady) {
        return;
    }

    // Create/recreate the pbo if it doesn't exist or requested data size exceeds allocated pbo buffer size
    if (m_pbo.GetHandle() == 0 || m_bufferSize > m_pbo.GetAllocatedBufferSize()) {
        m_pbo.CleanUp();
        m_pbo.Init(m_bufferSize);
    }

    m_pbo.UpdateState();

    // Still waiting for PBO sync? then return early
    if (!m_pbo.IsSyncComplete()) {
        return;
    }

    // PBO work done? The access/store the data in the PBO
    if (m_requestInProgress && m_pbo.IsSyncComplete()) {
        const uint8_t* mappedBuffer = reinterpret_cast<const uint8_t*>(m_pbo.GetPersistentBuffer());
        if (mappedBuffer) {
            m_data.assign(mappedBuffer, mappedBuffer + (m_width * m_height)); // 1 byte per pixel
            m_resultReady = true;
            m_requestInProgress = false;
            return;
        }
    }

    // If you made it this far, then the job is not done, and the pbo is available for work, so issue the work
    if (!m_pbo.IsSyncInProgress()) {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, m_pbo.GetHandle());
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebufferHandle);
        glReadBuffer(m_attachmentSlot);
        glReadPixels(m_offsetX, m_offsetY, m_width, m_height, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        m_pbo.SyncStart();
        m_pbo.UpdateState();
        m_requestInProgress = true;
    }
}

void OpenGLTextureReadBackR8::Reset() {
    m_resultReady = false;
    m_requestInProgress = false;
}
