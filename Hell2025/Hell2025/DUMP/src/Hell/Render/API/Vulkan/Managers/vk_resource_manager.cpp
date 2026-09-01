#include "vk_resource_manager.h"

#include "Hell/Render/API/Vulkan/Managers/vk_device_manager.h"

#include "Hell/Logging.h"
#include "Hell/Containers/SlotMap.h"
#include "Hell/MemoryTracker/MemoryTracker.h"
#include "Hell/ResourceManagement/ResourceID.h"

#include <algorithm>
#include <unordered_set>
#include <utility>

namespace VulkanResourceManager {
    std::unordered_map<std::string, AllocatedImage> g_allocatedImages;
    std::unordered_map<std::string, VulkanCubeMapArray> g_cubeMapArrays;
    std::unordered_map<std::string, VulkanCubemap> g_cubemaps;
    std::unordered_map<std::string, VulkanDescriptorSetResource> g_descriptorSets;
    std::unordered_map<std::string, VulkanPipeline> g_pipelines;
    std::unordered_map<std::string, VulkanRaytracingPipeline> g_raytracingPipelines;
    std::unordered_map<std::string, VulkanRenderState> g_renderStates;
    std::unordered_map<std::string, VulkanSampler> g_samplers;
    std::unordered_map<std::string, VulkanShader> g_shaders;

    Hell::SlotMap<VulkanAccelerationStructure> g_accelerationStructures;
    Hell::SlotMap<VulkanBuffer> g_buffers;
    Hell::SlotMap<VulkanGenericMesh> g_genericMeshes;
    Hell::SlotMap<VulkanMeshBuffer> g_meshBuffers;
    Hell::SlotMap<VulkanTexture> g_textures;

    std::unordered_map<std::string, uint64_t> g_bufferIdsByName;
    std::unordered_map<std::string, uint64_t> g_meshBufferIdsByName;

    namespace {
        void SortMemoryReportCategory(Hell::MemoryTracker::MemoryReportCategory& category) {
            std::sort(category.entries.begin(), category.entries.end(), [](const auto& a, const auto& b) {
                return a.name < b.name;
                });
        }

        void AppendCategory(Hell::MemoryTracker::MemoryReport& report, Hell::MemoryTracker::MemoryReportCategory&& category) {
            if (category.entries.empty()) {
                return;
            }

            SortMemoryReportCategory(category);
            report.categories.push_back(std::move(category));
        }

        std::string MakeResourceIdName(const char* label, uint64_t id) {
            return std::string(label) + " " + std::to_string(id);
        }
    }

    void Cleanup() {
        for (auto& object : g_accelerationStructures)  { object.Cleanup(); } g_accelerationStructures.clear();
        for (auto& object : g_genericMeshes)           { object.CleanUp(); } g_genericMeshes.clear();
        for (auto& object : g_meshBuffers)             { object.Cleanup(); } g_meshBuffers.clear(); g_meshBufferIdsByName.clear();
        for (auto& object : g_buffers)                 { object.Cleanup(); } g_buffers.clear(); g_bufferIdsByName.clear();
        for (auto& object : g_textures)                { object.Cleanup(); } g_textures.clear();

        for (auto& [name, object] : g_descriptorSets)  { object.Cleanup(); } g_descriptorSets.clear();
        for (auto& [name, object] : g_allocatedImages) { object.Cleanup(); } g_allocatedImages.clear();
        for (auto& [name, object] : g_cubeMapArrays)   { object.Cleanup(); } g_cubeMapArrays.clear();
        for (auto& [name, object] : g_cubemaps)        { object.Cleanup(); } g_cubemaps.clear();
        for (auto& [name, object] : g_samplers)        { object.Cleanup(); } g_samplers.clear();
        for (auto& [name, object] : g_renderStates)    { object.CleanUp(); } g_renderStates.clear();
        for (auto& [name, shader] : g_shaders)         { shader.Cleanup(); } g_shaders.clear();

        CleanUpPipelines();
    }

