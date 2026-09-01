#pragma once

#include "Hell/ResourceManagement/Types/Texture.h"

#include <vector>

namespace OpenGL::TextureUploader {
    bool ImmediateUpload(Texture& texture);
    void QueueUpload(Texture& texture);
    void Update();
    std::vector<Texture*> ConsumeCompletedUploads();
    void CleanUp();
}
