#include "MeshBuffer.h"

#include "Hell/Common/String.h"
#include "Hell/Render/API/OpenGL/GL_resource_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_raytracing_manager.h"
#include "Hell/Render/API/Vulkan/Managers/vk_resource_manager.h"
#include "Hell/Backend/BackEnd.h"
#include "Hell/Logging.h"

#include <algorithm>
#include <limits>
#include <utility>

namespace Hell {

MeshBuffer::MeshBuffer(const std::string& name) {
    m_name = name;
}

void MeshBuffer::Initialize() {
    Reset();

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        if (m_openGLId == 0) {
            m_openGLId = OpenGL::ResourceManager::CreateMeshBuffer(m_name);
        }

        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
        meshBuffer.Init(Vertex::GetLayout());
    }
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        if (m_vulkanId == 0 || !VulkanResourceManager::MeshBufferExists(m_vulkanId)) {
            m_vulkanId = VulkanResourceManager::CreateMeshBuffer(m_name);
        }

        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
        if (meshBuffer) {
            meshBuffer->Init(Vertex::GetLayout());
        }
    }

    m_initialized = true;
}

void MeshBuffer::Reset() {
    DestroyAllVulkanBlas();
    m_version++;

    m_meshes.clear();
    m_meshIdsByName.clear();
    m_skinnedMeshMetadata.clear();
    m_vertices.clear();
    m_indices.clear();
    m_vertexWeights.clear();
    m_morphDeltas.clear();

    m_freeVertexMemoryBlocks.clear();
    m_freeIndexMemoryBlocks.clear();

    m_nextMeshId = 0;
    m_vertexCapacity = 0;
    m_indexCapacity = 0;
    m_vertexWeightCapacity = 0;
    m_morphDeltaCapacity = 0;

    if (Hell::BackEnd::GetAPI() == API::OPENGL && m_openGLId != 0) {
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
        meshBuffer.Reset();
    }
    if (Hell::BackEnd::GetAPI() == API::VULKAN && m_vulkanId != 0) {
        if (VulkanResourceManager::MeshBufferExists(m_vulkanId)) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            meshBuffer->Reset();
        }
        else {
            m_vulkanId = 0;
        }
    }

    m_initialized = false;
}

void MeshBuffer::CleanUp() {
    Reset();

    if (m_openGLId != 0) {
        OpenGL::ResourceManager::RemoveMeshBuffer(m_openGLId);
        m_openGLId = 0;
    }
    if (m_vulkanId != 0) {
        if (VulkanResourceManager::MeshBufferExists(m_vulkanId)) {
            VulkanResourceManager::RemoveMeshBuffer(m_vulkanId);
        }
        m_vulkanId = 0;
    }
}

uint32_t MeshBuffer::AddMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name) {
    if (!m_initialized) Initialize();

    if (vertices.empty() || indices.empty()) {
        Logging::Error() << "MeshBuffer::AddMesh(..) failed: vertex count'" << vertices.size() << "' index count '" << indices.size() << "'\n";
        return 0;
    }

    if (m_nextMeshId == std::numeric_limits<uint32_t>::max()) {
        Logging::Error() << "MeshBuffer::AddMesh(..) failed: mesh ID space exhausted for '" << m_name << "'\n";
        return 0;
    }

    m_nextMeshId++;

    Mesh& mesh = m_meshes[m_nextMeshId];
    mesh.baseVertex = AddVertices(vertices);
    mesh.baseIndex = AddIndices(indices);
    mesh.vertexCount = static_cast<uint32_t>(vertices.size());
    mesh.indexCount = static_cast<uint32_t>(indices.size());
    mesh.name = name;
    m_meshIdsByName.emplace(name, m_nextMeshId);

    // Compute axis aligned bounding box limits
    glm::vec3 aabbMin(std::numeric_limits<float>::max());
    glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

    for (const Vertex& vertex : vertices) {
        aabbMin = glm::min(aabbMin, vertex.position);
        aabbMax = glm::max(aabbMax, vertex.position);
    }

    mesh.aabbMin = aabbMin;
    mesh.aabbMax = aabbMax;
    mesh.extents = aabbMax - aabbMin;
    mesh.boundingSphereRadius = std::max(mesh.extents.x, std::max(mesh.extents.y, mesh.extents.z)) * 0.5f;
    if (m_createVulkanBlasForNewMeshes) {
        CreateVulkanBlas(mesh);
    }

    m_version++;

    return m_nextMeshId;
}