    void AppendMemoryReport(Hell::MemoryTracker::MemoryReport& report) {
        if (!g_allocatedImages.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Allocated Images";
            category.entries.reserve(g_allocatedImages.size());

            for (const auto& [name, image] : g_allocatedImages) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = image.GetCPUAllocatedByteCount();
                entry.gpuBytes = image.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_cubemaps.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Cubemaps";
            category.entries.reserve(g_cubemaps.size());

            for (const auto& [name, cubemap] : g_cubemaps) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = cubemap.GetCPUAllocatedByteCount();
                entry.gpuBytes = cubemap.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_cubeMapArrays.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Cube Map Arrays";
            category.entries.reserve(g_cubeMapArrays.size());

            for (const auto& [name, cubeMapArray] : g_cubeMapArrays) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = cubeMapArray.GetCPUAllocatedByteCount();
                entry.gpuBytes = cubeMapArray.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        std::unordered_set<uint64_t> genericMeshBufferIds;
        genericMeshBufferIds.reserve(g_genericMeshes.size() * 2);
        for (const VulkanGenericMesh& mesh : g_genericMeshes) {
            if (mesh.GetVertexBufferId() != 0) {
                genericMeshBufferIds.insert(mesh.GetVertexBufferId());
            }
            if (mesh.GetIndexBufferId() != 0) {
                genericMeshBufferIds.insert(mesh.GetIndexBufferId());
            }
        }

        if (!g_buffers.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Buffers";
            category.entries.reserve(g_buffers.size());

            for (size_t i = 0; i < g_buffers.size(); i++) {
                const uint64_t id = g_buffers.id_at(i);
                if (genericMeshBufferIds.contains(id)) {
                    continue;
                }

                const VulkanBuffer& buffer = g_buffers[i];
                if (buffer.GetBuffer() == VK_NULL_HANDLE) {
                    continue;
                }

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = MakeResourceIdName("Buffer", id);
                entry.cpuBytes = buffer.GetCPUAllocatedByteCount();
                entry.gpuBytes = buffer.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_accelerationStructures.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Acceleration Structures";
            category.entries.reserve(g_accelerationStructures.size());

            for (size_t i = 0; i < g_accelerationStructures.size(); i++) {
                const uint64_t id = g_accelerationStructures.id_at(i);
                const VulkanAccelerationStructure& accelerationStructure = g_accelerationStructures[i];

                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = MakeResourceIdName("Acceleration Structure", id);
                entry.cpuBytes = accelerationStructure.GetCPUAllocatedByteCount();
                entry.gpuBytes = accelerationStructure.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_descriptorSets.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Descriptor Sets";
            category.entries.reserve(g_descriptorSets.size());

            for (const auto& [name, descriptorSet] : g_descriptorSets) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = descriptorSet.GetCPUAllocatedByteCount();
                entry.gpuBytes = descriptorSet.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_pipelines.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Pipelines";
            category.entries.reserve(g_pipelines.size());

            for (const auto& [name, pipeline] : g_pipelines) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = pipeline.GetCPUAllocatedByteCount();
                entry.gpuBytes = pipeline.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_raytracingPipelines.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Raytracing Pipelines";
            category.entries.reserve(g_raytracingPipelines.size());

            for (const auto& [name, pipeline] : g_raytracingPipelines) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = pipeline.GetCPUAllocatedByteCount();
                entry.gpuBytes = pipeline.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_renderStates.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Render States";
            category.entries.reserve(g_renderStates.size());

            for (const auto& [name, renderState] : g_renderStates) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = renderState.GetCPUAllocatedByteCount();
                entry.gpuBytes = renderState.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_samplers.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Samplers";
            category.entries.reserve(g_samplers.size());

            for (const auto& [name, sampler] : g_samplers) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = sampler.GetCPUAllocatedByteCount();
                entry.gpuBytes = sampler.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }

        if (!g_shaders.empty()) {
            Hell::MemoryTracker::MemoryReportCategory category;
            category.name = "Vulkan Shaders";
            category.entries.reserve(g_shaders.size());

            for (const auto& [name, shader] : g_shaders) {
                Hell::MemoryTracker::MemoryReportEntry& entry = category.entries.emplace_back();
                entry.name = name;
                entry.cpuBytes = shader.GetCPUAllocatedByteCount();
                entry.gpuBytes = shader.GetGPUAllocatedByteCount();
            }

            AppendCategory(report, std::move(category));
        }
    }

    void CleanUpPipelines() {
        for (auto& [name, object] : g_pipelines)           { object.Cleanup(); } g_pipelines.clear();
        for (auto& [name, object] : g_raytracingPipelines) { object.Cleanup(); } g_raytracingPipelines.clear();
    }

    // Acceleration Structures    
                                               
    uint64_t CreateAccelerationStructure() {
        const uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::VULKAN_ACCELERATION_STRUCTURE);
        g_accelerationStructures.emplace_with_id(id);

        if (!g_accelerationStructures.get(id)) {
            Logging::Error() << "VulkanResourceManager::CreateAccelerationStructure(..) failed to create acceleration structure with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    bool AccelerationStructureExists(uint64_t id) {
        return g_accelerationStructures.get(id) != nullptr;
    }

    VulkanAccelerationStructure* GetAccelerationStructure(uint64_t id) {
        VulkanAccelerationStructure* accelerationStructure = g_accelerationStructures.get(id);

        if (!accelerationStructure) {
            Logging::Error() << "VulkanResourceManager::GetAccelerationStructure(..) no acceleration structure with id '" << id << "'.\n";
        }

        return accelerationStructure;
    }

    void RemoveAccelerationStructure(uint64_t id) {
        if (VulkanAccelerationStructure* accelerationStructure = g_accelerationStructures.get(id)) {
            accelerationStructure->Cleanup();
            g_accelerationStructures.erase(id);
        }
        else {
            Logging::Error() << "VulkanResourceManager::RemoveAccelerationStructure(..) no acceleration structure with id '" << id << "'.\n";
        }
    }
    
    // Allocated Images
     
    AllocatedImage& CreateAllocatedImage(const std::string& name, uint32_t width, uint32_t height, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageUsageFlags usage, bool allocateMips) {
        return CreateAllocatedImageArray(name, width, height, 1, sampleCount, format, usage, allocateMips);
    }

    AllocatedImage& CreateAllocatedImageArray(const std::string& name, uint32_t width, uint32_t height, uint32_t layerCount, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageUsageFlags usage, bool allocateMips) {
        if (width == 0 || height == 0) {
            Logging::Error() << "VulkanResourceManager::CreateAllocatedImage(..) zero dimension image '" << name << "' requested.\n";
            __debugbreak();
        }
        if (layerCount == 0) {
            Logging::Error() << "VulkanResourceManager::CreateAllocatedImageArray(..) zero layer image '" << name << "' requested.\n";
            __debugbreak();
        }
        if (sampleCount == 0) {
            Logging::Error() << "VulkanResourceManager::CreateAllocatedImage(..) invalid sample count for image '" << name << "'.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_allocatedImages.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        VkExtent3D extent{ width, height, 1 };
        it->second = AllocatedImage(format, extent, sampleCount, usage, name, layerCount, allocateMips);
        return it->second;
    }

    AllocatedImage* GetAllocatedImage(const std::string& name) {
        auto it = g_allocatedImages.find(name);
        if (it != g_allocatedImages.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetAllocatedImage(..) no allocated image named '" << name << "'.\n";
        return nullptr;
    }

    bool AllocatedImageExists(const std::string& name) {
        return g_allocatedImages.find(name) != g_allocatedImages.end();
    }

    void RemoveAllocatedImage(const std::string& name) {
        auto it = g_allocatedImages.find(name);
        if (it == g_allocatedImages.end()) return;

        it->second.Cleanup();
        g_allocatedImages.erase(it);
    }

    // Cubemaps

    VulkanCubemap& CreateCubemap(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateCubemap(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_cubemaps.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        it->second = VulkanCubemap();
        return it->second;
    }

    VulkanCubemap* GetCubemap(const std::string& name) {
        auto it = g_cubemaps.find(name);
        if (it != g_cubemaps.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetCubemap(..) no cubemap named '" << name << "'.\n";
        return nullptr;
    }

    bool CubemapExists(const std::string& name) {
        return g_cubemaps.find(name) != g_cubemaps.end();
    }

    void RemoveCubemap(const std::string& name) {
        auto it = g_cubemaps.find(name);
        if (it == g_cubemaps.end()) {
            Logging::Error() << "VulkanResourceManager::RemoveCubemap(..) no cubemap named '" << name << "'.\n";
            return;
        }

        it->second.Cleanup();
        g_cubemaps.erase(it);
    }

    // Cube Map Arrays

    VulkanCubeMapArray& CreateCubeMapArray(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateCubeMapArray(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_cubeMapArrays.try_emplace(name);
        if (!inserted) it->second.Cleanup();

        it->second = VulkanCubeMapArray();
        return it->second;
    }

    VulkanCubeMapArray* GetCubeMapArray(const std::string& name) {
        auto it = g_cubeMapArrays.find(name);
        if (it != g_cubeMapArrays.end()) return &it->second;

        Logging::Error() << "VulkanResourceManager::GetCubeMapArray(..) no cube map array named '" << name << "'.\n";
        return nullptr;
    }

    bool CubeMapArrayExists(const std::string& name) {
        return g_cubeMapArrays.find(name) != g_cubeMapArrays.end();
    }

    void RemoveCubeMapArray(const std::string& name) {
        auto it = g_cubeMapArrays.find(name);
        if (it == g_cubeMapArrays.end()) {
            Logging::Error() << "VulkanResourceManager::RemoveCubeMapArray(..) no cube map array named '" << name << "'.\n";
            return;
        }

        it->second.Cleanup();
        g_cubeMapArrays.erase(it);
    }
                            
    // Buffers              

    uint64_t CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
        if (size == 0) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) zero-sized buffer requested.\n";
            __debugbreak();
        }

        if (usage == 0) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) buffer with no usage flags requested.\n";
            __debugbreak();
        }

        const uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::VULKAN_BUFFER);
        g_buffers.emplace_with_id(id, size, usage, memoryUsage, vmaFlags);

        VulkanBuffer* buffer = g_buffers.get(id);
        if (!buffer || buffer->GetBuffer() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) failed to create buffer with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    uint64_t CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateBuffer(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto it = g_bufferIdsByName.find(name);
        if (it != g_bufferIdsByName.end()) {
            if (VulkanBuffer* buffer = g_buffers.get(it->second)) {
                buffer->Cleanup();
                g_buffers.erase(it->second);
            }
            g_bufferIdsByName.erase(it);
        }

        const uint64_t id = CreateBuffer(size, usage, memoryUsage, vmaFlags);
        g_bufferIdsByName[name] = id;
        return id;
    }

    bool BufferExists(const std::string& name) {
        auto it = g_bufferIdsByName.find(name);
        return it != g_bufferIdsByName.end() && g_buffers.get(it->second) != nullptr;
    }

    VulkanBuffer* GetBuffer(uint64_t id) {
        VulkanBuffer* buffer = g_buffers.get(id);

        if (!buffer) {
            Logging::Error() << "VulkanResourceManager::GetBuffer(..) no buffer with id '" << id << "'.\n";
        }

        return buffer;
    }

    VulkanBuffer* GetBuffer(const std::string& name) {
        auto it = g_bufferIdsByName.find(name);
        if (it == g_bufferIdsByName.end()) {
            Logging::Error() << "VulkanResourceManager::GetBuffer(..) no buffer named '" << name << "'.\n";
            return nullptr;
        }

        return GetBuffer(it->second);
    }

    void UploadBufferData(uint64_t id, const void* data, VkDeviceSize size) {
        if (VulkanBuffer* buffer = g_buffers.get(id)) {
            buffer->UploadData(data, size);
        }
        else {
            Logging::Error() << "VulkanResourceManager::UploadBufferData(..) no buffer with id '" << id << "'.\n";
        }
    }

    void RemoveBuffer(uint64_t id) {
        if (VulkanBuffer* buffer = g_buffers.get(id)) {
            for (auto it = g_bufferIdsByName.begin(); it != g_bufferIdsByName.end(); ) {
                if (it->second == id) {
                    it = g_bufferIdsByName.erase(it);
                }
                else {
                    it++;
                }
            }

            buffer->Cleanup();
            g_buffers.erase(id);
        }
        else {
            Logging::Error() << "VulkanResourceManager::RemoveBuffer(..) no buffer with id '" << id << "'.\n";
        }
    }

    void RemoveBuffer(const std::string& name) {
        auto it = g_bufferIdsByName.find(name);
        if (it == g_bufferIdsByName.end()) return;

        const uint64_t id = it->second;
        g_bufferIdsByName.erase(it);

        if (VulkanBuffer* buffer = g_buffers.get(id)) {
            buffer->Cleanup();
            g_buffers.erase(id);
        }
    }

    // Generic Mesh

    uint64_t CreateGenericMesh() {
        const uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::VULKAN_GENERIC_MESH);
        g_genericMeshes.emplace_with_id(id);

        if (!g_genericMeshes.get(id)) {
            Logging::Error() << "VulkanResourceManager::CreateGenericMesh() failed to create generic mesh with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    bool GenericMeshExists(uint64_t id) {
        return g_genericMeshes.get(id) != nullptr;
    }

    VulkanGenericMesh* GetGenericMesh(uint64_t id) {
        VulkanGenericMesh* genericMesh = g_genericMeshes.get(id);

        if (!genericMesh) {
            Logging::Error() << "VulkanResourceManager::GetGenericMesh(..) no generic mesh with id '" << id << "'.\n";
        }

        return genericMesh;
    }

    void RemoveGenericMesh(uint64_t id) {
        if (VulkanGenericMesh* genericMesh = g_genericMeshes.get(id)) {
            genericMesh->CleanUp();
            g_genericMeshes.erase(id);
        }
        else {
            Logging::Error() << "VulkanResourceManager::RemoveGenericMesh(..) no generic mesh with id '" << id << "'.\n";
        }
    }

    // Mesh Buffers

    uint64_t CreateMeshBuffer() {
        const uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::VULKAN_MESH_BUFFER);
        g_meshBuffers.emplace_with_id(id);

        if (!g_meshBuffers.get(id)) {
            Logging::Error() << "VulkanResourceManager::CreateMeshBuffer() failed to create mesh buffer with id '" << id << "'.\n";
            __debugbreak();
        }

        return id;
    }

    uint64_t CreateMeshBuffer(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateMeshBuffer(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto it = g_meshBufferIdsByName.find(name);
        if (it != g_meshBufferIdsByName.end()) {
            if (MeshBufferExists(it->second)) {
                RemoveMeshBuffer(it->second);
            }
            else {
                g_meshBufferIdsByName.erase(it);
            }
        }

        uint64_t id = CreateMeshBuffer();
        g_meshBufferIdsByName[name] = id;
        return id;
    }

    bool MeshBufferExists(uint64_t id) {
        return g_meshBuffers.get(id) != nullptr;
    }

    VulkanMeshBuffer* GetMeshBuffer(uint64_t id) {
        VulkanMeshBuffer* meshBuffer = g_meshBuffers.get(id);

        if (!meshBuffer) {
            Logging::Error() << "VulkanResourceManager::GetMeshBuffer(..) no mesh buffer with id '" << id << "'.\n";
        }

        return meshBuffer;
    }

    VulkanMeshBuffer* GetMeshBuffer(const std::string& name) {
        auto it = g_meshBufferIdsByName.find(name);
        if (it == g_meshBufferIdsByName.end()) {
            Logging::Error() << "VulkanResourceManager::GetMeshBuffer(..) no mesh buffer named '" << name << "'.\n";
            return nullptr;
        }

        return GetMeshBuffer(it->second);
    }

    void RemoveMeshBuffer(uint64_t id) {
        if (VulkanMeshBuffer* meshBuffer = g_meshBuffers.get(id)) {
            for (auto it = g_meshBufferIdsByName.begin(); it != g_meshBufferIdsByName.end(); ) {
                if (it->second == id) {
                    it = g_meshBufferIdsByName.erase(it);
                }
                else {
                    it++;
                }
            }

            meshBuffer->Cleanup();
            g_meshBuffers.erase(id);
        }
        else {
            Logging::Error() << "VulkanResourceManager::RemoveMeshBuffer(..) no mesh buffer with id '" << id << "'.\n";
        }
    }

    // Descriptor Sets

    VulkanDescriptorSetResource& CreateDescriptorSet(const std::string& name, VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (layoutInfo.sType != VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) invalid descriptor set layout create info for '" << name << "'.\n";
            __debugbreak();
        }

        if (layoutInfo.bindingCount > 0 && !layoutInfo.pBindings) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) null bindings pointer for '" << name << "'.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_descriptorSets.try_emplace(name);

        if (!inserted) {
            it->second.Cleanup();
        }

        it->second = VulkanDescriptorSetResource(layoutInfo, lifetime);

        if (it->second.GetLayout() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateDescriptorSet(..) failed to create descriptor set resource '" << name << "'.\n";
            __debugbreak();
        }

        return it->second;
    }

    bool DescriptorSetExists(const std::string& name) {
        return g_descriptorSets.find(name) != g_descriptorSets.end();
    }

    VulkanDescriptorSetResource* GetDescriptorSetResource(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSetResource(..) no descriptor set resource named '" << name << "'.\n";
            return nullptr;
        }

        return &it->second;
    }

