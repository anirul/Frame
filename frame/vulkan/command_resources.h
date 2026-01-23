#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "frame/vulkan/vulkan_dispatch.h"

namespace frame::vulkan
{

class CommandResources
{
  public:
    CommandResources(vk::Device device, std::uint32_t graphics_queue_family_index);

    void Create();
    void Destroy();

    void AllocateBuffers(std::uint32_t count);
    void FreeBuffers();

    bool HasPool() const
    {
        return static_cast<bool>(command_pool_);
    }

    const vk::UniqueCommandPool& GetPool() const
    {
        return command_pool_;
    }

    const std::vector<vk::CommandBuffer>& GetBuffers() const
    {
        return command_buffers_;
    }

    vk::CommandBuffer GetBuffer(std::size_t index) const
    {
        return command_buffers_.at(index);
    }

  private:
    vk::Device device_;
    std::uint32_t graphics_queue_family_index_ = 0;
    vk::UniqueCommandPool command_pool_;
    std::vector<vk::CommandBuffer> command_buffers_;
};

} // namespace frame::vulkan
