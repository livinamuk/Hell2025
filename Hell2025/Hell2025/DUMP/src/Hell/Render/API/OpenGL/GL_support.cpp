#include "GL_support.h"

#include "Hell/Logging.h"

#include <glad/gl.h>
#include <string>
#include <vector>

namespace OpenGL::Support {

    struct SupportQuery {
        std::string featureName;
        std::vector<int> gladPointers;
    };

	struct DeviceCapabilities {
        int maxAttachments = 0;
        int maxDrawBuffers = 0;
		int maxShaderStorageBlockSize = 0;
        int maxComputeWorkGroupInvocations = 0;
        float maxAnisotropy = 0.0f;
	} g_deviceCapabilities;

    bool CheckSupport() {
        glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &g_deviceCapabilities.maxAttachments);
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &g_deviceCapabilities.maxDrawBuffers);
        glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &g_deviceCapabilities.maxShaderStorageBlockSize);
        glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &g_deviceCapabilities.maxComputeWorkGroupInvocations);
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &g_deviceCapabilities.maxAnisotropy);

        Logging::Support() << "Max color attachments: " << g_deviceCapabilities.maxAttachments << "\n";
        Logging::Support() << "Max draw buffers: " << g_deviceCapabilities.maxDrawBuffers << "\n";
        Logging::Support() << "Max SSBO size: " << (g_deviceCapabilities.maxShaderStorageBlockSize / 1024 / 1024) << " MB\n";
        Logging::Support() << "Max compute invocations: " << g_deviceCapabilities.maxComputeWorkGroupInvocations << "\n";
        Logging::Support() << "Max anisotropy: " << g_deviceCapabilities.maxAnisotropy << "x\n";

        std::vector<SupportQuery> requirements = {
            { "Bindless Textures", { GLAD_GL_ARB_bindless_texture } },
            // { "Mesh Shaders", { GLAD_GL_NV_mesh_shader, GLAD_GL_EXT_mesh_shader } }
        };

        bool allFound = true;

        for (const auto& query : requirements) {
            bool featureSupported = false;

            for (int ptr : query.gladPointers) {
                if (ptr != 0) {
                    featureSupported = true;
                    break;
                }
            }

            if (featureSupported) {
                Logging::Support() << query.featureName << " supported\n";
            }
            else {
                Logging::Error() << query.featureName << " not supported\n";
                allFound = false;
            }
        }

        if (allFound) {
            Logging::Support() << "All hardware requirements met\n";
            return true;
        }
        else {
            Logging::Fatal() << "Hardware does not meet minimum requirements\n";
            return false;
        }
	}
}