uint32_t MeshBuffer::AddSkinnedMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::vector<VertexWeight>& vertexWeights, const std::vector<MorphTargetData>& morphTargets, SkinnedMeshMetadata metadata, const std::string& name) {
    if (metadata.requiresSkinning && vertexWeights.size() != vertices.size()) {
        Logging::Error() << "MeshBuffer::AddSkinnedMesh(..) failed for '" << name << "': skinned meshes need one vertex weight per vertex\n";
        return 0;
    }

    uint32_t meshId = AddMesh(vertices, indices, name);
    if (meshId == 0) {
        return 0;
    }

    if (metadata.requiresSkinning) {
        metadata.baseVertexWeight = AddVertexWeights(vertexWeights);
    }
    else {
        metadata.baseVertexWeight = -1;
    }

    metadata.morphTargets.reserve(morphTargets.size());
    for (const MorphTargetData& morphTarget : morphTargets) {
        MeshMorphTargetMetadata& morphMetadata = metadata.morphTargets.emplace_back();
        morphMetadata.name = morphTarget.name;
        morphMetadata.positionDeltaCount = static_cast<uint32_t>(morphTarget.positionDeltas.size());
        morphMetadata.normalDeltaCount = static_cast<uint32_t>(morphTarget.normalDeltas.size());
        morphMetadata.tangentDeltaCount = static_cast<uint32_t>(morphTarget.tangentDeltas.size());

        const int32_t positionDeltaOffset = AddMorphDeltas(morphTarget.positionDeltas);
        const int32_t normalDeltaOffset = AddMorphDeltas(morphTarget.normalDeltas);
        const int32_t tangentDeltaOffset = AddMorphDeltas(morphTarget.tangentDeltas);
        if (positionDeltaOffset >= 0) morphMetadata.positionDeltaOffset = static_cast<uint32_t>(positionDeltaOffset);
        if (normalDeltaOffset >= 0) morphMetadata.normalDeltaOffset = static_cast<uint32_t>(normalDeltaOffset);
        if (tangentDeltaOffset >= 0) morphMetadata.tangentDeltaOffset = static_cast<uint32_t>(tangentDeltaOffset);
    }

    m_skinnedMeshMetadata[meshId] = metadata;
    return meshId;
}

