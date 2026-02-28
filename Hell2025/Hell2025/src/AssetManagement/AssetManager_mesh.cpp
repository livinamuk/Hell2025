#include "AssetManager.h"
#include "Bvh/Cpu/CpuBvh.h"
#include "Timer.hpp"
#include <mutex>

namespace AssetManager {

    int g_nextVertexInsert = 0;
    int g_nextIndexInsert = 0;

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3 aabbMin, glm::vec3 aabbMax, int parentIndex, glm::mat4 localTransform, glm::mat4 inverseBindTransform) {
        std::vector<Mesh>& meshes = GetMeshes();
        std::vector<Vertex>& allVertices = GetVertices();
        std::vector<uint32_t>& allIndices = GetIndies();

        Mesh& mesh = meshes.emplace_back();
        mesh.baseVertex = g_nextVertexInsert;
        mesh.baseIndex = g_nextIndexInsert;
        mesh.vertexCount = (uint32_t)vertices.size();
        mesh.indexCount = (uint32_t)indices.size();
        mesh.SetName(name);
        mesh.aabbMin = aabbMin;
        mesh.aabbMax = aabbMax;
        mesh.extents = aabbMax - aabbMin;
        mesh.boundingSphereRadius = std::max(mesh.extents.x, std::max(mesh.extents.y, mesh.extents.z)) * 0.5f;
        mesh.parentIndex = parentIndex;
        mesh.localTransform = localTransform;
        mesh.inverseBindTransform = inverseBindTransform;

        allVertices.reserve(allVertices.size() + vertices.size());
        allVertices.insert(std::end(allVertices), std::begin(vertices), std::end(vertices));
        allIndices.reserve(allIndices.size() + indices.size());
        allIndices.insert(std::end(allIndices), std::begin(indices), std::end(indices));

        g_nextVertexInsert += mesh.vertexCount;
        g_nextIndexInsert += mesh.indexCount;

        return meshes.size() - 1;
    }

    int CreateMesh(const std::string& name, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices) {
        if (vertices.empty()) {
            std::cout << "AssetManager::CreateMesh(\"" << name << "\") failed: vertices was empty\n";
            return CreateMesh(name, vertices, indices, glm::vec3(0.0f), glm::vec3(0.0f), -1, glm::mat4(1.0f), glm::mat4(1.0f));
        }

        // Initialize AABB min and max with first vertex
        glm::vec3 aabbMin = vertices[0].position;
        glm::vec3 aabbMax = vertices[0].position;

        // Calculate AABB by iterating over all vertices
        for (const Vertex& v : vertices) {
            aabbMin = glm::min(aabbMin, v.position);
            aabbMax = glm::max(aabbMax, v.position);
        }

        return CreateMesh(name, vertices, indices, aabbMin, aabbMax, -1, glm::mat4(1.0f), glm::mat4(1.0f));
    }

    int GetMeshIndexByName(const std::string& name) {
        std::vector<Mesh>& meshes = GetMeshes();
        for (int i = 0; i < meshes.size(); i++) {
            if (meshes[i].GetName() == name)
                return i;
        }
        std::cout << "AssetManager::GetMeshIndexByName() failed because '" << name << "' does not exist\n";
        return -1;
    }

    Mesh* GetMeshByName(const std::string& name) {
        std::vector<Mesh>& meshes = GetMeshes();
        for (int i = 0; i < meshes.size(); i++) {
            if (meshes[i].GetName() == name)
                return &meshes[i];
        }
        std::cout << "AssetManager::GetMeshByName() failed because '" << name << "' does not exist\n";
        return nullptr;
    }

    Mesh* GetMeshByIndex(int index) {
        std::vector<Mesh>& meshes = GetMeshes();
        if (index >= 0 && index < meshes.size()) {
            return &meshes[index];
        }
        else {
            std::cout << "AssetManager::GetMeshByIndex() failed because index '" << index << "' is out of range. Size is " << meshes.size() << "!\n";
            return nullptr;
        }
    }

    const std::string& GetMeshNameByMeshIndex(int index) {
        std::vector<Mesh>& meshes = GetMeshes();
        if (index >= 0 && index < meshes.size()) {
            return meshes[index].GetName();
        }
        else {
            const static std::string notFound = "NOT_FOUND";
            return notFound;
        }
    }

    std::vector<Vertex> GetMeshVertices(Mesh* mesh) {
        if (!mesh) {
            std::cout << "AssetManager::GetMeshVertices() failed: mesh was nullptr\n";
            return std::vector<Vertex>();
        }

        std::vector<Vertex>& vertices = AssetManager::GetVertices();
        std::vector<uint32_t>& indices = AssetManager::GetIndices();

        std::vector<Vertex> result;
        result.reserve(mesh->vertexCount);

        for (uint32_t i = mesh->baseIndex; i < mesh->baseIndex + mesh->indexCount; i++) {
            uint32_t index = indices[i];
            const Vertex& vertex = vertices[index + mesh->baseVertex];
            result.push_back(vertex);
        }

        return result;
    }

    std::span<Vertex> GetMeshVerticesSpan(Mesh* mesh) {
        static const std::span<Vertex> empty;
        auto& vertices = GetVertices();

        if (!mesh || mesh->baseVertex < 0 || mesh->vertexCount < 0) {
            return empty;
        }
        size_t base = static_cast<size_t>(mesh->baseVertex);
        size_t count = static_cast<size_t>(mesh->vertexCount);
        
        if (base > vertices.size() || count > vertices.size() - base) {
            return empty;
        }
        return { vertices.data() + base, count };
    }

    std::span<uint32_t> GetMeshIndicesSpan(Mesh* mesh) {
        static const std::span<uint32_t> empty;
        auto& indices = GetIndices();

        if (!mesh || mesh->baseIndex < 0 || mesh->indexCount < 0) {
            return empty;
        }        
        size_t base = static_cast<size_t>(mesh->baseIndex);
        size_t count = static_cast<size_t>(mesh->indexCount);
        
        if (base > indices.size() || count > indices.size() - base) {
            return empty;
        }
        return { indices.data() + base, count };
    }


    void CreateMeshBvhs() {
        Timer timer ("CreateMeshBVHs()");
       
        for (Mesh& mesh : GetMeshes()) {
            Timer timer("CreateMeshBVHs() " + mesh.GetName() + " " + std::to_string(mesh.indexCount));
            
            std::vector<Vertex> vertices = GetMeshVertices(&mesh);
        
            std::vector<uint32_t> indices(vertices.size());
            for (uint32_t i = 0; i < indices.size(); i++) {
                indices[i] = i;
            }
        
            std::span<Vertex> verticesSpan = GetMeshVerticesSpan(&mesh);
            std::span<uint32_t> indicesSpan = GetMeshIndicesSpan(&mesh);

            vertices.clear();
            indices.clear();
           
            vertices.reserve(verticesSpan.size());
            indices.reserve(indicesSpan.size());
           
            for (Vertex& vertex : verticesSpan) {
                vertices.push_back(vertex);
            }
           
            for (uint32_t& index : indicesSpan) {
                indices.push_back(index);
            }

            Bvh::Cpu::DestroyMeshBvh(mesh.meshBvhId);
            mesh.meshBvhId = Bvh::Cpu::CreateMeshBvhFromVertexData(vertices, indices);
        }
        Bvh::Cpu::FlatternMeshBvhNodes();
    }
}
