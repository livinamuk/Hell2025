#include "MeshBufferOLD.h"

namespace Unloved {

void MeshBufferOLD::Reset() {
    m_meshes.clear();
    m_vertices.clear();
    m_indices.clear();

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        m_opengMeshBuffer.ReleaseBuffers();
    }
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

uint32_t MeshBufferOLD::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name) {
    // Compute AABB
    glm::vec3 aabbMin = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 aabbMax = glm::vec3(std::numeric_limits<float>::lowest());
    for (const Vertex& v : vertices) {
        aabbMin = glm::min(aabbMin, v.position);
        aabbMax = glm::max(aabbMax, v.position);
    }

    // Create mesh
    Mesh& mesh = m_meshes.emplace_back();
    mesh.baseVertex = m_vertices.size();
    mesh.baseIndex = m_indices.size();
    mesh.vertexCount = vertices.size();
    mesh.indexCount = indices.size();
    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.extents = aabbMax - aabbMin;
    mesh.boundingSphereRadius = std::max(mesh.extents.x, std::max(mesh.extents.y, mesh.extents.z)) * 0.5f;
    mesh.name = name;
    mesh.meshBvhId = 0;

    // Reserve memory
    m_vertices.reserve(m_vertices.size() + vertices.size());
    m_indices.reserve(m_indices.size() + indices.size());

    // Append vertex data
    m_vertices.insert(std::end(m_vertices), std::begin(vertices), std::end(vertices));
    m_indices.insert(std::end(m_indices), std::begin(indices), std::end(indices));

    // Return index to this mesh
    return m_meshes.size() - 1;
}

Mesh* MeshBufferOLD::GetMeshByIndex(uint32_t meshIndex) {
    if (meshIndex >= m_meshes.size()) return nullptr;
    return &m_meshes[meshIndex];
}

void MeshBufferOLD::UpdateBuffers() {
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        m_opengMeshBuffer.UpdateBuffers(m_vertices, m_indices);
    }
    else if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        // TODO
    }
}

void MeshBufferOLD::SetName(const std::string& name) {
    m_name = name;
}
}
