#include "Unloved/Render/API/OpenGL/GL_renderer.h"
#include "World/LegacyWorld.h"

namespace OpenGL::Renderer {
    using namespace Unloved;


    void BlitRoads() {
        OpenGLFrameBuffer* roadFramebuffer = OpenGL::ResourceManager::GetFrameBufferPtr("Road");
        OpenGLShader* shader = OpenGL::ResourceManager::GetShaderPtr("BlitRoad");

        if (!roadFramebuffer) return;
        if (!shader) return;

        if (LegacyWorld::GetRoads().empty()) return;

        roadFramebuffer->ClearTexImage("RoadMask", 0.0f, 0.0f, 0.0f, 1.0f);

        Road& road = LegacyWorld::GetRoads()[0];

        std::vector<glm::vec4> controlPoints;
        for (glm::vec3 point : road.m_worldPoints) {
            controlPoints.push_back(glm::vec4(point, 1.0f));
        }

        if (controlPoints.size() < 2) return;

        static GLuint roadPointsWorldSsbo = 0;
        if (roadPointsWorldSsbo == 0) glGenBuffers(1, &roadPointsWorldSsbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_IDX_ROAD_BLIT_POINTS, roadPointsWorldSsbo);
        glBufferData(GL_SHADER_STORAGE_BUFFER, controlPoints.size() * sizeof(glm::vec4), controlPoints.data(), GL_DYNAMIC_DRAW);

        GLuint roadMapTextureHandle = roadFramebuffer->GetColorAttachmentHandleByName("RoadMask");
        glBindImageTexture(0, roadMapTextureHandle, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);

        int textureWidthInPixels = roadFramebuffer->GetWidth();
        int textureHeightInPixels = roadFramebuffer->GetHeight();

        glm::vec2 u_worldSpanXZ = glm::vec2(LegacyWorld::GetWorldSpaceWidth(), LegacyWorld::GetWorldSpaceDepth());

        float u_roadWidthInMeters = 4.3f;
        float u_roadEdgeFeatherInMeters = 0.3f;
        float u_falloffExponent = 1.0f;

        OpenGL::BindShader("BlitRoad");
        OpenGL::SetUniformInt("u_numberOfControlPoints", (int)controlPoints.size());
        OpenGL::SetUniformIVec2("u_textureSizeInPixels", { textureWidthInPixels, textureHeightInPixels });
        OpenGL::SetUniformVec2("u_worldSpanXZ", u_worldSpanXZ);
        OpenGL::SetUniformFloat("u_roadWidthInMeters", u_roadWidthInMeters);
        OpenGL::SetUniformFloat("u_roadEdgeFeatherInMeters", u_roadEdgeFeatherInMeters);
        OpenGL::SetUniformFloat("u_falloffExponent", u_falloffExponent);

        GLuint groupCountX = (textureWidthInPixels + 8 - 1) / 8;
        GLuint groupCountY = (textureHeightInPixels + 8 - 1) / 8;
        OpenGL::DispatchCompute(groupCountX, groupCountY, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }
}