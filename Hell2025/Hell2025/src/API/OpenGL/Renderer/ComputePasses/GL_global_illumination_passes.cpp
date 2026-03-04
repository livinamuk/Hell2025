#include "API/OpenGL/Renderer/GL_renderer.h"
#include "API/OpenGL/GL_backend.h"
#include "AssetManagement/AssetManager.h"
#include "Bvh/Gpu/Bvh.h"
#include "Renderer/RenderDataManager.h"
#include "Renderer/GlobalIllumination.h"
#include "Viewport/ViewportManager.h"
#include "World/World.h"

#include "Input/Input.h" // REMOVE ME!

namespace OpenGLRenderer {

    GLuint g_pointCloudVao = 0;
    GLuint g_pointCloudVbo = 0;
    //GLuint g_pointGridBuffer = 0;
    //GLuint g_pointIndicesBuffer = 0;

    void UploadPointCloud();
    void RaytraceScene();

    void UpdateGlobalIllumintation() {
        if (GlobalIllumination::PointCloudNeedsGpuUpdate()) {
            UploadPointCloud();
        }

        OpenGLSSBO* triangleDataSSBO = GetSSBO("TriangleData");
        OpenGLSSBO* sceneBvhSSBO = GetSSBO("SceneBvh");
        OpenGLSSBO* meshesBvhSSBO = GetSSBO("MeshesBvh");
        OpenGLSSBO* entityInstancesSSBO = GetSSBO("EntityInstances");
        OpenGLSSBO* lightsSSBO = GetSSBO("Lights");

        OpenGLSSBO* pointGridBufferSSBO = GetSSBO("PointGridBuffer");
        OpenGLSSBO* pointIndicesBufferSSBO = GetSSBO("PointIndicesBuffer");
        if (!triangleDataSSBO || !sceneBvhSSBO || !meshesBvhSSBO || !entityInstancesSSBO || !lightsSSBO || !pointGridBufferSSBO || !pointIndicesBufferSSBO) {
            return;
        }
        
        uint64_t sceneBvhId = GlobalIllumination::GetSceneBvhId();

        const std::vector<BvhNode>& sceneNodes = GlobalIllumination::GetSceneNodes();
        const std::vector<BvhNode>& meshBvhNodes = Bvh::Gpu::GetMeshGpuBvhNodes();
        const std::vector<GpuPrimitiveInstance>& entityInstances = Bvh::Gpu::GetGpuEntityInstances(sceneBvhId);
        const std::vector<float>& triData = Bvh::Gpu::GetTriangleData();
        std::vector<PointCloudOctrant>& pointCloudOctrants = GlobalIllumination::GetPointCloudOctrants();
        std::vector<unsigned int>& pointIndices = GlobalIllumination::GetPointIndices();

        const void* sceneNodeData = sceneNodes.empty() ? nullptr : sceneNodes.data();
        const void* meshBvhNodeData = meshBvhNodes.empty() ? nullptr : meshBvhNodes.data();
        const void* entityInstanceData = entityInstances.empty() ? nullptr : entityInstances.data();
        const void* triDataPtr = triData.empty() ? nullptr : triData.data();
        const void* pointCloudOctrantData = pointCloudOctrants.empty() ? nullptr : pointCloudOctrants.data();
        const void* pointIndexData = pointIndices.empty() ? nullptr : pointIndices.data();

        UpdateSSBO("SceneBvh", sceneNodes.size() * sizeof(BvhNode), sceneNodeData);
        UpdateSSBO("MeshesBvh", meshBvhNodes.size() * sizeof(BvhNode), meshBvhNodeData);
        UpdateSSBO("EntityInstances", entityInstances.size() * sizeof(GpuPrimitiveInstance), entityInstanceData);
        UpdateSSBO("TriangleData", triData.size() * sizeof(float), triDataPtr);
        UpdateSSBO("PointGridBuffer", pointCloudOctrants.size() * sizeof(PointCloudOctrant), pointCloudOctrantData);
        UpdateSSBO("PointIndicesBuffer", pointIndices.size() * sizeof(unsigned int), pointIndexData);

        //std::cout << "pointCloudOctrants.size(): " << pointCloudOctrants.size() << "\n";
        //std::cout << "pointIndices.size(): " << pointIndices.size() << "\n";

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, g_pointCloudVbo);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleDataSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, sceneBvhSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, meshesBvhSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, entityInstancesSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightsSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, pointGridBufferSSBO->GetHandle());
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, pointIndicesBufferSSBO->GetHandle());
    }

    void UploadPointCloud() {
        if (g_pointCloudVao == 0) {
            glGenVertexArrays(1, &g_pointCloudVao);
            glGenBuffers(1, &g_pointCloudVbo);
        }

        std::vector<CloudPoint>& pointCloud = GlobalIllumination::GetPointCloud();
        std::vector<PointCloudOctrant>& pointCloudOctrants = GlobalIllumination::GetPointCloudOctrants();
        std::vector<unsigned int>& pointIndics = GlobalIllumination::GetPointIndices();

        // Point cloud
        glBindBuffer(GL_ARRAY_BUFFER, g_pointCloudVbo);
        glBindVertexArray(g_pointCloudVao);
        glBufferData(GL_ARRAY_BUFFER, sizeof(CloudPoint) * pointCloud.size(), pointCloud.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, position));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(CloudPoint), (void*)offsetof(CloudPoint, directLighting));

        // Octrants
        //glGenBuffers(1, &g_pointGridBuffer);
        //glGenBuffers(1, &g_pointIndicesBuffer);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_pointGridBuffer);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(PointCloudOctrant) * pointCloudOctrants.size(), pointCloudOctrants.data(), GL_DYNAMIC_DRAW);
        //glBindBuffer(GL_SHADER_STORAGE_BUFFER, g_pointIndicesBuffer);
        //glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(unsigned int) * pointIndics.size(), pointIndics.data(), GL_DYNAMIC_DRAW);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        GlobalIllumination::SetPointCloudNeedsGpuUpdateState(false);
        std::cout << "Uploaded point cloud to GPU\n";
    }

    void PointCloudDirectLighting() {
        OpenGLShader* shader = GetShader("PointCloudLighting");
        shader->Bind();
        shader->SetInt("u_lightCount", World::GetLightCount());

        GLuint numGroupsX = (GlobalIllumination::GetPointCloud().size() + 127) / 128;

        glDispatchCompute(numGroupsX, 1, 1);
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

       // RaytraceScene();
    }

    //////void RaytraceScene() {
    //////    OpenGLShader* shader = GetShader("RaytraceScene");
    //////    OpenGLFrameBuffer* miscFullSizeFBO = GetFrameBuffer("MiscFullSize");
    //////
    //////    if (!miscFullSizeFBO) return;
    //////    if (!shader) return;
    //////
    //////    const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
    //////
    //////    shader->Bind();
    //////    shader->SetMat4("u_projectionMatrix", viewportData[0].projection);
    //////    shader->SetMat4("u_viewMatrix", viewportData[0].view);
    //////
    //////    glBindImageTexture(0, miscFullSizeFBO->GetColorAttachmentHandleByName("RaytracedScene"), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA8);
    //////
    //////    glDispatchCompute(miscFullSizeFBO->GetWidth() / 8, miscFullSizeFBO->GetHeight() / 8, 1);
    //////}

    void DrawPointCloud() {
        if (g_pointCloudVao == 0) return;

        OpenGLShader* shader = GetShader("DebugPointCloud");
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");

        if (!gBuffer) return;
        if (!shader) return;

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();

        gBuffer->Bind();
        gBuffer->DrawBuffer("FinalLighting");

        shader->Bind();

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        std::vector<CloudPoint>& pointCloud = GlobalIllumination::GetPointCloud();
        int pointCloudCount = pointCloud.size();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            shader->SetInt("u_viewportIndex", i);
            shader->SetMat4("u_projectionView", viewportData[i].projectionView);

            glBindVertexArray(g_pointCloudVao);
            glDrawArrays(GL_POINTS, 0, pointCloudCount);
            glBindVertexArray(0);
        }
    }

    void DrawLightVolume() {

        //static bool renderProbes = false;
        static bool showMask = false;
        //if (Input::KeyPressed(HELL_KEY_Z)) {
        //    renderProbes = !renderProbes;
        //}
        //if (Input::KeyPressed(HELL_KEY_C)) {
        //    showMask = !showMask;
        //}
        //if (!renderProbes) return;

        OpenGLShader* shader = GetShader("DebugLightVolume");
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");

        shader->Bind();

        gBuffer->Bind();
        gBuffer->DrawBuffer("FinalLighting");

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);
        glPointSize(2.0f);
        glPointSize(8.0f);

        glEnable(GL_DEPTH_TEST);

        glBindVertexArray(OpenGLBackEnd::GetVertexDataVAO());

        const std::vector<ViewportData>& viewportData = RenderDataManager::GetViewportData();
        Mesh* mesh = AssetManager::GetCubeMesh();

        for (int i = 0; i < 4; i++) {
            Viewport* viewport = ViewportManager::GetViewportByIndex(i);
            if (!viewport->IsVisible()) continue;

            OpenGLRenderer::SetViewport(gBuffer, viewport);
            shader->SetInt("u_viewportIndex", i);
            shader->SetMat4("u_projectionView", viewportData[i].projectionView);

            for (LightVolume& lightVolume : GlobalIllumination::GetLightVolumes()) {

                shader->SetFloat("u_spacing", GlobalIllumination::GetProbeSpacing());
                shader->SetVec3("u_offset", lightVolume.m_offset);
                shader->SetInt("u_textureWidth", lightVolume.m_textureWidth);
                shader->SetInt("u_textureHeight", lightVolume.m_textureHeight);
                shader->SetInt("u_textureDepth", lightVolume.m_textureDepth);
                shader->SetInt("u_worldSpaceWidth", lightVolume.m_worldSpaceWidth);
                shader->SetInt("u_worldSpaceHeight", lightVolume.m_worldSpaceHeight);
                shader->SetInt("u_worldSpaceDepth", lightVolume.m_worldSpaceDepth);
                shader->SetInt("u_showMask", showMask);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_3D, lightVolume.GetLightingTextureHandle());
                glBindImageTexture(1, lightVolume.GetMaskTextureHandle(), 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);

                int instanceCount = lightVolume.m_textureWidth * lightVolume.m_textureHeight * lightVolume.m_textureDepth;

                glDrawElementsInstancedBaseVertex(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * mesh->baseIndex), instanceCount, mesh->baseVertex);

            }
        }
    }

    void ComputeLightVolumeMask() {
        OpenGLShader* shader = GetShader("LightVolumeMask");
        OpenGLFrameBuffer* gBuffer = GetFrameBuffer("GBuffer");

        for (LightVolume& lightVolume : GlobalIllumination::GetLightVolumes()) {
            // Zero out the texture
            GLuint zero = 0;
            glClearTexImage(lightVolume.GetMaskTextureHandle(), 0, GL_RED_INTEGER, GL_UNSIGNED_INT, &zero);

            glBindImageTexture(0, lightVolume.GetMaskTextureHandle(), 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentHandleByName("WorldPosition"));
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gBuffer->GetColorAttachmentHandleByName("Normal"));

            shader->Bind();
            shader->SetVec3("u_lightVolumeOffset", lightVolume.m_offset);
            shader->SetFloat("u_lightVolumeSpacing", GlobalIllumination::GetProbeSpacing());

            int halfW = (gBuffer->GetWidth() + 1) / 2;
            int halfH = (gBuffer->GetHeight() + 1) / 2;
            int groupsX = (halfW + 8 - 1) / 8;
            int groupsY = (halfH + 8 - 1) / 8;
            glDispatchCompute(groupsX, groupsY, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
        }
    }

    void ComputeProbeLighting() {

        for (LightVolume& lightVolume : GlobalIllumination::GetLightVolumes()) {

            static int frameIndex = -1;

            if (Input::KeyPressed(HELL_KEY_T)) {
                // Enable GI
                if (frameIndex == -1) {
                    frameIndex = 0;
                }
                // Disable GI
                else {
                    frameIndex = -1;
                    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                    glClearTexImage(lightVolume.m_lightVolumeA, 0, GL_RGBA, GL_FLOAT, &clearColor);
                    glClearTexImage(lightVolume.m_lightVolumeB, 0, GL_RGBA, GL_FLOAT, &clearColor);
                }
            }

            if (frameIndex != -1) {

                // Lighting pass
                OpenGLShader* shader = GetShader("LightVolumeLighting");
                shader->Bind();
                shader->SetInt("u_width", lightVolume.m_textureWidth);
                shader->SetInt("u_height", lightVolume.m_textureHeight);
                shader->SetInt("u_depth", lightVolume.m_textureDepth);
                shader->SetFloat("u_spacing", GlobalIllumination::GetProbeSpacing());
                shader->SetVec3("u_offset", lightVolume.m_offset);
                shader->SetFloat("u_bounceRange", 5.0f);
                shader->SetInt("u_frameIndex", frameIndex);

                // Set point grid uniforms
                shader->SetUVec3("u_pointGridDimensions", GlobalIllumination::GetPointCloudGridDimensions());
                shader->SetVec3("u_pointGridWorldMin", GlobalIllumination::GetPointGridWorldMin());
                shader->SetVec3("u_pointGridCellSize", GlobalIllumination::GetPointGridWorldMax());

                glBindImageTexture(0, lightVolume.m_lightVolumeTextures[lightVolume.m_pingPongReadIndex], 0, GL_TRUE, 0, GL_READ_ONLY, GL_RGBA16F);
                glBindImageTexture(1, lightVolume.m_lightVolumeTextures[lightVolume.m_pingPongWriteIndex], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_RGBA16F);
                glBindImageTexture(2, lightVolume.m_lightVolumeMaskTexture, 0, GL_TRUE, 0, GL_READ_ONLY, GL_R32UI);

                glDispatchCompute((lightVolume.m_textureWidth + 8 - 1) / 8, (lightVolume.m_textureHeight + 8 - 1) / 8, (lightVolume.m_textureDepth + 8 - 1) / 8);

                glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

                // Ping Pong Swap!
                std::swap(lightVolume.m_pingPongReadIndex, lightVolume.m_pingPongWriteIndex);

                frameIndex++;
                if (frameIndex == 4) {
                    frameIndex = 0;
                }
            }
        }
    }
}
