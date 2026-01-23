#pragma once

#include <string>
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
    void BuildUniformBuffer(vk::DeviceSize size_bytes);
    void UpdateUniform(const void* data, std::size_t byte_count) const;

    const std::vector<BufferResource>& GetStorageBuffers() const
    {
        return storage_buffers_;
    }

    const BufferResource* GetUniformBuffer() const
    {
        return uniform_buffer_.buffer ? &uniform_buffer_ : nullptr;
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
    BufferResource uniform_buffer_;
};

} // namespace frame::vulkan