int32_t MeshBuffer::AddVertices(const std::vector<Vertex>& newVertices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free vertex block
    for (int32_t i = 0; i < static_cast<int32_t>(m_freeVertexMemoryBlocks.size()); i++) {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[i];

        if (newVertices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_vertices.size());
        size_t requiredCount = m_vertices.size() + newVertices.size();

        // Grow capacity
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_vertices.size() * m_growthMultiplier));

        if (newCount > m_vertexCapacity) {
            m_vertexCapacity = CalculateNewCapacity(newCount, m_vertexCapacity);

            if (Hell::BackEnd::GetAPI() == API::OPENGL) {
                OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
                meshBuffer.ResizeVertexBuffer(m_vertexCapacity, m_vertices);
            }
            if (Hell::BackEnd::GetAPI() == API::VULKAN) {
                VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
                if (meshBuffer) {
                    meshBuffer->ResizeVertexBuffer(m_vertexCapacity, m_vertices);
                }
            }
        }

        m_vertices.resize(newCount);
        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.InsertVertices(newVertices, insertOffset);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->InsertVertices(newVertices, insertOffset);
            }
        }

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeVertexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeVertexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = static_cast<int32_t>(memoryBlock.begin);

        std::copy(newVertices.begin(), newVertices.end(), m_vertices.begin() + insertOffset);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.InsertVertices(newVertices, insertOffset);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->InsertVertices(newVertices, insertOffset);
            }
        }

        // Handle block resizing
        if (memoryBlock.GetSize() == newVertices.size()) {
            m_freeVertexMemoryBlocks.erase(m_freeVertexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newVertices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AddIndices(const std::vector<uint32_t>& newIndices) {
    int32_t freeMemoryBlockIndex = -1;
    int32_t insertOffset = 0;

    // Search for free index block
    for (int32_t i = 0; i < static_cast<int32_t>(m_freeIndexMemoryBlocks.size()); i++) {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[i];

        if (newIndices.size() <= memoryBlock.GetSize()) {
            freeMemoryBlockIndex = i;
            break;
        }
    }

    // Allocate memory and insert data
    if (freeMemoryBlockIndex == -1) {
        insertOffset = static_cast<uint32_t>(m_indices.size());
        size_t requiredCount = m_indices.size() + newIndices.size();

        // Grow capacity
        size_t newCount = std::max(requiredCount, static_cast<size_t>(m_indices.size() * m_growthMultiplier));

        if (newCount > m_indexCapacity) {
            m_indexCapacity = CalculateNewCapacity(newCount, m_indexCapacity);

            if (Hell::BackEnd::GetAPI() == API::OPENGL) {
                OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
                meshBuffer.ResizeIndexBuffer(m_indexCapacity, m_indices);
            }
            if (Hell::BackEnd::GetAPI() == API::VULKAN) {
                VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
                if (meshBuffer) {
                    meshBuffer->ResizeIndexBuffer(m_indexCapacity, m_indices);
                }
            }
        }

        m_indices.resize(newCount);
        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.InsertIndices(newIndices, insertOffset);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->InsertIndices(newIndices, insertOffset);
            }
        }

        // Register newly allocated excess space as a free block
        if (newCount > requiredCount) {
            MemoryBlock extraBlock;
            extraBlock.begin = static_cast<uint32_t>(requiredCount);
            extraBlock.end = static_cast<uint32_t>(newCount);
            m_freeIndexMemoryBlocks.push_back(extraBlock);
        }
    }

    // Insert into free memory
    else {
        MemoryBlock& memoryBlock = m_freeIndexMemoryBlocks[freeMemoryBlockIndex];
        insertOffset = static_cast<int32_t>(memoryBlock.begin);

        std::copy(newIndices.begin(), newIndices.end(), m_indices.begin() + insertOffset);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.InsertIndices(newIndices, insertOffset);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->InsertIndices(newIndices, insertOffset);
            }
        }

        // Handle block resizing
        if (memoryBlock.GetSize() == newIndices.size()) {
            m_freeIndexMemoryBlocks.erase(m_freeIndexMemoryBlocks.begin() + freeMemoryBlockIndex);
        }
        else {
            memoryBlock.begin += newIndices.size();
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AddVertexWeights(const std::vector<VertexWeight>& newVertexWeights) {
    if (newVertexWeights.empty()) {
        return -1;
    }

    int32_t insertOffset = static_cast<int32_t>(m_vertexWeights.size());
    size_t requiredCount = m_vertexWeights.size() + newVertexWeights.size();

    if (requiredCount > m_vertexWeightCapacity) {
        m_vertexWeightCapacity = CalculateNewCapacity(requiredCount, m_vertexWeightCapacity);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.ResizeVertexWeightBuffer(m_vertexWeightCapacity, m_vertexWeights);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->ResizeVertexWeightBuffer(m_vertexWeightCapacity, m_vertexWeights);
            }
        }
    }

    m_vertexWeights.insert(m_vertexWeights.end(), newVertexWeights.begin(), newVertexWeights.end());

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
        meshBuffer.InsertVertexWeights(newVertexWeights, insertOffset);
    }
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
        if (meshBuffer) {
            meshBuffer->InsertVertexWeights(newVertexWeights, insertOffset);
        }
    }

    return insertOffset;
}

int32_t MeshBuffer::AddMorphDeltas(const std::vector<MorphTargetVertexDelta>& newMorphDeltas) {
    if (newMorphDeltas.empty()) return -1;

    const int32_t insertOffset = static_cast<int32_t>(m_morphDeltas.size());
    const size_t requiredCount = m_morphDeltas.size() + newMorphDeltas.size();

    if (requiredCount > m_morphDeltaCapacity) {
        m_morphDeltaCapacity = CalculateNewCapacity(requiredCount, m_morphDeltaCapacity);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.ResizeMorphDeltaBuffer(m_morphDeltaCapacity, m_morphDeltas);
        }
    }

    m_morphDeltas.insert(m_morphDeltas.end(), newMorphDeltas.begin(), newMorphDeltas.end());

    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
        meshBuffer.InsertMorphDeltas(newMorphDeltas, insertOffset);
    }

    return insertOffset;
}

