#include "frame/vulkan/command_resources.h"

namespace frame::vulkan
{

CommandResources::CommandResources(
    vk::Device device,
    std::uint32_t graphics_queue_family_index)
    : device_(device),
      graphics_queue_family_index_(graphics_queue_family_index)
{
}

void CommandResources::Create()
{
    if (command_pool_)
    {
        return;
    }

    vk::CommandPoolCreateInfo pool_info(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        graphics_queue_family_index_);
    command_pool_ = device_.createCommandPoolUnique(pool_info);
}

void CommandResources::Destroy()
{
    FreeBuffers();
    command_pool_.reset();
}

void CommandResources::AllocateBuffers(std::uint32_t count)
{
    if (!command_pool_)
    {
        return;
    }

    FreeBuffers();

    if (count == 0)
    {
        return;
    }

    vk::CommandBufferAllocateInfo allocate_info(
        *command_pool_,
        vk::CommandBufferLevel::ePrimary,
        count);
    command_buffers_ = device_.allocateCommandBuffers(allocate_info);
}

void CommandResources::FreeBuffers()
{
    if (command_pool_ && !command_buffers_.empty())
    {
        device_.freeCommandBuffers(*command_pool_, command_buffers_);
        command_buffers_.clear();
    }
}

} // namespace frame::vulkan
