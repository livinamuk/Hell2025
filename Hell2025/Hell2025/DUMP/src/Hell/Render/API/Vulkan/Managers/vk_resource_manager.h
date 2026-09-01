#pragma once
#include "Hell/Render/API/Vulkan/vk_common.h"
#include "Hell/Render/API/Vulkan/Types/vk_acceleration_structure.h"
#include "Hell/Render/API/Vulkan/Types/vk_allocated_image.h"
#include "Hell/Render/API/Vulkan/Types/vk_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_cube_map_array.h"
#include "Hell/Render/API/Vulkan/Types/vk_cubemap.h"
#include "Hell/Render/API/Vulkan/Types/vk_descriptor_set.h"
#include "Hell/Render/API/Vulkan/Types/vk_generic_mesh.h"
#include "Hell/Render/API/Vulkan/Types/vk_mesh_buffer.h"
#include "Hell/Render/API/Vulkan/Types/vk_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/vk_raytracing_pipeline.h"
#include "Hell/Render/API/Vulkan/Types/VK_render_state.h"
#include "Hell/Render/API/Vulkan/Types/vk_sampler.h"
#include "Hell/Render/API/Vulkan/Types/vk_shader.h"
#include "Hell/Render/API/Vulkan/Types/vk_texture.h"
#include <unordered_map>
#include <string>

namespace Hell::MemoryTracker {
    struct MemoryReport;
}

namespace VulkanResourceManager {
    void Cleanup();
    void AppendMemoryReport(Hell::MemoryTracker::MemoryReport& report);

    // Acceleration Structures
    uint64_t CreateAccelerationStructure();
    bool AccelerationStructureExists(uint64_t id);
    VulkanAccelerationStructure* GetAccelerationStructure(uint64_t id);
    void RemoveAccelerationStructure(uint64_t id);

    // Allocated Images
    AllocatedImage& CreateAllocatedImage(const std::string& name, uint32_t width, uint32_t height, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageUsageFlags usage, bool allocateMips = false);
    AllocatedImage& CreateAllocatedImageArray(const std::string& name, uint32_t width, uint32_t height, uint32_t layerCount, VkSampleCountFlagBits sampleCount, VkFormat format, VkImageUsageFlags usage, bool allocateMips = false);
    AllocatedImage* GetAllocatedImage(const std::string& name);
    bool AllocatedImageExists(const std::string& name);
    void RemoveAllocatedImage(const std::string& name);

    // Cubemaps
    VulkanCubemap& CreateCubemap(const std::string& name);
    VulkanCubemap* GetCubemap(const std::string& name);
    bool CubemapExists(const std::string& name);
    void RemoveCubemap(const std::string& name);

    // Cube Map Arrays
    VulkanCubeMapArray& CreateCubeMapArray(const std::string& name);
    VulkanCubeMapArray* GetCubeMapArray(const std::string& name);
    bool CubeMapArrayExists(const std::string& name);
    void RemoveCubeMapArray(const std::string& name);

    // Buffers
    uint64_t CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);
    uint64_t CreateBuffer(const std::string& name, VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocationCreateFlags vmaFlags = 0);
    bool BufferExists(const std::string& name);
    VulkanBuffer* GetBuffer(uint64_t id);
    VulkanBuffer* GetBuffer(const std::string& name);
    void UploadBufferData(uint64_t id, const void* data, VkDeviceSize size);
    void RemoveBuffer(uint64_t id);
    void RemoveBuffer(const std::string& name);

    // Generic Meshes
    uint64_t CreateGenericMesh();
    bool GenericMeshExists(uint64_t id);
    VulkanGenericMesh* GetGenericMesh(uint64_t id);
    void RemoveGenericMesh(uint64_t id);

    // Mesh Buffers
    uint64_t CreateMeshBuffer();
    uint64_t CreateMeshBuffer(const std::string& name);
    bool MeshBufferExists(uint64_t id);
    VulkanMeshBuffer* GetMeshBuffer(uint64_t id);
    VulkanMeshBuffer* GetMeshBuffer(const std::string& name);
    void RemoveMeshBuffer(uint64_t id);

    // Descriptor Sets
    VulkanDescriptorSetResource& CreateDescriptorSet(const std::string& name, VkDescriptorSetLayoutCreateInfo layoutInfo, DescriptorSetLifetime lifetime);
    bool DescriptorSetExists(const std::string& name);
    VulkanDescriptorSetResource* GetDescriptorSetResource(const std::string& name);
    VulkanDescriptorSet* GetDescriptorSet(const std::string& name);
    VkDescriptorSetLayout GetDescriptorSetLayout(const std::string& name);
    void RemoveDescriptorSet(const std::string& name);

    // Pipelines
    VulkanPipeline& CreatePipeline(const std::string& name);
    VulkanPipeline* GetPipeline(const std::string& name);
    VulkanRaytracingPipeline& CreateRaytracingPipeline(const std::string& name);
    VulkanRaytracingPipeline* GetRaytracingPipeline(const std::string& name);
    void CleanUpPipelines();

    // Render States
    VulkanRenderState& CreateRenderState(const std::string& name);
    VulkanRenderState* GetRenderState(const std::string& name);

    // Samplers
    VulkanSampler& CreateSampler(const std::string& name, VkFilter magFilter, VkFilter minFilter, VkSamplerAddressMode addressMode, float maxAnisotropy = 1.0f);
    VulkanSampler* GetSampler(const std::string& name);
    bool SamplerExists(const std::string& name);

    // Textures
    uint64_t CreateTexture();
    VulkanTexture& GetTexture(uint64_t id);
    VulkanTexture* GetTexturePtr(uint64_t id);
    void RemoveTexture(uint64_t id);

    // Shaders
    VulkanShader& CreateShader(const std::string& name, const std::vector<std::string>& paths);
    VulkanShader* GetShader(const std::string& name);
    bool ShaderExists(const std::string& name);
    bool HotloadShaders(std::string& failedShaders);
    bool HotloadShaders();
}