int32_t MeshBuffer::AllocateExtraVertexSpace(size_t vertexCount) {
    size_t blockBegin = m_vertices.size();
    size_t blockEnd = m_vertices.size() + vertexCount;

    if (blockEnd > m_vertexCapacity) {
        m_vertexCapacity = CalculateNewCapacity(blockEnd, m_vertexCapacity);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.ResizeVertexBuffer(m_vertexCapacity, m_vertices);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->ResizeVertexBuffer(m_vertexCapacity, m_vertices);
            }
        }
    }

    MemoryBlock& block = m_freeVertexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_vertices.resize(blockEnd);

    return static_cast<int32_t>(m_freeVertexMemoryBlocks.size() - 1);
}

int32_t MeshBuffer::AllocateExtraIndexSpace(size_t indexCount) {
    size_t blockBegin = m_indices.size();
    size_t blockEnd = m_indices.size() + indexCount;

    if (blockEnd > m_indexCapacity) {
        m_indexCapacity = CalculateNewCapacity(blockEnd, m_indexCapacity);

        if (Hell::BackEnd::GetAPI() == API::OPENGL) {
            OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
            meshBuffer.ResizeIndexBuffer(m_indexCapacity, m_indices);
        }
        if (Hell::BackEnd::GetAPI() == API::VULKAN) {
            VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
            if (meshBuffer) {
                meshBuffer->ResizeIndexBuffer(m_indexCapacity, m_indices);
            }
        }
    }

    MemoryBlock& block = m_freeIndexMemoryBlocks.emplace_back();
    block.begin = blockBegin;
    block.end = blockEnd;

    m_indices.resize(blockEnd);

    return static_cast<int32_t>(m_freeIndexMemoryBlocks.size() - 1);
}

void MeshBuffer::RemoveMesh(uint32_t meshId) {
    auto it = m_meshes.find(meshId);
    if (it == m_meshes.end()) return;

    Mesh& mesh = it->second;
    DestroyVulkanBlas(mesh);

    // Create free vertex memory block
    MemoryBlock vertexBlock;
    vertexBlock.begin = mesh.baseVertex;
    vertexBlock.end = mesh.baseVertex + mesh.vertexCount;
    m_freeVertexMemoryBlocks.push_back(vertexBlock);

    // Sort vertex blocks by starting offset
    std::sort(m_freeVertexMemoryBlocks.begin(), m_freeVertexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
    });

    // Merge adjacent free vertex memory blocks
    std::vector<MemoryBlock> mergedVertexBlocks;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        if (mergedVertexBlocks.empty()) {
            mergedVertexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedVertexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedVertexBlocks.push_back(block);
            }
        }
    }
    m_freeVertexMemoryBlocks = std::move(mergedVertexBlocks);

    // Create free index memory block
    MemoryBlock indexBlock;
    indexBlock.begin = mesh.baseIndex;
    indexBlock.end = mesh.baseIndex + mesh.indexCount;
    m_freeIndexMemoryBlocks.push_back(indexBlock);

    // Sort index blocks by starting offset
    std::sort(m_freeIndexMemoryBlocks.begin(), m_freeIndexMemoryBlocks.end(), [](const MemoryBlock& a, const MemoryBlock& b) {
        return a.begin < b.begin;
    });

    // Merge adjacent free index memory blocks
    std::vector<MemoryBlock> mergedIndexBlocks;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        if (mergedIndexBlocks.empty()) {
            mergedIndexBlocks.push_back(block);
        }
        else {
            MemoryBlock& last = mergedIndexBlocks.back();
            if (last.end >= block.begin) {
                last.end = std::max(last.end, block.end);
            }
            else {
                mergedIndexBlocks.push_back(block);
            }
        }
    }
    m_freeIndexMemoryBlocks = std::move(mergedIndexBlocks);

    const std::string removedMeshName = mesh.name;
    auto nameIt = m_meshIdsByName.find(removedMeshName);
    if (nameIt != m_meshIdsByName.end() && nameIt->second == meshId) {
        m_meshIdsByName.erase(nameIt);

        for (const auto& [otherMeshId, otherMesh] : m_meshes) {
            if (otherMeshId != meshId && otherMesh.name == removedMeshName) {
                m_meshIdsByName[removedMeshName] = otherMeshId;
                break;
            }
        }
    }

    // Remove the mesh
    m_skinnedMeshMetadata.erase(meshId);
    m_meshes.erase(it);
    m_version++;
}

