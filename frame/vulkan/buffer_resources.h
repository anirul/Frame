#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "frame/vulkan/vulkan_dispatch.h"

#include "frame/level_interface.h"
#include "frame/logger.h"
#include "frame/vulkan/buffer.h"
#include "frame/vulkan/command_queue.h"
#include "frame/vulkan/gpu_memory_manager.h"

namespace frame::vulkan
{

struct BufferResource
{
    std::string name;
    vk::UniqueBuffer buffer;
    vk::UniqueDeviceMemory memory;
    vk::DeviceSize size = 0;
};

class BufferResourceManager
{
  public:
    BufferResourceManager(
        vk::Device device,
        GpuMemoryManager& memory_manager,
        CommandQueue& command_queue,
        const Logger& logger);

    void Clear();
    void BuildStorageBuffers(
        LevelInterface& level,
        const std::vector<EntityId>& buffer_ids);
    bool UpdateStorageBuffer(
        const std::string& name,
        const std::vector<std::uint8_t>& bytes);
    bool UpdateStorageBufferRange(
        const std::string& name,
        const std::vector<std::uint8_t>& bytes,
        std::size_t offset_bytes);
    void BuildUniformBuffers(
        std::size_t count,
        vk::DeviceSize size_bytes);
    void BuildUniformBuffer(vk::DeviceSize size_bytes)
    {
        BuildUniformBuffers(1, size_bytes);
    }
    void UpdateUniform(
        std::size_t index,
        const void* data,
        std::size_t byte_count) const;
    void UpdateUniform(const void* data, std::size_t byte_count) const
    {
        UpdateUniform(0, data, byte_count);
    }

    const std::vector<BufferResource>& GetStorageBuffers() const
    {
        return storage_buffers_;
    }

    const BufferResource* GetUniformBuffer(std::size_t index) const
    {
        if (index >= uniform_buffers_.size())
        {
            return nullptr;
        }
        return uniform_buffers_[index].buffer ? &uniform_buffers_[index]
                                              : nullptr;
    }
    const BufferResource* GetUniformBuffer() const
    {
        return GetUniformBuffer(0);
    }

    void LogCpuBufferSamples(
        LevelInterface& level,
        bool debug_dump_done) const;
    void LogGpuBufferSamples() const;

  private:
    BufferResource MakeGpuBuffer(
        const std::string& name,
        const std::vector<std::uint8_t>& bytes,
        vk::BufferUsageFlags extra_flags);

    vk::Device device_;
    GpuMemoryManager* memory_manager_;
    CommandQueue* command_queue_;
    const Logger* logger_;
    std::vector<BufferResource> storage_buffers_;
    std::unordered_map<std::string, std::size_t> storage_buffer_indices_;
    std::vector<BufferResource> uniform_buffers_;
};

} // namespace frame::vulkan