    VulkanDescriptorSet* GetDescriptorSet(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSet(..) no descriptor set resource named '" << name << "'.\n";
            return nullptr;
        }

        return &it->second.GetSet();
    }

    VkDescriptorSetLayout GetDescriptorSetLayout(const std::string& name) {
        auto it = g_descriptorSets.find(name);

        if (it == g_descriptorSets.end()) {
            Logging::Error() << "VulkanResourceManager::GetDescriptorSetLayout(..) no descriptor set resource named '" << name << "'.\n";
            return VK_NULL_HANDLE;
        }

        return it->second.GetLayout();
    }

    void RemoveDescriptorSet(const std::string& name) {
        auto it = g_descriptorSets.find(name);
        if (it == g_descriptorSets.end()) return;

        it->second.Cleanup();
        g_descriptorSets.erase(it);
    }
    
    // Pipelines
                        
    VulkanPipeline& CreatePipeline(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreatePipeline(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            it->second.Cleanup();
            g_pipelines.erase(it);
        }

        VulkanPipeline& pipeline = g_pipelines[name];
        pipeline.SetName(name);
        return pipeline;
    }

    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateRaytracingPipeline(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto it = g_raytracingPipelines.find(name);
        if (it != g_raytracingPipelines.end()) {
            it->second.Cleanup();
            g_raytracingPipelines.erase(it);
        }

        return g_raytracingPipelines[name];
    }

    VulkanPipeline* GetPipeline(const std::string& name) {
        auto it = g_pipelines.find(name);
        if (it != g_pipelines.end()) {
            if (it->second.GetHandle() == VK_NULL_HANDLE) {
                Logging::Error() << "VulkanResourceManager::GetPipeline(..) graphics pipeline '" << name << "' has no handle.\n";
                return nullptr;
            }
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetPipeline(..) no graphics pipeline named '" << name << "'.\n";
        return nullptr;
    }

    VulkanRaytracingPipeline* GetRaytracingPipeline(const std::string& name) {
        auto it = g_raytracingPipelines.find(name);
        if (it != g_raytracingPipelines.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetRaytracingPipeline(..) no raytracing pipeline named '" << name << "'.\n";
        return nullptr;
    }

    // Render States

    VulkanRenderState& CreateRenderState(const std::string& name) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateRenderState(..) empty resource name requested.\n";
            __debugbreak();
        }

        auto [it, inserted] = g_renderStates.try_emplace(name);

        if (!inserted) {
            it->second.CleanUp();
        }

        it->second = VulkanRenderState();
        return it->second;
    }

    VulkanRenderState* GetRenderState(const std::string& name) {
        auto it = g_renderStates.find(name);
        if (it != g_renderStates.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetRenderState(..) no render state named '" << name << "'.\n";
        return nullptr;
    }

    // Samplers
             
    VulkanSampler& CreateSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (maxAnisotropy <= 0.0f) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) non-positive max anisotropy requested for '" << name << "'.\n";
            __debugbreak();
        }

        // Get iterator to the new or existing element
        auto [it, inserted] = g_samplers.try_emplace(name);

        // If it already existed, clean up the old one
        if (!inserted) {
            it->second.Cleanup();
        }

        // Move a new sampler into the slot
        it->second = VulkanSampler(magFilter, minFilter, addressMode, maxAnisotropy);

        if (it->second.GetSampler() == VK_NULL_HANDLE) {
            Logging::Error() << "VulkanResourceManager::CreateSampler(..) failed to create sampler '" << name << "'.\n";
            __debugbreak();
        }

        return it->second;
    }

    VulkanSampler* GetSampler(const std::string& name) {
        auto it = g_samplers.find(name);
        if (it != g_samplers.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetSampler(..) no sampler named '" << name << "'.\n";
        return nullptr;
    }

    bool SamplerExists(const std::string& name) {
        return g_samplers.find(name) != g_samplers.end();
    }

    uint64_t CreateTexture() {
        uint64_t id = Hell::ResourceManagement::GetNextID(Hell::ResourceManagement::ResourceType::VULKAN_TEXTURE);
        g_textures.emplace_with_id(id);
        return id;
    }

    VulkanTexture& GetTexture(uint64_t id) {
        VulkanTexture* texture = GetTexturePtr(id);
        if (texture) {
            return *texture;
        }
        static VulkanTexture invalid;
        return invalid;
    }

    VulkanTexture* GetTexturePtr(uint64_t id) {
        return g_textures.get(id);
    }

    void RemoveTexture(uint64_t id) {
        if (g_textures.contains(id)) {
            VulkanTexture* texture = g_textures.get(id);
            texture->Cleanup();
            g_textures.erase(id);
        }
    }

    // Shaders

    VulkanShader& CreateShader(const std::string& name, const std::vector<std::string>& paths) {
        if (name.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) empty resource name requested.\n";
            __debugbreak();
        }

        if (paths.empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) no shader paths supplied for '" << name << "'.\n";
            __debugbreak();
        }

        g_shaders.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(name),
            std::forward_as_tuple(paths)
        );

        VulkanShader& shader = g_shaders.at(name);
        if (shader.GetStageCreateInfos().empty()) {
            Logging::Error() << "VulkanResourceManager::CreateShader(..) failed to create any shader modules for '" << name << "'.\n";
            __debugbreak();
        }

        return shader;
    }

    VulkanShader* GetShader(const std::string& name) {
        auto it = g_shaders.find(name);
        if (it != g_shaders.end()) {
            return &it->second;
        }

        Logging::Error() << "VulkanResourceManager::GetShader(..) no shader named '" << name << "'.\n";
        return nullptr;
    }

    bool ShaderExists(const std::string& name) {
        return g_shaders.find(name) != g_shaders.end();
    }

    bool HotloadShaders(std::string& failedShaders) {
        bool success = true;

        for (auto& [name, shader] : g_shaders) {
            if (!shader.Hotload()) {
                success = false;

                failedShaders += "\n- ";
                for (const std::string& path : shader.GetPaths()) {
                    failedShaders += path + " ";
                }
            }
        }

        return success;
    }

    bool HotloadShaders() {
        std::string failedShaders = "FAILED TO HOTLOAD";
        return HotloadShaders(failedShaders);
    }
}