void MeshBuffer::CreateVulkanBlas(Mesh& mesh) {
    if (Hell::BackEnd::GetAPI() != API::VULKAN) return;
    if (mesh.vertexCount == 0 || mesh.indexCount < 3) return;
    if (m_vulkanId == 0 || !VulkanResourceManager::MeshBufferExists(m_vulkanId)) return;

    if (mesh.vulkanBlasId == 0 || !VulkanResourceManager::AccelerationStructureExists(mesh.vulkanBlasId)) {
        mesh.vulkanBlasId = VulkanResourceManager::CreateAccelerationStructure();
    }

    VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
    if (!meshBuffer || !VulkanRaytracingManager::BuildBottomLevelAS(mesh.vulkanBlasId, *meshBuffer, mesh)) {
        DestroyVulkanBlas(mesh);
    }
}

void MeshBuffer::DestroyVulkanBlas(Mesh& mesh) {
    if (mesh.vulkanBlasId == 0) return;

    if (VulkanResourceManager::AccelerationStructureExists(mesh.vulkanBlasId)) {
        VulkanResourceManager::RemoveAccelerationStructure(mesh.vulkanBlasId);
    }
    mesh.vulkanBlasId = 0;
}

void MeshBuffer::DestroyAllVulkanBlas() {
    for (auto& entry : m_meshes) {
        DestroyVulkanBlas(entry.second);
    }
}

void MeshBuffer::PreAllocate(size_t maxVertices, size_t maxIndices, size_t maxVertexWeights, size_t maxMorphDeltas) {
    if (maxVertices == 0 || maxIndices == 0) {
        Logging::Warning() << "MeshBuffer::PreAllocate() called with zero " << maxVertices << " vertices and " << maxIndices << " indices\n";
        return;
    }

    Initialize();

    m_vertices.resize(maxVertices);
    m_indices.resize(maxIndices);
    m_vertexWeights.reserve(maxVertexWeights);
    m_morphDeltas.reserve(maxMorphDeltas);
    m_vertexCapacity = maxVertices;
    m_indexCapacity = maxIndices;
    m_vertexWeightCapacity = maxVertexWeights;
    m_morphDeltaCapacity = maxMorphDeltas;

    // Allocate new GPU memory
    if (Hell::BackEnd::GetAPI() == API::OPENGL) {
        OpenGLMeshBuffer& meshBuffer = OpenGL::ResourceManager::GetMeshBuffer(m_openGLId);
        meshBuffer.PreAllocate(m_vertexCapacity, m_indexCapacity, m_vertexWeightCapacity, m_morphDeltaCapacity);
    }
    if (Hell::BackEnd::GetAPI() == API::VULKAN) {
        VulkanMeshBuffer* meshBuffer = VulkanResourceManager::GetMeshBuffer(m_vulkanId);
        if (meshBuffer) {
            meshBuffer->PreAllocate(m_vertexCapacity, m_indexCapacity, m_vertexWeightCapacity);
        }
    }

    // Add one continuous free vertex memory block
    MemoryBlock initialVertexBlock;
    initialVertexBlock.begin = 0;
    initialVertexBlock.end = static_cast<uint32_t>(maxVertices);
    m_freeVertexMemoryBlocks.push_back(initialVertexBlock);

    // Add one continuous free index memory block
    MemoryBlock initialIndexBlock;
    initialIndexBlock.begin = 0;
    initialIndexBlock.end = static_cast<uint32_t>(maxIndices);
    m_freeIndexMemoryBlocks.push_back(initialIndexBlock);

    m_initialized = true;
}

size_t MeshBuffer::CalculateNewCapacity(size_t requiredCount, size_t currentCapacity) {
    if (currentCapacity == 0) {
        return std::max(requiredCount, m_minCapacity);
    }

    return std::max(requiredCount, static_cast<size_t>(currentCapacity * m_growthMultiplier));
}

Mesh* MeshBuffer::GetMeshById(uint32_t meshId) {
    auto it = m_meshes.find(meshId);
    if (it != m_meshes.end()) {
        return &it->second;
    }

    return nullptr;
}

Mesh* MeshBuffer::GetMeshByName(const std::string& name) {
    const uint32_t meshId = GetMeshIdByName(name);
    if (meshId == 0) {
        return nullptr;
    }

    return GetMeshById(meshId);
}

uint32_t MeshBuffer::GetMeshIdByName(const std::string& name) {
    auto it = m_meshIdsByName.find(name);
    if (it != m_meshIdsByName.end()) {
        return it->second;
    }

    Logging::Error() << "MeshBuffer::GetMeshIdByName(..) failed for '" << m_name << "' because mesh '" << name << "' does not exist\n";
    return 0;
}

