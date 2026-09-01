#pragma once
#include "Hell/Render/API/OpenGL/Types/GL_mesh_buffer_old.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/ResourceManagement/Types/Mesh.h"

namespace Unloved {

struct MeshBufferOLD {
    uint32_t AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = "UNDEFINED");
    Mesh* GetMeshByIndex(uint32_t meshIndex); 
    void SetName(const std::string& name);
    void UpdateBuffers();
    void Reset();

    std::string& GetName()                  { return m_name; }
    OpenGLMeshBufferOLD& GetGLMeshBuffer()     { return m_opengMeshBuffer; }
    // VulkanDetachedMesh& GetVKMesh()         { return m_vulkanDetachedMesh; }
    std::vector<Vertex>& GetVertices()      { return m_vertices; }
    std::vector<uint32_t>& GetIndices()     { return m_indices; }
    int GetMeshCount()                      { return (int)m_meshes.size(); }

    glm::vec3 m_aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 m_aabbMax = glm::vec3(-std::numeric_limits<float>::max());

    std::string m_name;
    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;

private:
    std::vector<Mesh> m_meshes;
    OpenGLMeshBufferOLD m_opengMeshBuffer;
    // VulkanDetachedMesh m_vulkanDetachedMesh;
};
}
