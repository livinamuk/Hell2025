#include "GL_texture_uploader.h"

#include "Hell/Render/API/OpenGL/GL_util.h"
#include "Hell/Logging.h"

#include <array>
#include <cstring>
#include <deque>
#include <limits>
#include <vector>

namespace OpenGL::TextureUploader {

    namespace {
        constexpr size_t MAX_IN_FLIGHT_UPLOAD_COUNT = 8;

        struct UploadSlot {
            GLuint pbo = 0;
            size_t capacity = 0;
            GLsync fence = nullptr;
            Texture* texture = nullptr;
        };

        std::array<UploadSlot, MAX_IN_FLIGHT_UPLOAD_COUNT> g_uploadSlots;
        std::deque<Texture*> g_uploadQueue;
        std::vector<Texture*> g_completedUploads;

        size_t AlignPBOOffset(size_t offset) {
            constexpr size_t alignment = 4;
            return (offset + alignment - 1) & ~(alignment - 1);
        }

        size_t GetUploadMipCount(Texture& texture) {
            return texture.MipmapsAreRequested() ? texture.GetImageData().mips.size() : 1;
        }

        bool ValidateTexture(Texture& texture) {
            const ImageData& imageData = texture.GetImageData();

            if (imageData.mips.empty()) {
                Logging::Error() << "OpenGL::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has no image data\n";
                return false;
            }

            if (OpenGL::Util::ImageFormatToGLFormat(imageData.format) == GL_NONE || OpenGL::Util::ImageFormatToGLInternalFormat(imageData.format) == GL_NONE) {
                Logging::Error() << "OpenGL::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has an unsupported image format\n";
                return false;
            }

            const size_t uploadMipCount = GetUploadMipCount(texture);
            for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
                const TextureMip& mip = imageData.mips[mipIndex];

                if (mip.width == 0 || mip.height == 0 || mip.data.empty()) {
                    Logging::Error() << "OpenGL::TextureUploader::ValidateTexture(..) failed because texture '" << texture.GetFileName() << "' has invalid mip " << mipIndex << "\n";
                    return false;
                }

                if (mip.data.size() > static_cast<size_t>(std::numeric_limits<GLsizei>::max())) {
                    Logging::Error() << "OpenGL::TextureUploader::ValidateTexture(..) failed because mip " << mipIndex << " of texture '" << texture.GetFileName() << "' is too large\n";
                    return false;
                }
            }

            return true;
        }

        OpenGLTexture& CreateGPUTexture(Texture& texture) {
            const ImageData& imageData = texture.GetImageData();
            const TextureMip& baseMip = imageData.mips[0];
            const bool generateMipmaps = texture.MipmapsAreRequested() && imageData.mips.size() == 1;
            const int allocatedMipCount = generateMipmaps ? texture.GetMipmapLevelCount() : static_cast<int>(GetUploadMipCount(texture));
            const GLenum internalFormat = OpenGL::Util::ImageFormatToGLInternalFormat(imageData.format);
            OpenGLTexture& glTexture = texture.GetGLTexture();

            glTexture.Create(static_cast<int>(baseMip.width), static_cast<int>(baseMip.height), internalFormat, allocatedMipCount);
            glTexture.SetWrapModeS(texture.GetTextureWrapModeS());
            glTexture.SetWrapModeT(texture.GetTextureWrapModeT());
            glTexture.SetMinFilter(texture.GetMinFilter());
            glTexture.SetMagFilter(texture.GetMagFilter());
            glTexture.SetBorderColor(texture.GetBorderColor());
            glTexture.MakeBindlessTextureResident();

            return glTexture;
        }

        void MarkUploadInProgress(Texture& texture) {
            texture.SetUploadState(UploadState::UPLOADING);

            for (int mipIndex = 0; mipIndex < texture.GetTextureDataCount(); ++mipIndex) {
                texture.SetTextureDataLevelBakeState(mipIndex, BakeState::BAKING_IN_PROGRESS);
            }
        }

        void MarkUploadComplete(Texture& texture) {
            texture.SetUploadState(UploadState::UPLOADED);

            for (int mipIndex = 0; mipIndex < texture.GetTextureDataCount(); ++mipIndex) {
                texture.SetTextureDataLevelBakeState(mipIndex, BakeState::BAKE_COMPLETE);
            }

            texture.CheckForBakeCompletion();
            g_completedUploads.push_back(&texture);
        }