const std::string& MeshBuffer::GetMeshNameByMeshId(uint32_t meshId) {
    if (Mesh* mesh = GetMeshById(meshId)) {
        return mesh->name;
    }

    const static std::string notFound = "NOT_FOUND";
    return notFound;
}

uint32_t MeshBuffer::GetBaseVertexByMeshId(uint32_t meshId) {
    if (Mesh* mesh = GetMeshById(meshId)) {
        return mesh->baseVertex;
    }

    return 0;
}

uint32_t MeshBuffer::GetBaseIndexByMeshId(uint32_t meshId) {
    if (Mesh* mesh = GetMeshById(meshId)) {
        return mesh->baseIndex;
    }

    return 0;
}

SkinnedMeshMetadata* MeshBuffer::GetSkinnedMeshMetadataByMeshId(uint32_t meshId) {
    auto it = m_skinnedMeshMetadata.find(meshId);
    if (it != m_skinnedMeshMetadata.end()) {
        return &it->second;
    }

    return nullptr;
}

bool MeshBuffer::HasSkinnedMeshMetadata(uint32_t meshId) const {
    return m_skinnedMeshMetadata.find(meshId) != m_skinnedMeshMetadata.end();
}

std::span<Vertex> MeshBuffer::GetMeshVertexSpan(uint32_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<Vertex>(m_vertices.data() + mesh->baseVertex, mesh->vertexCount);
}

std::span<uint32_t> MeshBuffer::GetMeshIndexSpan(uint32_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    if (!mesh) return {};

    return std::span<uint32_t>(m_indices.data() + mesh->baseIndex, mesh->indexCount);
}

std::span<VertexWeight> MeshBuffer::GetMeshVertexWeightSpan(uint32_t meshId) {
    Mesh* mesh = GetMeshById(meshId);
    SkinnedMeshMetadata* metadata = GetSkinnedMeshMetadataByMeshId(meshId);

    if (!mesh || !metadata || !metadata->requiresSkinning || metadata->baseVertexWeight < 0) {
        return {};
    }

    size_t base = static_cast<size_t>(metadata->baseVertexWeight);
    size_t count = static_cast<size_t>(mesh->vertexCount);

    if (base > m_vertexWeights.size() || count > m_vertexWeights.size() - base) {
        return {};
    }

    return std::span<VertexWeight>(m_vertexWeights.data() + base, count);
}

size_t MeshBuffer::GetCPUAllocatedByteCount() const {
    return (m_vertices.capacity() * sizeof(Vertex)) +
           (m_indices.capacity() * sizeof(uint32_t)) +
           (m_vertexWeights.capacity() * sizeof(VertexWeight)) +
           (m_morphDeltas.capacity() * sizeof(MorphTargetVertexDelta));
}

size_t MeshBuffer::GetGPUAllocatedByteCount() const {
    const size_t morphDeltaByteCount = Hell::BackEnd::GetAPI() == API::OPENGL
        ? m_morphDeltaCapacity * sizeof(MorphTargetVertexDelta)
        : 0;
    return (m_vertexCapacity * Vertex::GetLayout().stride) +
           (m_indexCapacity * sizeof(uint32_t)) +
           (m_vertexWeightCapacity * sizeof(VertexWeight)) +
           morphDeltaByteCount;
}

void MeshBuffer::PrintDebugInfo() {
    size_t usedVertexCount = 0;
    size_t usedIndexCount = 0;

    for (const auto& [meshId, mesh] : m_meshes) {
        usedVertexCount += mesh.vertexCount;
        usedIndexCount += mesh.indexCount;
    }

    size_t freeVertexCount = 0;
    size_t largestFreeVertexBlock = 0;

    for (const MemoryBlock& block : m_freeVertexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeVertexCount += blockSize;
        largestFreeVertexBlock = std::max(largestFreeVertexBlock, blockSize);
    }

    size_t freeIndexCount = 0;
    size_t largestFreeIndexBlock = 0;

    for (const MemoryBlock& block : m_freeIndexMemoryBlocks) {
        size_t blockSize = block.end - block.begin;
        freeIndexCount += blockSize;
        largestFreeIndexBlock = std::max(largestFreeIndexBlock, blockSize);
    }

    std::string message;
    message += "MeshBuffer Debug Info\n";
    message += "\n";

    message += "Meshes\n";
    message += "  Mesh count: " + std::to_string(GetMeshCount()) + "\n";
    message += "  Skinned mesh metadata count: " + std::to_string(GetSkinnedMeshMetadataCount()) + "\n";
    message += "  Used vertex count: " + std::to_string(usedVertexCount) + "\n";
    message += "  Used index count: " + std::to_string(usedIndexCount) + "\n";
    message += "  Used vertex weight count: " + std::to_string(m_vertexWeights.size()) + "\n";
    message += "  Used morph delta count: " + std::to_string(m_morphDeltas.size()) + "\n";
    message += "\n";

    message += "CPU storage\n";
    message += "  CPU allocated vertex count: " + std::to_string(GetAllocatedVertexCount()) + "\n";
    message += "  CPU allocated index count: " + std::to_string(GetAllocatedIndexCount()) + "\n";
    message += "  CPU allocated vertex weight count: " + std::to_string(GetAllocatedVertexWeightCount()) + "\n";
    message += "  CPU allocated morph delta count: " + std::to_string(GetAllocatedMorphDeltaCount()) + "\n";
    message += "  CPU free vertex count: " + std::to_string(freeVertexCount) + "\n";
    message += "  CPU free index count: " + std::to_string(freeIndexCount) + "\n";
    message += "\n";

    message += "GPU storage\n";
    message += "  GPU vertex capacity: " + std::to_string(m_vertexCapacity) + "\n";
    message += "  GPU index capacity: " + std::to_string(m_indexCapacity) + "\n";
    message += "  GPU vertex weight capacity: " + std::to_string(m_vertexWeightCapacity) + "\n";
    message += "  GPU morph delta capacity: " + std::to_string(m_morphDeltaCapacity) + "\n";
    message += "\n";

    message += "Fragmentation\n";
    message += "  Free vertex block count: " + std::to_string(m_freeVertexMemoryBlocks.size()) + "\n";
    message += "  Free index block count:  " + std::to_string(m_freeIndexMemoryBlocks.size()) + "\n";
    message += "  Largest free vertex block: " + std::to_string(largestFreeVertexBlock) + "\n";
    message += "  Largest free index block:  " + std::to_string(largestFreeIndexBlock) + "\n";
    message += "\n";

    message += "Free vertex blocks\n";
    for (size_t i = 0; i < m_freeVertexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeVertexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Free index blocks\n";
    for (size_t i = 0; i < m_freeIndexMemoryBlocks.size(); i++) {
        const MemoryBlock& block = m_freeIndexMemoryBlocks[i];
        message += "  [" + std::to_string(i) + "] begin: " + std::to_string(block.begin) + ", end: " + std::to_string(block.end) + ", size: " + std::to_string(block.end - block.begin) + "\n";
    }
    message += "\n";

    message += "Mesh list\n";
    for (const auto& [meshId, mesh] : m_meshes) {
        message += "  Mesh id: " + std::to_string(meshId) + "\n";
        message += "    Name: " + mesh.name + "\n";
        message += "    Base vertex: " + std::to_string(mesh.baseVertex) + "\n";
        message += "    Vertex count: " + std::to_string(mesh.vertexCount) + "\n";
        message += "    Base index: " + std::to_string(mesh.baseIndex) + "\n";
        message += "    Index count: " + std::to_string(mesh.indexCount) + "\n";
        message += "    AABB min: " + String::FormatVec3(mesh.aabbMin) + "\n";
        message += "    AABB max: " + String::FormatVec3(mesh.aabbMax) + "\n";
        message += "    Extents: " + String::FormatVec3(mesh.extents) + "\n";

        if (auto metadataIt = m_skinnedMeshMetadata.find(meshId); metadataIt != m_skinnedMeshMetadata.end()) {
            const SkinnedMeshMetadata& metadata = metadataIt->second;
            message += "    Skinned mesh metadata\n";
            message += "      Requires skinning: " + std::string(metadata.requiresSkinning ? "true" : "false") + "\n";
            message += "      Base vertex weight: " + std::to_string(metadata.baseVertexWeight) + "\n";
            message += "      Non deforming bone index: " + std::to_string(metadata.nonDeformingBoneIndex) + "\n";
            message += "      Morph target count: " + std::to_string(metadata.morphTargets.size()) + "\n";
        }

        message += "\n";
    }

    Logging::Debug() << message << "\n";
}

} // namespace