        bool SubmitUpload(UploadSlot& slot, Texture& texture) {
            if (!ValidateTexture(texture)) {
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            const ImageData& imageData = texture.GetImageData();
            const size_t uploadMipCount = GetUploadMipCount(texture);
            std::vector<size_t> mipOffsets(uploadMipCount);
            size_t requiredSize = 0;

            for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
                requiredSize = AlignPBOOffset(requiredSize);
                mipOffsets[mipIndex] = requiredSize;

                if (imageData.mips[mipIndex].data.size() > std::numeric_limits<size_t>::max() - requiredSize) {
                    Logging::Error() << "OpenGL::TextureUploader::SubmitUpload(..) failed because texture '" << texture.GetFileName() << "' is too large\n";
                    texture.SetUploadState(UploadState::FAILED);
                    return false;
                }

                requiredSize += imageData.mips[mipIndex].data.size();
            }

            if (requiredSize > static_cast<size_t>(std::numeric_limits<GLsizeiptr>::max())) {
                Logging::Error() << "OpenGL::TextureUploader::SubmitUpload(..) failed because texture '" << texture.GetFileName() << "' exceeds the maximum PBO size\n";
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            if (slot.pbo == 0) {
                glCreateBuffers(1, &slot.pbo);
            }

            if (requiredSize > slot.capacity) {
                glNamedBufferData(slot.pbo, static_cast<GLsizeiptr>(requiredSize), nullptr, GL_STREAM_DRAW);
                slot.capacity = requiredSize;
            }

            std::byte* mappedData = static_cast<std::byte*>(glMapNamedBufferRange(slot.pbo, 0, static_cast<GLsizeiptr>(requiredSize), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT));
            if (!mappedData) {
                Logging::Error() << "OpenGL::TextureUploader::SubmitUpload(..) failed to map a PBO for texture '" << texture.GetFileName() << "'\n";
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
                const TextureMip& mip = imageData.mips[mipIndex];
                std::memcpy(mappedData + mipOffsets[mipIndex], mip.data.data(), mip.data.size());
            }

            if (glUnmapNamedBuffer(slot.pbo) == GL_FALSE) {
                Logging::Error() << "OpenGL::TextureUploader::SubmitUpload(..) failed to unmap a PBO for texture '" << texture.GetFileName() << "'\n";
                texture.SetUploadState(UploadState::FAILED);
                return false;
            }

            OpenGLTexture& glTexture = CreateGPUTexture(texture);
            const GLenum format = OpenGL::Util::ImageFormatToGLFormat(imageData.format);
            const GLenum internalFormat = OpenGL::Util::ImageFormatToGLInternalFormat(imageData.format);
            const GLenum dataType = OpenGL::Util::ImageFormatToGLDataType(imageData.format);

            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, slot.pbo);

            for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
                const TextureMip& mip = imageData.mips[mipIndex];
                const void* pboOffset = reinterpret_cast<const void*>(mipOffsets[mipIndex]);

                if (IsCompressedImageFormat(imageData.format)) {
                    glCompressedTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), internalFormat, static_cast<GLsizei>(mip.data.size()), pboOffset);
                }
                else {
                    glTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), format, dataType, pboOffset);
                }
            }

            if (texture.MipmapsAreRequested() && imageData.mips.size() == 1) {
                glGenerateTextureMipmap(glTexture.GetHandle());
            }

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            MarkUploadInProgress(texture);
            slot.fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            slot.texture = &texture;
            return true;
        }
    }

    bool ImmediateUpload(Texture& texture) {
        if (!ValidateTexture(texture)) {
            texture.SetUploadState(UploadState::FAILED);
            return false;
        }

        const ImageData& imageData = texture.GetImageData();
        const size_t uploadMipCount = GetUploadMipCount(texture);
        const GLenum format = OpenGL::Util::ImageFormatToGLFormat(imageData.format);
        const GLenum internalFormat = OpenGL::Util::ImageFormatToGLInternalFormat(imageData.format);
        const GLenum dataType = OpenGL::Util::ImageFormatToGLDataType(imageData.format);
        OpenGLTexture& glTexture = CreateGPUTexture(texture);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        for (size_t mipIndex = 0; mipIndex < uploadMipCount; ++mipIndex) {
            const TextureMip& mip = imageData.mips[mipIndex];

            if (IsCompressedImageFormat(imageData.format)) {
                glCompressedTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), internalFormat, static_cast<GLsizei>(mip.data.size()), mip.data.data());
            }
            else {
                glTextureSubImage2D(glTexture.GetHandle(), static_cast<GLint>(mipIndex), 0, 0, static_cast<GLsizei>(mip.width), static_cast<GLsizei>(mip.height), format, dataType, mip.data.data());
            }
        }

        if (texture.MipmapsAreRequested() && imageData.mips.size() == 1) {
            glGenerateTextureMipmap(glTexture.GetHandle());
        }

        MarkUploadComplete(texture);
        return true;
    }

    void QueueUpload(Texture& texture) {
        texture.SetUploadState(UploadState::QUEUED);
        g_uploadQueue.push_back(&texture);
    }

    void Update() {
        for (UploadSlot& slot : g_uploadSlots) {
            if (!slot.fence) {
                continue;
            }

            const GLenum result = glClientWaitSync(slot.fence, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
            if (result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED) {
                glDeleteSync(slot.fence);
                slot.fence = nullptr;
                MarkUploadComplete(*slot.texture);
                slot.texture = nullptr;
            }
            else if (result == GL_WAIT_FAILED) {
                Logging::Error() << "OpenGL::TextureUploader::Update(..) encountered a failed texture upload fence\n";
                if (slot.texture) {
                    slot.texture->SetUploadState(UploadState::FAILED);
                }
                glDeleteSync(slot.fence);
                slot.fence = nullptr;
                slot.texture = nullptr;
            }
        }

        for (UploadSlot& slot : g_uploadSlots) {
            if (g_uploadQueue.empty()) {
                break;
            }

            if (slot.fence || slot.texture) {
                continue;
            }

            Texture* texture = g_uploadQueue.front();
            g_uploadQueue.pop_front();
            SubmitUpload(slot, *texture);
        }
    }

    std::vector<Texture*> ConsumeCompletedUploads() {
        std::vector<Texture*> completedUploads = std::move(g_completedUploads);
        g_completedUploads.clear();
        return completedUploads;
    }

    void CleanUp() {
        g_uploadQueue.clear();
        g_completedUploads.clear();

        for (UploadSlot& slot : g_uploadSlots) {
            if (slot.fence) {
                glDeleteSync(slot.fence);
                slot.fence = nullptr;
            }

            if (slot.pbo) {
                glDeleteBuffers(1, &slot.pbo);
                slot.pbo = 0;
            }

            slot.capacity = 0;
            slot.texture = nullptr;
        }
    }
}
